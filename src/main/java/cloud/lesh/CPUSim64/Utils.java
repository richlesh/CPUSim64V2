package cloud.lesh.CPUSim64;

import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.ParserRuleContext;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.misc.Interval;
import org.antlr.v4.runtime.tree.ParseTree;
import org.antlr.v4.runtime.tree.TerminalNode;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.StringReader;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Utils {
	public static double asinh(double x) {
		return Math.log(x + Math.sqrt(x * x + 1.0));
	}

	public static double acosh(double x) {
		return Math.log(x + Math.sqrt(x * x - 1.0));
	}

	public static double atanh(double x) {
		return 0.5 * Math.log((1.0 + x) / (1.0 - x));
	}

	public static long parseCharLiteral(String s) {
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
					case 'b' -> {
						sb.append('\b');
						i++;
					}
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

	public static byte[] parseStringLiteral(String str) {
		int[] s = str.codePoints().toArray();
		if (s.length >= 2 && s[0] == '\"' && s[s.length - 1] == '\"') {
			s = Arrays.copyOfRange(s, 1, s.length - 1);
		}
		// Handle escape sequences
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < s.length; ++i) {
			int ch = s[i];
			if (ch == '\\' && i + 1 < s.length) {
				int next = s[++i];
				switch (next) {
					case 'b' -> {
						sb.appendCodePoint('\b');
					}
					case 'n' -> {
						sb.appendCodePoint('\n');
					}
					case 't' -> {
						sb.appendCodePoint('\t');
					}
					case 'r' -> {
						sb.appendCodePoint('\r');
					}
					case '\\' -> {
						sb.appendCodePoint('\\');
					}
					case '\'' -> {
						sb.appendCodePoint('\'');
					}
					case '\"' -> {
						sb.appendCodePoint('\"');
					}
					case '0' -> {
						sb.appendCodePoint(0);
					}
					case 'u', 'U' -> {
						StringBuffer hex = new StringBuffer();
						boolean done = false;
						while (!done) {
							next = s[++i];
							switch (next) {
								case '{':
									break;
								case 'A', 'B', 'C', 'D', 'E', 'F',
									 'a', 'b', 'c', 'd', 'e', 'f',
									 '0', '1', '2', '3', '4', '5', '6', '7', '8', '9':
									hex.appendCodePoint(next);
									break;
								case '}':
									done = true;
									break;
								default:
									done = true;
							}
						}
						sb.appendCodePoint(Integer.parseInt(hex.toString(), 16));
					}
					default -> sb.appendCodePoint(ch); // Unknown escape, keep as-is
				}
			} else {
				sb.appendCodePoint(ch);
			}
		}
		String unescaped = sb.toString();
		return unescaped.getBytes(StandardCharsets.UTF_8);
	}

	public static String escapeString(String s) {
		s = s.replace("\\", "\\\\");
		s = s.replace("\0", "\\0");
		s = s.replace("\b", "\\b");
		s = s.replace("\t", "\\t");
		s = s.replace("\n", "\\n");
		s = s.replace("\r", "\\r");
		s = s.replace("\f", "\\f");
		s = s.replace("\"", "\\\"");
		s = s.replace("\'", "\\'");
		return s;
	}

	public static String rebuildWithSingleSpaces(CommonTokenStream tokens, ParserRuleContext ctx) {
		Token start = ctx.getStart();
		Token stop  = ctx.getStop();
		if (start == null || stop == null) return "";

		int a = start.getTokenIndex();
		int b = stop.getTokenIndex();

		StringBuilder out = new StringBuilder();
		boolean pendingSpace = false;

		for (int i = a; i <= b; i++) {
			Token t = tokens.get(i);

			// Any hidden token (WS, comments) ⇒ one space
			if (t.getChannel() == Token.HIDDEN_CHANNEL) {
				pendingSpace = true;
				continue;
			}

			if (pendingSpace && out.length() > 0 &&
					t.getText() != ",") {
				out.append(' ');
			}
			pendingSpace = false;

			out.append(t.getText());
		}

		return out.toString().trim();
	}

	public static String rebuildWithSingleSpaces(CommonTokenStream tokens, ParseTree node) {
		if (node == null) return "";

		if (node instanceof ParserRuleContext prc) {
			return rebuildWithSingleSpaces(tokens, prc);
		}

		if (node instanceof TerminalNode tn) {
			return tn.getText().trim();
		}

		return node.getText();
	}

	/** Rebuild a directive line with spaces, rather than ctx.getText(). */
	@Deprecated
	public static String reflowTokens(ParserRuleContext ctx) {
		Token start = ctx.getStart();
		Token stop  = ctx.getStop();
		if (start == null || stop == null) return "";
		// CharStream slice from the original input
		System.out.println("orig: " + ctx.getText());
		String result = start.getInputStream().getText(Interval.of(start.getStartIndex(), stop.getStopIndex()));
		System.out.println("reflow: " + result);
		return result;
	}

	// Matches:
	// .LINE «temp.asm», 12
	// .LINE_BEGIN «temp.asm», 16
	private static final Pattern LINE_PATTERN = Pattern.compile(
			"^\\.(LINE|LINE_BEGIN)\\s+(.*)$"
	);

	public static HashMap<Integer, String> readLineDirectives(String text) {
		HashMap<Integer, String> map = new HashMap<>();

		try (BufferedReader br = new BufferedReader(new StringReader(text))) {
			String line;
			int fileLineNumber = 0;

			while ((line = br.readLine()) != null) {
				fileLineNumber++;

				Matcher m = LINE_PATTERN.matcher(line);
				if (m.matches()) {
					String payload = m.group(2).trim();
					map.put(fileLineNumber, payload);
				}
			}
		} catch (IOException impossible) {
			// StringReader does not throw IOException
			throw new AssertionError(impossible);
		}
		return map;
	}

	public static String extractSourceLine(Token t) {
		CharStream input = t.getInputStream();
		if (input == null) return "";

		int start = t.getStartIndex();
		int end   = t.getStopIndex();

		int a = start;
		int b = end;

		// Walk backward to start of line
		while (a > 0) {
			char c = input.getText(new Interval(a - 1, a - 1)).charAt(0);
			if (c == '\n' || c == '\r') break;
			a--;
		}

		// Walk forward to end of line
		int size = input.size();
		while (b + 1 < size) {
			char c = input.getText(new Interval(b + 1, b + 1)).charAt(0);
			if (c == '\n' || c == '\r') break;
			b++;
		}

		return input.getText(new Interval(a, b));
	}
}
