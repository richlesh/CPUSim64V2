// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
	/**
	 * Computes the inverse hyperbolic sine of {@code x}.
	 *
	 * @param x the value
	 * @return asinh(x) = ln(x + sqrt(x² + 1))
	 */
	public static double asinh(double x) {
		return Math.log(x + Math.sqrt(x * x + 1.0));
	}

	/**
	 * Computes the inverse hyperbolic cosine of {@code x}.
	 *
	 * @param x the value (must be &ge; 1)
	 * @return acosh(x) = ln(x + sqrt(x² - 1))
	 */
	public static double acosh(double x) {
		return Math.log(x + Math.sqrt(x * x - 1.0));
	}

	/**
	 * Computes the inverse hyperbolic tangent of {@code x}.
	 *
	 * @param x the value (must be in the range (-1, 1))
	 * @return atanh(x) = 0.5 * ln((1 + x) / (1 - x))
	 */
	public static double atanh(double x) {
		return 0.5 * Math.log((1.0 + x) / (1.0 - x));
	}

	/**
	 * Parses a string representing an integer with an optional SI suffix
	 * (K, M, G, T, P, E) and returns the corresponding {@code int} value.
	 * <p>
	 * Examples: {@code "1K"} → 1000, {@code "2.5M"} → 2500000, {@code "42"} → 42.
	 *
	 * @param s the string to parse; must not be {@code null} or empty
	 * @return the parsed integer value
	 * @throws IllegalArgumentException if {@code s} is null or empty
	 * @throws NumberFormatException if the value exceeds {@link Integer#MAX_VALUE}
	 *         or is negative, or if the numeric part cannot be parsed
	 */
	public static int decodeSI(String s) {
		if (s == null)
			throw new IllegalArgumentException("null input");

		s = s.trim();
		if (s.isEmpty())
			throw new IllegalArgumentException("empty input");

		// Separate numeric part and suffix
		int len = s.length();
		char last = s.charAt(len - 1);

		double multiplier = 1.0;
		String numberPart = s;

		switch (Character.toUpperCase(last)) {
			case 'K': multiplier = 1e3;  numberPart = s.substring(0, len - 1); break;
			case 'M': multiplier = 1e6;  numberPart = s.substring(0, len - 1); break;
			case 'G': multiplier = 1e9;  numberPart = s.substring(0, len - 1); break;
			case 'T': multiplier = 1e12; numberPart = s.substring(0, len - 1); break;
			case 'P': multiplier = 1e15; numberPart = s.substring(0, len - 1); break;
			case 'E': multiplier = 1e18; numberPart = s.substring(0, len - 1); break;
			default:
				// No suffix → plain number
				break;
		}

		double value;
		value = Double.parseDouble(numberPart.trim());

		double result = Math.round(value * multiplier);

		if (result > Integer.MAX_VALUE || result < 0)
			throw new NumberFormatException("Value out of range for integer: " + s);
		return (int)result;
	}

	/**
	 * Parses a character literal string (with or without surrounding single quotes)
	 * and returns its Unicode code point as a {@code long}.
	 * <p>
	 * Supports standard escape sequences ({@code \b}, {@code \n}, {@code \t},
	 * {@code \r}, {@code \\}, {@code \'}, {@code \"}, {@code \0}) and Unicode
	 * escapes of the form {@code \\u{XXXXX}} or {@code \\U{XXXXX}}.
	 *
	 * @param s the character literal string to parse
	 * @return the Unicode code point of the character
	 * @throws IllegalStateException if the literal does not resolve to exactly one character
	 */
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

	/**
	 * Parses a string literal (with or without surrounding double quotes),
	 * processing escape sequences, and returns the result encoded as UTF-8 bytes.
	 * <p>
	 * Supports standard escape sequences ({@code \b}, {@code \n}, {@code \t},
	 * {@code \r}, {@code \\}, {@code \'}, {@code \"}, {@code \0}) and Unicode
	 * escapes of the form {@code \\u{XXXXX}} or {@code \\U{XXXXX}}.
	 *
	 * @param str the string literal to parse
	 * @return the UTF-8 encoded bytes of the unescaped string content
	 */
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

	/**
	 * Escapes special characters in {@code s} so it can be safely embedded in a
	 * string or character literal.
	 * <p>
	 * Replaces {@code \}, null, {@code \b}, {@code \t}, {@code \n}, {@code \r},
	 * {@code \f}, {@code "}, and {@code '} with their corresponding escape sequences.
	 *
	 * @param s the string to escape
	 * @return a new string with special characters replaced by escape sequences
	 */
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

	/**
	 * Rebuilds the source text of a parser rule context from the token stream,
	 * collapsing all whitespace and hidden-channel tokens into single spaces.
	 * Commas are not preceded by a space.
	 *
	 * @param tokens the full token stream from parsing
	 * @param ctx    the parser rule context whose tokens should be rebuilt
	 * @return the reconstructed text with normalized spacing
	 */
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

	/**
	 * Rebuilds the source text of a parse tree node from the token stream,
	 * collapsing whitespace into single spaces.
	 * <p>
	 * Delegates to {@link #rebuildWithSingleSpaces(CommonTokenStream, ParserRuleContext)}
	 * for rule contexts and returns trimmed text for terminal nodes.
	 *
	 * @param tokens the full token stream from parsing
	 * @param node   the parse tree node to rebuild; returns {@code ""} if {@code null}
	 * @return the reconstructed text with normalized spacing
	 */
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

	/**
	 * Scans preprocessed assembly text for {@code .LINE} and {@code .LINE_BEGIN}
	 * directives and returns a map from each directive's line number (1-based) to
	 * its payload (the filename and source line info following the directive keyword).
	 *
	 * @param text the preprocessed assembly source text to scan
	 * @return a map from file line number to the directive payload string
	 */
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

	/**
	 * Extracts the full source line from the input stream that contains the given token.
	 * <p>
	 * Walks backward and forward from the token's position to find the surrounding
	 * newline characters, then returns the entire line as a string.
	 *
	 * @param t the token whose source line should be extracted
	 * @return the full line of source text containing {@code t}, or {@code ""} if
	 *         the token has no associated input stream
	 */
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
