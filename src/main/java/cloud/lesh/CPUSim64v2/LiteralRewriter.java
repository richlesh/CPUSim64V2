package cloud.lesh.CPUSim64v2;

import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.tree.ParseTree;

import java.util.ArrayList;
import java.util.List;

public class LiteralRewriter extends LiteralSubstitutionBaseVisitor<Void> {
	private final StringBuilder out = new StringBuilder();
	private final List<String> floats = new ArrayList<>();
	private final List<String> strings = new ArrayList<>();

	public String rewrite(String src) {
		CharStream input = CharStreams.fromString(src);
		LiteralSubstitutionLexer lexer = new LiteralSubstitutionLexer(input);
		CommonTokenStream tokens = new CommonTokenStream(lexer);
		LiteralSubstitutionParser parser = new LiteralSubstitutionParser(tokens);
		ParseTree tree = parser.file();
		visit(tree);

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
			case 'n':  return '\n';
			case 't':  return '\t';
			case 'r':  return '\r';
			case 'b':  return '\b';
			case 'f':  return '\f';
			case '\\': return '\\';
			case '\'': return '\'';
			case '\"': return '\"';

			// Octal escapes: \0 .. \377 (up to 3 octal digits, C/Java-style)
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': {
				int i = 1, val = 0, count = 0;
				while (i < s.length() && count < 3) {
					char c = s.charAt(i);
					if (c < '0' || c > '7') break;
					val = (val << 3) + (c - '0');
					i++; count++;
				}
				return val & 0xFF;
			}

			// Unicode escape: \\uXXXX (exactly 4 hex digits)
			case 'u': case 'U': {
				if (s.length() >= 6) {
					String hex = s.substring(2, 6);
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
