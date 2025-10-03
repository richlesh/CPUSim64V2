package cloud.lesh.CPUSim64v2;

import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.Interval;
import org.antlr.v4.runtime.tree.ParseTree;
import org.antlr.v4.runtime.tree.ErrorNode;
import org.antlr.v4.runtime.tree.TerminalNode;
import org.apache.commons.lang3.StringEscapeUtils;

import java.util.*;
import java.util.regex.*;
import java.nio.charset.StandardCharsets;

public class LabelVisitor extends CPUSim64v2BaseVisitor<Void> implements HasLocation {
	private final StringBuilder out = new StringBuilder();
	private final Map<String, Long> labelMap = new HashMap<>();
	private final Set<String> definedLabels = new HashSet<>();
	private final Stack<String> blockNames = new Stack<>();
	private long currentAddress = 0;

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
		return labelMap;
	}

	private static Token startToken(ParseTree node) {
		if (node instanceof ParserRuleContext r) return r.getStart();
		if (node instanceof TerminalNode t)      return t.getSymbol();
		if (node instanceof ErrorNode e)         return e.getSymbol();
		return null;
	}

	@Override
	public Void visitProgram(CPUSim64v2Parser.ProgramContext ctx) {
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
		labelMap.putIfAbsent("__CODE_END__", currentAddress);
		labelMap.putIfAbsent("__HEAP_START__", currentAddress);
		return null;
	}

	private String getScopeName() {
		return String.join("$", blockNames);
	}

	@Override
	public Void visitLabelDef(CPUSim64v2Parser.LabelDefContext ctx) {
		String labelName = ctx.IDENT().getText();
		if (definedLabels.contains(labelName)) {
			errors.add(getLocation() + ": Error: Duplicate label '" + labelName + "'");
		} else {
			if (labelName.charAt(0) == '$')
				labelName = getScopeName() + labelName;
			definedLabels.add(labelName);
			labelMap.put(labelName, currentAddress);
		}
		return null;
	}

	@Override
	public Void visitInstruction(CPUSim64v2Parser.InstructionContext ctx) {
		++currentAddress;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitData_Directive(CPUSim64v2Parser.Data_DirectiveContext ctx) {
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
				s = StringEscapeUtils.unescapeJava(s);
				byte[] utf8 = s.getBytes(StandardCharsets.UTF_8);
				currentAddress += 1 + (utf8.length + 7) / 8;  // round up to nearest 8 bytes
			} else if (ctx.dataDirective().DCB() != null) {
				currentAddress += 1 + (ctx.dataDirective().byteList().bLiteral().size() + 7) / 8;
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
	public Void visitORG_Directive(CPUSim64v2Parser.ORG_DirectiveContext ctx) {
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
	public Void visitLINE_Directive(CPUSim64v2Parser.LINE_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		--lineNum;
		pauseLineIncrement = false;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_BEGIN_Directive(CPUSim64v2Parser.LINE_BEGIN_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		pauseLineIncrement = true;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_END_Directive(CPUSim64v2Parser.LINE_END_DirectiveContext ctx) {
		pauseLineIncrement = false;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitBLOCK_BEGIN_Directive(CPUSim64v2Parser.BLOCK_BEGIN_DirectiveContext ctx) {
		String blockname = ctx.IDENT().getText();
		blockNames.push(blockname);
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitBLOCK_END_Directive(CPUSim64v2Parser.BLOCK_END_DirectiveContext ctx) {
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
		var lex = new cloud.lesh.CPUSim64v2.CPUSim64v2Lexer(input);
		var lexerListener = new CollectingErrorListener(errors, null);
		lex.removeErrorListeners();                // remove ConsoleErrorListener
		lex.addErrorListener(lexerListener);       // collect lexer errors
		CommonTokenStream toks = new CommonTokenStream(lex);
//		if (errors.size() > 0) return "";

		var parser = new cloud.lesh.CPUSim64v2.CPUSim64v2Parser(toks);
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
