package cloud.lesh.CPUSim64;

import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.tree.ParseTree;

import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class LiteralRewriter extends LiteralSubstitutionBaseVisitor<Void> implements HasLocation {
	private final StringBuilder out = new StringBuilder();
	private final List<String> floats = new ArrayList<>();
	private final List<String> strings = new ArrayList<>();
	private static Vector<String> errors = new Vector<String>();
	public static Vector<String> getErrors() { return errors; }
	private Map<Integer, String> sourceLocations;

	public String getLocation(int offendingLine) {
		String originalSourceLocation = null;
		int line = offendingLine;
		originalSourceLocation = sourceLocations.get(line);
		while (originalSourceLocation == null && line > 0) {
			originalSourceLocation = sourceLocations.get(line);
			if (originalSourceLocation == null) {
				--line;
			}
		}
		return originalSourceLocation == null ? "" : originalSourceLocation;
	}

	public String rewrite(String src) {
		return rewrite(src, new HashMap<Integer, String>());
	}

	public String rewrite(String src, Map<Integer, String> sourceLocations) {
		this.sourceLocations = sourceLocations;
		errors = new Vector<String>();
		CharStream input = CharStreams.fromString(src);
		LiteralSubstitutionLexer lexer = new LiteralSubstitutionLexer(input);
		var lexerListener = new CollectingErrorListener(errors, this);
		lexer.removeErrorListeners();                // remove ConsoleErrorListener
		lexer.addErrorListener(lexerListener);       // collect lexer errors
		CommonTokenStream tokens = new CommonTokenStream(lexer);
		tokens.fill();

		LiteralSubstitutionParser parser = new LiteralSubstitutionParser(tokens);
		var parserListener = new CollectingErrorListener(errors, this);
		parser.removeErrorListeners();             // remove ConsoleErrorListener
		parser.addErrorListener(parserListener);   // collect parser errors
		ParseTree tree = parser.file();
		visit(tree);

		for (int i = 0; i < errors.size(); ++i) {
			String s = errors.get(i);
			if (s.startsWith("Preprocessed line")) {
				// Match and capture the line number
				Matcher m = Pattern.compile("Preprocessed line (\\d+)").matcher(s);
				if (m.find()) {
					int preLine = Integer.parseInt(m.group(1));
					errors.set(i, m.replaceAll(Integer.toString(preLine)));
				}
			}
		}

		if (errors.size() > 0) {
			for (int i = 0; i < errors.size(); ++i) {
				System.err.println(errors.get(i));
			}
			System.exit(1);
		}

		// Append trailer with symbol definitions
		out.append(System.lineSeparator());
		for (int i = 0; i < floats.size(); i++) {
			out.append(symFP(i + 1)).append(": .DCF ").append(floats.get(i)).append(System.lineSeparator());
		}
		for (int i = 0; i < strings.size(); i++) {
			out.append(symSTR(i + 1)).append(": .DCS ").append(strings.get(i)).append(System.lineSeparator());
		}
		return out.toString();
	}

	private static String symFP(int i)   { return String.format("__FP_%d", i); }
	private static String symSTR(int i)  { return String.format("__STR_%d", i); }

	@Override
	public Void visitFile(LiteralSubstitutionParser.FileContext ctx) {
		return visitChildren(ctx);
	}

	@Override
	public Void visitPiece(LiteralSubstitutionParser.PieceContext ctx) {
		Token t = ctx.getStart();
		switch (t.getType()) {
			case LiteralSubstitutionLexer.FLOAT: {
				if (floats.contains(t.getText())) {
					int idx = floats.indexOf(t.getText()) + 1;
					out.append(symFP(idx));
				} else {
					int idx = floats.size() + 1;
					floats.add(t.getText());
					out.append(symFP(idx));
				}
				break;
			}
			case LiteralSubstitutionLexer.STRING: {
				if (strings.contains(t.getText())) {
					int idx = strings.indexOf(t.getText()) + 1;
					out.append(symSTR(idx));
				} else {
					int idx = strings.size() + 1;
					strings.add(t.getText());
					out.append(symSTR(idx));
				}
				break;
			}
			case LiteralSubstitutionLexer.CHARLIT: {
				int val = parseCharLiteral(t.getText());
				out.append(String.format("0x%X", val));
				break;
			}
			default:
				// NEWLINE and OTHER → pass through verbatim
				out.append(t.getText());
				break;
		}
		return null;
	}

	private int parseCharLiteral(String s) {
		if (s == null || s.isEmpty()) return 0xFFFD;

		// Strip surrounding single quotes if present:  'X'  or  '\n'  or  '\u1234'
		if (s.length() >= 2 && s.charAt(0) == '\'' && s.charAt(s.length() - 1) == '\'') {
			s = s.substring(1, s.length() - 1);
		}
		if (s.isEmpty()) return 0xFFFD;

		// Non-escape: return full code point (handles supplementary chars)
		if (s.charAt(0) != '\\') {
			return s.codePointAt(0);
		}

		// Escape sequences
		if (s.length() == 1) return 0xFFFD; // lone backslash is invalid
		char next = s.charAt(1);
		switch (next) {
			case '0':  return '\0';
			case 'n':  return '\n';
			case 't':  return '\t';
			case 'r':  return '\r';
			case 'b':  return '\b';
			case 'f':  return '\f';
			case '\\': return '\\';
			case '\'': return '\'';
			case '\"': return '\"';

			// Unicode escape: \\u{XXXX} (xxxx is 1-6 hex digits)
			case 'u': case 'U': {
				int len = s.length();
				if (len > 4 && len <= 10) {
					String hex = s.substring(3, len - 1);
					if (hex.chars().allMatch(ch -> Character.digit(ch, 16) != -1)) {
						return Integer.parseInt(hex, 16);
					}
				}
				return 0xFFFD; // invalid \\u escape
			}

			default:
				// Treat unknown escape as the char itself (e.g., '\;' -> ';'), or return error marker
				return next;
		}
	}
}
