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

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.*;

/**
 * Links relocatable modules ({@code .ro64} + {@code .rel}, written by
 * {@code assemble -r}) into one ordinary, runnable {@code .o64} program with
 * its {@code .sym}, {@code .sym1}, {@code .sym2} and {@code .srcmap} files.
 *
 * <p>Layout of the linked program (the same as a program assembled in one piece):
 * <pre>
 *   0                  entry address (__MAIN__ if a module defines it, else 1)
 *   1  __CODE__:       READONLY __CODE_END__
 *   2                  JUMP __MAIN__            (only if __MAIN__ is defined)
 *      ...             code of every module, in link order
 *      ...             literals (strings, floats) of every module, in link order
 *      __CODE_END__, __DATA__:
 *      ...             data (#global) of every module, in link order
 *      __DATA_END__, __HEAP_START__:
 * </pre>
 * Link order: the modules a module requires (its {@code #include}d {@code .asm}
 * files) come before it, as they would if they were included at the top of the
 * file; the main module (the one defining {@code __MAIN__}, else the first
 * input) comes last so that the start-up code of every other module runs first.
 *
 * <p>Required modules are found next to their source in {@code obj/}, or for the
 * system and ADT libraries in a cache directory; they are (re)assembled when
 * missing or out of date (see {@link #fingerprint}).
 */
public final class Linker {

	/** Symbols provided by the linker; modules may reference but not define them. */
	public static final List<String> LINKER_SYMBOLS =
			List.of("__START__", "__CODE__", "__CODE_END__", "__DATA__", "__DATA_END__", "__HEAP_START__");

	public static final String MAIN = "__MAIN__";

	/** A link error (undefined or duplicate symbol, overflow, missing module...). */
	public static final class LinkException extends Exception {
		public LinkException(String msg) { super(msg); }
		public LinkException(String msg, Throwable cause) { super(msg, cause); }
	}

	/** A relocatable module: its words and relocation information. */
	public record Module(Path file, RelocInfo rel, List<Long> words) {
		/** Identity of the module: the source it was assembled from. */
		public String key() { return rel.source; }
		public String name() { return rel.module; }
	}

	private final List<String> defines;
	private final PrintStream log;
	private Path cacheDir;

	/**
	 * @param defines preprocessor options ({@code --DEBUG}, {@code -Dname[=value]})
	 *                used when a required module has to be assembled
	 * @param log     where progress messages go (assembling a required module)
	 */
	public Linker(List<String> defines, PrintStream log) {
		this.defines = List.copyOf(defines);
		this.log = log;
	}

	/** Directory for assembled system/ADT library modules (default: see {@link #defaultCacheDir()}). */
	public Linker cacheDir(Path dir) { this.cacheDir = dir; return this; }

	public static Path defaultCacheDir() {
		String p = System.getProperty("cpusim64.cache");
		if (p == null || p.isBlank()) p = System.getenv("CPUSIM64_CACHE");
		if (p != null && !p.isBlank()) return Path.of(p);
		return Path.of(System.getProperty("user.home"), ".cpusim64", "modules");
	}

	// ------------------------------------------------------------------
	// Loading modules
	// ------------------------------------------------------------------

	/** The {@code .rel} file belonging to a {@code .ro64} file. */
	public static Path relFile(Path ro64) {
		return sibling(ro64, ".rel");
	}

	/** {@code base} with {@code ext} appended to its file name. */
	public static Path withExt(Path base, String ext) {
		return base.resolveSibling(base.getFileName().toString() + ext);
	}

	/** {@code file} with its extension replaced by {@code ext}. */
	static Path sibling(Path file, String ext) {
		String n = file.getFileName().toString();
		int dot = n.lastIndexOf('.');
		if (dot > 0) n = n.substring(0, dot);
		return file.resolveSibling(n + ext);
	}

	public static Module load(Path ro64) throws LinkException {
		Path rel = relFile(ro64);
		if (!Files.isRegularFile(ro64)) throw new LinkException("Module not found: " + ro64);
		if (!Files.isRegularFile(rel)) throw new LinkException("Relocation file not found: " + rel
				+ " (assemble the module with -r)");
		try {
			RelocInfo info = RelocInfo.read(rel);
			List<Long> words = AsmIO.readU64BE(ro64.toFile());
			if (words.size() != info.imageSize())
				throw new LinkException(ro64 + ": size " + words.size() + " does not match " + rel
						+ " (" + info.imageSize() + "); reassemble the module");
			return new Module(ro64, info, words);
		} catch (IOException ex) {
			throw new LinkException("Cannot read module " + ro64 + ": " + ex.getMessage(), ex);
		}
	}

	// ------------------------------------------------------------------
	// Fingerprints (is a module up to date?)
	// ------------------------------------------------------------------

	/**
	 * Hash over the assembler version, the preprocessor defines and the name and
	 * text of every input of a module ({@code resource:} inputs are read from the
	 * classpath). Returns null if an input cannot be read.
	 */
	public static String fingerprint(List<String> inputs, List<String> defines) {
		try {
			MessageDigest md = MessageDigest.getInstance("SHA-256");
			md.update(("cpusim64 " + BuildInfo.VERSION + " rel " + RelocInfo.FORMAT + "\n").getBytes(StandardCharsets.UTF_8));
			for (String d : defines) md.update(("define " + d + "\n").getBytes(StandardCharsets.UTF_8));
			for (String in : inputs) {
				String text = readInput(in);
				if (text == null) return null;
				md.update(("input " + in + "\n").getBytes(StandardCharsets.UTF_8));
				md.update(text.getBytes(StandardCharsets.UTF_8));
				md.update((byte) 0);
			}
			return HexFormat.of().formatHex(md.digest());
		} catch (NoSuchAlgorithmException ex) {
			throw new IllegalStateException(ex);
		}
	}

	/** Text of a module input: {@code resource:path} from the classpath, else a file. */
	static String readInput(String id) {
		try {
			if (id.startsWith("resource:")) {
				String res = id.substring("resource:".length());
				try (InputStream in = Linker.class.getClassLoader().getResourceAsStream(res)) {
					return in == null ? null : new String(in.readAllBytes(), StandardCharsets.UTF_8);
				}
			}
			Path p = Path.of(id);
			return Files.isRegularFile(p) ? Files.readString(p) : null;
		} catch (IOException ex) {
			return null;
		}
	}

	/** Preprocessor options that change a module's code. */
	public static List<String> definesOf(Collection<String> options) {
		List<String> out = new ArrayList<>();
		for (String o : options) if (o.equals("--DEBUG") || o.startsWith("-D")) out.add(o);
		return out;
	}

	// ------------------------------------------------------------------
	// Required modules
	// ------------------------------------------------------------------

	private Path cacheRoot() {
		Path base = cacheDir != null ? cacheDir : defaultCacheDir();
		String key = defines.isEmpty() ? "default"
				: fingerprint(List.of(), defines).substring(0, 16);
		return base.resolve(BuildInfo.VERSION).resolve(key);
	}

	/** Where the module for a required {@code .asm} file is kept. */
	Path moduleFileFor(RelocInfo.Require req) {
		if (req.system()) {
			String p = req.path().endsWith(".asm") ? req.path().substring(0, req.path().length() - 4) : req.path();
			return cacheRoot().resolve(p + ".ro64");
		}
		Path src = Path.of(req.path());
		return src.resolveSibling("obj").resolve(sibling(src, ".ro64").getFileName());
	}

	/** Loads the module for {@code req}, assembling it first if it is missing or out of date. */
	Module requireModule(RelocInfo.Require req, String requiredBy) throws LinkException {
		Path ro64 = moduleFileFor(req);
		String sourceId = req.system() ? "resource:" + req.path() : req.path();
		if (Files.isRegularFile(ro64) && Files.isRegularFile(relFile(ro64))) {
			try {
				Module m = load(ro64);
				String fp = fingerprint(m.rel().inputs, defines);
				if (fp != null && fp.equals(m.rel().fingerprint) && sourceId.equals(m.rel().source)) return m;
			} catch (LinkException ignored) {
				// unreadable or inconsistent: reassemble
			}
		}
		String text = readInput(sourceId);
		if (text == null)
			throw new LinkException("Cannot find " + (req.system() ? "<" + req.path() + ">" : req.path())
					+ " required by " + requiredBy);
		if (log != null) log.println("Assembling module " + (req.system() ? "<" + req.path() + ">" : req.path()));
		String fileName = Path.of(req.path()).getFileName().toString();
		Path baseDir = req.system() ? Path.of(".") : Path.of(req.path()).getParent();
		Assembler.ModuleResult r = Assembler.assembleModule(text, fileName, sourceId,
				new IncludeLoader(baseDir), defines);
		if (r == null) throw new LinkException("Assembly of " + fileName + " (required by " + requiredBy + ") failed");
		try {
			Files.createDirectories(ro64.getParent());
			Assembler.writeModule(r, sibling(ro64, ""));
		} catch (IOException ex) {
			throw new LinkException("Cannot write module " + ro64 + ": " + ex.getMessage(), ex);
		}
		return new Module(ro64, r.rel(), r.words());
	}

	// ------------------------------------------------------------------
	// Linking
	// ------------------------------------------------------------------

	/** The linked program. */
	public record Result(List<Long> words, Map<String, Long> labels, SortedMap<Long, String> reverse,
						 Map<String, String> types, SortedMap<Long, String> srcmap, List<Module> order) {}

	/** Links {@code roots} (and every module they require) and writes {@code outBase}.o64 etc. */
	public Result linkAndWrite(List<Module> roots, Path outBase) throws LinkException {
		Result r = link(roots);
		try {
			write(r, outBase);
		} catch (IOException ex) {
			throw new LinkException("Cannot write " + outBase + ".o64: " + ex.getMessage(), ex);
		}
		return r;
	}

	public Result link(List<Module> roots) throws LinkException {
		if (roots.isEmpty()) throw new LinkException("Nothing to link");

		// ---- main module: the one defining __MAIN__, else the first input
		Module main = null;
		for (Module m : roots) {
			if (definesMain(m)) {
				if (main != null && definesMain(main))
					throw new LinkException("Duplicate " + MAIN + ": defined in modules " + main.name()
							+ " (" + main.file() + ") and " + m.name() + " (" + m.file() + ")");
				main = m;
			}
		}
		if (main == null) main = roots.get(0);

		// ---- link order: required modules first (depth-first, in #include order), main last
		Map<String, Module> byKey = new LinkedHashMap<>();
		for (Module m : roots) {
			Module prev = byKey.putIfAbsent(m.key(), m);
			if (prev != null && prev != m)
				throw new LinkException("Module " + m.name() + " is given twice (" + prev.file() + ", " + m.file() + ")");
		}
		List<Module> order = new ArrayList<>();
		Set<String> visited = new HashSet<>();
		for (Module m : roots) if (m != main) visit(m, byKey, visited, order);
		visit(main, byKey, visited, order);

		// ---- layout
		boolean hasMain = order.stream().anyMatch(Linker::definesMain);
		List<Long> words = new ArrayList<>();
		// start-up code (as for a program assembled in one piece): READONLY __CODE_END__,
		// call of every module's __INIT__ functions in link order, call __MAIN__, exit
		List<String> inits = new ArrayList<>();
		Map<String, Module> initModule = new HashMap<>();
		for (Module m : order) {
			for (String i : m.rel().inits) {
				String key = m.key() + "\u0000" + i;
				inits.add(key);
				initModule.put(key, m);
			}
		}
		List<String> startup = new ArrayList<>(PreprocessorVisitor.startupLines(
				inits.stream().map(k -> "__LINK_INIT_" + inits.indexOf(k)).toList(), hasMain));
		// Without __MAIN__ the program starts with the main module's code. Included
		// (required) modules jump over their own code, as they must when they are
		// assembled in one piece, but other modules given on the command line need not,
		// so if one of them comes first, jump to the main module's code.
		final Module mainModule = main;
		Set<String> rootKeys = new HashSet<>();
		for (Module m : roots) rootKeys.add(m.key());
		boolean jumpToMain = !hasMain && order.stream().takeWhile(m -> m != mainModule)
				.anyMatch(m -> rootKeys.contains(m.key()));
		if (jumpToMain) startup.add("jump __LINK_MAIN_MODULE");
		words.add(0L);								// entry: 1
		words.add(0L);								// READONLY __CODE_END__
		for (int i = 0; i < startup.size(); ++i) words.add(0L);
		Map<RelocInfo.Section, Map<Module, Long>> base = new EnumMap<>(RelocInfo.Section.class);
		for (RelocInfo.Section sec : RelocInfo.Section.values()) {
			Map<Module, Long> b = new HashMap<>();
			for (Module m : order) {
				b.put(m, (long) words.size());
				long start = m.rel().sectionStart(sec);
				words.addAll(m.words().subList((int) start, (int) (start + m.rel().sectionSize(sec))));
			}
			base.put(sec, b);
		}
		long codeEnd = words.size() - order.stream().mapToLong(m -> m.rel().dataSize).sum();
		long dataEnd = words.size();
		Layout layout = new Layout(base);
		if (dataEnd >= (1L << 40))
			throw new LinkException("Linked program is too large (" + dataEnd + " words)");

		Map<String, Long> linkerSyms = new LinkedHashMap<>();
		linkerSyms.put("__START__", 0L);
		linkerSyms.put("__CODE__", 1L);
		linkerSyms.put("__CODE_END__", codeEnd);
		linkerSyms.put("__DATA__", codeEnd);
		linkerSyms.put("__DATA_END__", dataEnd);
		linkerSyms.put("__HEAP_START__", dataEnd);

		// ---- global symbol table (exported symbols)
		Map<String, Long> globals = new HashMap<>();
		Map<String, Module> definedBy = new HashMap<>();
		for (Module m : order) {
			for (RelocInfo.Symbol s : m.rel().exports()) {
				if (linkerSyms.containsKey(s.name()))
					throw new LinkException("Module " + m.name() + " defines " + s.name() + ", which is provided by the linker");
				Module prev = definedBy.putIfAbsent(s.name(), m);
				if (prev != null) {
					if (MAIN.equals(s.name()))
						throw new LinkException("Duplicate " + MAIN + ": defined in modules " + prev.name()
								+ " (" + prev.file() + ") and " + m.name() + " (" + m.file() + ")");
					throw new LinkException("Duplicate symbol " + s.name() + ": exported by modules "
							+ prev.name() + " (" + prev.file() + ") and " + m.name() + " (" + m.file() + ")");
				}
				globals.put(s.name(), layout.address(s, m));
			}
		}

		// ---- relocation
		for (Module m : order) {
			RelocInfo rel = m.rel();
			for (RelocInfo.Reloc r : rel.relocs) {
				long target;
				if (r.isSymbolic()) {
					Long v = linkerSyms.get(r.symbol());
					if (v == null) v = globals.get(r.symbol());
					if (v == null) throw new LinkException(undefinedMessage(r.symbol(), m, order));
					target = v;
				} else {
					target = layout.base(r.section(), m) + r.offset();
				}
				long value = target + r.addend();
				if (!r.field().fits(value))
					throw new LinkException("Relocation overflow in module " + m.name() + " at offset " + r.at()
							+ ": " + value + " does not fit in " + r.field().bits + " bits");
				long at = layout.imageAddress(r.at(), m);
				words.set((int) at, r.field().put(words.get((int) at), value));
			}
		}

		// ---- start-up words
		Map<String, Long> stubLabels = new HashMap<>();
		stubLabels.put("__CODE_END__", codeEnd);
		if (hasMain) stubLabels.put(MAIN, globals.get(MAIN));
		for (int i = 0; i < inits.size(); ++i) {
			Module m = initModule.get(inits.get(i));
			String name = inits.get(i).substring(inits.get(i).indexOf('\u0000') + 1);
			RelocInfo.Symbol s = m.rel().symbols.stream().filter(x -> x.name().equals(name)).findFirst()
					.orElseThrow(() -> new LinkException("Module " + m.name() + ": start-up function " + name + " not found"));
			stubLabels.put("__LINK_INIT_" + i, layout.address(s, m));
		}
		if (jumpToMain) stubLabels.put("__LINK_MAIN_MODULE", layout.base(RelocInfo.Section.CODE, main));
		List<Long> stub = assembleStartup(startup, stubLabels);
		words.set(0, 1L);
		for (int i = 0; i < stub.size(); ++i) words.set(1 + i, stub.get(i));

		// ---- merged symbol information
		Map<String, Integer> nameCount = new HashMap<>();
		for (Module m : order)
			for (RelocInfo.Symbol s : m.rel().symbols) nameCount.merge(s.name(), 1, Integer::sum);
		Map<Module, String> qualifier = uniqueQualifiers(order);

		Map<String, Long> labels = new LinkedHashMap<>();
		Map<String, String> types = new LinkedHashMap<>();
		SortedMap<Long, String> reverse = new TreeMap<>();
		SortedMap<Long, String> srcmap = new TreeMap<>();
		labels.putAll(linkerSyms);
		Map<Module, Map<String, String>> renames = new HashMap<>();
		for (Module m : order) {
			Map<String, String> rename = new HashMap<>();
			for (RelocInfo.Symbol s : m.rel().symbols) {
				String name = s.name();
				boolean clash = !s.exported() && (nameCount.get(name) > 1 || globals.containsKey(name)
						|| linkerSyms.containsKey(name));
				if (clash) name = qualifier.get(m) + "$" + name;
				rename.put(s.name(), name);
				labels.put(name, layout.address(s, m));
				if (s.type() != null) types.put(name, s.type());
			}
			renames.put(m, rename);
			for (var e : m.rel().srcmap.entrySet())
				srcmap.put(layout.imageAddress(e.getKey(), m), e.getValue());
		}
		// reverse map in address order of definition, later labels at the same address win
		// (as in a program assembled in one piece)
		reverse.put(1L, "__CODE__");
		for (Module m : order) addReverse(reverse, m, RelocInfo.Section.CODE, renames.get(m), layout);
		for (Module m : order) addReverse(reverse, m, RelocInfo.Section.LIT, renames.get(m), layout);
		reverse.put(codeEnd, "__CODE_END__");
		reverse.put(codeEnd, "__DATA__");
		for (Module m : order) addReverse(reverse, m, RelocInfo.Section.DATA, renames.get(m), layout);
		reverse.put(dataEnd, "__DATA_END__");
		reverse.put(dataEnd, "__HEAP_START__");
		return new Result(words, labels, reverse, types, srcmap, order);
	}

	private void visit(Module m, Map<String, Module> byKey, Set<String> visited, List<Module> order) throws LinkException {
		if (!visited.add(m.key())) return;
		for (RelocInfo.Require req : m.rel().requires) {
			String key = req.system() ? "resource:" + req.path() : req.path();
			if (visited.contains(key)) continue;		// already linked or being linked (include cycle)
			Module dep = byKey.get(key);
			if (dep == null) {
				dep = requireModule(req, m.name());
				byKey.put(key, dep);
			}
			visit(dep, byKey, visited, order);
		}
		order.add(m);
	}

	private static void addReverse(SortedMap<Long, String> reverse, Module m, RelocInfo.Section sec,
								   Map<String, String> rename, Layout layout) {
		for (var e : m.rel().reverse.entrySet()) {
			if (m.rel().sectionOf(e.getKey()) == sec)
				reverse.put(layout.imageAddress(e.getKey(), m), rename.getOrDefault(e.getValue(), e.getValue()));
		}
	}

	/** Where each section of each module was placed. */
	private record Layout(Map<RelocInfo.Section, Map<Module, Long>> bases) {
		long base(RelocInfo.Section s, Module m) { return bases.get(s).get(m); }
		long address(RelocInfo.Symbol s, Module m) { return base(s.section(), m) + s.offset(); }
		/** Address in the linked program of image offset {@code at} of module {@code m}. */
		long imageAddress(long at, Module m) {
			RelocInfo.Section s = m.rel().sectionOf(at);
			return base(s, m) + m.rel().sectionOffset(at);
		}
	}

	private static boolean definesMain(Module m) {
		return m.rel().symbols.stream().anyMatch(s -> MAIN.equals(s.name()));
	}

	private static String undefinedMessage(String sym, Module m, List<Module> order) {
		StringBuilder sb = new StringBuilder("Undefined symbol " + sym + " (referenced in module " + m.name()
				+ ", " + m.file() + ")");
		for (Module o : order) {
			for (RelocInfo.Symbol s : o.rel().symbols) {
				if (s.name().equals(sym) && !s.exported()) {
					sb.append("; module ").append(o.name()).append(" defines it but does not #export it");
					return sb.toString();
				}
			}
		}
		return sb.toString();
	}

	/** Unique upper-case module names used to qualify clashing private labels (MODULE$LABEL). */
	private static Map<Module, String> uniqueQualifiers(List<Module> order) {
		Map<Module, String> q = new HashMap<>();
		Map<String, Integer> used = new HashMap<>();
		for (Module m : order) {
			String base = m.name().toUpperCase(Locale.ROOT).replaceAll("[^\\p{L}\\p{Nd}_]", "_");
			int n = used.merge(base, 1, Integer::sum);
			q.put(m, n == 1 ? base : base + n);
		}
		return q;
	}

	/** Assembles READONLY __CODE_END__ followed by {@code lines} with the given label values. */
	private static List<Long> assembleStartup(List<String> lines, Map<String, Long> labels) throws LinkException {
		StringBuilder src = new StringBuilder("READONLY __CODE_END__\n");
		for (String l : lines) src.append(l).append('\n');
		AssemblerVisitor asm = new AssemblerVisitor(new HashMap<>(labels), new HashMap<>());
		asm.assemble(src.toString());
		if (asm.hasErrors() || asm.result().size() != lines.size() + 1)
			throw new LinkException("internal error: cannot assemble the start-up code");
		return asm.result();
	}

	/** A Type-1 instruction word (opcode + 56-bit immediate), as the assembler encodes JUMP C / READONLY C. */
	static long encodeType1(int opcode, long imm56) {
		return (1L << 62) | (((long) opcode & 0x3FL) << 56) | (imm56 & ((1L << 56) - 1));
	}

	// ------------------------------------------------------------------
	// Output
	// ------------------------------------------------------------------

	/** Writes {@code base}.o64, .sym, .sym1, .sym2 and .srcmap. */
	public static void write(Result r, Path base) throws IOException {
		Path dir = base.toAbsolutePath().getParent();
		if (dir != null) Files.createDirectories(dir);
		AsmIO.writeU64BE(withExt(base, ".o64"), r.words());
		writeSymbolFiles(base, r.labels(), r.reverse(), r.types(), r.srcmap());
	}

	/** Writes {@code base}.sym (NAME: address), .sym1 (address: NAME), .sym2 (NAME: TYPE), .srcmap. */
	public static void writeSymbolFiles(Path base, Map<String, Long> labels, Map<Long, String> reverse,
										Map<String, String> types, Map<Long, String> srcmap) throws IOException {
		List<Map.Entry<String, Long>> sorted = new ArrayList<>(labels.entrySet());
		sorted.sort(Map.Entry.<String, Long>comparingByValue().thenComparing(Map.Entry.comparingByKey()));
		try (BufferedWriter w = Files.newBufferedWriter(withExt(base, ".sym"))) {
			for (var e : sorted) { w.write(e.getKey() + ": " + e.getValue()); w.newLine(); }
		}
		try (BufferedWriter w = Files.newBufferedWriter(withExt(base, ".sym1"))) {
			for (var e : new TreeMap<>(reverse).entrySet()) { w.write(e.getKey() + ": " + e.getValue()); w.newLine(); }
		}
		try (BufferedWriter w = Files.newBufferedWriter(withExt(base, ".sym2"))) {
			for (var e : types.entrySet()) { w.write(e.getKey() + ": " + e.getValue()); w.newLine(); }
		}
		try (BufferedWriter w = Files.newBufferedWriter(withExt(base, ".srcmap"))) {
			for (var e : new TreeMap<>(srcmap).entrySet()) { w.write(e.getKey() + ": " + e.getValue()); w.newLine(); }
		}
	}

	// ------------------------------------------------------------------
	// Command line: link [-o name] [--DEBUG] [-Dname[=value]] a.ro64 [b.ro64 ...]
	// ------------------------------------------------------------------

	/** Output base path for {@code -o name}: an optional .o64 extension is removed. */
	public static Path outputBase(String out) {
		if (out.endsWith(".o64")) out = out.substring(0, out.length() - 4);
		return Path.of(out).toAbsolutePath();
	}

	public static void main(String[] args) {
		System.exit(run(args));
	}

	public static int run(String[] args) {
		System.out.println("=".repeat(80));
		System.out.println("CPUSim64 Linker " + BuildInfo.VERSION);
		System.out.println(BuildInfo.COPYRIGHT + " Richard Lesh");
		System.out.println("Links relocatable .ro64 modules into a .o64 program.");
		System.out.println("=".repeat(80));
		String out = null;
		List<String> options = new ArrayList<>();
		List<String> inputs = new ArrayList<>();
		for (int i = 0; i < args.length; ++i) {
			String a = args[i];
			if (a.equals("-o")) {
				if (i + 1 >= args.length) { System.err.println("Error: -o requires a file name"); return 2; }
				out = args[++i];
			} else if (a.startsWith("-")) {
				options.add(a);
			} else {
				inputs.add(a);
			}
		}
		if (inputs.isEmpty()) {
			System.err.println("Usage: link [-o name] [--DEBUG] [-Dsymbol[=value]] <module.ro64> [more.ro64 ...]");
			return 2;
		}
		if (inputs.size() > 1 && out == null) {
			System.err.println("Error: -o <name> is required when linking more than one module");
			return 2;
		}
		try {
			List<Module> roots = new ArrayList<>();
			for (String in : inputs) {
				Path p = Path.of(in.endsWith(".ro64") ? in : in + ".ro64").toAbsolutePath();
				roots.add(load(p));
			}
			List<String> defs = definesOf(options);
			if (defs.isEmpty()) defs = roots.get(0).rel().defines;
			Path base = out != null ? outputBase(out) : sibling(roots.get(0).file(), "");
			Result r = new Linker(defs, System.out).linkAndWrite(roots, base);
			System.out.println("Wrote " + r.words().size() + " words to " + withExt(base, ".o64"));
			return 0;
		} catch (LinkException ex) {
			System.err.println("LINKERROR: " + ex.getMessage());
			return 1;
		}
	}
}
