package cloud.lesh.CPUSim64v2;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.VarHandle;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.RecursiveAction;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;

/**
 * Minimal but functional CPUSim64v2 simulator.
 *
 * Registers (intexgers): R0..R28, SF (R29), SP (R30), PC (R31), SR (flags PZSO).
 * Registers (floats):   F0..F31 (IEEE-754 64-bit).
 * Memory: word-addressed 64-bit cells (long). Addressing is absolute (word index).
 *
 * Instruction encoding:
 *   Type 0 (standard: up to 4 operands):
 *     [63:62] tt, [61:56] opcode, [55:54] a, [53:52] b, [51:50] c, [49:48] d,
 *     then four 12-bit fields: v0=[47:36], v1=[35:24], v2=[23:12], v3=[11:0]
 *   Type 1 (single const C1, signed 56-bit):
 *     [63:62] tt, [61:56] opcode, c1=[55:0] sign-extended
 *   Type 2 ([YZ]C2):
 *     [63:62] tt, [61:56] opcode, [55:54] a, op0 12-bit in v0=[47:36], c2=[41:0] sign-extended
 *   Type 3 ([YZ][YZ]C3):
 *     [63:62] tt, [61:56] opcode, [55:54] a, [53:52] b,
 *     v0=[47:36], v1=[35:24], c3=[27:0] sign-extended
 *
 * Operand type codes (2 bits): 0 NONE, 1 CONST, 2 REG, 3 FP  (see doc)
 *
 * NOTE: This file implements a substantial subset of the ISA. It’s structured so you
 *       can complete additional forms quickly (PACK/UNPACK/CAS/ENDIAN/etc.).
 *
 * References:
 *  - Instruction format & programmer’s model (registers/flags): Simulator docs.  (see inline citations in readme)
 */
public class Simulator {

	public class CPUException extends RuntimeException {
		CPUException(String msg) {
			super(String.format("PC:0x%08X -> ", R[R_PC]) + msg);
		}
		CPUException(String msg, Object... args) {
			super(String.format("PC:0x%08X -> " + msg, R[R_PC], args));
		}
	}

	// ===== CPU STATE =====
	public static final int GPR_COUNT = 32;  // We store R0..R31 (R29=SF, R30=SP, R31=PC)
	public static final int FPR_COUNT = 32;
	public static final int NUM_PORTS = 255;

	// Index aliases
	public static final int R_SF = 29;
	public static final int R_SP = 30;
	public static final int R_PC = 31;

	// Status Register flag bit positions: PZSO in low 4 bits (bit0=P,1=Z,2=S,3=O)
	public static final int SR_P = 1 << 0; // Parity (1=odd, 0=even)
	public static final int SR_Z = 1 << 1; // Zero
	public static final int SR_S = 1 << 2; // Signed (negative)
	public static final int SR_O = 1 << 3; // Overflow

	// Memory (word-addressed). Adjust size as needed.
	long[] mem;
	long[] stack;
	VarHandle atomicMem;
	long heapStart = 0;		// End of code / beginning of free heap
	long heapLimit = 0;		// End of free heap / max stack limit
	long stackSize = 2048;	// Maximum stack size
	long stackBase = 0;		// Maximum memory address / stack base
	private long heapList;	// Linked list of all heap blocks (alloc and free)
	private LinkedList<Long> freeList;	// Linked list of free blocks

	// Integer and floating-point register files
	long[] R = new long[GPR_COUNT];     // R0..R31 (R29=SF, R30=SP, R31=PC, SR kept separate)
	double[] F = new double[FPR_COUNT]; // F0..F31
	long SR = 0;

	// Execution controls
	private boolean running = false;
	private boolean debug = false;
	private long pid;
	private static AtomicLong nextPID = new AtomicLong(0);
	private Vector<Simulator> childCPUs = new Vector<>();
	private ChildProcess process = null;	// If this is a child what is its process object.
	private ChildThread thread = null;

	private InterruptHandler interruptHandler;

	private final static String fmtRegName = "%3s: ";
	private final static String fmtAddress = "%08x";
	private final static int hexSize = 16;
	private final static int decSize = 21;
	private final static int floatSize = 23;
	private final static int fractionSize = 16;

	private final static String padHex = String.format("%" + hexSize + "s","");
	private final static String fmtHex = "%0" + hexSize + "x";
	private final static String padDec = String.format("%" + decSize + "s","");
	private final static String fmtDec = "%" + decSize + "d";
	private final static String fmtFloat = "%" + floatSize + "." + fractionSize + "g";
	private final static String fmtReg = fmtRegName + fmtHex + " (%d)  ";
	private final static String fmtFP = fmtRegName + fmtHex + " (%." + fractionSize + "g)  ";
	private final static String fmtCPU = fmtRegName + fmtHex + " " + fmtDec + " " + fmtRegName + fmtFloat;
	private final static String fmtCPUAlt = fmtRegName + fmtHex + " " + padDec + " " + fmtRegName + fmtFloat;
	private final static String fmtStack = " %2sSP+%02d: "  + fmtHex + " " + fmtDec + " " + fmtFloat;
	private final static String fmtDisassemble = fmtAddress + ": " + fmtHex + " ";
	private final static String fmtPC = fmtRegName + fmtHex;
	private final static String fmtHeading = "%3s  %" + hexSize + "s %" + decSize + "s %3s  %" + floatSize + "s   %5s  %" + hexSize + "s %" + decSize + "s %" + floatSize + "s\n";
	private final static String[] cpuLabels = {"R","hex","dec","FP","float","Stack","hex","dec","float"};

	// Symbols for the general purpose (integer/address) registers: r0, r1, r2, ... sf, sp, pc, sr
	public static final String[] registers = new String[GPR_COUNT];
	// Symbols for the floating point registers: f0, f1, f2, ...
	public static final String[] registersFP = new String[FPR_COUNT];
	static {
		for (int i = 0; i < GPR_COUNT - 3; ++i) {registers[i] = "R"+i;}
		registers[GPR_COUNT-3] = "SF";
		registers[GPR_COUNT-2] = "SP";
		registers[GPR_COUNT-1] = "PC";
		for (int i=0; i<FPR_COUNT; ++i) {registersFP[i] = "F"+i;}
	}

	public Simulator(int memoryWords, String[] args) {
		this(memoryWords, memoryWords / 10, args);
	}

	public Simulator(int memoryWords, int stackSize, String[] args) {
		pid = nextPID.getAndIncrement();
		this.mem = new long[memoryWords];
		this.stack = new long[stackSize];
		this.stackSize = stackSize;
		heapLimit = memoryWords;
		stackBase = memoryWords + stackSize;
		atomicMem = MethodHandles.arrayElementVarHandle(long[].class);
		interruptHandler = new StdInterruptHandler(this);
		PortHandler ph = new StdIOPortHandler(this);
		setPortHandler(0, ph);
		setPortHandler(1, ph);
		setPortHandler(2, ph);
		this.args = args;
	}

	public Simulator(Simulator cloneMe, boolean makeProcess) throws CPUException {
		pid = nextPID.getAndIncrement();

		startClock = cloneMe.startClock;
		totalSystemTime = cloneMe.totalSystemTime;
		cycles = cloneMe.cycles;
		args = cloneMe.args;
		R = cloneMe.R.clone();
		F = cloneMe.F.clone();
		ports = (HashMap<Integer, PortHandler>)(cloneMe.ports.clone());
		interruptHandler = new StdInterruptHandler(this);

		try {
			stackBase = cloneMe.stackBase;
			stackSize = cloneMe.stackSize;
			heapLimit = cloneMe.heapLimit;
			heapStart = cloneMe.heapStart;
			if (makeProcess) {	// for processes
				freeList = (LinkedList<Long>)cloneMe.freeList.clone();
				mem = cloneMe.mem.clone();
				atomicMem = MethodHandles.arrayElementVarHandle(long[].class);
			} else {			// for threads
				freeList = cloneMe.freeList;
				mem = cloneMe.mem;
				stack = new long[(int)stackSize];
				atomicMem = cloneMe.atomicMem;
			}
			for (var i : ports.keySet()) {
				if (ports.get(i) != null) {
					ports.put(i, ports.get(i).duplicate(this));
				}
			}
		} catch (OutOfMemoryError ex) {
			int memoryMax = (int)(stackSize + heapLimit);
			throw new CPUException("Stack plus Heap size of " + memoryMax + " words is too large for child process!");
		}
	}

	// ===== PROGRAM LOADING =====
	public void loadProgram(long[] words, long loadAddr) {
		for (int i = 0; i < words.length; i++) {
			mem[Math.toIntExact(loadAddr + i)] = words[i];
		}
		R[R_PC] = loadAddr;
		R[R_SP] = stackBase - 1;
		R[R_SF] = R[R_SP];
		heapStart = words.length + loadAddr;
		memWrite(heapStart, -1L);						//prev
		memWrite(heapStart + 1, -1L);					//next
		memWrite(heapStart + 2, heapStart - heapLimit);	//block size (neg for free block)
		heapList = heapStart;
		freeList = new LinkedList<Long>();
		freeList.push(heapList);
	}

	public void loadProgram(List<Long> words, long loadAddr) {
		for (int i = 0; i < words.size(); i++) {
			mem[Math.toIntExact(loadAddr + i)] = words.get(i);
		}
		R[R_PC] = loadAddr;
		R[R_SP] = stackBase - 1;
		R[R_SF] = R[R_SP];
		heapStart = words.size() + loadAddr;
		memWrite(heapStart, -1L);						//prev
		memWrite(heapStart + 1, -1L);					//next
		memWrite(heapStart + 2, heapStart - heapLimit);	//block size (neg for free block)
		heapList = heapStart;
		freeList = new LinkedList<Long>();
		freeList.push(heapList);
	}

	private static String[] condition = {"u","z","nz","n","p","nn","np","o","no","pe","po"};
	// ===== FETCH/DECODE =====
	public final static class Decoded {
		public int tt; int op;
		public int a, b, c, d;   // operand kinds (2-bit each)
		public int v0, v1, v2, v3; // 12-bit fields
		public long c1; long c2; int c3;

		public static Decoded decode(long w) {
			Decoded d = new Decoded();
			d.tt = (int)((w >>> 62) & 0x3);
			d.op = (int)((w >>> 56) & 0x3F);
			switch (d.tt) {
				case 0 -> {
					d.a = (int)((w >>> 54) & 0x3);
					d.b = (int)((w >>> 52) & 0x3);
					d.c = (int)((w >>> 50) & 0x3);
					d.d = (int)((w >>> 48) & 0x3);
					d.v0 = (int)((w >>> 36) & 0xFFF);
					d.v1 = (int)((w >>> 24) & 0xFFF);
					d.v2 = (int)((w >>> 12) & 0xFFF);
					d.v3 = (int)(w & 0xFFF);
					if (isConstKind(d.a)) d.v0 = (int)signExtend(d.v0, 12);
					if (isConstKind(d.b)) d.v1 = (int)signExtend(d.v1, 12);
					if (isConstKind(d.c)) d.v2 = (int)signExtend(d.v2, 12);
					if (isConstKind(d.d)) d.v3 = (int)signExtend(d.v3, 12);
				}
				case 1 -> {
					long imm = w & ((1L << 56) - 1);
					d.c1 = signExtend(imm, 56);
				}
				case 2 -> {
					d.a = (int)((w >>> 54) & 0x3);
					d.v0 = (int)((w >>> 42) & 0xFFF);
					long imm = w & ((1L << 42) - 1);
					d.c2 = signExtend(imm, 42);
				}
				case 3 -> {
					d.a = (int)((w >>> 54) & 0x3);
					d.b = (int)((w >>> 52) & 0x3);
					d.v0 = (int)((w >>> 40) & 0xFFF);
					d.v1 = (int)((w >>> 28) & 0xFFF);
					int raw = (int)(w & ((1L << 28) - 1));
					d.c3 = (int)signExtend(raw, 28);
				}
			}
			return d;
		}

		public int getArgCount() {
			int count = 0;
			switch (tt) {
				case 0:
					if (isNoneKind(a))
						count = 0;
					else if (isNoneKind(b))
						count = 1;
					else if (isNoneKind(c))
						count = 2;
					else if (isNoneKind(d))
						count = 3;
					else
						count = 4;
					break;
				case 1:
					count = 1;
					break;
				case 2:
					count = 2;
					break;
				case 3:
					count = 3;
					break;
			}
			return count;
		}

		public static String getRegisterOrValue(int type, long num) {
			return switch (type) {
				case 0 -> "None";
				case 1 -> Long.toString(num);
				case 2 -> registers[(int)num];
				case 3 -> registersFP[(int)num];
				default -> throw new RuntimeException("Unexpected op type: " + type);
			};
		}

		public String get(int i) {
			switch (i) {
				case 0:
					return getRegisterOrValue(a, v0);
				case 1:
					return getRegisterOrValue(b, v1);
				case 2:
					return getRegisterOrValue(c, v2);
				case 3:
					return getRegisterOrValue(d, v3);
				default:
					throw new RuntimeException("Illegal op index: " + i);
			}
		}
		public int getType() { return tt; }
		public int getOpCode() { return op; }
		public String getOpName() {
			if (op == Opcode.NOP.getCode() && getArgCount() == 0)
				return Opcode.NOP.getName();
			else
				return Opcode.fromCode(op).getName();
		}
		public String disassemble() {
			String v0, v1, v2, v3;
			StringBuffer buffer = new StringBuffer();
			buffer.append(String.format("%-10s\t", getOpName()));
			switch (getType()) {
				case 0:
					if (a != 0) {
						v0 = get(0);
						buffer.append(v0);
						if (b != 0) {
							buffer.append(", ");
							v1 = get(1);
							buffer.append(v1);
							if (c != 0) {
								buffer.append(", ");
								v2 = get(2);
								buffer.append(v2);
								if (d != 0) {
									buffer.append(", ");
									v3 = get(3);
									buffer.append(v3);
								}
							}
						}
					}
					break;
				case 1:
					if (getOpCode() == Opcode.JUMP.code ||
						getOpCode() == Opcode.CALL.code)
						v0 = "0x" + Long.toString(c1, 16);
					else
						v0 = Long.toString(c1);
					buffer.append(v0);
					break;
				case 2:
					v0 = get(0);
					if (a == 1 && (getOpCode() == Opcode.JUMP.code ||
							getOpCode() == Opcode.CALL.code))
						v0 = condition[this.v0];
					buffer.append(v0);
					buffer.append(", ");
					if (getOpCode() == Opcode.JUMP.code ||
							getOpCode() == Opcode.CALL.code)
						v1 = "0x" + Long.toString(c2, 16);
					else
						v1 = Long.toString(c2);
					buffer.append(v1);
					break;
				case 3:
					v0 = get(0);
					if (a == 1 && (getOpCode() == Opcode.JUMP.code ||
							getOpCode() == Opcode.CALL.code))
						v0 = condition[this.v0];
					buffer.append(v0);
					buffer.append(", ");
					v1 = get(1);
					buffer.append(v1);
					buffer.append(", ");
					if (getOpCode() == Opcode.JUMP.code ||
							getOpCode() == Opcode.CALL.code)
						v2 = "0x" + Long.toString(c3, 16);
					else
						v2 = Integer.toString(c3);
					buffer.append(v2);
					break;
			}
			buffer.append("\n");
			return buffer.toString();
		}

	}

	long startClock = 0;
	long totalSystemTime = 0;
	long cycles = 0;
	String[] args;

	// ===== UTILITIES =====
	public static long signExtend(long v, int bits) {
		long m = 1L << (bits - 1);
		long mask = (1L << bits) - 1;
		v &= mask;
		return (v ^ m) - m;
	}

	public int toRegIndex(int kind, int val12) {
		if (isRegKind(kind)) checkIntReg(val12);
		if (isFPKind(kind)) checkFPReg(val12);
		// kind 2=REG, 3=FP. val12 indexes register number.
		return val12 & 0x3F;
	}

	private static boolean isNoneKind(int k){ return k == 0; }
	private static boolean isConstKind(int k){ return k == 1; }
	private static boolean isRegKind(int k) { return k == 2; }
	private static boolean isFPKind(int k)  { return k == 3; }
	private static boolean isXKind(int k)  { return k == 2 || k == 3; }
	private static boolean isYKind(int k)  { return k == 2 || k == 3; }
	private static boolean isOKind(int k)  { return k == 1 || k == 2; }
	private static boolean isQKind(int k)  { return k != 0; }

	private void setFlags(long x, boolean overflowHappened) {
		boolean neg = x < 0;
		boolean zero = x == 0;
		boolean oddParity = (Long.bitCount(x) & 1) == 1;
		SR = (oddParity ? SR_P : 0) | (zero ? SR_Z : 0) | (neg ? SR_S : 0) | (overflowHappened ? SR_O : 0);
	}

	private void setFlagsFromSubtract(long a, long b, long result) {
		boolean of = ((a ^ b) & (a ^ result) & (1L << 63)) != 0; // 2’s comp overflow on a - b
		setFlags(result, of);
	}

	private void setFlags(double x) {
		boolean neg = x < 0;
		boolean zero = x == 0;
		boolean oddParity = (Long.bitCount(Double.doubleToRawLongBits(x)) & 1) == 1;
		boolean overflowHappened = Double.isInfinite(x);
		SR = (oddParity ? SR_P : 0) | (zero ? SR_Z : 0) | (neg ? SR_S : 0) | (overflowHappened ? SR_O : 0);
	}

	public String formatSR() {
		boolean P = (SR & SR_P) != 0;
		boolean Z = (SR & SR_Z) != 0;
		boolean S = (SR & SR_S) != 0;
		boolean O = (SR & SR_O) != 0;
		return  (O ? "O" : "o") +
				(S ? "S" : "s") +
				(Z ? "Z" : "z") +
				(P ? "P" : "p");
	}

	private boolean testCond(int z) {
		boolean P = (SR & SR_P) != 0;
		boolean Z = (SR & SR_Z) != 0;
		boolean S = (SR & SR_S) != 0;
		boolean O = (SR & SR_O) != 0;
		return switch (z) {
			case 0 -> true;              // u (uncond)
			case 1 -> Z;                 // z
			case 2 -> !Z;                // nz
			case 3 -> S;                 // n
			case 4 -> !S && !Z;          // p
			case 5 -> !S;                // nn (not negative) == p
			case 6 -> S || Z;            // np (not positive) == n (keep legacy aliases)
			case 7 -> O;                 // o
			case 8 -> !O;                // no
			case 9 -> !P;                // pe (even parity)
			case 10 -> P;                // po (odd parity)
			default -> false;
		};
	}

	public long memRead(long addr) {
		long val = 0;
		try {
			int a = Math.toIntExact(addr);
			if (a < heapLimit)
				val = mem[a];
			else
				val = stack[a - (int)heapLimit];
			++cycles;
		} catch (Exception ex) {
			throw new CPUException("Illegal memory access at " + addr);
		}
		return val;
	}

	public void memWrite(long addr, long val) {
		try {
			int a = Math.toIntExact(addr);
			if (a < heapLimit)
				mem[a] = val;
			else
				stack[a - (int)heapLimit] = val;
			++cycles;
		} catch (Exception ex) {
			throw new CPUException("Illegal memory access at " + addr);
		}
	}

	public int getConst(int kind, int val12) {
		if (!isConstKind(kind))
			throw new CPUException("Illegal constant argument.");
		return val12;
	}

	public long getR(int kind, int val12) {
		// A can be R0..R28, SF, SP, PC. We encode them as registers too.
		if (!isRegKind(kind))
			throw new CPUException("Illegal A/R argument.");
		int r = toRegIndex(kind, val12);
		return R[r];
	}

	public void setR(int kind, int val12, long wordBits) {
		if (isRegKind(kind)){ // address/int
			int r = toRegIndex(kind, val12);
			R[r] = wordBits;
			setFlags(wordBits, false);
		} else throw new CPUException("Illegal A/R argument.");
	}

	public double getFP(int kind, int val12) {
		// A can be F0..F31
		if (!isFPKind(kind))
			throw new CPUException("Illegal F argument.");
		int r = toRegIndex(kind, val12);
		return F[r];
	}

	public void setFP(int kind, int val12, double f) {
		if (isFPKind(kind)){ // address/int
			int r = toRegIndex(kind, val12);
			F[r] = f;
			setFlags(f);
		} else throw new CPUException("Illegal F argument.");
	}

	private long getY(int kind, int val12) {
		if (isFPKind(kind)) {
			int f = toRegIndex(kind, val12);
			return Double.doubleToRawLongBits(F[f]);
		} else if (isRegKind(kind)){ // address/int
			int r = toRegIndex(kind, val12);
			return R[r];
		}
		throw new CPUException("Illegal Y argument.");
	}

	private void setY(int kind, int val12, long wordBits) {
		if (isFPKind(kind)) {
			int f = toRegIndex(kind, val12);
			F[f] = Double.longBitsToDouble(wordBits);
		} else if (isRegKind(kind)){ // address/int
			int r = toRegIndex(kind, val12);
			R[r] = wordBits;
		} else throw new CPUException("Illegal Y argument.");
	}

	private long getO(int kind, int val12) {
		if (isRegKind(kind)){ // address/int
			int r = toRegIndex(kind, val12);
			return R[r];
		} else if (isConstKind(kind)) {
			return val12;
		}
		throw new CPUException("Illegal O argument.");
	}

	private long getQ(int kind, int val12) {
		if (isFPKind(kind)) {
			int f = toRegIndex(kind, val12);
			return Double.doubleToRawLongBits(F[f]);
		} else if (isRegKind(kind)){ // address/int
			int r = toRegIndex(kind, val12);
			return R[r];
		} else if (isConstKind(kind)) {
			return val12;
		} else throw new CPUException("Illegal Q argument.");
	}

	private void checkIntReg(int r) {
		if (r < 0 || r >= GPR_COUNT) throw new CPUException("Bad int register: R" + r);
	}
	private void checkFPReg(int f) {
		if (f < 0 || f >= FPR_COUNT) throw new CPUException("Bad FP register: F" + f);
	}

	// ===== EXECUTION =====
	public int run() {
		running = true;
		startClock = System.nanoTime();
		totalSystemTime = 0;
		cycles = 0;

		while (running) {
			long pc = R[R_PC];
			long instr = memRead(pc); // This increments cycles by 1
			R[R_PC] = pc + 1;

			Decoded d = Decoded.decode(instr);

			if (debug) {
				System.out.print(String.format("%08X ", pc));
				System.out.print(d.disassemble());
			}
			exec(d);
		}
		return (int)R[0];
	}

	public String disassemble() {
		int numStops = 0;
		StringBuffer buffer = new StringBuffer();
		for (int i = (int)R[R_PC]; i < heapStart; ++i) {
			buffer.append(String.format("%08X ", i));
			long instr = memRead(i);
			var d = Decoded.decode(instr);
			buffer.append(d.disassemble());
			if (d.getOpCode() == Opcode.STOP.code)
				++numStops;
			if (numStops > 1) break;
		}
		return buffer.toString();
	}

	private void exec(Decoded d) {
		switch (d.op) {
			case 0 -> opNOP_DEBUG(d);
			case 1 -> opCLEAR(d);
			case 2 -> opMOVE(d);
			case 3 -> opLOAD(d);
			case 4 -> opSTORE(d);
			case 5 -> opPOP(d);
			case 6 -> opPUSH(d);
			case 7 -> opJUMP(d);
			case 8 -> opCALL(d);
			case 9 -> opRETURN(d);
			case 10 -> opINTERRUPT(d);
			case 11 -> running = false; // STOP
			case 12 -> opNEGATE(d);
			case 13 -> opADD(d);
			case 14 -> opSUB(d);
			case 15 -> opMULT(d);
			case 16 -> opDIV_or_RECIP(d);
			case 17 -> opCOMPL(d);
			case 18 -> bitwise(d);	// AND
			case 19 -> bitwise(d);	// OR
			case 20 -> bitwise(d);	// XOR
			case 21 -> opTEST(d);
			case 22 -> opCMP(d);
			case 23 -> bitwise(d);  // LSH
			case 24 -> bitwise(d);  // RSH
			case 25 -> bitwise(d);  // arithmetic RSH>>
			case 26 -> bitwise(d);  // LROT
			case 27 -> bitwise(d);	// RROT
			case 28 -> opIN(d);
			case 29 -> opOUT(d);
			case 30 -> opPACK(d);
			case 31 -> opPACK64(d);
			case 32 -> opUNPACK(d);
			case 33 -> opUNPACK64(d);
			case 34 -> opCAS(d);
			case 35 -> opENDIAN(d);
			case 36 -> opSAVE(d);
			case 37 -> opRESTORE(d);
			default -> throw new IllegalStateException("Unimplemented opcode: " + d.op);
		}
	}

	// ---- 0: NOP/DEBUG ----
	private void opNOP_DEBUG(Decoded d) {
		// If operand kinds are NONE => NOP; otherwise DEBUG forms (Y,YY,YYY,YYYY or dumps)
		boolean noOps = (d.tt == 0 && d.a == 0 && d.b == 0 && d.c == 0 && d.d == 0);
		if (noOps) return; // NOP
		if (!debug) return;

		if (d.tt == 0) {
			// Print up to 4 registers (int or float)
			printX(d.a, d.v0);
			printX(d.b, d.v1);
			printX(d.c, d.v2);
			printX(d.d, d.v3);
			System.out.println();
		} else {
			printCPUState();
		}
	}

	private void printX(int kind, int v) {
		if (kind == 0) return;
		String reg = Decoded.getRegisterOrValue(kind, v);
		if (isFPKind(kind)) {
			int f = toRegIndex(kind, v);
			if (f < FPR_COUNT) System.out.printf("%s: %.17g  ", reg, F[f]);
		} else if (isRegKind(kind)) {
			int r = toRegIndex(kind, v);
			if (r < GPR_COUNT) System.out.printf("%s: %d  ", reg, R[r]);
		} else if (isConstKind(kind)) {
			System.out.printf("C(12)=%d  ", v);
		}
	}

	public void clearCPUState() {
		for (int i = 0; i < GPR_COUNT - 3; ++i)
			R[i] = 0;
		Arrays.fill(F, 0.0);
		SR = SR_Z;
	}

	// ---- 1: CLEAR ---- (Clear all or up to 4 specific)
	private void opCLEAR(Decoded d) {
		if (d.tt == 0 && d.a == 0) {
			clearCPUState();
			return;
		}
		if (isRegKind(d.a)) setR(d.a, d.v0, 0);
		if (isFPKind(d.a)) setFP(d.a, d.v0, 0);
		if (isRegKind(d.b)) setR(d.b, d.v1, 0);
		if (isFPKind(d.b)) setFP(d.b, d.v1, 0);
		if (isRegKind(d.c)) setR(d.c, d.v2, 0);
		if (isFPKind(d.c)) setFP(d.c, d.v2, 0);
		if (isRegKind(d.d)) setR(d.d, d.v3, 0);
		if (isFPKind(d.d)) setFP(d.d, d.v3, 0);
		SR = SR_Z;
	}

	// ---- 2: MOVE ---- (covers many Y/A/F forms; here: core)
	private void opMOVE(Decoded d) {
		// Common forms:
		//  - YY: Y1 <- Y2
		//  - YC: Y <- C (C may be 12/28/42/56 depending on type; here use Type2/3/1)
		//  - AAR/AAC/ACA: address arithmetic (A1 <- A2 + R/C or C + A2)
		//  - ZYQQ (conditional move): if Z, Y <- Q1 else Y <- Q2

		double fp;
		long k;
		if (d.tt == 0) {
			// Try YY (a and b are types, v0 dest, v1 src)
			if (d.getArgCount() == 2) {
				if (isRegKind(d.b)) {
					k = getR(d.b, d.v1);
					if (isRegKind(d.a)) {
						setR(d.a, d.v0, k);
						return;
					} else if (isFPKind(d.a)){
						setFP(d.a, d.v0, k);
						return;
					}
				} else if (isFPKind(d.b)){
					fp = getFP(d.b, d.v1);
					if (isRegKind(d.a)) {
						setR(d.a, d.v0, (long)fp);
						return;
					} else if (isFPKind(d.a)){
						setFP(d.a, d.v0, fp);
						return;
					}
				}
			} else if (d.getArgCount() == 3) {
				// AAR: A1 <- A2 + R (both REG-kind in a,b and reg in c)
				if (isRegKind(d.a) && isRegKind(d.b) && isRegKind(d.c)) {
					setR(d.a, d.v0, getR(d.b, d.v1) + getR(d.c, d.v2));
					return;
				}
			} else if (d.getArgCount() == 4) {
				// ZYQQ conditional move using imm28 as condition code? In the table, Z is 4-bit; here we expect v0=Z (4b),
				// then two Q sources in v1/v2 kinds…but full generality needs more metadata than Type3 provides.
				if (isConstKind(d.a) && isYKind(d.b) && isQKind(d.c) && isQKind(d.d)) {
					if (testCond(d.v0)) {
						if (isFPKind(d.c)) {
							fp = getFP(d.c, d.v2);
							k = (long)fp;
						} else {
							k = getO(d.c, d.v2);
							fp = k;
						}
					} else {
						if (isFPKind(d.d)) {
							fp = getFP(d.d, d.v3);
							k = (long)fp;
						} else {
							k = getO(d.d, d.v3);
							fp = k;
						}
					}
					if (isRegKind(d.b)) {
						setR(d.b, d.v1, k);
						return;
					} else if (isFPKind(d.b)) {
						setFP(d.b, d.v1, fp);
						return;
					}
				}
			}
		} else if (d.tt == 2) {
			// YC (C2)
			k = d.c2;
			if (isRegKind(d.a)) {
				setR(d.a, d.v0, k);
				return;
			} else if (isFPKind(d.a)) {
				setFP(d.a, d.v0, k);
				return;
			}
		} else if (d.tt == 3) {
			// ACA: A1 <- C + A2 (a=REG, b=CONST, c=REG)
			// AAC: A1 <- A2 + C (a=REG, b=REG, c=CONST)
			if (isRegKind(d.a) && isRegKind(d.b)) {
				setR(d.a, d.v0, getR(d.b, d.v1) + d.c3);
				return;
			}
		}
		throw new CPUException("Illegal MOVE arguments.");
	}

	// ---- 3: LOAD ----
	private void opLOAD(Decoded d) {
		if (d.tt == 2) { // YC: Y <- [C2]
			long addr = d.c2;
			long word = memRead(addr);
			setY(d.a, d.v0, word);
			return;
		} else if (d.tt == 0) {
			int count = d.getArgCount();
			// YA: Y <- [A]   (a=Y, b=A(reg))
			if (count == 2 && (isYKind(d.a)) && isRegKind(d.b)) {
				long addr = getR(d.b, d.v1);
				long w = memRead(addr);
				setY(d.a, d.v0, w);
				return;
			}
			// YAR: [A + R]
			if (count == 3 && (isYKind(d.a)) && isRegKind(d.b) && isRegKind(d.c)) {
				long base = getR(d.b, d.v1);
				long off = getR(d.c, d.v2);
				setY(d.a, d.v0, memRead(base + off));
				return;
			}
		} else if (d.tt == 3) {
			// YAC: [A + C] == YCA: [C + A]
			// YCC: [C + C12]
			if ((isYKind(d.a)) && (isOKind(d.b))) {
				long base = getO(d.b, d.v1);
				long addr = base + d.c3;
				setY(d.a, d.v0, memRead(addr));
				return;
			}
		}
		throw new IllegalStateException("Illegal LOAD arguments.");
	}

	// ---- 4: STORE ----
	private void opSTORE(Decoded d) {
		if (d.tt == 2) { // QC: [C2] <- Q
			long val = getQ(d.a, d.v0);
			long addr = d.c2;
			memWrite(addr, val);
			return;
		}
		if (d.tt == 0) {
			int count = d.getArgCount();
			// QA: [A] <- Q  (a=Q, b=A)
			if (count == 2 && isQKind(d.a) && isRegKind(d.b)) {
				long addr = getR(d.b, d.v1);
				long val = getQ(d.a, d.v0);
				memWrite(addr, val);
				return;
			}
			// QAR: [A + R]
			if (count == 3 && isQKind(d.a) && isRegKind(d.b) && isRegKind(d.c)) {
				long base = getR(d.b, d.v1);
				long addr = base + getR(d.c, d.v2);
				long val = getQ(d.a, d.v0);
				memWrite(addr, val);
				return;
			}
		} else if (d.tt == 3) {
			// QAC: [A + C]
			// QCC: [C + C12]
			if (isQKind(d.a) && isRegKind(d.b)) {
				long base = getR(d.b, d.v1);
				long addr = base + d.c3;
				long val = getQ(d.a, d.v0);
				memWrite(addr, val);
				return;
			} else if (isQKind(d.a) && isConstKind(d.b)) {
				long base = getO(d.b, d.v1);
				long addr = base + d.c3;
				long val = getQ(d.a, d.v0);
				memWrite(addr, val);
				return;
			}
		}
		throw new IllegalStateException("Illegal STORE arguments.");
	}

	// ---- 5: POP ----
	private void opPOP(Decoded d) {
		R[R_SP] = R[R_SP] + 1;
		long val = memRead(R[R_SP]);
		if (d.tt == 0) {
			if (d.a == 0) {
				// N: discard
				return;
			} else if (isYKind(d.a)) {
				setY(d.a, d.v0, val);
				return;
			}
		}
		throw new IllegalStateException("POP form not implemented");
	}

	// ---- 6: PUSH ----
	private void opPUSH(Decoded d) {
		long val;
		if (d.tt == 1) { // C1
			val = d.c1;
		} else if (d.tt == 0) {
			if (isYKind(d.a)) {
				val = getY(d.a, d.v0);
			} else {
				throw new CPUException("Illegal PUSH instruction");
			}
		} else {
			throw new CPUException("Illegal PUSH instruction");
		}
		memWrite(R[R_SP], val);
		R[R_SP] = R[R_SP] - 1;
		if (R[R_SP] < heapLimit) throw new CPUException("Stack/Heap collision");
	}

	// ---- 7: JUMP ----
	private void opJUMP(Decoded d) {
		if (d.tt == 1) {		// C
			R[R_PC] = d.c1;
			return;
		} else if (d.tt == 2) { // AC, ZC
			if (isRegKind(d.a)) {
				R[R_PC] = getR(d.v0) + d.c2;
				return;
			} else if (isConstKind(d.a)) {
				if (testCond(getConst(d.a, d.v0)))
					R[R_PC] = d.c2;
				return;
			}
		} else if (d.tt == 3) { // ZAC, ZCA, ZCC
			if (testCond(getConst(d.a, d.v0)))
				R[R_PC] = getO(d.b, d.v1) + d.c3;
			return;
		} else if (d.tt == 0) {
			// A, AR, ZA, ZAR
			int count = d.getArgCount();
			if (count == 1 && isRegKind(d.a)) {
				R[R_PC] = getR(d.a, d.v0);
				return;
			} else if (count == 2) {
				if (isRegKind(d.a)) {
					R[R_PC] = getR(d.a, d.v0) + getR(d.b, d.v1);
					return;
				} else if (isConstKind(d.a)) {
					if (testCond(getConst(d.a, d.v0)))
						R[R_PC] = getR(d.b, d.v1);
					return;
				}
			} else if (count == 3) {
				if (testCond(getConst(d.a, d.v0)))
					R[R_PC] = getR(d.b, d.v1) + getR(d.c, d.v2);
				return;
			}
		}
		throw new IllegalStateException("Illegal JUMP arguments.");
	}

	// ---- 8: CALL ----
	private void opCALL(Decoded d) {
		// Prologue:
		memWrite(R[R_SP], R[R_PC]); R[R_SP]--;
		memWrite(R[R_SP], R[R_SF]); R[R_SP]--;
		R[R_SF] = R[R_SP];

		if (d.tt == 1) {		// C
			R[R_PC] = d.c1;
			return;
		} else if (d.tt == 2) { // AC, ZC
			if (isRegKind(d.a)) {
				R[R_PC] = getR(d.v0) + d.c2;
				return;
			} else if (isConstKind(d.a)) {
				if (testCond(getConst(d.a, d.v0)))
					R[R_PC] = d.c2;
				return;
			}
		} else if (d.tt == 3) { // ZAC, ZCA, ZCC
			if (testCond(getConst(d.a, d.v0)))
				R[R_PC] = getO(d.b, d.v1) + d.c3;
			return;
		} else if (d.tt == 0) {
			// A, AR, ZA, ZAR
			int count = d.getArgCount();
			if (count == 1 && isRegKind(d.a)) {
				R[R_PC] = getR(d.a, d.v0);
				return;
			} else if (count == 2) {
				if (isRegKind(d.a)) {
					R[R_PC] = getR(d.a, d.v0) + getR(d.b, d.v1);
					return;
				} else if (isConstKind(d.a)) {
					if (testCond(getConst(d.a, d.v0)))
						R[R_PC] = getR(d.b, d.v1);
					return;
				}
			} else if (count == 3) {
				if (testCond(getConst(d.a, d.v0)))
					R[R_PC] = getR(d.b, d.v1) + getR(d.c, d.v2);
				return;
			}
		}
		throw new IllegalStateException("Illegal CALL arguments.");
	}

	// ---- 9: RETURN ----
	private void opRETURN(Decoded d) {
		R[R_SP] = R[R_SF];
		R[R_SP] = R[R_SP] + 1; R[R_SF] = memRead(R[R_SP]);
		R[R_SP] = R[R_SP] + 1; R[R_PC] = memRead(R[R_SP]);
		if (R[R_PC] < 0) exit((int)R[R_PC]);
	}

	// ---- 10: INTERRUPT ----
	private void opINTERRUPT(Decoded d) {
		long code = -1;			// -1 Illegal, 0 don't interrupt, 1... valid interrupts
		if (d.tt == 1) {		// C
			code = d.c1;
		} else if (d.tt == 2) { // ZC
			if (testCond(getConst(d.a, d.v0)))
				code = d.c2;
			else code = 0;
		} else if (d.tt == 0) {
			// R, ZR,
			int count = d.getArgCount();
			if (count == 1 && isOKind(d.a)) {
				code = getO(d.a, d.v0);
			} else if (count == 2 && isOKind(d.b)) {
				if (testCond(getConst(d.a, d.v0)))
					code = getO(d.b, d.v1);
				else code = 0;
			}
		}
		if (code > 0) {
			long start = System.nanoTime();
			interruptHandler.dispatch((int) code);
			long stop = System.nanoTime();
			totalSystemTime += stop - start;
		} else if (code < 0)
			throw new IllegalStateException("Illegal INTERRUPT arguments.");
	}

	// ---- 12: NEGATE ----
	private void opNEGATE(Decoded d) {
		if (d.tt == 0 && isFPKind(d.a)) {
			int f = toRegIndex(d.a, d.v0);
			F[f] = -F[f];
		} else if (d.tt == 0 && isRegKind(d.a)) {
			int r = toRegIndex(d.a, d.v0);
			long res = -R[r];
			setFlags(res, (R[r] == Long.MIN_VALUE)); // overflow if negating MIN
			R[r] = res;
		} else {
			throw new IllegalStateException("NEGATE form not implemented");
		}
	}

	// ---- 13: ADD ----
	private void opADD(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 2) {
				if (isRegKind(d.a) && isRegKind(d.b)) {
					// AR
					int rd = toRegIndex(d.a, d.v0);
					long before = R[rd];
					long rhs = R[toRegIndex(d.b, d.v1)];
					long res = before + rhs;
					boolean of = ((before ^ res) & (rhs ^ res)) < 0;
					R[rd] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a)) {
					// FX
					int rd = toRegIndex(d.a, d.v0);
					double before = F[rd];
					double rhs = isFPKind(d.b) ? F[toRegIndex(d.b, d.v1)] : R[toRegIndex(d.b, d.v1)];
					double res = before + rhs;
					F[rd] = res;
					setFlags(res);
					return;
				}
			} else if (count == 3) {
				// AAR
				if (isRegKind(d.a) && isRegKind(d.b) && isRegKind(d.c)) {
					int rd = toRegIndex(d.b, d.v1);
					long before = R[rd];
					long rhs = R[toRegIndex(d.c, d.v2)];
					long res = before + rhs;
					boolean of = ((before ^ res) & (rhs ^ res)) < 0;
					R[toRegIndex(d.a, d.v0)] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a) && isFPKind(d.b)) {
					// FFX
					int rd = toRegIndex(d.b, d.v1);
					double before = F[rd];
					double rhs = isFPKind(d.c) ? F[toRegIndex(d.c, d.v2)] : R[toRegIndex(d.c, d.v2)];
					double res = before + rhs;
					F[toRegIndex(d.a, d.v0)] = res;
					setFlags(res);
					return;
				}
			}
		} else if (d.tt == 2 && isYKind(d.a)) {
			// YC: Y <- Y + C2
			if (isRegKind(d.a)) {
				// AC
				int rd = toRegIndex(d.a, d.v0);
				long before = R[rd];
				long rhs = d.c2;
				long res = before + rhs;
				boolean of = ((before ^ res) & (rhs ^ res)) < 0;
				R[rd] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a)) {
				// FC
				int rd = toRegIndex(d.a, d.v0);
				double before = F[rd];
				double rhs = d.c2;
				double res = before + rhs;
				F[rd] = res;
				setFlags(res);
				return;
			}
		} else if (d.tt == 3) {
			// AAC, FFC
			if (isRegKind(d.a) && isRegKind(d.b)) {
				// AC
				int rd = toRegIndex(d.b, d.v1);
				long before = R[rd];
				long rhs = d.c3;
				long res = before + rhs;
				boolean of = ((before ^ res) & (rhs ^ res)) < 0;
				R[toRegIndex(d.a, d.v0)] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a) && isFPKind(d.b)) {
				// FC
				int rd = toRegIndex(d.b, d.v1);
				double before = F[rd];
				double rhs = d.c3;
				double res = before + rhs;
				F[toRegIndex(d.a, d.v0)] = res;
				setFlags(res);
				return;
			}
		}
		throw new IllegalStateException("ADD form not implemented");
	}

	// ---- 14: SUB ----
	private void opSUB(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 2) {
				if (isRegKind(d.a) && isRegKind(d.b)) {
					// AR
					int rd = toRegIndex(d.a, d.v0);
					long before = R[rd];
					long rhs = R[toRegIndex(d.b, d.v1)];
					long res = before - rhs;
					boolean of = ((before ^ rhs) & (before ^ res)) < 0;
					R[rd] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a)) {
					// FX
					int rd = toRegIndex(d.a, d.v0);
					double before = F[rd];
					double rhs = isFPKind(d.b) ? F[toRegIndex(d.b, d.v1)] : R[toRegIndex(d.b, d.v1)];
					double res = before - rhs;
					F[rd] = res;
					setFlags(res);
					return;
				}
			} else if (count == 3) {
				// AAR
				if (isRegKind(d.a) && isRegKind(d.b) && isRegKind(d.c)) {
					int rd = toRegIndex(d.b, d.v1);
					long before = R[rd];
					long rhs = R[toRegIndex(d.c, d.v2)];
					long res = before - rhs;
					boolean of = ((before ^ rhs) & (before ^ res)) < 0;
					R[toRegIndex(d.a, d.v0)] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a) && isFPKind(d.b)) {
					// FFX
					int rd = toRegIndex(d.b, d.v1);
					double before = F[rd];
					double rhs = isFPKind(d.c) ? F[toRegIndex(d.c, d.v2)] : R[toRegIndex(d.c, d.v2)];
					double res = before - rhs;
					F[toRegIndex(d.a, d.v0)] = res;
					setFlags(res);
					return;
				}
			}
		} else if (d.tt == 2 && isYKind(d.a)) {
			// YC: Y <- Y + C2
			if (isRegKind(d.a)) {
				// AC
				int rd = toRegIndex(d.a, d.v0);
				long before = R[rd];
				long rhs = d.c2;
				long res = before - rhs;
				boolean of = ((before ^ rhs) & (before ^ res)) < 0;
				R[rd] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a)) {
				// FC
				int rd = toRegIndex(d.a, d.v0);
				double before = F[rd];
				double rhs = d.c2;
				double res = before - rhs;
				F[rd] = res;
				setFlags(res);
				return;
			}
		} else if (d.tt == 3) {
			// AAC, FFC
			if (isRegKind(d.a) && isRegKind(d.b)) {
				// AC
				int rd = toRegIndex(d.b, d.v1);
				long before = R[rd];
				long rhs = d.c3;
				long res = before - rhs;
				boolean of = ((before ^ rhs) & (before ^ res)) < 0;
				R[toRegIndex(d.a, d.v0)] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a) && isFPKind(d.b)) {
				// FC
				int rd = toRegIndex(d.b, d.v1);
				double before = F[rd];
				double rhs = d.c3;
				double res = before - rhs;
				F[toRegIndex(d.a, d.v0)] = res;
				setFlags(res);
				return;
			}
		}
		throw new IllegalStateException("SUB form not implemented");
	}

	// ---- 15: MULT ----
	private void opMULT(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 2) {
				if (isRegKind(d.a) && isRegKind(d.b)) {
					// AR
					int rd = toRegIndex(d.a, d.v0);
					long before = R[rd];
					long rhs = R[toRegIndex(d.b, d.v1)];
					long res = before * rhs;
					boolean of = res / rhs != before;
					R[rd] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a)) {
					// FX
					int rd = toRegIndex(d.a, d.v0);
					double before = F[rd];
					double rhs = isFPKind(d.b) ? F[toRegIndex(d.b, d.v1)] : R[toRegIndex(d.b, d.v1)];
					double res = before * rhs;
					F[rd] = res;
					setFlags(res);
					return;
				}
			} else if (count == 3) {
				// AAR
				if (isRegKind(d.a) && isRegKind(d.b) && isRegKind(d.c)) {
					int rd = toRegIndex(d.b, d.v1);
					long before = R[rd];
					long rhs = R[toRegIndex(d.c, d.v2)];
					long res = before * rhs;
					boolean of = res / rhs != before;
					R[toRegIndex(d.a, d.v0)] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a) && isFPKind(d.b)) {
					// FFX
					int rd = toRegIndex(d.b, d.v1);
					double before = F[rd];
					double rhs = isFPKind(d.c) ? F[toRegIndex(d.c, d.v2)] : R[toRegIndex(d.c, d.v2)];
					double res = before * rhs;
					F[toRegIndex(d.a, d.v0)] = res;
					setFlags(res);
					return;
				}
			}
		} else if (d.tt == 2 && isYKind(d.a)) {
			// YC: Y <- Y + C2
			if (isRegKind(d.a)) {
				// AC
				int rd = toRegIndex(d.a, d.v0);
				long before = R[rd];
				long rhs = d.c2;
				long res = before * rhs;
				boolean of = res / rhs != before;
				R[rd] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a)) {
				// FC
				int rd = toRegIndex(d.a, d.v0);
				double before = F[rd];
				double rhs = d.c2;
				double res = before * rhs;
				F[rd] = res;
				setFlags(res);
				return;
			}
		} else if (d.tt == 3) {
			// AAC, FFC
			if (isRegKind(d.a) && isRegKind(d.b)) {
				// AC
				int rd = toRegIndex(d.b, d.v1);
				long before = R[rd];
				long rhs = d.c3;
				long res = before * rhs;
				boolean of = res / rhs != before;
				R[toRegIndex(d.a, d.v0)] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a) && isFPKind(d.b)) {
				// FC
				int rd = toRegIndex(d.b, d.v1);
				double before = F[rd];
				double rhs = d.c3;
				double res = before * rhs;
				F[toRegIndex(d.a, d.v0)] = res;
				setFlags(res);
				return;
			}
		}
		throw new IllegalStateException("MULT form not implemented");
	}

	// ---- 16: DIV / RECIP ----
	private void opDIV_or_RECIP(Decoded d) {
		// RECIP: F <- 1/F (use Type0 with F single)
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 1) {
				double res = 1.0 / getFP(d.a, d.v0);
				F[toRegIndex(d.a, d.v0)] = res;
				setFlags(res);
				return;
			} else if (count == 2) {
				if (isRegKind(d.a) && isRegKind(d.b)) {
					// AR
					int rd = toRegIndex(d.a, d.v0);
					long before = R[rd];
					long rhs = R[toRegIndex(d.b, d.v1)];
					long res = before / rhs;
					boolean of = (before == Long.MIN_VALUE && rhs == -1);
					R[rd] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a)) {
					// FX
					int rd = toRegIndex(d.a, d.v0);
					double before = F[rd];
					double rhs = isFPKind(d.b) ? F[toRegIndex(d.b, d.v1)] : R[toRegIndex(d.b, d.v1)];
					double res = before / rhs;
					F[rd] = res;
					setFlags(res);
					return;
				}
			} else if (count == 3) {
				// AAR
				if (isRegKind(d.a) && isRegKind(d.b) && isRegKind(d.c)) {
					int rd = toRegIndex(d.b, d.v1);
					long before = R[rd];
					long rhs = R[toRegIndex(d.c, d.v2)];
					long res = before / rhs;
					boolean of = (before == Long.MIN_VALUE && rhs == -1);
					R[toRegIndex(d.a, d.v0)] = res;
					setFlags(res, of);
					return;
				} else if (isFPKind(d.a) && isFPKind(d.b)) {
					// FFX
					int rd = toRegIndex(d.b, d.v1);
					double before = F[rd];
					double rhs = isFPKind(d.c) ? F[toRegIndex(d.c, d.v2)] : R[toRegIndex(d.c, d.v2)];
					double res = before / rhs;
					F[toRegIndex(d.a, d.v0)] = res;
					setFlags(res);
					return;
				}
			} else if (count == 4) {
				long a = getR(d.c, d.v2);
				long b = getO(d.d, d.v3);
				setR(d.a, d.v0, a / b);
				setR(d.b, d.v1, a % b);
				boolean of = (a == Long.MIN_VALUE && b == -1);
				setFlags(getR(d.a), of);
				return;
			}
		} else if (d.tt == 2 && isYKind(d.a)) {
			// YC: Y <- Y + C2
			if (isRegKind(d.a)) {
				// AC
				int rd = toRegIndex(d.a, d.v0);
				long before = R[rd];
				long rhs = d.c2;
				long res = before / rhs;
				boolean of = (before == Long.MIN_VALUE && rhs == -1);
				R[rd] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a)) {
				// FC
				int rd = toRegIndex(d.a, d.v0);
				double before = F[rd];
				double rhs = d.c2;
				double res = before / rhs;
				F[rd] = res;
				setFlags(res);
				return;
			}
		} else if (d.tt == 3) {
			// AAC, FFC
			if (isRegKind(d.a) && isRegKind(d.b)) {
				// AC
				int rd = toRegIndex(d.b, d.v1);
				long before = R[rd];
				long rhs = d.c3;
				long res = before / rhs;
				boolean of = (before == Long.MIN_VALUE && rhs == -1);
				R[toRegIndex(d.a, d.v0)] = res;
				setFlags(res, of);
				return;
			} else if (isFPKind(d.a) && isFPKind(d.b)) {
				// FC
				int rd = toRegIndex(d.b, d.v1);
				double before = F[rd];
				double rhs = d.c3;
				double res = before / rhs;
				F[toRegIndex(d.a, d.v0)] = res;
				setFlags(res);
				return;
			}
		}
		throw new IllegalStateException("DIV/RECIP form not implemented");
	}

	// ---- 17: COMPL ----
	private void opCOMPL(Decoded d) {
		if (d.tt == 0 && isRegKind(d.a)) {
			setR(d.a, d.v0, ~getR(d.a, d.v0));
			return;
		}
		throw new IllegalStateException("COMPL form not implemented");
	}

	// ---- 18/19/20: AND/OR/XOR ----

	private void bitwise(Decoded d) {
		assert(Opcode.AND.code == 18);
		assert(Opcode.OR.code == 19);
		assert(Opcode.XOR.code == 20);
		assert(Opcode.LSHIFT.code == 23);
		assert(Opcode.RSHIFT.code == 24);
		assert(Opcode.ARSHIFT.code == 25);
		assert(Opcode.LROTATE.code == 26);
		assert(Opcode.RROTATE.code == 27);
		boolean isGood = false;
		long lhs = 0, rhs = 0;
		if (d.tt == 0 && isRegKind(d.a) && isRegKind(d.b)) {
			// RR, RRR
			int count = d.getArgCount();
			if (count == 2 || count == 3) {
				if (count == 2) {
					lhs = getR(d.a, d.v0);
					rhs = getR(d.b, d.v1);
				} else {
					lhs = getR(d.b, d.v1);
					rhs = getR(d.c, d.v2);
				}
				isGood = true;
			}
		} else if (d.tt == 2) {
			// RC
			lhs = getR(d.a, d.v0);
			rhs = d.c2;
			isGood = true;
		} else if (d.tt == 3) {
			// RRC
			lhs = getR(d.b, d.v1);
			rhs = d.c3;
			isGood = true;
		}
		if (isGood) {
			if (rhs < 0 && (d.op >= 23)) throw new CPUException("Illegal negative shift on bitwise operator");
			long res = switch (d.op) {
				case 18 -> lhs & rhs;
				case 19 -> lhs | rhs;
				case 20 -> lhs ^ rhs;
				case 23 -> lhs << rhs;
				case 24 -> lhs >>> rhs;
				case 25 -> lhs >> rhs;
				case 26 -> Long.rotateLeft(lhs, (int)rhs);
				case 27 -> Long.rotateRight(lhs, (int)rhs);
				default -> throw new CPUException("Illegal bitwise operator");
			};
			setR(d.a, d.v0, res);
		} else
			throw new CPUException("Illegal bitwise arguments");
	}

	// ---- 21: TEST (sets SR from X) ----
	private void opTEST(Decoded d) {
		if (d.tt != 1) {
			if (isFPKind(d.a)) {
				double f = getFP(d.a, d.v0);
				setFlags(f);
				return;
			} else {
				long bits = getO(d.a, d.v0);
				setFlags(bits, false);
				return;
			}
		}
		throw new IllegalStateException("TEST form not implemented");
	}

	// ---- 22: CMP (AA/AC/FF -> set SR from (lhs - rhs)) ----
	private void opCMP(Decoded d) {
		if (d.tt == 0 && !isFPKind(d.a) && !isFPKind(d.b)) {
			long a1 = getY(d.a, d.v0);
			long a2 = getY(d.b, d.v1);
			long res = a1 - a2;
			setFlagsFromSubtract(a1, a2, res);
			return;
		}
		if (d.tt == 2 && !isFPKind(d.a)) {
			long a = getY(d.a, d.v0);
			long c = d.c2;
			long res = a - c;
			setFlagsFromSubtract(a, c, res);
			return;
		}
		if (d.tt == 0 && isFPKind(d.a) && isFPKind(d.b)) {
			double f1 = F[toRegIndex(d.a, d.v0)];
			double f2 = F[toRegIndex(d.b, d.v1)];
			long res = (long)Math.signum(f1 - f2); // encode as -1/0/+1 for flags
			setFlags(res, false);
			return;
		}
		throw new IllegalStateException("CMP form not implemented");
	}

	private void opOUT(Decoded d) {
		if (d.tt == 0) {
			// QOO
			int count = d.getArgCount();
			if (count == 3) {
				int bytes = (int)getO(d.b, d.v1);
				int port = (int)getO(d.c, d.v2);
				long val = getQ(d.a, d.v0);
				if (isFPKind(d.a) && bytes != 8)
					throw new CPUException("OUT for FP must be 8 bytes");
				else if (bytes < 0 || bytes > 8)
					throw new CPUException("OUT number of bytes must be 0-8");
				var ph = getPortHandler(port);
				if (ph == null)
					throw new CPUException("OUT port " + port + " handler not set");
				ph.write(val, bytes);
				return;
			}
		}
		throw new CPUException("OUT form not implemented");
	}

	private void opIN(Decoded d) {
		if (d.tt == 0) {
			// XOO
			int count = d.getArgCount();
			if (count == 3) {
				int bytes = (int)getO(d.b, d.v1);
				int port = (int)getO(d.c, d.v2);
				if (isFPKind(d.a) && bytes != 8)
					throw new CPUException("IN for FP must be 8 bytes");
				else if (bytes < 0 || bytes > 8)
					throw new CPUException("IN number of bytes must be 0-8");
				var ph = getPortHandler(port);
				if (ph == null)
					throw new CPUException("IN port " + port + " handler not set");
				long val = ph.read(bytes);
				setY(d.a, d.v0, val);
				return;
			}
		}
		throw new CPUException("OUT form not implemented");
	}

	private void opPACK(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 2) {
				setR(d.a, d.v0, ((getR(d.a, d.v0) & 0xFFFF) << 16) | (getR(d.b, d.v1) & 0xFFFF));
				return;
			} else if (count == 4) {
				setR(d.a, d.v0,
					((getR(d.a, d.v0) & 0xFF) << 24) |
					((getR(d.b, d.v1) & 0xFF) << 16) |
					((getR(d.c, d.v2) & 0xFF) << 8) |
					(getR(d.d, d.v3) & 0xFF)
				);
				return;
			}
		}
		throw new CPUException("PACK form not implemented");
	}

	private void opPACK64(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 2) {
				setR(d.a, d.v0, ((getR(d.a, d.v0) & 0xFFFFFFFFL) << 32) | (getR(d.b, d.v1) & 0xFFFFFFFFL));
				return;
			} else if (count == 4) {
				setR(d.a, d.v0,
					((getR(d.a, d.v0) & 0xFFFF) << 48) |
					((getR(d.b, d.v1) & 0xFFFF) << 32) |
					((getR(d.c, d.v2) & 0xFFFF) << 16) |
					(getR(d.d, d.v3) & 0xFFFF)
				);
				return;
			}
		}
		throw new CPUException("PACK64 form not implemented");
	}

	private void opUNPACK(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 2) {
				long v = getR(d.a, d.v0);
				setR(d.a, d.v0, (v >> 16) & 0xFFFF);
				setR(d.b, d.v1, v & 0xFFFF);
				return;
			} else if (count == 4) {
				long v = getR(d.a, d.v0);
				setR(d.d, d.v3, v & 0xFF);
				v >>= 8;
				setR(d.c, d.v2, v & 0xFF);
				v >>= 8;
				setR(d.b, d.v1, v & 0xFF);
				v >>= 8;
				setR(d.a, d.v0, v & 0xFF);
				return;
			}
		}
		throw new CPUException("UNPACK form not implemented");
	}

	private void opUNPACK64(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 2) {
				long v = getR(d.a, d.v0);
				setR(d.a, d.v0, (v >> 32) & 0xFFFFFFFFL);
				setR(d.b, d.v1, v & 0xFFFFFFFFL);
				return;
			} else if (count == 4) {
				long v = getR(d.a, d.v0);
				setR(d.d, d.v3, v & 0xFFFFL);
				v >>= 16;
				setR(d.c, d.v2, v & 0xFFFFL);
				v >>= 16;
				setR(d.b, d.v1, v & 0xFFFFL);
				v >>= 16;
				setR(d.a, d.v0, v & 0xFFFFL);
				return;
			}
		}
		throw new CPUException("UNPACK64 form not implemented");
	}

	private void opCAS(Decoded d) {
		// OOAO
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 3 || count == 4) {
				long oldVal = getO(d.a, d.v0);
				long newVal = getO(d.b, d.v1);
				long base = getR(d.c, d.v2);
				long offset = (count == 4) ? getO(d.d, d.v3) : 0;
				int addr = Math.toIntExact(base + offset);
				try {
					boolean ok = atomicMem.compareAndSet(mem, addr, oldVal, newVal);
					setFlags(0, ok);
					return;
				} catch (Exception ex) {
					throw new CPUException("Illegal memory access at " + addr);
				}
			}
		}
		throw new CPUException("CAS form not implemented");
	}

	private void opENDIAN(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count != 2)
				throw new CPUException("ENDIAN must have two arguments");
			boolean littleEndian = getO(d.b, d.v1) == 0 ? false : true;
			var ph = getPortHandler((int)getO(d.a, d.v0));
			if (ph != null)
				ph.setEndian(littleEndian);
		}
	}

	public void opSAVE(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 1 || count == 2) {
				if (d.a != d.b)
					throw new CPUException("SAVE operands must be same type");
				if (isRegKind(d.a)) {
					if (count == 1) d.v1 = 28;
					for (int i = d.v0; i <= d.v1; ++i)
						push(R[i]);
					return;
				} else if (isFPKind(d.a)) {
					if (count == 1) d.v1 = 31;
					for (int i = d.v0; i <= d.v1; ++i)
						fpush(F[i]);
					return;
				}
			}
		}
		throw new CPUException("SAVE form not implemented");
	}

	public void opRESTORE(Decoded d) {
		if (d.tt == 0) {
			int count = d.getArgCount();
			if (count == 1 || count == 2) {
				if (d.a != d.b)
					throw new CPUException("RESTORE operands must be same type");
				if (isRegKind(d.a)) {
					if (count == 1) d.v1 = 28;
					for (int i = d.v1; i >= d.v0; --i)
						R[i] = pop();
					return;
				} else if (isFPKind(d.a)) {
					if (count == 1) d.v1 = 31;
					for (int i = d.v1; i >= d.v0; --i)
						F[i] = fpop();
					return;
				}
			}
		}
		throw new CPUException("RESTORE form not implemented");
	}

	// ====== PUBLIC CONTROLS ======
	public long getPID() {return pid;}
	public Vector<Long> getChildPIDs() {
		return childCPUs.stream().map(x -> x.getPID()).collect(Collectors.toCollection(Vector::new));
	}
	public Simulator getChildCPU(int pid) {
		for (var child : childCPUs) {
			if (child.getPID() == pid)
				return child;
		}
		return null;
	}
	public ChildProcess getProcess() {return process;}
	public ChildThread getThread() {return thread;}
	// Return elapsed clock time
	public long getClock(){return System.nanoTime()-startClock;}
	// Return CPU cycles elapsed
	public long getCycles(){return cycles;}
	// Return elapsed time spent in interrupts
	public long systemClock(){return totalSystemTime;}

	public int getCommandLineCount() {
		return args.length;
	}

	public String getCommandLineArg(int i) {
		return args[i];
	}

	public void setSP(long sp) { R[R_SP] = sp; }
	public void setSF(long sf) { R[R_SF] = sf; }
	public void setPC(long pc) { R[R_PC] = pc; }
	public void setDebug(boolean on) { this.debug = on; }

	public long getR(int r) { return R[r & 0x1F]; }
	public void setR(int r, long v) { R[r & 0x1F] = v; setFlags(v, false); }
	public double getFP(int f) { return F[f & 0x1F]; }
	public void setFP(int f, double v) { F[f & 0x1F] = v; setFlags(v); }
	public void push(long val) {
		memWrite(R[R_SP], val);
		R[R_SP] = R[R_SP] - 1;
	}
	public long pop() {
		R[R_SP] = R[R_SP] + 1;
		return memRead(R[R_SP]);
	}
	public void fpush(double val) {
		memWrite(R[R_SP], Double.doubleToRawLongBits(val));
		R[R_SP] = R[R_SP] - 1;
	}
	public double fpop() {
		R[R_SP] = R[R_SP] + 1;
		return Double.longBitsToDouble(memRead(R[R_SP]));
	}

	// Pointer to the first free block in the heap.  The free list is composed of a
	// linked list of allocation blocks in the heap.  Allocation blocks are preceded by
	// a list data structure containing three codewords ==> PREV:NEXT:SIZE:DATA....
	// The PREV pointer contains the address of the prior block in the free list or 0x0.
	// The NEXT pointer contains the address of the next block in the free list or 0x0.
	// The SIZE word contains the size of the DATA portion of the block.
	// SIZE < 0 indicates a free block, whereas SIZE > 0 indicates an allocated block.
	// The remaining SIZE words comprise the DATA portion of the block.
	// Pointers are actually offsets into the heap array.  The physical address for
	// a pointer is really p + codeLimit.  Pointers to a block always point to the
	// beginning of the block so PREV is p + 0, NEXT is p + 1, SIZE is p + 2 and
	// the data begins as p + 3

// Block structure
// prev:next:size:data....
// prev or next == -1 for null link
// size block where < 0 for free block, size > 0 for alloc block
// size includes the three word header
// Pointers are actually offsets into the heap array so their

	private static long[] fibonacci = {8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584,
			4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418, 317811, 514229, 832040,
			1346269, 2178309, 3524578, 5702887, 9227465, 14930352, 24157817, 39088169, 63245986,
			102334155, 165580141, 267914296, 433494437, 701408733, 1134903170, 1836311903};
	private static long fibonacciSize(long size)
	{
		int i;
		for (i = 0; i < fibonacci.length && fibonacci[i] < size; ++i) { /* nop */ }
		return i == fibonacci.length ? size : fibonacci[i];
	}

	public long alloc(long requestedWords) throws CPUException {
		long start = System.nanoTime();
		if (freeList.isEmpty()) return 0L;
		long p = 0;
		boolean found = false;
		synchronized(mem) {
			if (requestedWords < 1 || requestedWords > Integer.MAX_VALUE - 3)
				throw new CPUException("Illegal memory allocation: " + requestedWords);
			long numWords = requestedWords;
			numWords = fibonacciSize(numWords + 3);

			//Utils.debug("Allocating %d (%d) words...",requested_words,num_words);
			// find free block big enough
			for (Iterator<Long> it = freeList.iterator(); it.hasNext(); ) {
				p = it.next();
				long size = memRead(p + 2);
				if (size < 0 && -size >= numWords) {
					if (-size - numWords < fibonacci[0]) {					// Just use existing block if excess is smaller than smallest possible block
						memWrite(p + 2, -size);								// Update size to reflect block is allocated
						it.remove();
						found = true;
						break;
					} else {												// Split block into 1 alloc and 1 free
						long new_p = p + numWords;							// Point to new block
						memWrite(new_p, p);									// set new free block's prev pointer
						memWrite(new_p + 1, memRead(p + 1));				// set new free block's next pointer
						memWrite(new_p + 2, size + numWords);				// calc new free block size
						memWrite(p + 1, new_p);
						memWrite(p + 2, numWords);
						it.remove();
						freeList.push(new_p);
						found = true;
						break;
					}
				}
			}
		}
		long stop = System.nanoTime();
		totalSystemTime += stop - start;
		return found ? p + 3 : 0;
	}

	public long realloc(long oldAddr, long newSize) {
		long start = System.nanoTime();
		//if (!validAllocation(oldAddr)) throw new CPUException("realloc() argument is invalid!");
		long newAddr = oldAddr;
		long oldSize = memRead(oldAddr - 1) ;				// old size
		newSize = fibonacciSize(newSize + 3);
		if (oldSize < newSize) {
			newAddr = alloc(newSize - 3);						// new address
			if (newAddr != 0) {
				memmove(newAddr, oldAddr, oldSize - 3);
				free(oldAddr);
			}
		} else if (oldSize > newSize) {						// trim
			newAddr = alloc(newSize - 3);						// new address
			if (newAddr != 0) {
				memmove(newAddr, oldAddr, newSize - 3);
				free(oldAddr);
			}
		}
		//if (!validAllocation(newAddr)) throw new CPUException("realloc() result is invalid!");
//walkHeap(0);
		long stop = System.nanoTime();
		totalSystemTime += stop - start;
		return newAddr;
	}

	// combines block at p with next block if both free
	private void combineBlocks(long p) {
		if (p <= 0) { return; }							// Don't free null (0)
		long size = memRead(p + 2);
		if (size >= 0) return;							// block is not free
		long prev = memRead(p);
		long next = memRead(p + 1);
		if (next > 0 ) {								// check for end of list
			long nextSize = memRead(next + 2);
			if (nextSize < 0) {							// confirm next block is free
				memWrite(p + 2, nextSize + size);		// calc joined size
				long next_next = memRead(next + 1);
				memWrite(p + 1, next_next);			    // assume next block's next pointer
				if (next_next > 0)
					memWrite(next_next, p);				// set new next block's prev pointer to p
				memWrite(next, -1);           			// invalidate old next's prev pointer
				memWrite(next + 1, -1);           		// invalidate old next's next pointer
				memWrite(next + 2, 0);           		// invalidate old next's size
				freeList.remove(next);
			}
		}
	}

	public void free(long address) {
		long start = System.nanoTime();
		synchronized(mem) {
//Utils.debug("Freeing @"+ADDRESS.format(address));
			//if (!validAllocation(address)) throw new CPUException("free() argument is invalid!");
			long p = address - 3;
			if (p <= 0) { return; }								// Don't free null (0)

			// Negate size to indicate free block in heap list
			long size = memRead(p + 2);
			if (size > 0) memWrite(p + 2, -size);
			else return;

			// Add to free list
			freeList.push(p);

			// Combine with next block if free
			combineBlocks(p);
			// Combine with previous block if free
			combineBlocks(memRead(p));

//			if (!validAllocation(prev+3+codeLimit)) throw new CPUException("free() low block is invalid!");
//			if (!validAllocation(next+3+codeLimit)) throw new CPUException("free() high block is invalid!");
		}
		long stop = System.nanoTime();
		totalSystemTime += stop - start;
	}

	public long countHeapBlocks(boolean countAlloc, boolean countFree) {
		long p = heapStart;
		long numAlloc = 0;
		long numFree = 0;
		while (p > 0) {
			if (memRead(p + 2) < 0)
				++numFree;
			else
				++numAlloc;
			p = memRead(p + 1);
		}
		if (countAlloc && countFree)
			return numAlloc + numFree;
		else if (countAlloc)
			return numAlloc;
		else if (countFree)
			return numFree;
		else return 0;
	}

	public long countHeapSize(boolean countAlloc, boolean countFree) {
		long p = heapStart;
		long numAlloc = 0;
		long numFree = 0;
		while (p > 0) {
			long size = memRead(p + 2);
			if (size < 0)
				numFree += -size;
			else
				numAlloc += size;
			p = memRead(p + 1);
		}
		if (countAlloc && countFree)
			return numAlloc + numFree;
		else if (countAlloc)
			return numAlloc;
		else if (countFree)
			return numFree;
		else return 0;
	}

	public void memmove(long dest, long src, long size) {
		long start = System.nanoTime();
		if (dest < 0 || dest > mem.length) throw new CPUException("Illegal memmove dest argument!");
		if (src < 0 || src > mem.length) throw new CPUException("Illegal memmove src argument!");
		if (src + size > mem.length || dest + size > mem.length) throw new CPUException("Illegal memmove size argument!");
		System.arraycopy(mem, (int)src, mem, (int)dest, (int)size);
		long stop = System.nanoTime();
		totalSystemTime += stop - start;
	}

	public void memclear(long src, long size) {
		long start = System.nanoTime();
		if (src < 0 || src > mem.length) throw new CPUException("Illegal memclear src argument!");
		if (src + size > mem.length) throw new CPUException("Illegal memclear size argument!");
		Arrays.fill(mem, (int)src, (int)(src + size), 0L);
		long stop = System.nanoTime();
		totalSystemTime += stop - start;
	}

	public long allocString(String s) {
		return allocString(s, 0);
	}

	public long allocString(String s, long allocBuffer) {
		byte[] utf8 = s.getBytes(StandardCharsets.UTF_8);
		long neededWords = (utf8.length + 7) / 8 + 1;
		long result;
		if (allocBuffer <= 0) {
			result = alloc(neededWords);
		} else if (memRead(allocBuffer - 1) - 3 >= neededWords) {
			result = allocBuffer;
		} else {
			free(allocBuffer);
			result = alloc(neededWords);
		}
		if (result != 0) {
			long p = result;
			memWrite(p++, (long) utf8.length);
			long buffer = 0;
			int index = 7;
			for (var c : utf8) {
				buffer |= (c & 0xFFL) << (index * 8);
				--index;
				if (index < 0) {
					memWrite(p++, buffer);
					buffer = 0;
					index = 7;
				}
			}
			if (index != 7) {
				memWrite(p, buffer);
			}
		}
		return result;
	}

	public String convertString(long addr) {
		if (addr < 0) throw new CPUException("Illegal string address");
		long byteCount = memRead(addr++);
		if (byteCount < 0 || byteCount > (long)Integer.MAX_VALUE)
			throw new CPUException("Invalid UTF-8 byte count: " + byteCount);

		byte[] bytes = new byte[(int) byteCount];
		int outIndex = 0;

		// unpack bytes from each subsequent long
		for (; outIndex < byteCount; ++addr) {
			long w = memRead(addr);
			for (int b = 7; b >= 0 && outIndex < byteCount; --b) {
				bytes[outIndex++] = (byte)((w >> (b * 8)) & 0xFF);
			}
		}
		return new String(bytes, StandardCharsets.UTF_8);
	}

	public static class ChildProcess extends RecursiveAction {
		Simulator cpu;
		Simulator parentCPU;

		public ChildProcess(Simulator cpu, Simulator parent) {
			this.cpu = cpu;
			this.cpu.process = this;
			parentCPU = parent;
		}

		@Override
		protected void compute() {
			try {
				cpu.run();
			} catch (Exception ex) {
				throw ex;
			}
		}
	}

	public long fork() throws CPUException
	{
		Simulator childCPU;
		try {
			childCPU = new Simulator(this, true);
			childCPU.setR(R_PC, childCPU.getR(R_PC) + 1);
			childCPU.setR(0, 0);
			ChildProcess child = new ChildProcess(childCPU, this);
			synchronized (childCPUs) {
				childCPUs.add(childCPU);
			}
			child.fork();
			return childCPU.getPID();
		} catch (Exception ex) {
//			throw new CPUException("Child process creation failed! %s", ex.getMessage());
			return -1;
		}
	}

	public void waitAll() throws CPUException {
		Vector<Long> pids = getChildPIDs();
		for (var pid : pids) {
			waitPID(pid);
		}
	}

	public void waitPID(long pid) {
		Simulator childCPU = getChildCPU((int)pid);
		if (childCPU != null) {
			Simulator.ChildProcess p = childCPU.getProcess();
			if (p != null) {
				p.join();
				synchronized (childCPUs) {
					childCPUs.remove(childCPU);
				}
			}
		}
	}

	public static class ChildThread extends Thread {
		Simulator cpu;

		public ChildThread(Simulator cpu) {
			this.cpu = cpu;
			this.cpu.thread = this;
		}

		@Override
		public void run() {
			try {
				cpu.run();
			} catch (Exception ex) {
//				throw new CPUException("Child thread run failed! %s", ex.getMessage());
			}
		}
	}

	public long thread(long function, long data) throws CPUException
	{
		Simulator childCPU;
		try {
			childCPU = new Simulator(this, false);
			childCPU.push(data);
			childCPU.push(-1);					// push return address, -1 exits
			childCPU.push(childCPU.getR(R_SF));	// push old stack frame so it can be restored on return
			childCPU.setR(R_SF, childCPU.getR(R_SP));// set new stack frame for call
			childCPU.setR(R_PC, function);
			ChildThread child = new ChildThread(childCPU);
			synchronized (childCPUs) {
				childCPUs.add(childCPU);
			}
			child.start();
			return childCPU.getPID();
		} catch (Exception ex) {
//			throw new CPUException("Child thread creation failed! %s", ex.getMessage());
		}
		return -1;
	}

	public void joinThread(long pid) {
		Simulator childCPU = getChildCPU((int)getR(0));
		if (childCPU != null) {
			Simulator.ChildThread t = childCPU.getThread();
			if (t != null) {
				try {
					t.join();
				} catch (InterruptedException e) {
				}
				synchronized (childCPUs) {
					childCPUs.remove(childCPU);
				}
			}
		}
	}

	private HashMap<Integer, PortHandler> ports = new HashMap<>();
	public PortHandler getPortHandler(int port) {
		return ports.get(port);
	}

	public void setPortHandler(int port, PortHandler ph) {
		ports.put(port, ph);
	}

	public void exit(int code) {
		setR(0, code);
		running = false;
	}

	public void printCPUState() {
		System.out.printf("\nCPU State    SR: %s\n", formatSR());
		System.out.printf(fmtHeading,(Object[])cpuLabels);
		int stack_base = (int)R[R_SP] + FPR_COUNT;
		if (stack_base >= stackBase) stack_base = (int)stackBase - 1;
		for (int i = 0; i < FPR_COUNT; ++i) {
			if (i >= R_SF) System.out.printf(fmtCPUAlt,registers[i],R[i],registersFP[i],F[i]);
			else System.out.printf(fmtCPU,registers[i],R[i],R[i],registersFP[i],F[i]);
			if (stack_base - i > R[R_SP]) {
				long v = memRead(stack_base - i);
				double f = Double.longBitsToDouble(v);
				System.out.printf(fmtStack, (stack_base - i == R[R_SF])?"=>":"  ",
						stack_base - i - R[R_SP], v, v, f);
			} else if (stack_base - i == R[R_SP] && stack_base - i == R[R_SF]) {
				System.out.print(" =>");
			}
			System.out.println("");
		}

		System.out.flush();
	}

	public static String formatAddress(long addr)
	{
		return String.format(fmtAddress,(int)addr);
	}

	public static String formatWord(long x)
	{
		return String.format(fmtHex,x);
	}

	// ===== Encoders for testing =====
	public static long encT0(int op, int a,int b,int c,int d, int v0,int v1,int v2,int v3) {
		long w = 0;
		w |= (0L << 62);
		w |= ((long)op & 0x3F) << 56;
		w |= ((long)a & 3) << 54;
		w |= ((long)b & 3) << 52;
		w |= ((long)c & 3) << 50;
		w |= ((long)d & 3) << 48;
		w |= ((long)v0 & 0xFFF) << 36;
		w |= ((long)v1 & 0xFFF) << 24;
		w |= ((long)v2 & 0xFFF) << 12;
		w |= (long)v3 & 0xFFF;
		return w;
	}
	public static long encT1(int op, long imm56) {
		long w = 0;
		w |= (1L << 62);
		w |= ((long)op & 0x3F) << 56;
		w |= imm56 & ((1L<<56)-1);
		return w;
	}
	public static long encT2(int op, int aKind, int v0, long imm42) {
		long w = 0;
		w |= (2L << 62);
		w |= ((long)op & 0x3F) << 56;
		w |= ((long)aKind & 3) << 54;
		w |= ((long)v0 & 0xFFF) << 42;
		w |= imm42 & ((1L<<42)-1);
		return w;
	}
	public static long encT3(int op, int aKind,int bKind,int v0,int v1, int imm28) {
		long w = 0;
		w |= (3L << 62);
		w |= ((long)op & 0x3F) << 56;
		w |= ((long)aKind & 3) << 54;
		w |= ((long)bKind & 3) << 52;
		w |= ((long)v0 & 0xFFF) << 40;
		w |= ((long)v1 & 0xFFF) << 28;
		w |= (long)imm28 & ((1L<<28)-1);
		return w;
	}

	public final class CPUState {
		long[] R = new long[GPR_COUNT];
		double[] F = new double[FPR_COUNT];
		long SR;
		long cycles;

		public CPUState() {
			for (int i = 0; i < GPR_COUNT; ++i)
				R[i] = Simulator.this.R[i];
			for (int i = 0; i < FPR_COUNT; ++i)
				F[i] = Simulator.this.F[i];
			SR = Simulator.this.SR;
			cycles = Simulator.this.cycles;
		}
	}

	public CPUState getState() {
		return new CPUState();
	}

	public List<String> diffState(CPUState start) {
		List<String> diffs = new ArrayList<String>();
		CPUState end = getState();
		for (int i = 0; i < GPR_COUNT; ++i)
			if (start.R[i] != end.R[i]) {
				if (i == R_SF)
					diffs.add("SF:" + end.R[R_SF]);
				else if (i == R_SP)
					diffs.add("SP:" + end.R[R_SP]);
				else if (i == R_PC)
					diffs.add("PC:" + end.R[R_PC]);
				else
					diffs.add("R" + i + ":" + end.R[i]);
			}
		for (int i = 0; i < FPR_COUNT; ++i)
			if (start.F[i] != end.F[i]) diffs.add("F" + i + ":" + end.F[i]);
		if (start.SR != end.SR) diffs.add("SR:" + end.SR);
		return diffs;
	}
}
