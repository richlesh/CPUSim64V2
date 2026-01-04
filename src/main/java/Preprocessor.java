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

import cloud.lesh.CPUSim64.IncludeLoader;
import cloud.lesh.CPUSim64.LiteralRewriter;
import cloud.lesh.CPUSim64.PreprocessorVisitor;
import org.antlr.v4.runtime.CommonTokenStream;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.stream.Collectors;

public class Preprocessor {
	public static void main(String[] args) throws Exception {
		System.out.println("=".repeat(80));
		System.out.println("CPUSim64 2.0 Preprocessor");
		System.out.println("By Richard Lesh ©2025");
		System.out.println("Preprocesses .asm source files into .pp.asm files.");
		System.out.println("=".repeat(80));
		if (args.length < 1) {
			System.err.println("Usage: preprocessor [--DEBUG] [--hasMain] [-Dsymbol[=value]] <input.asm>");
			System.exit(2);
		}

		boolean hasMain = false;

		Path inPath = Path.of("");
		for (int i = 0; i < args.length; ++i) {
			String arg = args[i];
			if (arg.charAt(0) == '-') {
				if (arg.equals("--hasMain")) {
					hasMain = true;
				}
			} else {
				inPath = Path.of(arg).toAbsolutePath();
			}
		}

		if (!Files.isRegularFile(inPath)) {
			System.err.println("Can't find file: " + inPath.toString());
			System.exit(3);
		}
		String filename = inPath.getFileName().toString();
		// Strip ".asm" if present
		if (filename.endsWith(".asm")) {
			filename = filename.substring(0, filename.length() - 4);
		}
		String outName = filename + ".pp.asm";
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
				System.exit(2);
			}
			Files.writeString(outPath, preprocessed);

			// 3) Rewrite literals
			LiteralRewriter rw = new LiteralRewriter();
			preprocessed = rw.rewrite(preprocessed, pp.getSourceLocations());

			// 4) Add global declarations
			preprocessed = PreprocessorVisitor.addGlobals(preprocessed, hasMain);

			// 5) Write preprocessed output
			Files.writeString(outPath, preprocessed);
		} catch (Exception ex) {
			System.err.println(ex.getMessage());
		}
	}
}
