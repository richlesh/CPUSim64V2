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

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;

/**
 * Relocation information for one relocatable module: the contents of a
 * {@code .rel} file (JSON), which accompanies the module's {@code .ro64} image.
 *
 * <p>A module image, starting at address 0, is the module's code section, its
 * literal section (the string and float literals the assembler collects, which
 * are read-only like code) and its data section ({@code #global} data). Every
 * address-holding instruction field is described by a {@link Reloc}; the linker
 * places the code sections of all modules first, then their literal sections,
 * then their data sections, and patches each recorded field.
 */
public final class RelocInfo {
	public static final int FORMAT = 1;

	/** Sections of a module image. */
	public enum Section {
		CODE, LIT, DATA;
		public String json() { return name().toLowerCase(Locale.ROOT); }
		public static Section of(String s) {
			if ("code".equals(s)) return CODE;
			if ("lit".equals(s)) return LIT;
			if ("data".equals(s)) return DATA;
			throw new MiniJson.JsonException("bad section '" + s + "'");
		}
	}

	/** Width of a relocatable immediate field. */
	public enum Field {
		IMM56(56), IMM42(42), IMM28(28);
		public final int bits;
		Field(int bits) { this.bits = bits; }
		public String json() { return name().toLowerCase(Locale.ROOT); }
		public static Field of(String s) {
			for (Field f : values()) if (f.json().equals(s)) return f;
			throw new MiniJson.JsonException("bad field '" + s + "'");
		}
		/** Field for an instruction word of type {@code tt}, or null for type 0 (no wide field). */
		public static Field forType(int tt) {
			return switch (tt) { case 1 -> IMM56; case 2 -> IMM42; case 3 -> IMM28; default -> null; };
		}
		public long mask() { return (1L << bits) - 1; }
		/** Signed value stored in this field of {@code word}. */
		public long get(long word) { return (word << (64 - bits)) >> (64 - bits); }
		/** {@code word} with this field replaced by {@code value} (must fit). */
		public long put(long word, long value) { return (word & ~mask()) | (value & mask()); }
		public boolean fits(long value) {
			long min = -(1L << (bits - 1)), max = (1L << (bits - 1)) - 1;
			return value >= min && value <= max;
		}
	}

	/** A module that must be linked in because this module {@code #include}d its {@code .asm} source. */
	public record Require(boolean system, String path) {}

	/** A label of the module. {@code offset} is relative to the start of {@code section}. */
	public record Symbol(String name, Section section, long offset, boolean exported, String type) {}

	/**
	 * One relocatable field. {@code at} is the image offset of the instruction word.
	 * The target is either the exported or linker symbol {@code symbol}, or (when
	 * {@code symbol} is null) the private location {@code section}+{@code offset}.
	 * The linker stores {@code target + addend} in {@code field}.
	 */
	public record Reloc(long at, Field field, String symbol, Section section, long offset, long addend) {
		public boolean isSymbolic() { return symbol != null; }
	}

	public String module = "";
	public String source = "";
	public List<String> defines = new ArrayList<>();
	public long codeSize;
	public long litSize;
	public long dataSize;
	public List<Require> requires = new ArrayList<>();
	public List<String> inputs = new ArrayList<>();
	/** Hash of the assembler version, the defines and the text of every input; see {@link Linker#fingerprint}. */
	public String fingerprint = "";
	public List<Symbol> symbols = new ArrayList<>();
	public List<String> imports = new ArrayList<>();
	/** The module's start-up functions (its file-local __INIT__ labels), in calling order. */
	public List<String> inits = new ArrayList<>();
	public List<String> linkerSymbols = new ArrayList<>();
	public List<Reloc> relocs = new ArrayList<>();
	/** Image offset -> label name (the module's {@code .sym1}). */
	public SortedMap<Long, String> reverse = new TreeMap<>();
	/** Image offset -> «file»:line (the module's {@code .srcmap}). */
	public SortedMap<Long, String> srcmap = new TreeMap<>();

	public long imageSize() { return codeSize + litSize + dataSize; }

	/** Image offset where {@code s} starts. */
	public long sectionStart(Section s) {
		return switch (s) { case CODE -> 0; case LIT -> codeSize; case DATA -> codeSize + litSize; };
	}

	public long sectionSize(Section s) {
		return switch (s) { case CODE -> codeSize; case LIT -> litSize; case DATA -> dataSize; };
	}

	/** Section containing image offset {@code at}. */
	public Section sectionOf(long at) {
		return at < codeSize ? Section.CODE : at < codeSize + litSize ? Section.LIT : Section.DATA;
	}

	/** Offset of image offset {@code at} within its section. */
	public long sectionOffset(long at) { return at - sectionStart(sectionOf(at)); }

	public List<Symbol> exports() {
		return symbols.stream().filter(Symbol::exported).toList();
	}

	// ------------------------------------------------------------------
	// JSON
	// ------------------------------------------------------------------

	public Map<String, Object> toJson() {
		Map<String, Object> m = new LinkedHashMap<>();
		m.put("format", (long) FORMAT);
		m.put("module", module);
		m.put("source", source);
		m.put("defines", new ArrayList<Object>(defines));
		m.put("codeSize", codeSize);
		m.put("litSize", litSize);
		m.put("dataSize", dataSize);
		List<Object> req = new ArrayList<>();
		for (Require r : requires) {
			Map<String, Object> o = new LinkedHashMap<>();
			o.put("kind", r.system() ? "system" : "local");
			o.put("path", r.path());
			req.add(o);
		}
		m.put("requires", req);
		m.put("inputs", new ArrayList<Object>(inputs));
		m.put("fingerprint", fingerprint);
		m.put("imports", new ArrayList<Object>(imports));
		m.put("inits", new ArrayList<Object>(inits));
		m.put("linkerSymbols", new ArrayList<Object>(linkerSymbols));
		List<Object> syms = new ArrayList<>();
		for (Symbol s : symbols) {
			Map<String, Object> o = new LinkedHashMap<>();
			o.put("name", s.name());
			o.put("section", s.section().json());
			o.put("offset", s.offset());
			if (s.exported()) o.put("exported", true);
			if (s.type() != null) o.put("type", s.type());
			syms.add(o);
		}
		m.put("symbols", syms);
		List<Object> rel = new ArrayList<>();
		for (Reloc r : relocs) {
			Map<String, Object> o = new LinkedHashMap<>();
			o.put("at", r.at());
			o.put("field", r.field().json());
			if (r.isSymbolic()) {
				o.put("target", "sym");
				o.put("name", r.symbol());
			} else {
				o.put("target", "local");
				o.put("section", r.section().json());
				o.put("offset", r.offset());
			}
			o.put("addend", r.addend());
			rel.add(o);
		}
		m.put("relocs", rel);
		m.put("reverse", longKeyMap(reverse));
		m.put("srcmap", longKeyMap(srcmap));
		return m;
	}

	private static Map<String, Object> longKeyMap(SortedMap<Long, String> src) {
		Map<String, Object> o = new LinkedHashMap<>();
		for (var e : src.entrySet()) o.put(Long.toString(e.getKey()), e.getValue());
		return o;
	}

	private static SortedMap<Long, String> readLongKeyMap(Object v, String what) {
		SortedMap<Long, String> out = new TreeMap<>();
		if (v == null) return out;
		for (var e : MiniJson.obj(v, what).entrySet()) {
			try {
				out.put(Long.parseLong(e.getKey()), String.valueOf(e.getValue()));
			} catch (NumberFormatException ex) {
				throw new MiniJson.JsonException(what + ": bad offset '" + e.getKey() + "'");
			}
		}
		return out;
	}

	private static List<String> strings(Object v, String what) {
		List<String> out = new ArrayList<>();
		for (Object o : MiniJson.arr(v, what)) out.add(String.valueOf(o));
		return out;
	}

	public static RelocInfo fromJson(Object json) {
		Map<String, Object> m = MiniJson.obj(json, ".rel");
		long fmt = MiniJson.lng(m, "format", -1);
		if (fmt != FORMAT)
			throw new MiniJson.JsonException("unsupported .rel format " + fmt + " (expected " + FORMAT + ")");
		RelocInfo r = new RelocInfo();
		r.module = Objects.requireNonNullElse(MiniJson.str(m, "module"), "");
		r.source = Objects.requireNonNullElse(MiniJson.str(m, "source"), "");
		r.defines = strings(m.get("defines"), "defines");
		r.codeSize = MiniJson.lng(m, "codeSize", 0);
		r.litSize = MiniJson.lng(m, "litSize", 0);
		r.dataSize = MiniJson.lng(m, "dataSize", 0);
		for (Object o : MiniJson.arr(m.get("requires"), "requires")) {
			var q = MiniJson.obj(o, "requires[]");
			r.requires.add(new Require("system".equals(MiniJson.str(q, "kind")), MiniJson.str(q, "path")));
		}
		r.inputs = strings(m.get("inputs"), "inputs");
		r.fingerprint = Objects.requireNonNullElse(MiniJson.str(m, "fingerprint"), "");
		r.imports = strings(m.get("imports"), "imports");
		r.inits = strings(m.get("inits"), "inits");
		r.linkerSymbols = strings(m.get("linkerSymbols"), "linkerSymbols");
		for (Object o : MiniJson.arr(m.get("symbols"), "symbols")) {
			var q = MiniJson.obj(o, "symbols[]");
			r.symbols.add(new Symbol(MiniJson.str(q, "name"), Section.of(MiniJson.str(q, "section")),
					MiniJson.lng(q, "offset", 0), MiniJson.bool(q, "exported", false), MiniJson.str(q, "type")));
		}
		for (Object o : MiniJson.arr(m.get("relocs"), "relocs")) {
			var q = MiniJson.obj(o, "relocs[]");
			Field f = Field.of(MiniJson.str(q, "field"));
			long at = MiniJson.lng(q, "at", 0);
			long addend = MiniJson.lng(q, "addend", 0);
			String target = MiniJson.str(q, "target");
			if ("sym".equals(target)) {
				r.relocs.add(new Reloc(at, f, MiniJson.str(q, "name"), null, 0, addend));
			} else if ("local".equals(target)) {
				r.relocs.add(new Reloc(at, f, null, Section.of(MiniJson.str(q, "section")),
						MiniJson.lng(q, "offset", 0), addend));
			} else {
				throw new MiniJson.JsonException("relocs[]: bad target '" + target + "'");
			}
		}
		r.reverse = readLongKeyMap(m.get("reverse"), "reverse");
		r.srcmap = readLongKeyMap(m.get("srcmap"), "srcmap");
		return r;
	}

	public void write(Path file) throws IOException {
		Files.writeString(file, MiniJson.write(toJson()), StandardCharsets.UTF_8);
	}

	public static RelocInfo read(Path file) throws IOException {
		try {
			return fromJson(MiniJson.parse(Files.readString(file, StandardCharsets.UTF_8)));
		} catch (MiniJson.JsonException ex) {
			throw new IOException(file + ": " + ex.getMessage(), ex);
		}
	}
}
