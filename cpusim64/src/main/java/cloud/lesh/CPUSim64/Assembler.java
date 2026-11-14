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

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;

// adjust package imports for your classes:
// import your.pkg.PreprocessVisitor;
// import your.pkg.IncludeLoader;
// import your.pkg.CPUSim64Lexer;
// import your.pkg.CPUSim64Parser;
// import your.pkg.SemanticsCheck;
// import your.pkg.AssemblerVisitorImpl;
// import your.pkg.AsmIO;

public class Assembler {
	static final String USAGE =
			"Usage: assemble [--DEBUG] [--hasMain] [-Dsymbol[=value]] [-r] [-o name] <input.asm>\n" +
			"       assemble [--DEBUG] [-Dsymbol[=value]] -o name <a.asm|a.ro64> <b.asm|b.ro64> ...\n" +
			"  -r       write a relocatable module (.ro64 + .rel) for the linker\n" +
			"  -o name  name of the output files (required with more than one input file:\n" +
			"           each .asm input is assembled with -r into obj/ and all are linked)\n" +
			"  -r -o name.o64  assemble a module and link it with the modules it #includes";

	public static void main(String[] args) throws Exception {
		System.exit(run(args));
	}

	public static int run(String[] args) throws Exception {
		System.out.println("=".repeat(80));
		System.out.println("CPUSim64 Assembler " + BuildInfo.VERSION);
		System.out.println(BuildInfo.COPYRIGHT + " Richard Lesh");
		System.out.println("Assembles .asm source files into .o64 binary files.");
		System.out.println("=".repeat(80));
		System.out.println("Arguments: " + Utils.displayArgs(args));
		if (args.length < 1) {
			System.err.println(USAGE);
			return 2;
		}

		boolean hasMain = false;
		boolean relocatable = false;
		String out = null;

		Path inPath = Path.of("");
		List<String> inputs = new ArrayList<>();
		List<String> ppOptions = new ArrayList<>();
		String options = "";
		for (int i = 0; i < args.length; ++i) {
			String arg = args[i];
			if (arg.isEmpty()) continue;
			if (arg.equals("-o")) {
				if (i + 1 >= args.length) {
					System.err.println("Error: -o requires a file name");
					System.err.println(USAGE);
					return 2;
				}
				out = args[++i];
				options += " -o " + out;
			} else if (arg.charAt(0) == '-') {
				if (arg.equals("--hasMain")) {
					hasMain = true;
				} else if (arg.equals("-r")) {
					relocatable = true;
				} else {
					ppOptions.add(arg);
				}
				options += " " + arg;
			} else {
				inputs.add(arg);
				inPath = Path.of(arg).toAbsolutePath();
			}
		}

		if (inputs.isEmpty()) {
			System.err.println(USAGE);
			return 2;
		}
		if (inputs.size() > 1 && out == null) {
			System.err.println("Error: -o <name> is required when assembling more than one input file");
			System.err.println(USAGE);
			return 2;
		}
		if (inputs.size() > 1) {
			return assembleAndLink(inputs, out, ppOptions);
		}
		if (!Files.isRegularFile(inPath)) {
			System.err.println("Can't find file: " + inPath.toString());
			return 3;
		}
		String filename = inPath.getFileName().toString();
		// Strip ".asm" if present
		if (filename.endsWith(".asm")) {
			filename = filename.substring(0, filename.length() - 4);
		}
		// Output files: next to the input file, or as given by -o
		Path outBase = out != null ? Linker.outputBase(out) : inPath.getParent().resolve(filename);
		if (relocatable && out != null && out.endsWith(".o64")) {
			// -r with a .o64 output: assemble the module into obj/ and link it with
			// the modules it requires (its #include'd .asm files) into a program
			return assembleAndLink(inputs, out, ppOptions);
		}
		if (relocatable) {
			if (out != null && out.endsWith(".ro64"))
				outBase = Linker.outputBase(out.substring(0, out.length() - 5));
			return assembleModuleFile(inPath, outBase, ppOptions) == null ? 2 : 0;
		}
		Path outPath = Linker.withExt(outBase, ".o64");

		try {
			// 1) Read source text
			String source = Files.readString(inPath);

			// 2) Preprocess
			var loader = new IncludeLoader(inPath.getParent());
			PreprocessorVisitor.resetGlobals();
			PreprocessorVisitor pp = new PreprocessorVisitor(inPath.getFileName().toString(), loader);
			String preprocessed = pp.preprocessText(source, ppOptions.toArray(new String[0]));
			if (preprocessed == null || preprocessed.isEmpty()) {
				System.err.println("Error: too many preprocessor errors!");
				return 2;
			}

			// 3) Rewrite literals
			LiteralRewriter rw = new LiteralRewriter();
			preprocessed = rw.rewrite(preprocessed, pp.getSourceLocations());

			// 3) Lex/parse the preprocessed source
/*
		CharStream input = CharStreams.fromString(preprocessed);
		var lex = new cloud.lesh.CPUSim64.CPUSim64Lexer(input);
		CommonTokenStream toks = new CommonTokenStream(lex);
		var parser = new cloud.lesh.CPUSim64.CPUSim64Parser(toks);

		parser.removeErrorListeners();
		parser.addErrorListener(new DiagnosticErrorListener());

		ParseTree tree = parser.program();

		// 4) Optional semantic pass
		var sema = new SemanticsCheck();
		ParseTreeWalker.DEFAULT.walk(sema, tree);
		if (!sema.getErrors().isEmpty()) {
			sema.getErrors().forEach(System.err::println);
			System.exit(1);
		}
*/

			// 4) Add global declarations
			preprocessed = PreprocessorVisitor.addGlobals(preprocessed, hasMain);

			// 5) Gather labels
			LabelVisitor labelVisitor = new LabelVisitor();
			String noLabels = labelVisitor.gatherLabels(preprocessed);
			if (noLabels == null || noLabels.isEmpty() || labelVisitor.hasErrors()) {
				System.err.println("Error: too many assembler errors!");
				return 2;
			}

			// 6) Assemble
			Map<String, Long> labelMap = labelVisitor.getLabelMap();
			Map<Long, String> reverseLabelMap = labelVisitor.getReverseLabelMap();
			var asm = new AssemblerVisitor(labelMap, reverseLabelMap);
			asm.assemble(noLabels);
			List<Long> words = asm.result();
			if (words == null || words.isEmpty() || asm.hasErrors()) {
				System.err.println("Error: too many assembler errors!");
				return 2;
			}
			// Start of program: the start-up code at address 1 (READONLY, calls of the
			// __INIT__ functions, call __MAIN__)
			words.set(0, 1L);

			cloud.lesh.CPUSim64.AsmIO.writeU64BE(outPath, words);
			System.out.println("Wrote " + words.size() + " words to " + outPath.toString());

			// Write label map for debugging
			Path symbolFile = Linker.withExt(outBase, ".sym");
			try (BufferedWriter writer = new BufferedWriter(new FileWriter(symbolFile.toFile()))) {
				for (Map.Entry<String, Long> entry : labelMap.entrySet()) {
					writer.write(entry.getKey() + ": " + entry.getValue());
					writer.newLine();
				}
			} catch (IOException e) {
				System.err.println("Error writing label map: " + e.getMessage());
			}

			Path symbolFile1 = Linker.withExt(outBase, ".sym1");
			try (BufferedWriter writer = new BufferedWriter(new FileWriter(symbolFile1.toFile()))) {
				for (var entry : reverseLabelMap.entrySet()) {
					writer.write(entry.getKey() + ": " + entry.getValue());
					writer.newLine();
				}
			} catch (IOException e) {
				System.err.println("Error writing reverse label map: " + e.getMessage());
			}

			var types = asm.getLabelTypes();
			Path symbolFile2 = Linker.withExt(outBase, ".sym2");
			try (BufferedWriter writer = new BufferedWriter(new FileWriter(symbolFile2.toFile()))) {
				for (var entry : types.entrySet()) {
					writer.write(entry.getKey() + ": " + entry.getValue());
					writer.newLine();
				}
			} catch (IOException e) {
				System.err.println("Error writing label type map: " + e.getMessage());
			}

			// Write address-to-source-line map for debugger breakpoint sync
			Path srcmapFile = Linker.withExt(outBase, ".srcmap");
			try (BufferedWriter writer = new BufferedWriter(new FileWriter(srcmapFile.toFile()))) {
				for (var entry : labelVisitor.getAddressToSourceLocation().entrySet()) {
					writer.write(entry.getKey() + ": " + entry.getValue());
					writer.newLine();
				}
			} catch (IOException e) {
				System.err.println("Error writing source map: " + e.getMessage());
			}
		} catch (Exception ex) {
			System.err.println(ex.getMessage());
			return 1;
		}
		return 0;
	}

	// ------------------------------------------------------------------
	// Relocatable modules (-r) and linking several inputs (-o)
	// ------------------------------------------------------------------

	/** A relocatable module produced by {@link #assembleModule}. */
	public record ModuleResult(List<Long> words, RelocInfo rel, Map<String, Long> labels,
							   SortedMap<Long, String> reverse, Map<String, String> types,
							   SortedMap<Long, String> srcmap) {}

	/** Marker label before a relocatable module's literal pool. */
	static final String MODULE_LIT_MARKER = "__MODULE$LIT";

	/** A label name that #export * exports (not local, not a generated literal, not a linker symbol). */
	static boolean exportable(String name) {
		if (name.matches("__(STR|FP)_\\d+") || Linker.LINKER_SYMBOLS.contains(name)) return false;
		if (name.startsWith(PreprocessorVisitor.INIT)) return false;	// a file's __INIT__ is local to it
		if (!name.contains("$")) return true;
		if (name.matches(".*\\$(ASM|DEF)")) return false;		// #include marker label (file$asm, file$def)
		// labels of user .block scopes (ANSI_COLOR$RED) are exported; labels of the scopes
		// the preprocessor generates (#def_func, #for, #if_cond, ...) stay private
		String[] parts = name.split("\\$", -1);
		for (int i = 0; i < parts.length; ++i) {
			String p = parts[i];
			if (p.isEmpty() || p.startsWith("_")) return false;
			if (i < parts.length - 1 && p.matches("(FOR|WHILE|DO_WHILE|COND|CONDSR|SYNC)_[0-9A-F]{4,}")) return false;
		}
		return true;
	}

	/** Assembles {@code inPath} as a relocatable module and writes {@code outBase}.ro64, .rel, .sym*, .srcmap. */
	static ModuleResult assembleModuleFile(Path inPath, Path outBase, List<String> ppOptions) {
		if (!Files.isRegularFile(inPath)) {
			System.err.println("Can't find file: " + inPath);
			return null;
		}
		try {
			String source = Files.readString(inPath);
			ModuleResult r = assembleModule(source, inPath.getFileName().toString(),
					inPath.toAbsolutePath().normalize().toString(), new IncludeLoader(inPath.getParent()),
					Linker.definesOf(ppOptions));
			if (r == null) return null;
			Path dir = outBase.toAbsolutePath().getParent();
			if (dir != null) Files.createDirectories(dir);
			writeModule(r, outBase);
			System.out.println("Wrote " + r.words().size() + " words to " + Linker.withExt(outBase, ".ro64"));
			return r;
		} catch (IOException ex) {
			System.err.println(ex.getMessage());
			return null;
		}
	}

	/**
	 * Assembles one relocatable module (assemble -r). Errors are printed to
	 * stderr and null is returned.
	 *
	 * @param source    the module's source text
	 * @param fileName  file name used in diagnostics and .LINE information
	 * @param sourceId  identity of the source: absolute file path, or resource:path
	 * @param loader    resolves the module's #include files
	 * @param defines   preprocessor options (--DEBUG, -Dname[=value])
	 */
	public static ModuleResult assembleModule(String source, String fileName, String sourceId,
											  PreprocessorVisitor.IncludeLoader loader, List<String> defines) {
		try {
			// 1) Preprocess: #include of .asm files records required modules instead of inlining them
			PreprocessorVisitor.resetGlobals();
			PreprocessorVisitor.setRelocatable(true);
			PreprocessorVisitor pp = new PreprocessorVisitor(fileName, loader);
			String preprocessed = pp.preprocessText(source, defines.toArray(new String[0]));
			if (preprocessed == null) {
				System.err.println("Error: too many preprocessor errors!");
				return null;
			}
			PreprocessorVisitor.LinkInfo info = PreprocessorVisitor.takeLinkInfo();

			// 2) Code, literals, #global data (no start-up code: the linker adds it)
			preprocessed = new LiteralRewriter().trailerLabel(MODULE_LIT_MARKER)
					.rewrite(preprocessed, pp.getSourceLocations());
			preprocessed = PreprocessorVisitor.addGlobalsRelocatable(preprocessed);

			// 3) Labels: offsets in the module image
			LabelVisitor labelVisitor = new LabelVisitor();
			labelVisitor.setRelocatable(true);
			String noLabels = labelVisitor.gatherLabels(preprocessed);
			if (noLabels == null || labelVisitor.hasErrors()) {
				System.err.println("Error: too many assembler errors!");
				return null;
			}
			Map<String, Long> labels = new HashMap<>(labelVisitor.getLabelMap());
			Long litStart = labels.remove(MODULE_LIT_MARKER);
			Long dataStart = labels.remove(PreprocessorVisitor.MODULE_DATA_MARKER);
			long imageSize = labelVisitor.getCurrentAddress();
			if (dataStart == null) dataStart = imageSize;
			if (litStart == null) litStart = dataStart;
			long codeSize = litStart;
			// reverse map without the section markers (another label at the same address takes its place)
			SortedMap<Long, String> reverse = new TreeMap<>();
			for (var e : labelVisitor.getReverseLabelMap().entrySet()) {
				String n = e.getValue();
				if (n.equals(MODULE_LIT_MARKER) || n.equals(PreprocessorVisitor.MODULE_DATA_MARKER)) {
					n = labels.entrySet().stream().filter(x -> x.getValue().equals(e.getKey()))
							.map(Map.Entry::getKey).sorted().findFirst().orElse(null);
				}
				if (n != null) reverse.put(e.getKey(), n);
			}
			if (imageSize >= AssemblerVisitor.MAX_MODULE_SIZE) {
				System.err.println(fileName + ":ERROR:module is too large to relocate (" + imageSize + " words)");
				return null;
			}
			for (String ls : Linker.LINKER_SYMBOLS) {
				if (labels.containsKey(ls)) {
					System.err.println(fileName + ":ERROR:label " + ls + " is provided by the linker and cannot be defined in a module");
					return null;
				}
			}
			boolean ok = true;
			for (String e : info.exports()) {
				if (!labels.containsKey(e)) {
					System.err.println(fileName + ":ERROR:#export " + e + ": no such label in this module");
					ok = false;
				}
			}
			if (!ok) return null;

			// 4) Assemble with biased label values and record relocations
			Map<String, Long> biased = new HashMap<>();
			for (var e : labels.entrySet()) biased.put(e.getKey(), AssemblerVisitor.RELOC_BIAS + e.getValue());
			Set<String> externs = new HashSet<>(info.externs());
			externs.addAll(info.moduleLabels());
			var asm = new AssemblerVisitor(biased, labelVisitor.getReverseLabelMap());
			asm.setRelocatable(externs, new HashSet<>(Linker.LINKER_SYMBOLS));
			asm.assemble(noLabels);
			List<Long> words = asm.result();
			if (words == null || asm.hasErrors()) {
				System.err.println("Error: too many assembler errors!");
				return null;
			}
			if (words.size() != imageSize) {
				System.err.println(fileName + ":ERROR:internal error: assembled " + words.size()
						+ " words, expected " + imageSize);
				return null;
			}

			// 5) Relocation information
			Map<String, String> types = new TreeMap<>();
			for (var e : asm.getLabelTypes().entrySet()) {
				if (labels.containsKey(e.getKey())) types.put(e.getKey(), e.getValue().name());
			}
			RelocInfo rel = new RelocInfo();
			String module = fileName.endsWith(".asm") ? fileName.substring(0, fileName.length() - 4) : fileName;
			rel.module = module;
			rel.source = sourceId;
			rel.defines = new ArrayList<>(defines);
			rel.codeSize = codeSize;
			rel.litSize = dataStart - litStart;
			rel.dataSize = imageSize - dataStart;
			rel.requires = info.requires();
			rel.inits = new ArrayList<>(info.inits());
			rel.inputs = new ArrayList<>();
			rel.inputs.add(sourceId);
			for (String in : info.inputs()) if (!rel.inputs.contains(in)) rel.inputs.add(in);
			String fp = Linker.fingerprint(rel.inputs, rel.defines);
			rel.fingerprint = fp == null ? "" : fp;
			List<Map.Entry<String, Long>> sorted = new ArrayList<>(labels.entrySet());
			sorted.sort(Map.Entry.<String, Long>comparingByValue().thenComparing(Map.Entry.comparingByKey()));
			for (var e : sorted) {
				String name = e.getKey();
				long off = e.getValue();
				boolean exported = Linker.MAIN.equals(name) || info.exports().contains(name)
						|| (info.exportAll() && exportable(name));
				rel.symbols.add(new RelocInfo.Symbol(name, rel.sectionOf(off), rel.sectionOffset(off),
						exported, types.get(name)));
			}
			Set<String> imports = new TreeSet<>(), linkerSyms = new TreeSet<>();
			for (RelocInfo.Reloc r : asm.getRelocs()) {
				if (!r.isSymbolic()) continue;
				if (Linker.LINKER_SYMBOLS.contains(r.symbol())) linkerSyms.add(r.symbol());
				else imports.add(r.symbol());
			}
			rel.imports = new ArrayList<>(imports);
			rel.linkerSymbols = new ArrayList<>(linkerSyms);
			for (RelocInfo.Reloc r : asm.getRelocs()) {
				if (r.isSymbolic()) { rel.relocs.add(r); continue; }
				long off = r.offset();		// image offset of the label
				rel.relocs.add(new RelocInfo.Reloc(r.at(), r.field(), null, rel.sectionOf(off), rel.sectionOffset(off), r.addend()));
			}
			rel.reverse = reverse;
			rel.srcmap = new TreeMap<>(labelVisitor.getAddressToSourceLocation());
			return new ModuleResult(words, rel, labels, reverse, types, rel.srcmap);
		} catch (RuntimeException ex) {
			if (ex.getMessage() != null) System.err.println(ex.getMessage());
			return null;
		} finally {
			PreprocessorVisitor.resetGlobals();
		}
	}

	/** Writes a module: {@code base}.ro64, .rel and the module-relative .sym, .sym1, .sym2, .srcmap. */
	public static void writeModule(ModuleResult r, Path base) throws IOException {
		AsmIO.writeU64BE(Linker.withExt(base, ".ro64"), r.words());
		r.rel().write(Linker.withExt(base, ".rel"));
		Linker.writeSymbolFiles(base, r.labels(), r.reverse(), r.types(), r.srcmap());
	}

	/**
	 * assemble -o name a.asm b.asm ...: each .asm input is assembled as a
	 * relocatable module into the obj/ directory next to it (.ro64 inputs are used
	 * as they are), then all are linked into name.o64.
	 */
	static int assembleAndLink(List<String> inputs, String out, List<String> ppOptions) {
		List<Linker.Module> roots = new ArrayList<>();
		try {
			for (String in : inputs) {
				Path p = Path.of(in).toAbsolutePath().normalize();
				if (in.endsWith(".ro64")) {
					roots.add(Linker.load(p));
					continue;
				}
				Path base = p.resolveSibling("obj").resolve(Linker.sibling(p, "").getFileName());
				ModuleResult r = assembleModuleFile(p, base, ppOptions);
				if (r == null) return 2;
				roots.add(new Linker.Module(Linker.withExt(base, ".ro64"), r.rel(), r.words()));
			}
			Path outBase = Linker.outputBase(out);
			Linker.printBanner(System.out);
			List<String> linkArgs = new ArrayList<>(Linker.definesOf(ppOptions));
			linkArgs.add("-o");
			linkArgs.add(out);
			for (Linker.Module m : roots) linkArgs.add(m.file().toString());
			System.out.println("Arguments: " + Utils.displayArgs(linkArgs));
			Linker.Result res = new Linker(Linker.definesOf(ppOptions), System.out).linkAndWrite(roots, outBase);
			System.out.println("Linked " + res.order().size() + " modules: wrote " + res.words().size()
					+ " words to " + Linker.withExt(outBase, ".o64"));
			return 0;
		} catch (Linker.LinkException ex) {
			System.err.println("LINKERROR: " + ex.getMessage());
			return 1;
		}
	}
}
