import cloud.lesh.CPUSim64v2.IncludeLoader;
import cloud.lesh.CPUSim64v2.LiteralRewriter;
import cloud.lesh.CPUSim64v2.PreprocessorVisitor;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;

public class Preprocessor {
	public static void main(String[] args) throws Exception {
		System.out.println("CPUSim64v2 Preprocessor");
		System.out.println("By Richard Lesh ©2025");
		System.out.println("Preprocesses .asm source files into .pp.asm files");
		if (args.length < 1) {
			System.err.println("Usage: preprocessor <input.asm>");
			System.exit(2);
		}

		Path inPath = Path.of("");
		HashMap<String, PreprocessorVisitor.DefVal> definitions = new HashMap<>();
		for (String arg : args) {
			if (arg.charAt(0) == '-') {
				if (arg.equals("--debug")) {
					definitions.put("__DEBUG", new PreprocessorVisitor.DefVal(PreprocessorVisitor.DefVal.Kind.STRING, "1"));
				} else if (arg.startsWith("-D")) {
					var def = arg.substring(2).split("=", 2);
					if (def.length == 2) {
						definitions.put(def[0], new PreprocessorVisitor.DefVal(PreprocessorVisitor.DefVal.Kind.STRING, def[1]));
					} else {
						definitions.put(def[0], new PreprocessorVisitor.DefVal(PreprocessorVisitor.DefVal.Kind.STRING, "1"));
					}
				} else {
					System.err.println("Unknown option: " + arg);
					System.exit(1);
				}
			} else  {
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
		String preprocessed = PreprocessorVisitor.preprocessText(inPath.getFileName().toString(), source, loader, definitions);

		// 2b) Rewrite literals
		LiteralRewriter rw = new LiteralRewriter();
		preprocessed = rw.rewrite(preprocessed);

		// 3) Write preprocessed output	}
		Files.writeString(outPath, preprocessed);
	}
}
