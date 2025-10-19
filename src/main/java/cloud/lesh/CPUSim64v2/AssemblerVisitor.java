package cloud.lesh.CPUSim64v2;

import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.tree.ParseTree;

import org.apache.commons.lang3.StringEscapeUtils;
import java.util.*;
import java.nio.charset.StandardCharsets;

public class AssemblerVisitor extends CPUSim64v2BaseVisitor<Void> implements HasLocation {
	Map<String, Long> labelMap;
	private final Stack<String> blockNames = new Stack<>();

	String filename = null;
	int lineNum = 1;
	boolean pauseLineIncrement = false;
	private final LinkedList errors = new LinkedList<String>();
	private long blockCount = 0;

	public String getLocation() {
		return (filename == null ? "" : filename + ":") + lineNum;
	}
	public List<String> getErrors() { return errors; }

	public class AssemblerException extends RuntimeException {
		public AssemblerException(String msg) {
			super("[" + getLocation() + "] " + msg);
		}
	}

	public AssemblerVisitor(Map<String, Long> labelMap) {
		this.labelMap = labelMap;
	}

	// Result: one 64-bit word per assembled instruction
	private final List<Long> out = new ArrayList<>();

	public List<Long> result() {
		return out;
	}

	// ---- Encoding helpers (64-bit only) ----
	// Type field: 2 bits (tt)
	private static final int TT_STD = 0;  // Type 0 (standard: up to 4 short operands)
	private static final int TT_C1 = 1;  // Type 1 (single long constant C1 = 56-bit signed)
	private static final int TT_YC2 = 2;   // Type 2 ([YZ]C2, 42-bit signed)
	private static final int TT_YYC3 = 3;  // Type 3 ([YZ][YZ]C3, 28-bit signed)

	private static final int C0_BITS = 12; // bits for each of op0..op3 in Type-0
	private static final int C1_BITS = 56; // bits for imm56 in Type-1
	private static final int C2_BITS = 42; // bits for imm42 in Type-2
	private static final int C3_BITS = 28; // bits for imm28 in Type-3

	// Operand type codes for aa/bb/cc/dd fields (2 bits each)
	private static final int OT_NONE = 0;
	private static final int OT_CONST = 1;
	private static final int OT_REG = 2;
	private static final int OT_FP = 3;

	// ---- packers for each format (bit layout chosen to match your 64-bit diagrams) ----
	// 64-bit Type-0:
	//   [63:62] tt
	//   [61:56] opcode
	//   [55:48] aabbccdd  (2b each)
	//   [47:36] op0 (12b)
	//   [35:24] op1 (12b)
	//   [23:12] op2 (12b)
	//   [11: 0] op3 (12b)
	//
	// NOTE: Your diagram labels bytes as 00/11/22/33; using 12-bit operand slots fits the 0..4095 (12-bit) range you gave for 64-bit.
	private long encType0(int opcode, int a, int b, int c, int d, int v0, int v1, int v2, int v3) {
		long w = 0;
		v0 = (int) fitSigned(v0, 12);
		v1 = (int) fitSigned(v1, 12);
		v2 = (int) fitSigned(v2, 12);
		v3 = (int) fitSigned(v3, 12);
		w |= ((long) TT_STD & 0x3L) << 62;
		w |= ((long) opcode & 0x3FL) << 56;
		long types = ((a & 3) << 6) | ((b & 3) << 4) | ((c & 3) << 2) | (d & 3);
		w |= (types & 0xFFL) << 48;
		w |= ((long) (v0 & 0xFFF)) << 36;
		w |= ((long) (v1 & 0xFFF)) << 24;
		w |= ((long) (v2 & 0xFFF)) << 12;
		w |= ((long) (v3 & 0xFFF));
		return w;
	}

	// 64-bit Type-1 (C1 = 56-bit signed):
	//   [63:62] tt=1
	//   [61:56] opcode
	//   [55: 0] imm56 (two’s complement)
	private long encType1C1(int opcode, long imm56) {
		long w = 0;
		imm56 = fitSigned(imm56, 56);
		w |= ((long) TT_C1 & 0x3L) << 62;
		w |= ((long) opcode & 0x3FL) << 56;
		long masked = imm56 & ((1L << 56) - 1);
		w |= masked;
		return w;
	}

	// 64-bit Type-2 ([YZ]C2, 42-bit signed):
	//   [63:62] tt=2
	//   [61:56] opcode
	//   [55:54] aa (type of operand 0: REG/FP/CONST per table; here [YZ] means REG/FP or A/FP → we encode as REG or FP)
	//   [53:42] op0 (12b)
	//   [41: 0] imm42 (two’s complement)
	private long encType2RC2(int opcode, int aType, int op0, long imm42) {
		long w = 0;
		op0 = (int) fitSigned(op0, 12);
		imm42 = fitSigned(imm42, 42);
		w |= ((long) TT_YC2 & 0x3L) << 62;
		w |= ((long) opcode & 0x3FL) << 56;
		w |= ((long) (aType & 0x3)) << 54;
		w |= ((long) (op0 & 0xFFF)) << 42;
		w |= (imm42 & ((1L << 42) - 1));
		return w;
	}

	// 64-bit Type-3 ([YZ][YZ]C3, 28-bit signed):
	//   [63:62] tt=3
	//   [61:56] opcode
	//   [55:54] aa (op0 type)
	//   [53:52] bb (op1 type)
	//   [51:40] op0 (12b)
	//   [39:28] op1 (12b)
	//   [27: 0] imm28 (two’s complement)
	private long encType3ZZC3(int opcode, int aType, int op0, int bType, int op1, int imm28) {
		long w = 0;
		op0 = (int) fitSigned(op0, 12);
		op1 = (int) fitSigned(op1, 12);
		imm28 = (int) fitSigned(imm28, 28);
		w |= ((long) TT_YYC3 & 0x3L) << 62;
		w |= ((long) opcode & 0x3FL) << 56;
		w |= ((long) (aType & 0x3)) << 54;
		w |= ((long) (bType & 0x3)) << 52;
		w |= ((long) (op0 & 0xFFF)) << 40;
		w |= ((long) (op1 & 0xFFF)) << 28;
		w |= (imm28 & ((1 << 28) - 1));
		return w;
	}

	// ---- Small helpers to classify operands and pull numeric values ----

	// Operand type codes
	private static int typeCodeForReg() {
		return OT_REG;
	}

	private static int typeCodeForFp() {
		return OT_FP;
	}

	private static int typeCodeForConst() {
		return OT_CONST;
	}

	private static int typeCodeNone() {
		return OT_NONE;
	}

	// R registers: r0..r28 (we let semantic pass restrict; here value is numeric index)
	private int regIndex(String rText) {
		// r or R followed by decimal
		int idx = Integer.parseInt(rText.substring(1));
		if (idx < 0 || idx > 28)
			throw new AssemblerException("Register index out of range (0..28): " + rText);
		return idx;
	}

	// F registers: f0..f31
	private int fpIndex(String fText) {
		int idx = Integer.parseInt(fText.substring(1));
		if (idx < 0 || idx > 31)
			throw new AssemblerException("FP index out of range (0..31): " + fText);
		return idx;
	}

	private int aIndexFromToken(Token t) {
		String s = t.getText().toUpperCase();
		if (s.equals("SF")) return 29;
		else if (s.equals("SP")) return 30;
		else if (s.equals("PC")) return 31;
		else if (s.startsWith("R")) return regIndex(s);
		throw new AssemblerException("Not an address-capable register: " + s);
	}

	private long parseIntLike(String text) {
		if (text.startsWith("0x") || text.startsWith("0X")) {
			return Long.parseUnsignedLong(text.substring(2), 16);
		} else if (text.startsWith("-0x") || text.startsWith("-0X")) {
			return -Long.parseUnsignedLong(text.substring(3), 16);
		} else if ((text.charAt(0) == '-') ||
				(text.charAt(0) >= '0' && text.charAt(0) <= '9')) {
			return Long.parseLong(text);
		} else {
			if (text.charAt(0) == '@')
				text = text.substring(1);
			if (text.charAt(0) == '$')
				text = getScopeName() + text;
			var v = labelMap.get(text.toUpperCase());
			if (v != null)
				return Long.valueOf(v);
		}
		throw new AssemblerException("Invalid integer/label: " + text);
	}

	// zPort or zCond (0..15)
	private int parseZSmall(String text) {
		int v = 0;
		switch (text.toLowerCase()) {
			case "u":
				return 0;
			case "z":
			case "eq":
				return 1;
			case "nz":
			case "ne":
				return 2;
			case "n":
			case "lt":
				return 3;
			case "p":
			case "gt":
				return 4;
			case "nn":
			case "ge":
				return 5;
			case "np":
			case "le":
				return 6;
			case "o":
			case "inf":
				return 7;
			case "no":
			case "ninf":
				return 8;
			case "pe":
				return 9;
			case "po":
				return 10;
			default:
				v = (int) parseIntLike(text);
		}
		if (v < 0 || v > 15) throw new AssemblerException("Z out of range (0..15): " + text);
		return (int) v;
	}

	// sign-range checks for immediates (throw for now; you can downgrade to errors list)
	private long fitSigned(long val, int bits) {
		long min = -(1L << (bits - 1));
		long max = (1L << (bits - 1)) - 1;
		if (val < min || val > max) {
			throw new AssemblerException("Immediate " + val + " does not fit in " + bits + " signed bits");
		}
		// stored as two's complement masked to width:
		long mask = (bits == 64) ? -1L : ((1L << bits) - 1);
		return val & mask;
	}

	private int fitUnsigned8(long v) {
		if (v < 0 || v > 0xFF) throw new AssemblerException("Field needs 8-bit unsigned (0..255): " + v);
		return (int) v;
	}

	private boolean fitsIn(long val, int bits, boolean isSigned) {
		if (isSigned) {
			long min = -(1L << (bits - 1));
			long max = (1L << (bits - 1)) - 1;
			return (val >= min && val <= max);
		} else {
			return (val >= 0 && val <= ((1L << bits) - 1));
		}
	}

	private long parseCharLiteral(String s) {
/*
		if (s.length() >= 2 && s.charAt(0) == '\'' && s.charAt(s.length() - 1) == '\'') {
			s = s.substring(1, s.length() - 1);
		}
		// Handle escape sequences
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < s.length(); i++) {
			char ch = s.charAt(i);
			if (ch == '\\' && i + 1 < s.length()) {
				char next = s.charAt(i + 1);
				switch (next) {
					case 'n' -> {
						sb.append('\n');
						i++;
					}
					case 't' -> {
						sb.append('\t');
						i++;
					}
					case 'r' -> {
						sb.append('\r');
						i++;
					}
					case '\\' -> {
						sb.append('\\');
						i++;
					}
					case '\'' -> {
						sb.append('\'');
						i++;
					}
					case '\"' -> {
						sb.append('\"');
						i++;
					}
					case '0' -> {
						sb.append('\0');
						i++;
					}
					case 'u', 'U' -> {
						if (i + 5 < s.length()) {
							String hex = s.substring(i + 2, i + 6);
							try {
								int codePoint = Integer.parseInt(hex, 16);
								sb.append((char) codePoint);
								i += 5;
							} catch (NumberFormatException e) {
								sb.append(s); // Invalid escape, keep as-is
								i++;
							}
						} else {
							sb.append(s); // Incomplete escape, keep as-is
							i++;
						}
					}
					default -> sb.append(ch); // Unknown escape, keep as-is
				}
			} else {
				sb.append(ch);
			}
		}
		String unescaped = sb.toString();
		if (unescaped.length() != 1) {
			throw new IllegalStateException("CHARLIT must be a single character");
		}
		return unescaped.codePointAt(0);
		*/
		return 0;
	}

	private String getScopeName() {
		return String.join("$", blockNames);
	}

	@Override
	public Void visitProgram(CPUSim64v2Parser.ProgramContext ctx) {
		for (var child : ctx.children) {
			try {
				visit(child);
			} catch (AssemblerException ex) {
				errors.add(ex.getMessage());
			}
			if (!pauseLineIncrement) ++lineNum;
		}
		return null;
	}

	// --------------------------------------------------------------------
	// VISITORS: each instruction rule chooses a format, gathers operands,
	//           and emits one encoded 64-bit word into `out`.
	// --------------------------------------------------------------------

	@Override
	public Void visitInstrNOP(CPUSim64v2Parser.InstrNOPContext ctx) {
		long w = encType0(Opcode.NOP.code, OT_NONE, OT_NONE, OT_NONE, OT_NONE, 0, 0, 0, 0);
		out.add(w);
		return null;
	}

	@Override
	public Void visitInstrDEBUG(CPUSim64v2Parser.InstrDEBUGContext ctx) {
		if (ctx.children.size() == 1) {
			// No operands: just DEBUG
			out.add(encType1C1(Opcode.DEBUG.code, -1));
		} else if (ctx.y1to4() != null) {
			var ys = ctx.y1to4().yOperand();
			int a = OT_NONE, b = OT_NONE, c = OT_NONE, d = OT_NONE;
			int v0 = 0, v1 = 0, v2 = 0, v3 = 0;
			for (int i = 0; i < ys.size(); i++) {
				var y = ys.get(i);
				boolean isFp = (y.fOperand() != null);
				int t = isFp ? OT_FP : OT_REG;   // Y = A or F; encode A (R/SF/SP/PC) as REG here with A-index
				int v = isFp ? fpIndex(y.fOperand().REG_F().getText())
						: aIndexFromToken(y.aOperand().start);
				if (i == 0) {
					a = t;
					v0 = v;
				} else if (i == 1) {
					b = t;
					v1 = v;
				} else if (i == 2) {
					c = t;
					v2 = v;
				} else {
					d = t;
					v3 = v;
				}
			}
			out.add(encType0(Opcode.DEBUG.code, a, b, c, d, v0, v1, v2, v3));
		} else {
			// TODO Is this really the best way to do this?
			// AC form: treat as Type-3: op0=addr (A), op1 unused, imm28=count
			int aType = OT_REG; // A encoded via REG code + A-index
			long cnt = parseIntLike(ctx.cLiteral().getText());
			int op0;
			long k;
			if (ctx.aOperand() != null) {
				op0 = aIndexFromToken(ctx.aOperand().start);
				out.add(encType0(Opcode.DEBUG.code, aType, OT_CONST, OT_NONE, OT_NONE,
						op0, (int)cnt, 0, 0));
			} else if (ctx.aLiteral() != null) {
				k = parseIntLike(ctx.aLiteral().getText());
				out.add(encType2RC2(Opcode.DEBUG.code, OT_CONST, (int)cnt, k));
			} else {
				throw new AssemblerException("DEBUG with C literal needs A or R operand");
			}
		}
		return null;
	}

	@Override
	public Void visitInstrCLEAR(CPUSim64v2Parser.InstrCLEARContext ctx) {
		// N or X..XXXX → always Type-0
		// We’ll flatten y1to4/x1to4 counts into up to four REG/FP fields; unused are NONE/0
		List<CPUSim64v2Parser.XOperandContext> xs = ctx.x1to4() == null ? List.of() : ctx.x1to4().xOperand();
		int a = OT_NONE, b = OT_NONE, c = OT_NONE, d = OT_NONE;
		int v0 = 0, v1 = 0, v2 = 0, v3 = 0;

		for (int i = 0; i < xs.size(); i++) {
			var x = xs.get(i);
			boolean isFp = x.fOperand() != null;
			int t = isFp ? OT_FP : OT_REG;
			int v = isFp ? fpIndex(x.fOperand().REG_F().getText())
					: regIndex(x.rOperand().REG_R().getText());
			if (i == 0) {
				a = t;
				v0 = v;
			} else if (i == 1) {
				b = t;
				v1 = v;
			} else if (i == 2) {
				c = t;
				v2 = v;
			} else {
				d = t;
				v3 = v;
			}
		}
		long w = encType0(Opcode.CLEAR.code, a, b, c, d, v0, v1, v2, v3);
		out.add(w);
		return null;
	}

	@Override
	public Void visitInstrMOVE(CPUSim64v2Parser.InstrMOVEContext ctx) {
		int a = OT_NONE, b = OT_NONE, c = OT_NONE, d = OT_NONE;
		int v0 = 0, v1 = 0, v2 = 0, v3 = 0;
		long k;
		if (ctx.zCond() != null) {
			// ZYQQ
			int z = parseZSmall(ctx.zCond().getText());
			a = OT_CONST;
			v0 = z;

			CPUSim64v2Parser.YOperandContext y = ctx.yOperand(0);
			boolean yIsFp = (y.fOperand() != null);
			b = yIsFp ? OT_FP : OT_REG;
			v1 = yIsFp
					? fpIndex(y.fOperand().REG_F().getText())
					: aIndexFromToken(y.aOperand().start);

			CPUSim64v2Parser.QOperandContext q1 = ctx.qOperand(0);
			if (q1.fOperand() != null) {
				c = OT_FP;
				v2 = fpIndex(q1.fOperand().REG_F().getText());
			} else if (q1.aOperand() != null) {
				c = OT_REG;
				v2 = aIndexFromToken(q1.aOperand().start);
			} else {
				c = OT_CONST;
				v2 = (int)parseIntLike(q1.cLiteral().getText());
			}

			CPUSim64v2Parser.QOperandContext q2 = ctx.qOperand(1);
			if (q2.fOperand() != null) {
				d = OT_FP;
				v3 = fpIndex(q2.fOperand().REG_F().getText());
			} else if (q2.aOperand() != null) {
				d = OT_REG;
				v3 = aIndexFromToken(q2.aOperand().start);
			} else {
				d = OT_CONST;
				v3 = (int)parseIntLike(q2.cLiteral().getText());
			}
			out.add(encType0(Opcode.MOVE.code, a, b, c, d, v0, v1, v2, v3));
		} else if (ctx.yOperand().size() == 2) {
			// YY
			CPUSim64v2Parser.YOperandContext y0 = ctx.yOperand(0);
			CPUSim64v2Parser.YOperandContext y1 = ctx.yOperand(1);

			a = (y0.fOperand() != null) ? OT_FP : OT_REG;
			v0 = (y0.fOperand() != null) ? fpIndex(y0.fOperand().REG_F().getText())
					: aIndexFromToken(y0.aOperand().start);

			b = (y1.fOperand() != null) ? OT_FP : OT_REG;
			v1 = (y1.fOperand() != null) ? fpIndex(y1.fOperand().REG_F().getText())
					: aIndexFromToken(y1.aOperand().start);
			out.add(encType0(Opcode.MOVE.code, a, b, c, d, v0, v1, v2, v3));
		} else if (ctx.yOperand().size() == 1 && ctx.cLiteral() != null) {
			// YC
			CPUSim64v2Parser.YOperandContext y = ctx.yOperand(0);
			a = (y.fOperand() != null) ? OT_FP : OT_REG;
			v0 = (y.fOperand() != null) ? fpIndex(y.fOperand().REG_F().getText())
					: aIndexFromToken(y.aOperand().start);
			b = OT_CONST;
			if (ctx.cLiteral().CHARLIT() != null)
				k =parseCharLiteral(ctx.cLiteral().CHARLIT().getText());
			else
				k =parseIntLike(ctx.cLiteral().getText());
			out.add(encType2RC2(Opcode.MOVE.code, a, v0, k));
		} else if (ctx.aOperand(0) != null && ctx.aOperand(1) != null && ctx.rOperand() != null) {
			// AAR
			a = OT_REG;
			v0 = aIndexFromToken(ctx.aOperand(0).start);
			b = OT_REG;
			v1 = aIndexFromToken(ctx.aOperand(1).start);
			c = OT_REG;
			v2 = regIndex(ctx.rOperand().REG_R().getText());
			out.add(encType0(Opcode.MOVE.code, a, b, c, d, v0, v1, v2, v3));
		} else if (ctx.aOperand(0) != null && ctx.aOperand(1) != null && ctx.aLiteral() != null) {
			// AAC
			a = OT_REG;
			v0 = aIndexFromToken(ctx.aOperand(0).start);
			b = OT_REG;
			v1 = aIndexFromToken(ctx.aOperand(1).start);
			c = OT_CONST;
			v2 = (int)parseIntLike(ctx.aLiteral().getText());
			out.add(encType3ZZC3(Opcode.MOVE.code, a, v0, b, v1, v2));
		} else if (ctx.aOperand().size() == 2 && ctx.aLiteral() != null) {
			// ACA
			a = OT_REG;
			v0 = aIndexFromToken(ctx.aOperand(0).start);
			c = OT_CONST;
			v2 = (int)parseIntLike(ctx.aLiteral().getText());
			b = OT_REG;
			v1 = aIndexFromToken(ctx.aOperand(1).start);
			out.add(encType3ZZC3(Opcode.MOVE.code, a, v0, b, v1, v2));
		} else {
			throw new AssemblerException("Unhandled MOVE form");
		}
		return null;
	}

	@Override
	public Void visitInstrLOAD(CPUSim64v2Parser.InstrLOADContext ctx) {
		var y = ctx.yOperand();
		boolean yIsFp = y.fOperand() != null;
		int aType = yIsFp ? OT_FP : OT_REG;
		int v0 = yIsFp ? fpIndex(y.fOperand().REG_F().getText())
				: aIndexFromToken(y.aOperand().start);

		int bType = OT_NONE, cType = OT_NONE, dType = OT_NONE;
		int v1 = 0, v2 = 0, v3 = 0;
		long k;

		if (ctx.memC() != null) {                  // Y[C]
			k =parseIntLike(ctx.memC().aLiteral().getText());
			out.add(encType2RC2(Opcode.LOAD.code, aType, v0, k));
		} else if (ctx.memA() != null) {           // Y[A]
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.memA().aOperand().start);
			out.add(encType0(Opcode.LOAD.code, aType, bType, cType, dType, v0, v1, v2, v3));
		} else if (ctx.memAplusC() != null) {      // Y[A+C]
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.memAplusC().aOperand().start);
			v2 = (int)parseIntLike(ctx.memAplusC().cLiteral().getText());
			out.add(encType3ZZC3(Opcode.LOAD.code, aType, v0, bType, v1, v2));
		} else if (ctx.memCplusA() != null) {      // Y[C+A]
			v1 = (int)parseIntLike(ctx.memCplusA().cLiteral().getText());
			cType = OT_REG;
			v2 = aIndexFromToken(ctx.memCplusA().aOperand().start);
			out.add(encType3ZZC3(Opcode.LOAD.code, aType, v0, cType, v2, v1));
		} else if (ctx.memCplusC() != null) {      // Y[C+C]
			long c1 = parseIntLike(ctx.memCplusC().aLiteral().getText());
			long c2 = parseIntLike(ctx.memCplusC().cLiteral().getText());
			bType = OT_CONST;
			if (fitsIn(c2, C0_BITS, true)) {
				v1 = (int)c2;
				out.add(encType3ZZC3(Opcode.LOAD.code, aType, v0, bType, v1, (int)c1));
			} else if (fitsIn(c1, C0_BITS, true)) {
				v1 = (int)c1;
				out.add(encType3ZZC3(Opcode.LOAD.code, aType, v0, bType, v1, (int)c2));
			} else {
				throw new AssemblerException("LOAD [C+C] requires one constant to fit in 12 bits");
			}
		} else if (ctx.memAplusR() != null) {      // Y[A+R]
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.memAplusR().aOperand().start);
			cType = OT_REG;
			v2 = regIndex(ctx.memAplusR().rOperand().REG_R().getText());
			out.add(encType0(Opcode.LOAD.code, aType, bType, cType, dType, v0, v1, v2, v3));
		} else {
			throw new AssemblerException("Unhandled LOAD form");
		}
		return null;
	}

	@Override
	public Void visitInstrSTORE(CPUSim64v2Parser.InstrSTOREContext ctx) {
		// Q , [addr] → Type-0
		int aType, v0;
		var q = ctx.qOperand();
		if (q.fOperand() != null) {
			aType = OT_FP;
			v0 = fpIndex(q.fOperand().REG_F().getText());
		} else if (q.aOperand() != null) {
			aType = OT_REG;
			v0 = aIndexFromToken(q.aOperand().start);
		} else {
			aType = OT_CONST;
			v0 = (int)parseIntLike(q.cLiteral().getText());
		}

		int bType = OT_NONE, cType = OT_NONE, dType = OT_NONE;
		int v1 = 0, v2 = 0, v3 = 0;
		long k;

		if (ctx.memC() != null) {                  // [C]
			k =parseIntLike(ctx.memC().aLiteral().getText());
			out.add(encType2RC2(Opcode.STORE.code, aType, v0, k));
		} else if (ctx.memA() != null) {           // [A]
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.memA().aOperand().start);
			out.add(encType0(Opcode.STORE.code, aType, bType, cType, dType, v0, v1, v2, v3));
		} else if (ctx.memAplusC() != null) {      // [A+C]
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.memAplusC().aOperand().start);
			v2 = (int)parseIntLike(ctx.memAplusC().cLiteral().getText());
			out.add(encType3ZZC3(Opcode.STORE.code, aType, v0, bType, v1, v2));
		} else if (ctx.memCplusA() != null) {      // [C+A]
			v1 = (int)parseIntLike(ctx.memCplusA().cLiteral().getText());
			cType = OT_REG;
			v2 = aIndexFromToken(ctx.memCplusA().aOperand().start);
			out.add(encType3ZZC3(Opcode.STORE.code, aType, v0, cType, v2, v1));
		} else if (ctx.memCplusC() != null) {      // [C+C]
			long c1 = (int)parseIntLike(ctx.memCplusC().aLiteral().getText());
			long c2 = (int)parseIntLike(ctx.memCplusC().cLiteral().getText());
			bType = OT_CONST;
			if (fitsIn(c2, C0_BITS, true)) {
				v1 = (int)c2;
				out.add(encType3ZZC3(Opcode.STORE.code, aType, v0, bType, v1, (int)c1));
			} else if (fitsIn(c1, C0_BITS, true)) {
				v1 = (int)c1;
				out.add(encType3ZZC3(Opcode.STORE.code, aType, v0, bType, v1, (int)c2));
			} else {
				throw new AssemblerException("LOAD [C+C] requires one constant to fit in 12 bits");
			}
		} else if (ctx.memAplusR() != null) {      // [A+R]
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.memAplusR().aOperand().start);
			cType = OT_REG;
			v2 = regIndex(ctx.memAplusR().rOperand().REG_R().getText());
			out.add(encType0(Opcode.STORE.code, aType, bType, cType, dType, v0, v1, v2, v3));
		} else {
			throw new AssemblerException("Unhandled LOAD form");
		}
		return null;
	}

	@Override
	public Void visitInstrPOP(CPUSim64v2Parser.InstrPOPContext ctx) {
		if (ctx.yOperand() == null) {
			out.add(encType0(Opcode.POP.code, OT_NONE, OT_NONE, OT_NONE, OT_NONE, 0, 0, 0, 0));
		} else {
			var y = ctx.yOperand();
			boolean isFp = y.fOperand() != null;
			int t = isFp ? OT_FP : OT_REG;
			int v = isFp ? fpIndex(y.fOperand().REG_F().getText())
					: aIndexFromToken(y.aOperand().start);
			out.add(encType0(Opcode.POP.code, t, OT_NONE, OT_NONE, OT_NONE, v, 0, 0, 0));
		}
		return null;
	}

	@Override
	public Void visitInstrPUSH(CPUSim64v2Parser.InstrPUSHContext ctx) {
		int aType, v0;
		if (ctx.yOperand() != null) {
			var y = ctx.yOperand();
			boolean isFp = y.fOperand() != null;
			aType = isFp ? OT_FP : OT_REG;
			v0 = isFp ? fpIndex(y.fOperand().REG_F().getText())
					: aIndexFromToken(y.aOperand().start);
			out.add(encType0(Opcode.PUSH.code, aType, OT_NONE, OT_NONE, OT_NONE, v0, 0, 0, 0));
		} else {
			long k =parseIntLike(ctx.cLiteral().getText());
			out.add(encType1C1(Opcode.PUSH.code, k));
		}
		return null;
	}

	@Override
	public Void visitInstrJUMP(CPUSim64v2Parser.InstrJUMPContext ctx) {
		visitInstrBRANCH(Opcode.JUMP, ctx.branchModes());
		return null;
	}

	@Override
	public Void visitInstrCALL(CPUSim64v2Parser.InstrCALLContext ctx) {
		visitInstrBRANCH(Opcode.CALL, ctx.branchModes());
		return null;
	}

	private Void visitInstrBRANCH(Opcode opc, CPUSim64v2Parser.BranchModesContext ctx) {
		// Non-conditional: JUMP A | JUMP C
		if (ctx.zCond() == null) {
			int aType, v0;
			if (ctx.aOperand() != null) {
				aType = OT_REG;
				v0 = aIndexFromToken(ctx.aOperand().start);
				out.add(encType0(opc.code, aType, OT_NONE, OT_NONE, OT_NONE, v0, 0, 0, 0));
			} else {
				long k =parseIntLike(ctx.cLiteral(0).getText());
				out.add(encType1C1(opc.code, k));
			}
			return null;
		}

		// Conditional forms: Z + (A | C | AC | CA | CC | AR)
		final int z = parseZSmall(ctx.zCond().getText());
		final int aType = OT_CONST; // op0 = Z (small)
		final int v0 = z;

		int bType = OT_NONE, v1 = 0;
		int imm28 = 0;
		long imm42 = 0;

		final boolean hasA = (ctx.aOperand() != null);
		final int nC = (ctx.cLiteral() == null) ? 0 : ctx.cLiteral().size();
		final boolean hasR = (ctx.rOperand() != null);

		if (hasA && nC == 0 && !hasR) {
			// ZA
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.aOperand().start);
			out.add(encType0(opc.code, aType, bType, OT_NONE, OT_NONE, v0, v1, 0, 0));
		} else if (!hasA && nC == 1 && !hasR) {
			// ZC
			imm42 =parseIntLike(ctx.cLiteral(0).getText());
			out.add(encType2RC2(opc.code, aType, v0, imm42));
		} else if (hasA && nC == 1 && !hasR) {
			// ZAC or ZCA → encode A in op1, C in imm28 (signed 28)
			bType = OT_REG;
			v1 = aIndexFromToken(ctx.aOperand().start);
			imm28 = (int)parseIntLike(ctx.cLiteral(0).getText());
			out.add(encType3ZZC3(opc.code, aType, v0, bType, v1, imm28));
		} else if (!hasA && nC == 2 && !hasR) {
			// ZCC → first C in op1 (12-bit), second C in imm28 (signed 28)
			long c1 = parseIntLike(ctx.cLiteral(0).getText());
			long c2 = parseIntLike(ctx.cLiteral(1).getText());
			if (fitsIn(c1, C0_BITS, true)) {
				bType = OT_CONST;
				v1 = (int)c1;
				imm28 = (int)c2;
			} else if (fitsIn(c2, C0_BITS, true)) {
				bType = OT_CONST;
				v1 = (int)c2;
				imm28 = (int)c1;
			} else {
				throw new AssemblerException("JUMP ZCC requires one constant to fit in 12 bits");
			}
			out.add(encType3ZZC3(opc.code, aType, v0, bType, v1, imm28));
		} else if (hasA && hasR) {
			// ZAR
			v1 = aIndexFromToken(ctx.aOperand().start);
			int v2 = regIndex(ctx.rOperand().REG_R().getText());
			out.add(encType0(opc.code, aType, OT_REG, OT_REG, OT_NONE, v0, v1, v2, 0));
		} else {
			throw new IllegalStateException("Unhandled JUMP conditional form");
		}
		return null;
	}

	@Override
	public Void visitInstrRETURN(CPUSim64v2Parser.InstrRETURNContext ctx) {
		long w = encType0(Opcode.RETURN.code, OT_NONE, OT_NONE, OT_NONE, OT_NONE, 0, 0, 0, 0);
		out.add(w);
		return null;
	}

	@Override
	public Void visitInstrINTERRUPT(CPUSim64v2Parser.InstrINTERRUPTContext ctx) {
		// Type-0: single short operand (R or C)
		int aType;
		long v0;
		if (ctx.rOperand() != null) {
			aType = OT_REG;
			v0 = regIndex(ctx.rOperand().REG_R().getText());
			out.add(encType0(Opcode.INTERRUPT.code, aType, OT_NONE, OT_NONE, OT_NONE, (int) v0, 0, 0, 0));
		} else {
			v0 = parseIntLike(ctx.cLiteral().getText());
			if (fitsIn(v0, C0_BITS, false)) {
				aType = OT_CONST;
				v0 = v0;
				out.add(encType0(Opcode.INTERRUPT.code, aType, OT_NONE, OT_NONE, OT_NONE, (int) v0, 0, 0, 0));
			} else {
				out.add(encType1C1(Opcode.INTERRUPT.code, v0));
			}
		}
		return null;
	}

	@Override
	public Void visitInstrSTOP(CPUSim64v2Parser.InstrSTOPContext ctx) {
		long w = encType0(Opcode.STOP.code, OT_NONE, OT_NONE, OT_NONE, OT_NONE, 0, 0, 0, 0);
		out.add(w);
		return null;
	}

	@Override
	public Void visitInstrNEG(CPUSim64v2Parser.InstrNEGContext ctx) {
		// instrNEG : NEG xOperand ;
		// xOperand : rOperand | fOperand ;

		int aType;
		int v0;

		// Prefer the explicit branch the parser chose
		if (ctx.xOperand().fOperand() != null) {
			aType = OT_FP;
			v0 = fpIndex(ctx.xOperand().fOperand().REG_F().getText());
		} else if (ctx.xOperand().rOperand() != null) {
			aType = OT_REG;
			v0 = regIndex(ctx.xOperand().rOperand().REG_R().getText());
		} else {
			// This should be unreachable with your xOperand rule
			throw new IllegalStateException("NEG: xOperand is neither rOperand nor fOperand");
		}

		out.add(encType0(Opcode.NEGATE.code, aType, OT_NONE, OT_NONE, OT_NONE, v0, 0, 0, 0
		));
		return null;
	}

/* ============================
   ARITHMETIC (ADD / SUB / MULT)
   ============================ */

	public Void visitArithmeticModes(Opcode opc, CPUSim64v2Parser.ArithmeticModesContext ctx) {
		int a = OT_NONE, b = OT_NONE, c = OT_NONE, d = OT_NONE;
		int v0 = 0, v1 = 0, v2 = 0, v3 = 0;
		long k = 0;

		// How many of each did this alt produce?
		final int nA = (ctx.aOperand() == null) ? 0 : ctx.aOperand().size();
		final int nF = (ctx.fOperand() == null) ? 0 : ctx.fOperand().size();
		final int nR = (ctx.rOperand() == null) ? 0 : 1;
		final int nX = (ctx.xOperand() == null) ? 0 : 1;
		final int nY = (ctx.yOperand() == null) ? 0 : 1;
		final int nC = (ctx.cLiteral() == null) ? 0 : 1;

		// AR
		if (nA == 1 && nR == 1 && nF == 0 && nX == 0 && nY == 0 && nC == 0) {
			a = OT_REG;
			v0 = aIndexFromToken(ctx.aOperand(0).start);
			b = OT_REG;
			v1 = regIndex(ctx.rOperand().getStart().getText());
			out.add(encType0(opc.code, a, b, c, d, v0, v1, v2, v3));
		}
		// FX  (F , X)  where X is R|F
		else if (nF == 1 && nX == 1) {
			a = OT_FP;
			v0 = fpIndex(ctx.fOperand(0).getStart().getText());
			if (ctx.xOperand().fOperand() != null) {
				b = OT_FP;
				v1 = fpIndex(ctx.xOperand().fOperand().getStart().getText());
			} else {
				b = OT_REG;
				v1 = regIndex(ctx.xOperand().rOperand().getStart().getText());
			}
			out.add(encType0(opc.code, a, b, c, d, v0, v1, v2, v3));
		}
		// YC  (Y , C)  where Y is A|F
		else if (nY == 1 && nC == 1) {
			boolean yIsFp = (ctx.yOperand().fOperand() != null);
			a = yIsFp ? OT_FP : OT_REG;
			v0 = yIsFp
					? fpIndex(ctx.yOperand().fOperand().getStart().getText())
					: aIndexFromToken(ctx.yOperand().aOperand().start);
			k =parseIntLike(ctx.cLiteral().getText());
			out.add(encType2RC2(opc.code, a, v0, k));
		}
		// AAR (A, A, R)
		else if (nA == 2 && nR == 1 && nC == 0 && nF <= 1 && nX == 0) {
			a = OT_REG;
			v0 = aIndexFromToken(ctx.aOperand(0).start);
			b = OT_REG;
			v1 = aIndexFromToken(ctx.aOperand(1).start);
			c = OT_REG;
			v2 = regIndex(ctx.rOperand().getStart().getText());
			out.add(encType0(opc.code, a, b, c, d, v0, v1, v2, v3));
		}
		// FFX (F, F, X)
		else if (nF == 2 && nX == 1) {
			a = OT_FP;
			v0 = fpIndex(ctx.fOperand(0).getStart().getText());
			b = OT_FP;
			v1 = fpIndex(ctx.fOperand(1).getStart().getText());
			if (ctx.xOperand().fOperand() != null) {
				c = OT_FP;
				v2 = fpIndex(ctx.xOperand().fOperand().getStart().getText());
			} else {
				c = OT_REG;
				v2 = regIndex(ctx.xOperand().rOperand().getStart().getText());
			}
			out.add(encType0(opc.code, a, b, c, d, v0, v1, v2, v3));
		}
		// AAC vs ACA  (both have nA==2 and nC==1) → decide by token order
		else if (nA == 2 && nC == 1 && nF == 0 && nX == 0) {
			a = OT_REG;
			v0 = aIndexFromToken(ctx.aOperand(0).start);
			b = OT_REG;
			v1 = aIndexFromToken(ctx.aOperand(1).start);
			v2 = (int)parseIntLike(ctx.cLiteral().getText());
			out.add(encType3ZZC3(opc.code, a, v0, b, v1, v2));
		}
		// FFC vs FCF (nF==2, nC==1)
		else if (nF == 2 && nC == 1 && nX == 0) {
			a = OT_FP;
			v0 = fpIndex(ctx.fOperand(0).getStart().getText());
			b = OT_FP;
			v1 = fpIndex(ctx.fOperand(1).getStart().getText());
			v2 = (int)parseIntLike(ctx.cLiteral().getText());
			out.add(encType3ZZC3(opc.code, a, v0, b, v1, v2));
		} else {
			throw new AssemblerException("Unhandled " + opc.name() + " form");
		}
		return null;
	}

	public Void visitInstrADD(CPUSim64v2Parser.InstrADDContext ctx) {
		visitArithmeticModes(Opcode.ADD, ctx.arithmeticModes());
		return null;
	}

	public Void visitInstrSUB(CPUSim64v2Parser.InstrSUBContext ctx) {
		visitArithmeticModes(Opcode.SUBTRACT, ctx.arithmeticModes());
		return null;
	}

	public Void visitInstrMULT(CPUSim64v2Parser.InstrMULTContext ctx) {
		visitArithmeticModes(Opcode.MULTIPLY, ctx.arithmeticModes());
		return null;
	}

	@Override
	public Void visitInstrDIV(CPUSim64v2Parser.InstrDIVContext ctx) {
		int a = OT_NONE, b = OT_NONE, c = OT_NONE, d = OT_NONE;
		int v0 = 0, v1 = 0, v2 = 0, v3 = 0;

		final int nR = (ctx.rOperand() == null) ? 0 : ctx.rOperand().size();
		final int nC = (ctx.cLiteral() == null) ? 0 : 1;

		// 10) RRRR
		if (nR == 4 && nC == 0) {
			a = OT_REG;
			v0 = regIndex(ctx.rOperand(0).getStart().getText());
			b = OT_REG;
			v1 = regIndex(ctx.rOperand(1).getStart().getText());
			c = OT_REG;
			v2 = regIndex(ctx.rOperand(2).getStart().getText());
			d = OT_REG;
			v3 = regIndex(ctx.rOperand(3).getStart().getText());
			out.add(encType0(Opcode.DIVIDE.code, a, b, c, d, v0, v1, v2, v3)); // use Opcode.DIV if that's your enum
		}
		// 11) RRRC (R, R, R, C)
		else if (nR == 3 && nC == 1) {
			a = OT_REG;
			v0 = regIndex(ctx.rOperand(0).getStart().getText());
			b = OT_REG;
			v1 = regIndex(ctx.rOperand(1).getStart().getText());
			c = OT_REG;
			v2 = regIndex(ctx.rOperand(2).getStart().getText());
			d = OT_CONST;
			v3 = (int)parseIntLike(ctx.cLiteral().getText());
			out.add(encType0(Opcode.DIVIDE.code, a, b, c, d, v0, v1, v2, v3)); // use Opcode.DIV if that's your enum
		} else {
			visitArithmeticModes(Opcode.DIVIDE, ctx.arithmeticModes());
		}
		return null;
	}

	@Override
	public Void visitInstrRECIP(CPUSim64v2Parser.InstrRECIPContext ctx) {
		// RECIP R
		int v0 = fpIndex(ctx.fOperand().REG_F().getText());
		out.add(encType0(Opcode.RECIP.code, OT_FP, OT_NONE, OT_NONE, OT_NONE,
				v0, 0, 0, 0));
		return null;
	}

	/* ============================
   LOGICAL (COMPL / AND / OR / XOR / TEST / CMP)
   ============================ */

	@Override
	public Void visitInstrCOMPL(CPUSim64v2Parser.InstrCOMPLContext ctx) {
		// COMPL R
		int v0 = regIndex(ctx.rOperand().REG_R().getText());
		out.add(encType0(Opcode.COMPL.code, OT_REG, OT_NONE, OT_NONE, OT_NONE,
				v0, 0, 0, 0));
		return null;
	}

	public Void visitLogicModes(Opcode opc, CPUSim64v2Parser.LogicModesContext ctx) {
		// RR | RC | RRR | RRC
		int a = OT_REG, v0 = regIndex(ctx.rOperand(0).REG_R().getText());
		int b, v1, c = OT_NONE, v2 = 0;
		long k;

		if (ctx.rOperand().size() == 2 && ctx.cLiteral() == null) {
			// RR
			b = OT_REG;
			v1 = regIndex(ctx.rOperand(1).REG_R().getText());
			out.add(encType0(opc.code, a, b, c, OT_NONE, v0, v1, v2, 0));
		} else if (ctx.rOperand().size() == 1 && ctx.cLiteral() != null) {
			// RC
			b = OT_CONST;
			k =parseIntLike(ctx.cLiteral().getText());
			out.add(encType2RC2(opc.code, a, v0, k));
		} else if (ctx.rOperand().size() == 3 && ctx.cLiteral() == null) {
			// RRR
			b = OT_REG;
			v1 = regIndex(ctx.rOperand(1).REG_R().getText());
			c = OT_REG;
			v2 = regIndex(ctx.rOperand(2).REG_R().getText());
			out.add(encType0(opc.code, a, b, c, OT_NONE, v0, v1, v2, 0));
		} else {
			// RRC
			b = OT_REG;
			v1 = regIndex(ctx.rOperand(1).REG_R().getText());
			v2 = (int)parseIntLike(ctx.cLiteral().getText());
			out.add(encType3ZZC3(opc.code, a, v0, b, v1, v2));
		}
		return null;
	}

	@Override
	public Void visitInstrAND(CPUSim64v2Parser.InstrANDContext ctx) {
		visitLogicModes(Opcode.AND, ctx.logicModes());
		return null;
	}

	@Override
	public Void visitInstrOR(CPUSim64v2Parser.InstrORContext ctx) {
		visitLogicModes(Opcode.OR, ctx.logicModes());
		return null;
	}

	@Override
	public Void visitInstrXOR(CPUSim64v2Parser.InstrXORContext ctx) {
		visitLogicModes(Opcode.XOR, ctx.logicModes());
		return null;
	}

	@Override
	public Void visitInstrTEST(CPUSim64v2Parser.InstrTESTContext ctx) {
		// TEST X
		int a = 0, v0 = 0;
		var x = ctx.xOperand();
		if (x.fOperand()!=null) { a=OT_FP; v0=fpIndex(x.fOperand().REG_F().getText()); }
		else if (x.rOperand()!=null) { a=OT_REG; v0=regIndex(x.rOperand().REG_R().getText()); }
		out.add(encType0(Opcode.TEST.code, a, OT_NONE, OT_NONE, OT_NONE,
				v0, 0, 0, 0));
		return null;
	}

	@Override
	public Void visitInstrCMP(CPUSim64v2Parser.InstrCMPContext ctx) {
		// AA | AC | FF
		int a,b, v0, v1;
		long k;

		if (ctx.aOperand(0)!=null && ctx.aOperand(1)!=null) {
			// AA
			a=OT_REG; v0=aIndexFromToken(ctx.aOperand(0).start);
			b=OT_REG; v1=aIndexFromToken(ctx.aOperand(1).start);
			out.add(encType0(Opcode.CMP.code, a,b, OT_NONE, OT_NONE, v0, v1, 0, 0));
		} else if (ctx.aOperand()!=null && ctx.cLiteral()!=null) {
			// AC
			a=OT_REG;   v0=aIndexFromToken(ctx.aOperand(0).start);
			k =parseIntLike(ctx.cLiteral().getText());
			out.add(encType2RC2(Opcode.CMP.code, a, v0, k));
		} else {
			// FF
			a=OT_FP; v0=fpIndex(ctx.fOperand(0).REG_F().getText());
			b=OT_FP; v1=fpIndex(ctx.fOperand(1).REG_F().getText());
			out.add(encType0(Opcode.CMP.code, a,b, OT_NONE, OT_NONE, v0, v1, 0, 0));
		}

		return null;
	}

/* ============================
   SHIFTS / ROTATES (LSHIFT / RSHIFT / ARSHIFT / LROTATE / RROTATE)
   ============================ */

	@Override
	public Void visitInstrLSHIFT(CPUSim64v2Parser.InstrLSHIFTContext ctx) {
		visitLogicModes(Opcode.LSHIFT, ctx.logicModes());
		return null;
	}

	@Override
	public Void visitInstrRSHIFT(CPUSim64v2Parser.InstrRSHIFTContext ctx) {
		visitLogicModes(Opcode.RSHIFT, ctx.logicModes());
		return null;
	}

	@Override
	public Void visitInstrARSHIFT(CPUSim64v2Parser.InstrARSHIFTContext ctx) {
		visitLogicModes(Opcode.ARSHIFT, ctx.logicModes());
		return null;
	}

	@Override
	public Void visitInstrLROTATE(CPUSim64v2Parser.InstrLROTATEContext ctx) {
		visitLogicModes(Opcode.LROTATE, ctx.logicModes());
		return null;
	}

	@Override
	public Void visitInstrRROTATE(CPUSim64v2Parser.InstrRROTATEContext ctx) {
		visitLogicModes(Opcode.RROTATE, ctx.logicModes());
		return null;
	}

   /* ============================
   PACK / PACK64 / UNPACK / UNPACK64
   ============================ */

	@Override
	public Void visitInstrPACK(CPUSim64v2Parser.InstrPACKContext ctx) {
		// RR | RRR | RRRR    (all R-only)
		int a=OT_REG, v0=regIndex(ctx.rOperand(0).REG_R().getText());
		int b=OT_NONE,c=OT_NONE,d=OT_NONE, v1=0,v2=0,v3=0;
		if (ctx.rOperand().size() >= 2) { b=OT_REG; v1=regIndex(ctx.rOperand(1).REG_R().getText()); }
		if (ctx.rOperand().size() >= 3) { c=OT_REG; v2=regIndex(ctx.rOperand(2).REG_R().getText()); }
		if (ctx.rOperand().size() >= 4) { d=OT_REG; v3=regIndex(ctx.rOperand(3).REG_R().getText()); }
		out.add(encType0(Opcode.PACK.code, a,b,c,d, v0, v1, v2, v3));
		return null;
	}

	@Override
	public Void visitInstrPACK64(CPUSim64v2Parser.InstrPACK64Context ctx) {
		int a=OT_REG, v0=regIndex(ctx.rOperand(0).REG_R().getText());
		int b=OT_NONE,c=OT_NONE,d=OT_NONE, v1=0,v2=0,v3=0;
		if (ctx.rOperand().size() >= 2) { b=OT_REG; v1=regIndex(ctx.rOperand(1).REG_R().getText()); }
		if (ctx.rOperand().size() >= 3) { c=OT_REG; v2=regIndex(ctx.rOperand(2).REG_R().getText()); }
		if (ctx.rOperand().size() >= 4) { d=OT_REG; v3=regIndex(ctx.rOperand(3).REG_R().getText()); }
		out.add(encType0(Opcode.PACK64.code, a,b,c,d, v0, v1, v2, v3));
		return null;
	}

	@Override
	public Void visitInstrUNPACK(CPUSim64v2Parser.InstrUNPACKContext ctx) {
		// RR | RRRR
		int a=OT_REG, v0=regIndex(ctx.rOperand(0).REG_R().getText());
		int b=OT_NONE,c=OT_NONE,d=OT_NONE, v1=0,v2=0,v3=0;
		if (ctx.rOperand().size() >= 2) { b=OT_REG; v1=regIndex(ctx.rOperand(1).REG_R().getText()); }
		if (ctx.rOperand().size() >= 3) { c=OT_REG; v2=regIndex(ctx.rOperand(2).REG_R().getText()); }
		if (ctx.rOperand().size() >= 4) { d=OT_REG; v3=regIndex(ctx.rOperand(3).REG_R().getText()); }
		out.add(encType0(Opcode.UNPACK.code, a,b,c,d, v0, v1, v2, v3));
		return null;
	}

	@Override
	public Void visitInstrUNPACK64(CPUSim64v2Parser.InstrUNPACK64Context ctx) {
		// RR | RRRR
		int a=OT_REG, v0=regIndex(ctx.rOperand(0).REG_R().getText());
		int b=OT_NONE,c=OT_NONE,d=OT_NONE, v1=0,v2=0,v3=0;
		if (ctx.rOperand().size() >= 2) { b=OT_REG; v1=regIndex(ctx.rOperand(1).REG_R().getText()); }
		if (ctx.rOperand().size() >= 3) { c=OT_REG; v2=regIndex(ctx.rOperand(2).REG_R().getText()); }
		if (ctx.rOperand().size() >= 4) { d=OT_REG; v3=regIndex(ctx.rOperand(3).REG_R().getText()); }
		out.add(encType0(Opcode.UNPACK64.code, a,b,c,d, v0, v1, v2, v3));
		return null;
	}

	/* ============================
   CAS (RRAO / CCAO / RCAO / CRAO)
   ============================ */

	@Override
	public Void visitInstrCAS(CPUSim64v2Parser.InstrCASContext ctx) {
		int a=OT_NONE,b=OT_NONE,c=OT_NONE,d=OT_NONE;
		int v0=0,v1=0,v2=0,v3=0;

//		ctx.children.forEach(ch -> System.out.println("  child: " + ch.getText() + " (" + ch.getClass().getSimpleName() + ")"));
		if (ctx.children.size()<4) throw new AssemblerException("CAS missing operands");
		if (ctx.getChild(1) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext) {
			// RRAO
			a = OT_REG; b = OT_REG;
			v0 = regIndex(ctx.rOperand(0).REG_R().getText());
			v1 = regIndex(ctx.rOperand(1).REG_R().getText());
		} else if (ctx.getChild(1) instanceof CPUSim64v2Parser.CLiteralContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.CLiteralContext) {
			// CCAO
			a = OT_CONST; b = OT_CONST;
			v0 = (int)parseIntLike(ctx.cLiteral(0).getText());
			v1 = (int)parseIntLike(ctx.cLiteral(1).getText());
		} else if (ctx.getChild(1) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.CLiteralContext) {
			// RCAO
			a = OT_REG; b = OT_CONST;
			v0 = regIndex(ctx.rOperand(0).REG_R().getText());
			v1 = (int)parseIntLike(ctx.cLiteral(0).getText());
		} else if (ctx.getChild(1) instanceof CPUSim64v2Parser.CLiteralContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext) {
			// CRAO
			a = OT_CONST; b = OT_REG;
			v0 = (int)parseIntLike(ctx.cLiteral(0).getText());
			v1 = (int)regIndex(ctx.rOperand(0).REG_R().getText());
		} else {
			throw new AssemblerException("CAS invalid operand types");
		}

		// op2: A (address-capable)
		c=OT_REG; v2=aIndexFromToken(ctx.aOperand().start);

		// op3: O (R or C)
		if (ctx.oOperand().rOperand()!=null) {
			d=OT_REG;
			v3=regIndex(ctx.oOperand().rOperand().REG_R().getText());
		} else{
			d=OT_CONST;
			v3=(int)parseIntLike(ctx.oOperand().cLiteral().getText());
		}
		out.add(encType0(Opcode.CAS.code, a,b,c,d, v0, v1, v2, v3));
		return null;
	}

	/* ============================
   ENDIAN (RR / RC / CR / CC)
   ============================ */

	@Override
	public Void visitInstrENDIAN(CPUSim64v2Parser.InstrENDIANContext ctx) {
		int a=OT_NONE,b=OT_NONE;
		int v0=0,v1=0;

		if (ctx.children.size()<2) throw new AssemblerException("ENDIAN missing operands");
		if (ctx.getChild(1) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext) {
			// RR
			a = OT_REG; b = OT_REG;
			v0 = regIndex(ctx.rOperand(0).REG_R().getText());
			v1 = regIndex(ctx.rOperand(1).REG_R().getText());
		} else if (ctx.getChild(1) instanceof CPUSim64v2Parser.ZPortContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.ZPortContext) {
			// ZZ
			a = OT_CONST; b = OT_CONST;
			v0 = (int)parseIntLike(ctx.zPort(0).getText());
			v1 = (int)parseIntLike(ctx.zPort(1).getText());
		} else if (ctx.getChild(1) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.ZPortContext) {
			// RZ
			a = OT_REG; b = OT_CONST;
			v0 = (int)regIndex(ctx.rOperand(0).REG_R().getText());
			v1 = (int)parseIntLike(ctx.zPort(0).getText());
		} else if (ctx.getChild(1) instanceof CPUSim64v2Parser.ZPortContext &&
				ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext) {
			// ZR
			a = OT_CONST; b = OT_REG;
			v0 = (int)parseIntLike(ctx.zPort(0).getText());
			v1 = regIndex(ctx.rOperand(0).REG_R().getText());
		} else {
			throw new AssemblerException("ENDIAN invalid operand types");
		}

		out.add(encType0(Opcode.ENDIAN.code, a,b,OT_NONE,OT_NONE, v0, v1, 0, 0));
		return null;
	}

	// IN / OUT (I/O)
	@Override
	public Void visitInstrIN(CPUSim64v2Parser.InstrINContext ctx) {
		int a=OT_NONE,b=OT_NONE,c=OT_NONE,d=OT_NONE;
		int v0=0,v1=0,v2=0,v3=0;

		// X
		boolean isFP = (ctx.xOperand().fOperand()!=null);
		if (isFP) { a=OT_FP; v0=fpIndex(ctx.xOperand().fOperand().REG_F().getText()); }
		else      { a=OT_REG; v0=regIndex(ctx.xOperand().rOperand().REG_R().getText()); }

		if (ctx.children.size()<3) throw new AssemblerException("IN missing operands");
		if (ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ROperandContext) {
			// XRR
			b = OT_REG; c = OT_REG;
			v1 = regIndex(ctx.rOperand(0).REG_R().getText());
			v2 = regIndex(ctx.rOperand(1).REG_R().getText());
		} else if (ctx.getChild(3) instanceof CPUSim64v2Parser.ZPortContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ZPortContext) {
			// XZZ
			b = OT_CONST; c = OT_CONST;
			v1 = (int)parseIntLike(ctx.zPort(0).getText());
			v2 = (int)parseIntLike(ctx.zPort(1).getText());
		} else if (ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ZPortContext) {
			// XRZ
			b = OT_REG; c = OT_CONST;
			v1 = regIndex(ctx.rOperand(0).REG_R().getText());
			v2 = (int)parseIntLike(ctx.zPort(0).getText());
		} else if (ctx.getChild(3) instanceof CPUSim64v2Parser.ZPortContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ROperandContext) {
			// XZR
			b = OT_CONST; c = OT_REG;
			v1 = (int)parseIntLike(ctx.zPort(0).getText());
			v2 = regIndex(ctx.rOperand(0).REG_R().getText());
		} else {
			throw new AssemblerException("IN invalid operand types");
		}

		out.add(encType0(Opcode.IN.code, a,b,c,d, v0, v1, v2, v3));
		return null;
	}

	@Override
	public Void visitInstrOUT(CPUSim64v2Parser.InstrOUTContext ctx) {
		int a=OT_NONE,b=OT_NONE,c=OT_NONE,d=OT_NONE;
		int v0=0,v1=0,v2=0,v3=0;

		// Q
		boolean isFP = (ctx.qOperand().fOperand()!=null);
		boolean isREG = (ctx.qOperand().aOperand()!=null);
		if (isFP) { a=OT_FP; v0=fpIndex(ctx.qOperand().fOperand().REG_F().getText()); }
		else if (isREG) {
			a = OT_REG;
			v0 = aIndexFromToken(ctx.qOperand().aOperand().start);
		} else {
			a = OT_CONST;
			v0 = (int)parseIntLike(ctx.qOperand().cLiteral().getText());
		}

		if (ctx.children.size()<3) throw new AssemblerException("IN missing operands");
		if (ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ROperandContext) {
			// XRR
			b = OT_REG; c = OT_REG;
			v1 = regIndex(ctx.rOperand(0).REG_R().getText());
			v2 = regIndex(ctx.rOperand(1).REG_R().getText());
		} else if (ctx.getChild(3) instanceof CPUSim64v2Parser.ZPortContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ZPortContext) {
			// XZZ
			b = OT_CONST; c = OT_CONST;
			v1 = (int)parseIntLike(ctx.zPort(0).getText());
			v2 = (int)parseIntLike(ctx.zPort(1).getText());
		} else if (ctx.getChild(3) instanceof CPUSim64v2Parser.ROperandContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ZPortContext) {
			// XRZ
			b = OT_REG; c = OT_CONST;
			v1 = regIndex(ctx.rOperand(0).REG_R().getText());
			v2 = (int)parseIntLike(ctx.zPort(0).getText());
		} else if (ctx.getChild(3) instanceof CPUSim64v2Parser.ZPortContext &&
				ctx.getChild(5) instanceof CPUSim64v2Parser.ROperandContext) {
			// XZR
			b = OT_CONST; c = OT_REG;
			v1 = (int)parseIntLike(ctx.zPort(0).getText());
			v2 = regIndex(ctx.rOperand(0).REG_R().getText());
		} else {
			throw new AssemblerException("OUT invalid operand types");
		}

		out.add(encType0(Opcode.OUT.code, a,b,c,d, v0, v1, v2, v3));
		return null;
	}

	@Override
	public Void visitInstrSAVE(CPUSim64v2Parser.InstrSAVEContext ctx) {
		if (ctx.xOperand() != null) {
			boolean isFp = (ctx.xOperand(0).fOperand() != null);
			int a = isFp ? OT_FP : OT_REG;   // Y = A or F; encode A (R/SF/SP/PC) as REG here with A-index
			int v0 = isFp ? fpIndex(ctx.xOperand(0).fOperand().REG_F().getText())
					: regIndex(ctx.xOperand(0).rOperand().REG_R().getText());
			int b, v1;
			if (ctx.xOperand().size() >= 2) {
				if (isFp != (ctx.xOperand(1).fOperand() != null)) {
					throw new AssemblerException("SAVE operands must be the same type");
				}
				b = isFp ? OT_FP : OT_REG;   // Y = A or F; encode A (R/SF/SP/PC) as REG here with A-index
				v1 = isFp ? fpIndex(ctx.xOperand(1).fOperand().REG_F().getText())
						: regIndex(ctx.xOperand(1).rOperand().REG_R().getText());
			} else {
				b = a;
				v1 = isFp ? 31 : 28;
			}
			if (v0 > v1)
				throw new AssemblerException("SAVE first operand must be less than second");
			out.add(encType0(Opcode.SAVE.code, a, b, 0, 0, v0, v1, 0, 0));
			return null;
		}
		throw new AssemblerException("SAVE invalid operand types");
	}

	@Override
	public Void visitInstrRESTORE(CPUSim64v2Parser.InstrRESTOREContext ctx) {
		if (ctx.xOperand() != null) {
			boolean isFp = (ctx.xOperand(0).fOperand() != null);
			int a = isFp ? OT_FP : OT_REG;   // Y = A or F; encode A (R/SF/SP/PC) as REG here with A-index
			int v0 = isFp ? fpIndex(ctx.xOperand(0).fOperand().REG_F().getText())
					: regIndex(ctx.xOperand(0).rOperand().REG_R().getText());
			int b, v1;
			if (ctx.xOperand().size() >= 2) {
				if (isFp != (ctx.xOperand(1).fOperand() != null)) {
					throw new AssemblerException("RESTORE operands must be the same type");
				}
				b = isFp ? OT_FP : OT_REG;   // Y = A or F; encode A (R/SF/SP/PC) as REG here with A-index
				v1 = isFp ? fpIndex(ctx.xOperand(1).fOperand().REG_F().getText())
						: regIndex(ctx.xOperand(1).rOperand().REG_R().getText());
			} else {
				b = a;
				v1 = isFp ? 31 : 28;
			}
			if (v0 > v1)
				throw new AssemblerException("RESTORE first operand must be less than second");
			out.add(encType0(Opcode.RESTORE.code, a, b, 0, 0, v0, v1, 0, 0));
			return null;
		}
		throw new AssemblerException("RESTORE invalid operand types");
	}

	public Void visitData_Directive(CPUSim64v2Parser.Data_DirectiveContext ddctx) {
		CPUSim64v2Parser.DataDirectiveContext ctx = ddctx.dataDirective();
		if (ctx.DCI() != null) {
			if (ctx.INTLIT() != null) {
				out.add(parseIntLike(ctx.INTLIT().getText()));
			} else if (ctx.HEXLIT() != null) {
				out.add(parseIntLike(ctx.HEXLIT().getText()));
			} else {
				throw new IllegalStateException("Invalid DCI literal");
			}
		} else if (ctx.DCF() != null) {
			if (ctx.FLOATLIT() != null) {
				out.add(Double.doubleToRawLongBits(Double.parseDouble(ctx.FLOATLIT().getText())));
			} else {
				throw new IllegalStateException("Invalid DCF literal");
			}
		} else if (ctx.DCA() != null) {
			long b = 0;
			if (ctx.INTLIT() != null) {
				b = parseIntLike(ctx.INTLIT().getText());
			} else if (ctx.HEXLIT() != null) {
				b = parseIntLike(ctx.HEXLIT().getText());
			}
			if (b != 0) {
				out.add(b);
				for (int i = 0; i < b; ++i)
					out.add(0L);
			}
		} else if (ctx.DCB() != null) {
			if (ctx.byteList() != null) {
				out.add((long) ctx.byteList().bLiteral().size());
				long buffer = 0;
				int index = 7;
				for (var bctx : ctx.byteList().bLiteral()) {
					long b = 0;
					if (bctx.INTLIT() != null) {
						b = parseIntLike(bctx.INTLIT().getText());
					} else if (bctx.HEXLIT() != null) {
						b = parseIntLike(bctx.HEXLIT().getText());
					} else if (bctx.CHARLIT() != null) {
						String s = bctx.CHARLIT().getText();
						b = parseCharLiteral(s);
					} else {
						throw new IllegalStateException("Invalid DCB byte literal");
					}
					buffer |= (b & 0xFFL) << (index * 8);
					--index;
					if (index < 0) {
						out.add(buffer);
						buffer = 0;
						index = 7;
					}
				}
				if (index != 7) {
					out.add(buffer);
				}
			} else {
				throw new IllegalStateException("Invalid DCB directive");
			}
		} else if (ctx.DCC() != null) {
			if (ctx.byteList() != null) {
				out.add((long) ctx.byteList().bLiteral().size());
				long buffer = 0;
				int index = 3;
				for (var bctx : ctx.byteList().bLiteral()) {
					long b = 0;
					if (bctx.INTLIT() != null) {
						b = parseIntLike(bctx.INTLIT().getText());
					} else if (bctx.HEXLIT() != null) {
						b = parseIntLike(bctx.HEXLIT().getText());
					} else if (bctx.CHARLIT() != null) {
						String s = bctx.CHARLIT().getText();
						b = parseCharLiteral(s);
					} else {
						throw new IllegalStateException("Invalid DCC char literal");
					}
					buffer |= (b & 0xFFFFL) << (index * 16);
					--index;
					if (index < 0) {
						out.add(buffer);
						buffer = 0;
						index = 3;
					}
				}
				if (index != 3) {
					out.add(buffer);
				}
			} else {
				throw new IllegalStateException("Invalid DCC directive");
			}
		} else if (ctx.DCW() != null) {
			if (ctx.intList() != null) {
				out.add((long) ctx.intList().kLiteral().size());
				for (var kctx : ctx.intList().kLiteral()) {
					long k = 0;
					if (kctx.INTLIT() != null) {
						k = parseIntLike(kctx.INTLIT().getText());
					} else if (kctx.HEXLIT() != null) {
						k = parseIntLike(kctx.HEXLIT().getText());
					} else {
						throw new IllegalStateException("Invalid DCW literal");
					}
					out.add(k);
				}
			} else if (ctx.floatList() != null) {
				out.add((long) ctx.floatList().FLOATLIT().size());
				for (var fctx : ctx.floatList().FLOATLIT()) {
					out.add(Double.doubleToRawLongBits(Double.parseDouble(fctx.getText())));
				}
			} else if (ctx.charList() != null) {
				out.add((long) ctx.charList().CHARLIT().size());
				for (var kctx : ctx.charList().CHARLIT()) {
					String s = kctx.getText();
					long k = parseCharLiteral(s);
					out.add(k);
				}
			} else {
				throw new IllegalStateException("Invalid DCW directive");
			}
		} else if (ctx.DCS() != null) {
			if (ctx.STRINGLIT() == null) throw new IllegalStateException("Invalid DCS directive");
			String s = ctx.STRINGLIT().getText();
			if (s != null && s.length() >= 2) {
				s = s.substring(1, s.length() - 1);
				s = StringEscapeUtils.unescapeJava(s);
				byte[] utf8 = s.getBytes(StandardCharsets.UTF_8);
				out.add((long)utf8.length);
				long buffer = 0;
				int index = 7;
				for (var c : utf8) {
					buffer |= (c & 0xFFL) << (index * 8);
					--index;
					if (index < 0) {
						out.add(buffer);
						buffer = 0;
						index = 7;
					}
				}
				if (index != 7) {
					out.add(buffer);
				}
			} else {
				throw new IllegalStateException("Invalid DCS directive");
			}
		}
		return null;
	}

	@Override
	public Void visitORG_Directive(CPUSim64v2Parser.ORG_DirectiveContext ctx) {
		long currentAddress = 0;
		if (ctx.INTLIT() != null) {
			currentAddress = Long.parseLong(ctx.INTLIT().getText());
		} else if (ctx.HEXLIT() != null) {
			currentAddress = Long.parseLong(ctx.HEXLIT().getText().substring(2), 16);
		} else {
			throw new AssemblerException(getLocation() + ": Error: Missing integer literal for .ORG directive");
		}
		currentAddress = Math.max(out.size(), currentAddress); // prevent negative addresses
		for (int i = out.size(); i < currentAddress; ++i) {
			out.add(0L);
		}
		return null;
	}

	@Override
	public Void visitLINE_Directive(CPUSim64v2Parser.LINE_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		pauseLineIncrement = false;
		return null;
	}

	@Override
	public Void visitLINE_BEGIN_Directive(CPUSim64v2Parser.LINE_BEGIN_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		pauseLineIncrement = true;
		return null;
	}

	@Override
	public Void visitLINE_END_Directive(CPUSim64v2Parser.LINE_END_DirectiveContext ctx) {
		pauseLineIncrement = false;
		return null;
	}

	@Override
	public Void visitBLOCK_BEGIN_Directive(CPUSim64v2Parser.BLOCK_BEGIN_DirectiveContext ctx) {
		String blockname = null;
		if (ctx.IDENT() != null) {
			blockname = ctx.IDENT().getText();
			if (blockname.contains("$"))
				throw new AssemblerException("Blocknames can not contain $: " + blockname);
		} else if (ctx.BLOCK_IDENT() != null) {
			blockname = ctx.BLOCK_IDENT().getText();
		}
		if (blockname == null)
			throw new AssemblerException(".block directive must have an argument@");
		if (blockname.contains("{}") || blockname.contains("%d") || blockname.contains("%x"))
			blockname = String.format(blockname.replace("{}", "%04x"), ++blockCount);
		blockNames.push(blockname);
		return null;
	}

	@Override
	public Void visitBLOCK_END_Directive(CPUSim64v2Parser.BLOCK_END_DirectiveContext ctx) {
		String name = blockNames.pop();
		return null;
	}

	public void assemble(String src) {
		CharStream input = CharStreams.fromString(src);
		var lex = new cloud.lesh.CPUSim64v2.CPUSim64v2Lexer(input);
		var lexerListener = new CollectingErrorListener(errors, null);
		lex.removeErrorListeners();                // remove ConsoleErrorListener
		lex.addErrorListener(lexerListener);       // collect lexer errors
		CommonTokenStream toks = new CommonTokenStream(lex);
//		if (errors.size() > 0) return;

		var parser = new cloud.lesh.CPUSim64v2.CPUSim64v2Parser(toks);
		var parserListener = new CollectingErrorListener(errors, null);
		parser.removeErrorListeners();             // remove ConsoleErrorListener
		parser.addErrorListener(parserListener);   // collect parser errors
		ParseTree tree = parser.program();
//		if (errors.size() > 0) return;
		visit(tree);
	}
}
