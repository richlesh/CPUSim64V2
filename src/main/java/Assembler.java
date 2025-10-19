import cloud.lesh.CPUSim64v2.*;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.List;
import java.util.stream.Collectors;

// adjust package imports for your classes:
// import your.pkg.PreprocessVisitor;
// import your.pkg.IncludeLoader;
// import your.pkg.CPUSim64v2Lexer;
// import your.pkg.CPUSim64v2Parser;
// import your.pkg.SemanticsCheck;
// import your.pkg.AssemblerVisitorImpl;
// import your.pkg.AsmIO;

public class Assembler {
	public static void main(String[] args) throws Exception {
		System.out.println("CPUSim64v2 Assembler");
		System.out.println("By Richard Lesh ©2025");
		System.out.println("Assembles .asm source files into .obj.gz binary files");
		if (args.length < 1) {
			System.err.println("Usage: assemble [--debug] [-Dsymbol[=value]] <input.asm>");
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
		String outName = filename + ".obj.gz";
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

		// 3) Lex/parse the preprocessed source
/*
		CharStream input = CharStreams.fromString(preprocessed);
		var lex = new cloud.lesh.CPUSim64v2.CPUSim64v2Lexer(input);
		CommonTokenStream toks = new CommonTokenStream(lex);
		var parser = new cloud.lesh.CPUSim64v2.CPUSim64v2Parser(toks);

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
		// 5) Gather labels
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(preprocessed);
		List<String> errors = labelVisitor.getErrors();

		if (errors.size() > 0) {
			System.out.println(errors.stream().collect(Collectors.joining(System.lineSeparator())));
			System.exit(2);
		}
		// 6) Assemble
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		errors = asm.getErrors();

		if (errors.size() > 0) {
			System.out.println(errors.stream().collect(Collectors.joining(System.lineSeparator())));
			System.exit(2);
		}

		cloud.lesh.CPUSim64v2.AsmIO.writeU64BE(outPath, words);
		System.out.println("Wrote " + words.size() + " words to " + outPath.toString());
		System.exit(0);
	}
}
