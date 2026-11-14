// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2026 Richard Lesh
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

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Minimal JSON reader and writer used for the linker's {@code .rel} files.
 *
 * <p>Values map to Java as follows: object = {@code LinkedHashMap<String,Object>}
 * (key order preserved), array = {@code ArrayList<Object>}, string = {@code String},
 * integer = {@code Long}, other numbers = {@code Double}, {@code true}/{@code false}
 * = {@code Boolean}, {@code null} = {@code null}.
 */
public final class MiniJson {
	private MiniJson() {}

	/** Thrown for malformed JSON text. */
	public static final class JsonException extends RuntimeException {
		public JsonException(String msg) { super(msg); }
	}

	// ------------------------------------------------------------------
	// Writing
	// ------------------------------------------------------------------

	/** Serializes {@code value} as indented JSON (two spaces per level). */
	public static String write(Object value) {
		StringBuilder sb = new StringBuilder();
		write(sb, value, 0);
		sb.append('\n');
		return sb.toString();
	}

	private static void write(StringBuilder sb, Object v, int indent) {
		if (v == null) {
			sb.append("null");
		} else if (v instanceof String s) {
			writeString(sb, s);
		} else if (v instanceof Boolean || v instanceof Long || v instanceof Integer
				|| v instanceof Short || v instanceof Byte) {
			sb.append(v);
		} else if (v instanceof Number n) {
			double d = n.doubleValue();
			if (Double.isNaN(d) || Double.isInfinite(d))
				throw new JsonException("JSON cannot represent " + d);
			sb.append(Double.toString(d));
		} else if (v instanceof Map<?, ?> m) {
			if (m.isEmpty()) { sb.append("{}"); return; }
			sb.append("{\n");
			int i = 0;
			for (var e : m.entrySet()) {
				indent(sb, indent + 1);
				writeString(sb, String.valueOf(e.getKey()));
				sb.append(": ");
				write(sb, e.getValue(), indent + 1);
				if (++i < m.size()) sb.append(',');
				sb.append('\n');
			}
			indent(sb, indent);
			sb.append('}');
		} else if (v instanceof List<?> l) {
			if (l.isEmpty()) { sb.append("[]"); return; }
			// Arrays of scalars stay on one line; arrays of objects/arrays are one per line.
			boolean simple = l.stream().noneMatch(x -> x instanceof Map || x instanceof List);
			if (simple) {
				sb.append('[');
				for (int i = 0; i < l.size(); ++i) {
					if (i > 0) sb.append(", ");
					write(sb, l.get(i), indent + 1);
				}
				sb.append(']');
				return;
			}
			sb.append("[\n");
			for (int i = 0; i < l.size(); ++i) {
				indent(sb, indent + 1);
				write(sb, l.get(i), indent + 1);
				if (i + 1 < l.size()) sb.append(',');
				sb.append('\n');
			}
			indent(sb, indent);
			sb.append(']');
		} else {
			throw new JsonException("Unsupported JSON value type: " + v.getClass().getName());
		}
	}

	private static void indent(StringBuilder sb, int n) {
		sb.append("  ".repeat(n));
	}

	private static void writeString(StringBuilder sb, String s) {
		sb.append('"');
		for (int i = 0; i < s.length(); ++i) {
			char c = s.charAt(i);
			switch (c) {
				case '"' -> sb.append("\\\"");
				case '\\' -> sb.append("\\\\");
				case '\n' -> sb.append("\\n");
				case '\r' -> sb.append("\\r");
				case '\t' -> sb.append("\\t");
				case '\b' -> sb.append("\\b");
				case '\f' -> sb.append("\\f");
				default -> {
					if (c < 0x20) sb.append(String.format("\\u%04x", (int) c));
					else sb.append(c);
				}
			}
		}
		sb.append('"');
	}

	// ------------------------------------------------------------------
	// Reading
	// ------------------------------------------------------------------

	/** Parses a complete JSON document. */
	public static Object parse(String text) {
		Parser p = new Parser(text);
		p.skipWs();
		Object v = p.value();
		p.skipWs();
		if (p.pos != text.length()) throw p.error("Unexpected trailing text");
		return v;
	}

	private static final class Parser {
		private final String s;
		private int pos;

		Parser(String s) { this.s = s; }

		JsonException error(String msg) {
			int line = 1, col = 1;
			for (int i = 0; i < pos && i < s.length(); ++i) {
				if (s.charAt(i) == '\n') { ++line; col = 1; } else ++col;
			}
			return new JsonException(msg + " at line " + line + ", column " + col);
		}

		void skipWs() {
			while (pos < s.length() && Character.isWhitespace(s.charAt(pos))) ++pos;
		}

		Object value() {
			if (pos >= s.length()) throw error("Unexpected end of JSON");
			char c = s.charAt(pos);
			return switch (c) {
				case '{' -> object();
				case '[' -> array();
				case '"' -> string();
				case 't' -> literal("true", Boolean.TRUE);
				case 'f' -> literal("false", Boolean.FALSE);
				case 'n' -> literal("null", null);
				default -> {
					if (c == '-' || (c >= '0' && c <= '9')) yield number();
					throw error("Unexpected character '" + c + "'");
				}
			};
		}

		Object literal(String word, Object v) {
			if (!s.startsWith(word, pos)) throw error("Expected " + word);
			pos += word.length();
			return v;
		}

		Map<String, Object> object() {
			Map<String, Object> m = new LinkedHashMap<>();
			++pos; // {
			skipWs();
			if (peek() == '}') { ++pos; return m; }
			while (true) {
				skipWs();
				if (peek() != '"') throw error("Expected string key");
				String key = string();
				skipWs();
				if (peek() != ':') throw error("Expected ':'");
				++pos;
				skipWs();
				m.put(key, value());
				skipWs();
				char c = peek();
				if (c == ',') { ++pos; continue; }
				if (c == '}') { ++pos; return m; }
				throw error("Expected ',' or '}'");
			}
		}

		List<Object> array() {
			List<Object> l = new ArrayList<>();
			++pos; // [
			skipWs();
			if (peek() == ']') { ++pos; return l; }
			while (true) {
				skipWs();
				l.add(value());
				skipWs();
				char c = peek();
				if (c == ',') { ++pos; continue; }
				if (c == ']') { ++pos; return l; }
				throw error("Expected ',' or ']'");
			}
		}

		char peek() {
			if (pos >= s.length()) throw error("Unexpected end of JSON");
			return s.charAt(pos);
		}

		String string() {
			++pos; // opening quote
			StringBuilder sb = new StringBuilder();
			while (true) {
				if (pos >= s.length()) throw error("Unterminated string");
				char c = s.charAt(pos++);
				if (c == '"') return sb.toString();
				if (c != '\\') { sb.append(c); continue; }
				if (pos >= s.length()) throw error("Unterminated escape");
				char e = s.charAt(pos++);
				switch (e) {
					case '"' -> sb.append('"');
					case '\\' -> sb.append('\\');
					case '/' -> sb.append('/');
					case 'b' -> sb.append('\b');
					case 'f' -> sb.append('\f');
					case 'n' -> sb.append('\n');
					case 'r' -> sb.append('\r');
					case 't' -> sb.append('\t');
					case 'u' -> {
						if (pos + 4 > s.length()) throw error("Bad \\u escape");
						try {
							sb.append((char) Integer.parseInt(s.substring(pos, pos + 4), 16));
						} catch (NumberFormatException ex) {
							throw error("Bad \\u escape");
						}
						pos += 4;
					}
					default -> throw error("Bad escape \\" + e);
				}
			}
		}

		Object number() {
			int start = pos;
			if (peek() == '-') ++pos;
			while (pos < s.length() && Character.isDigit(s.charAt(pos))) ++pos;
			boolean isInt = true;
			if (pos < s.length() && s.charAt(pos) == '.') {
				isInt = false;
				++pos;
				while (pos < s.length() && Character.isDigit(s.charAt(pos))) ++pos;
			}
			if (pos < s.length() && (s.charAt(pos) == 'e' || s.charAt(pos) == 'E')) {
				isInt = false;
				++pos;
				if (pos < s.length() && (s.charAt(pos) == '+' || s.charAt(pos) == '-')) ++pos;
				while (pos < s.length() && Character.isDigit(s.charAt(pos))) ++pos;
			}
			String t = s.substring(start, pos);
			if (t.equals("-") || t.isEmpty()) throw error("Bad number");
			try {
				return isInt ? (Object) Long.parseLong(t) : (Object) Double.parseDouble(t);
			} catch (NumberFormatException ex) {
				throw error("Bad number " + t);
			}
		}
	}

	// ------------------------------------------------------------------
	// Typed accessors for parsed objects
	// ------------------------------------------------------------------

	@SuppressWarnings("unchecked")
	public static Map<String, Object> obj(Object v, String what) {
		if (v instanceof Map<?, ?> m) return (Map<String, Object>) m;
		throw new JsonException(what + ": expected a JSON object");
	}

	@SuppressWarnings("unchecked")
	public static List<Object> arr(Object v, String what) {
		if (v == null) return new ArrayList<>();
		if (v instanceof List<?> l) return (List<Object>) l;
		throw new JsonException(what + ": expected a JSON array");
	}

	public static String str(Map<String, Object> m, String key) {
		Object v = m.get(key);
		if (v == null) return null;
		if (v instanceof String s) return s;
		throw new JsonException("'" + key + "': expected a string");
	}

	public static long lng(Map<String, Object> m, String key, long dflt) {
		Object v = m.get(key);
		if (v == null) return dflt;
		if (v instanceof Long l) return l;
		throw new JsonException("'" + key + "': expected an integer");
	}

	public static boolean bool(Map<String, Object> m, String key, boolean dflt) {
		Object v = m.get(key);
		if (v == null) return dflt;
		if (v instanceof Boolean b) return b;
		throw new JsonException("'" + key + "': expected true or false");
	}
}
