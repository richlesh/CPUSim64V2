import cloud.lesh.CPUSim64.IncludeLoader;
import cloud.lesh.CPUSim64.LiteralRewriter;
import cloud.lesh.CPUSim64.PreprocessorVisitor;

import java.nio.file.Files;
import java.nio.file.Path;

public class Preprocessor {
	public static void main(String[] args) throws Exception {
		System.out.println("=".repeat(80));
		System.out.println("CPUSim64 2.0 Preprocessor");
		System.out.println("By Richard Lesh ©2025");
		System.out.println("Preprocesses .asm source files into .pp.asm files.");
		System.out.println("=".repeat(80));
		if (args.length < 1) {
			System.err.println("Usage: preprocessor <input.asm>");
			System.exit(2);
		}

		boolean hasMain = false;

		Path inPath = Path.of("");
		for (int i = 0; i < args.length; ++i) {
			String arg = args[i];
			if (arg.charAt(0) == '-') {
				if (arg.equals("--hasMain")) {
					hasMain = true;
				} else {
					System.err.println("Unknown option: " + arg);
					System.exit(1);
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

		// 1) Read source text
		String source = Files.readString(inPath);

		// 2) Preprocess
		var loader = new IncludeLoader(inPath.getParent());
		PreprocessorVisitor.resetGlobals();
		String preprocessed = PreprocessorVisitor.preprocessText(inPath.getFileName().toString(), source, loader, args);

		// 3) Rewrite literals
		LiteralRewriter rw = new LiteralRewriter();
		preprocessed = rw.rewrite(preprocessed);

		// 4) Add global declarations
		preprocessed = PreprocessorVisitor.addGlobals(preprocessed, hasMain);

		// 5) Write preprocessed output	}
		Files.writeString(outPath, preprocessed);
	}
}
