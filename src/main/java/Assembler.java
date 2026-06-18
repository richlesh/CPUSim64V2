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

import cloud.lesh.CPUSim64.*;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

// adjust package imports for your classes:
// import your.pkg.PreprocessVisitor;
// import your.pkg.IncludeLoader;
// import your.pkg.CPUSim64Lexer;
// import your.pkg.CPUSim64Parser;
// import your.pkg.SemanticsCheck;
// import your.pkg.AssemblerVisitorImpl;
// import your.pkg.AsmIO;

public class Assembler {
	public static void main(String[] args) throws Exception {
		System.exit(run(args));
	}

	public static int run(String[] args) throws Exception {
		System.out.println("=".repeat(80));
		System.out.println("CPUSim64 " + BuildInfo.VERSION + " Assembler");
		System.out.println(BuildInfo.COPYRIGHT + " Richard Lesh");
		System.out.println("Assembles .asm source files into .o64 binary files.");
		System.out.println("=".repeat(80));
		if (args.length < 1) {
			System.err.println("Usage: assemble [--DEBUG] [--hasMain] [-Dsymbol[=value]] <input.asm>");
			return 2;
		}

		boolean hasMain = false;

		Path inPath = Path.of("");
		String options = "";
		for (int i = 0; i < args.length; ++i) {
			String arg = args[i];
			if (arg.charAt(0) == '-') {
				if (arg.equals("--hasMain")) {
					hasMain = true;
				}
				options += " " + arg;
			} else {
				inPath = Path.of(arg).toAbsolutePath();
			}
		}
		if (!options.isEmpty())
			System.out.println("Options:" + options);

		if (!Files.isRegularFile(inPath)) {
			System.err.println("Can't find file: " + inPath.toString());
			return 3;
		}
		String filename = inPath.getFileName().toString();
		// Strip ".asm" if present
		if (filename.endsWith(".asm")) {
			filename = filename.substring(0, filename.length() - 4);
		}
		String outName = filename + ".o64";
		// Put it in the same directory as the input file
		Path outPath = inPath.getParent().resolve(outName);

		try {
			// 1) Read source text
			String source = Files.readString(inPath);

			// 2) Preprocess
			var loader = new IncludeLoader(inPath.getParent());
			PreprocessorVisitor.resetGlobals();
			PreprocessorVisitor pp = new PreprocessorVisitor(inPath.getFileName().toString(), loader);
			String preprocessed = pp.preprocessText(source, args);
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
			if (labelMap.containsKey("__MAIN__")) {
				words.set(0, labelMap.get("__MAIN__"));    // Set start of program
			} else {
				words.set(0, 1L);                    // Set start of program
			}

			cloud.lesh.CPUSim64.AsmIO.writeU64BE(outPath, words);
			System.out.println("Wrote " + words.size() + " words to " + outPath.toString());

			// Write label map for debugging
			Path symbolFile = inPath.getParent().resolve(filename + ".sym");
			try (BufferedWriter writer = new BufferedWriter(new FileWriter(symbolFile.toFile()))) {
				for (Map.Entry<String, Long> entry : labelMap.entrySet()) {
					writer.write(entry.getKey() + ": " + entry.getValue());
					writer.newLine();
				}
			} catch (IOException e) {
				System.err.println("Error writing label map: " + e.getMessage());
			}

			Path symbolFile1 = inPath.getParent().resolve(filename + ".sym1");
			try (BufferedWriter writer = new BufferedWriter(new FileWriter(symbolFile1.toFile()))) {
				for (var entry : reverseLabelMap.entrySet()) {
					writer.write(entry.getKey() + ": " + entry.getValue());
					writer.newLine();
				}
			} catch (IOException e) {
				System.err.println("Error writing reverse label map: " + e.getMessage());
			}

			var types = asm.getLabelTypes();
			Path symbolFile2 = inPath.getParent().resolve(filename + ".sym2");
			try (BufferedWriter writer = new BufferedWriter(new FileWriter(symbolFile2.toFile()))) {
				for (var entry : types.entrySet()) {
					writer.write(entry.getKey() + ": " + entry.getValue());
					writer.newLine();
				}
			} catch (IOException e) {
				System.err.println("Error writing label type map: " + e.getMessage());
			}
		} catch (Exception ex) {
			System.err.println(ex.getMessage());
			return 1;
		}
		return 0;
	}
}
