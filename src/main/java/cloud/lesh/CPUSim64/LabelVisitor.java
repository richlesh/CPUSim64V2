package cloud.lesh.CPUSim64;

import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.Interval;
import org.antlr.v4.runtime.tree.ParseTree;
import org.antlr.v4.runtime.tree.ErrorNode;
import org.antlr.v4.runtime.tree.TerminalNode;

import java.util.*;
import java.util.regex.*;

public class LabelVisitor extends CPUSim64BaseVisitor<Void> implements HasLocation {
	private final StringBuilder out = new StringBuilder();
	private final Map<String, Long> labelMap = new HashMap<>();
	private final Map<Long, String> reverseLabelMap = new HashMap<>();
	private final Set<String> definedLabels = new HashSet<>();
	private final Stack<String> blockNames = new Stack<>();
	private long currentAddress = 0;
	private long blockCount = 0;

	String filename = null;
	int lineNum = 1;
	boolean pauseLineIncrement = false;
	private final Vector<String> errors = new Vector<String>();
	Map<Integer, String> lineMap = new HashMap<Integer, String>();

	public String getLocation() {
		return (filename == null ? "" : filename + ":") + lineNum;
	}
	public List<String> getErrors() {
		return errors;
	}
	public Map<Integer, String> getLineMap() { return lineMap; }

	public Map<String, Long> getLabelMap() {
		labelMap.putIfAbsent("__START__", 0L);
		labelMap.putIfAbsent("__CODE__", 0L);
		labelMap.putIfAbsent("__CODE_END__", currentAddress);
		labelMap.putIfAbsent("__DATA__", currentAddress);
		labelMap.putIfAbsent("__DATA_END__", currentAddress);
		labelMap.putIfAbsent("__HEAP_START__", currentAddress);
		return labelMap;
	}

	public Map<Long, String> getReverseLabelMap() {
		return reverseLabelMap;
	}

	private static Token startToken(ParseTree node) {
		if (node instanceof ParserRuleContext r) return r.getStart();
		if (node instanceof TerminalNode t)      return t.getSymbol();
		if (node instanceof ErrorNode e)         return e.getSymbol();
		return null;
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
			throw new IllegalArgumentException("Can't parse integer: " + text);
		}
	}

	private long parseStringLiteral(String s) {
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
						Pattern p = Pattern.compile("\\{([0-9A-Fa-f]{1,5})\\}");
						Matcher m = p.matcher(s.substring(i));
						if (m.find()) {
							String hex = m.group(1);   // the 4 hex digits
							int codePoint = Integer.parseInt(hex, 16);
							return codePoint;
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
	}

	@Override
	public Void visitProgram(CPUSim64Parser.ProgramContext ctx) {
		for (var child : ctx.children) {
			visit(child);
			Token t = startToken(child);
			if (t != null) {
				int line = t.getLine();
				int col  = t.getCharPositionInLine();
				lineMap.put(line, getLocation());
			}
			if (!pauseLineIncrement) ++lineNum;
		}
		return null;
	}

	private String getScopeName() {
		return String.join("$", blockNames).toUpperCase();
	}

	@Override
	public Void visitLabelDef(CPUSim64Parser.LabelDefContext ctx) {
		String labelName = ctx.IDENT().getText().toUpperCase();
		if (definedLabels.contains(labelName)) {
			errors.add(getLocation() + ": Error: Duplicate label '" + labelName + "'");
		} else {
			if (labelName.charAt(0) == '$')
				labelName = getScopeName() + labelName;
			definedLabels.add(labelName);
			labelMap.put(labelName, currentAddress);
			reverseLabelMap.put(currentAddress, labelName);
		}
		return null;
	}

	@Override
	public Void visitInstruction(CPUSim64Parser.InstructionContext ctx) {
		++currentAddress;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitData_Directive(CPUSim64Parser.Data_DirectiveContext ctx) {
		if (ctx.dataDirective() != null) {
			if (ctx.dataDirective().DCI() != null) {
				++currentAddress;
			} else if (ctx.dataDirective().DCF() != null) {
				++currentAddress;
			} else if (ctx.dataDirective().DCS() != null) {
				if (ctx.dataDirective().STRINGLIT() == null || ctx.dataDirective().STRINGLIT().getText().length() < 2)
				{
					errors.add(getLocation() + ": Error: Missing string literal for .DCS directive");
					return null;
				}
				String s = ctx.dataDirective().STRINGLIT().getText();
				s = s.substring(1, s.length() - 1);
				byte[] utf8 = Utils.parseStringLiteral(s);
				currentAddress += 1 + (utf8.length + 7) / 8;  // round up to nearest 8 bytes
			} else if (ctx.dataDirective().DCA() != null) {
				long b = 0;
				if (ctx.dataDirective().INTLIT() != null) {
					b = parseIntLike(ctx.dataDirective().INTLIT().getText());
				} else if (ctx.dataDirective().HEXLIT() != null) {
					b = parseIntLike(ctx.dataDirective().HEXLIT().getText());
				}
				currentAddress += 1 + b;
			} else if (ctx.dataDirective().DCB() != null) {
				currentAddress += 1 + (ctx.dataDirective().byteList().bLiteral().size() + 7) / 8;
			} else if (ctx.dataDirective().DCC() != null) {
				currentAddress += 1 + (ctx.dataDirective().byteList().bLiteral().size() + 3) / 4;
			} else if (ctx.dataDirective().DCW() != null) {
				int count = 0;
				if (ctx.dataDirective().intList() != null) {
					count = ctx.dataDirective().intList().kLiteral().size();
				} else if (ctx.dataDirective().floatList() != null) {
					count = ctx.dataDirective().floatList().FLOATLIT().size();
				} else if (ctx.dataDirective().charList() != null) {
					count = ctx.dataDirective().charList().CHARLIT().size();
				}
				currentAddress += 1 + count;
			}
		}
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitORG_Directive(CPUSim64Parser.ORG_DirectiveContext ctx) {
		if (ctx.INTLIT() != null) {
			currentAddress = Long.parseLong(ctx.INTLIT().getText());
		} else if (ctx.HEXLIT() != null) {
			currentAddress = Long.parseLong(ctx.HEXLIT().getText().substring(2), 16);
		} else {
			errors.add(getLocation() + ": Error: Missing integer literal for .ORG directive");
		}
		currentAddress = Math.max(0, currentAddress); // prevent negative addresses
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_Directive(CPUSim64Parser.LINE_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		--lineNum;
		pauseLineIncrement = false;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_BEGIN_Directive(CPUSim64Parser.LINE_BEGIN_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		pauseLineIncrement = true;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_END_Directive(CPUSim64Parser.LINE_END_DirectiveContext ctx) {
		pauseLineIncrement = false;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitBLOCK_BEGIN_Directive(CPUSim64Parser.BLOCK_BEGIN_DirectiveContext ctx) {
		String blockname = null;
		if (ctx.IDENT() != null) {
			blockname = ctx.IDENT().getText();
			if (blockname.contains("$"))
				blockname = null;
		} else if (ctx.BLOCK_IDENT() != null) {
			blockname = ctx.BLOCK_IDENT().getText();
		}
		if (blockname == null)
			throw new IllegalArgumentException(".block directive must have an argument!");
		if (blockname.contains("{}") || blockname.contains("%d") || blockname.contains("%x"))
			blockname = String.format(blockname.replace("{}", "%04x"), ++blockCount);
		blockNames.push(blockname);
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitBLOCK_END_Directive(CPUSim64Parser.BLOCK_END_DirectiveContext ctx) {
		blockNames.pop();
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	protected Void defaultResult() {
		return null;
	}

	/** Rebuild a directive line with spaces, rather than ctx.getText(). */
	private static String reflowTokens(ParserRuleContext ctx) {
		Token start = ctx.getStart();
		Token stop  = ctx.getStop();
		if (start == null || stop == null) return "";
		// CharStream slice from the original input
		return start.getInputStream().getText(Interval.of(start.getStartIndex(), stop.getStopIndex()));
	}

	public String gatherLabels(String src) {
		CharStream input = CharStreams.fromString(src);
		var lex = new cloud.lesh.CPUSim64.CPUSim64Lexer(input);
		var lexerListener = new CollectingErrorListener(errors, null);
		lex.removeErrorListeners();                // remove ConsoleErrorListener
		lex.addErrorListener(lexerListener);       // collect lexer errors
		CommonTokenStream toks = new CommonTokenStream(lex);
//		if (errors.size() > 0) return "";

		var parser = new cloud.lesh.CPUSim64.CPUSim64Parser(toks);
		var parserListener = new CollectingErrorListener(errors, null);
		parser.removeErrorListeners();             // remove ConsoleErrorListener
		parser.addErrorListener(parserListener);   // collect parser errors
		ParseTree tree = parser.program();
//		if (errors.size() > 0) return "";
		visit(tree);
		Map<Integer, String> lineMap = getLineMap();
		for (int i = 0; i < errors.size(); ++i) {
			String s = (String)errors.get(i);
			if (s.startsWith("Preprocessed line")) {
				// Match and capture the line number
				Matcher m = Pattern.compile("Preprocessed line (\\d+)").matcher(s);
				if (m.find()) {
					int preLine = Integer.parseInt(m.group(1));
					String mapped = lineMap.get(preLine);
					if (mapped != null) {
						// Replace with the mapped value
						errors.set(i, m.replaceAll("Line " + mapped));
					}
				}
			}
		}
		return out.toString();
	}
}
