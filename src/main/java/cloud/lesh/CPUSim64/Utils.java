package cloud.lesh.CPUSim64;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
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
}
