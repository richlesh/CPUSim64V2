package cloud.lesh.CPUSim64;

import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.tree.*;
import org.antlr.v4.runtime.ParserRuleContext;
import org.antlr.v4.runtime.misc.Interval;
import org.antlr.v4.runtime.Token;

import java.nio.file.Path;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.commons.lang3.tuple.Pair;

/**
 * Walks a preprocesor.g4 parse tree and produces preprocessed assembly text.
 * - Supports: #include, #define, #if/#elseif/#else/#endif
 * - Performs token-wise substitution for defined symbols on code lines (and on
 *   directive passthrough lines if you want—see emit(...) calls).
 *
 * You provide an IncludeLoader that maps include targets to file contents.
 */
public class PreprocessorVisitor extends PreprocessorParserBaseVisitor<Void> {
	public class PreprocessorException extends RuntimeException {
		PreprocessorException(String msg) {
			super(getLocation() + " -> " + msg);
		}
		PreprocessorException(String msg, Object... args) {
			super(String.format(getLocation() + " -> " + msg, args));
		}
		PreprocessorException(Exception ex) {
			super(getLocation() + " -> " + ex.getMessage());
		}
	}

	String filename = null;
	int lineNum = 1;
	int sourceLineNum = 1;
	int pauseLineSync = 0;
	Stack<String> lineDirectives = new Stack<>();
	String funcName = "";			// current function name for return handling

	String getLocation() {
		return (filename == null ? "" : filename + ":") + lineNum;
	}

	/** Provide include contents for a path like <system/io.asm> or "path". */
	public interface IncludeLoader {
		/** Return the full text of the included file, or throw if not found. */
		String load(String includePath, boolean isSystemPath);
	}

	/** Holds a #define value and kind so we can coerce in conditions. */
	public static final class DefVal {
		public enum Kind { INT, FLOAT, CHAR, STRING, SYMBOL }
		final Kind kind;
		final String text; // original text (e.g., "123", "3.14", "'c'", "\"str\"", or "1" for symbol-only)
		public DefVal(Kind kind, String text) { this.kind = kind; this.text = text; }
	}

	private final IncludeLoader includeLoader;
	private final StringBuilder out = new StringBuilder();

	/** Simple symbol table for #define */
	private Map<String, DefVal> defines = new HashMap<>();
	private Stack<Map<String, String>> scopes = new Stack<>();
	/** When true, we also apply #define substitution to directive passthrough lines. */
	private final boolean substituteInsideDirectives;

	/** Word boundary pattern for safe token replacement */
	private static final Pattern TOKEN = Pattern.compile("[A-Za-z_.$][A-Za-z0-9_.$]*");
	private static final Pattern PLACEHOLDER = Pattern.compile("\\$\\{(([A-Za-z_.$][A-Za-z0-9_.$]*)|\\.\\.\\.)\\}");

	public PreprocessorVisitor(String filename, IncludeLoader loader) {
		this(filename, 1, loader, /*substituteInsideDirectives*/ false, 0);
	}

	public PreprocessorVisitor(String filename, int lineNum, IncludeLoader loader, boolean substituteInsideDirectives, int pauseLineSync) {
		this.filename = filename;
		this.lineNum = lineNum;
		this.includeLoader = Objects.requireNonNull(loader, "IncludeLoader is required");
		this.substituteInsideDirectives = substituteInsideDirectives;
		this.pauseLineSync = pauseLineSync;
		pushScope();
	}

	/** Resulting preprocessed source text. */
	public String getOutput() {
		return out.toString();
	}

	public void addDefine(String name, DefVal value) {
		if (name == null || name.isEmpty() || value == null) {
			throw new PreprocessorException("Define name and value must be non-null");
		}
		if (defines.containsKey(name.toUpperCase())) {
			System.err.println(getLocation() + "-> Define name already exists: " + name);
		}
		defines.put(name.toUpperCase(), value);
	}

	public void addVar(String name, String value) {
		if (name == null || name.isEmpty() || value == null) {
			throw new PreprocessorException("Variable name and value must be non-null");
		}
		name = name.toUpperCase();
		if (lookupVar(name) != null) {
			throw new PreprocessorException("Variable name already exists: " + name);
		}
		scopes.lastElement().put(name, value);
	}

	public String lookupVar(String name) {
		name = name.toUpperCase();
		for (int i = scopes.size() - 1; i >= 0; i--) {
			var s = scopes.get(i);
			if (s.containsKey(name)) {
				return s.get(name);
			}
		}
		return null;
	}

	public void pushScope() {
		scopes.push(new HashMap<String, String>());
	}

	public void popScope() {
		scopes.pop();
	}
    /* =========================================================
       Top-level and line emission helpers
       ========================================================= */

	private static int lineOf(ParseTree node) {
		if (node instanceof ParserRuleContext prc) {
			Token t = prc.getStart();
			return (t != null) ? t.getLine() : -1;
		} else if (node instanceof TerminalNode tn) {
			Token t = tn.getSymbol();
			return (t != null) ? t.getLine() : -1;
		}
		return -1; // unknown
	}

	@Override
	public Void visitPreproc(PreprocessorParser.PreprocContext ctx) {
		emitLineDirective(filename, 1);
		for (ParseTree child : ctx.children) {
			child.accept(this);
		}
		return null;
	}

	@Override
	public Void visitDirective(PreprocessorParser.DirectiveContext ctx) {
		sourceLineNum = lineOf(ctx);
//		System.out.println("Visiting directive at source line " + sourceLineNum + ": " + ctx.getText().replace("\n", "\\n"));
		if (pauseLineSync <= 0 && sourceLineNum != lineNum) {
			emitLineDirective(filename, sourceLineNum);
		}
		return visitChildren(ctx);
	}

	@Override
	public Void visitCodeLine(PreprocessorParser.CodeLineContext ctx) {
		sourceLineNum = lineOf(ctx);
//		System.out.println("Visiting code line at source line " + sourceLineNum + ": " + ctx.getText().replace("\n", "\\n"));
		if (pauseLineSync <= 0 && sourceLineNum != lineNum) {
			emitLineDirective(filename, sourceLineNum);
		}
		if (ctx.IDENT() != null) {
			emitLine(ctx.IDENT().getText(), true);
		} else if (ctx.CODE_TEXT() != null && !ctx.CODE_TEXT().getText().isEmpty()) {
			emitLine(ctx.CODE_TEXT().getText(), true); // apply #define substitution on code lines
		}
		return null;
	}

    /* =========================================================
       Directives
       ========================================================= */

	Set<String> previouslyIncluded = new HashSet<String>();
	@Override
	public Void visitIncludeDir(PreprocessorParser.IncludeDirContext ctx) {
		final boolean isSystem = ctx.ANGLE_PATH() != null;
		final String raw;

		if (isSystem) {
			raw = ctx.ANGLE_PATH().getText();            // like "<system/io.asm>"
		} else if (ctx.STRING() != null) {
			raw = stripQuotes(ctx.STRING().getText());   // like "local/file.asm"
		} else {
			// Fallback: try to recover or fail with a helpful message
			throw new PreprocessorException(
					"Invalid #include; expected <path> or \"path\" at line " +
							ctx.getStart().getLine() + ": " + ctx.getText()
			);
		}

		final String path = normalizeIncludeTarget(raw); // strip <> when needed
		if (!previouslyIncluded.contains(path)) {
			try {
				previouslyIncluded.add(path);
				final String included = includeLoader.load(path, isSystem);
				Path filename = Path.of(path).getFileName();
				final String preprocessed = preprocessText(filename.toString(), 1, included, includeLoader, defines, macros, previouslyIncluded, substituteInsideDirectives, 0);
				out.append(filename.toString().replace("/","$").replace(".","$") + ":\n");
				out.append(preprocessed);
			} catch (IllegalArgumentException ex) {
				throw new PreprocessorException(ex);
			}
		}
		return null;
	}

	@Override
	public Void visitDefineDir(PreprocessorParser.DefineDirContext ctx) {
		String name = ctx.IDENT().getText();
		PreprocessorParser.LiteralContext lit = ctx.literal();

		if (lit == null) {
			addDefine(name, new DefVal(DefVal.Kind.SYMBOL, "1")); // bare symbol → truthy, expands to 1
		} else if (lit.INT() != null) {
			addDefine(name, new DefVal(DefVal.Kind.INT, lit.INT().getText()));
		} else if (lit.FLOAT() != null) {
			addDefine(name, new DefVal(DefVal.Kind.FLOAT, lit.FLOAT().getText()));
		} else if (lit.CHAR() != null) {
			addDefine(name, new DefVal(DefVal.Kind.CHAR, lit.CHAR().getText()));
		} else if (lit.STRING() != null) {
			addDefine(name, new DefVal(DefVal.Kind.STRING, lit.STRING().getText()));
		} else {
			// Fallback: store text
			addDefine(name, new DefVal(DefVal.Kind.SYMBOL, lit.getText()));
		}
		// We do not emit the #define line; it only affects substitutions.
		return null;
	}

	@Override
	public Void visitUndefDir(PreprocessorParser.UndefDirContext ctx) {
		String name = ctx.IDENT().getText();
		defines.remove(name);
		return null;
	}

	@Override
	public Void visitCallDir(PreprocessorParser.CallDirContext ctx) {
		if (ctx.argList() != null) {
			emitLineBeginDirective(filename, lineNum);
			for (var param : ctx.argList().callArg().reversed()) {
				emitLine("push " + param.getText(), true);
			}
		}
		emitLine("call " + ctx.IDENT().getText(), true);
		if (ctx.argList() != null) {
			emitLine("add sp, " + ctx.argList().callArg().size(), true);
			emitLineEndDirective(filename, lineNum);
		}
		return null;
	}

	private static List<String> globals = new ArrayList<>();

	@Override
	public Void visitGlobalDir(PreprocessorParser.GlobalDirContext ctx) {
		globals.add(ctx.CODE_TEXT().getText());
		return null;
	}

	public static List<String> getGlobals() { return globals; }

	public static void resetGlobals() { globals.clear(); }

	public static String addGlobals(String preprocessed, boolean hasMain) {
		preprocessed =
				".org 1" + System.lineSeparator() +
				"__CODE__: READONLY __CODE_END__" + System.lineSeparator() +
				(hasMain ? "jump __MAIN__" + System.lineSeparator() : "") +
				preprocessed + System.lineSeparator() +
				"__CODE_END__:" + System.lineSeparator() +
				"__DATA__:" + System.lineSeparator() +
				String.join(System.lineSeparator(), PreprocessorVisitor.getGlobals()) + System.lineSeparator() +
				"__DATA_END__:" + System.lineSeparator() +
				"__HEAP_START__:" + System.lineSeparator();
		resetGlobals();
		return preprocessed;
	}

	private final Set<String> svarSet = new HashSet<>();
	@Override
	public Void visitSvarDir(PreprocessorParser.SvarDirContext ctx) {
		if (ctx.identList().IDENT().size() == 0) {
			return null;
		} else if (ctx.identList().IDENT().size() == 1) {
			emitLine("push 0", false);
			svarSet.add(ctx.identList().IDENT(0).getText());
			addVar(ctx.identList().IDENT(0).getText(), "sf[0]");
			return null;
		} else {
			int i = 0;
			for (var ident : ctx.identList().IDENT()) {
				svarSet.add(ident.getText());
				addVar(ident.getText(), "sf[" + i + "]");
				i--;
			}
			emitLine("add sp, " + i, false);
			return null;
		}
	}

	private final Set<String> varSet = new HashSet<>();

	@Override
	public Void visitVarDir(PreprocessorParser.VarDirContext ctx) {
		if (ctx.identList().IDENT().size() == 0) {
			return null;
		} else if (ctx.identList().IDENT().size() == 1) {
			emitLine("push r28", false);
			varSet.add(ctx.identList().IDENT(0).getText());
			addVar(ctx.identList().IDENT(0).getText(), "R28");
			return null;
		} else {
			int reg = 28;
			for (var ident : ctx.identList().IDENT()) {
				varSet.add(ident.getText());
				addVar(ident.getText(), "R" + reg);
				reg--;
			}
			emitLine("save r" + (reg + 1) + ", r28", false);
			return null;
		}
	}

	private final Set<String> fvarSet = new HashSet<>();

	@Override
	public Void visitFvarDir(PreprocessorParser.FvarDirContext ctx) {
		if (ctx.identList().IDENT().size() == 0) {
			return null;
		} else if (ctx.identList().IDENT().size() == 1) {
			emitLine("push F31", false);
			fvarSet.add(ctx.identList().IDENT(0).getText());
			addVar(ctx.identList().IDENT(0).getText(), "F31");
			return null;
		} else {
			int reg = 31;
			for (var ident : ctx.identList().IDENT()) {
				fvarSet.add(ident.getText());
				addVar(ident.getText(), "F" + reg);
				reg--;
			}
			emitLine("save f" + (reg + 1) + ", f31", false);
			return null;
		}
	}

	@Override
	public Void visitReturnDir(PreprocessorParser.ReturnDirContext ctx) {
		if (ctx.primary() != null) {
			emitLine("MOVE R0, " + ctx.primary().getText(), substituteInsideDirectives);
			emitLine("JUMP " + funcName + "$_RETURN", false);
		}
		return null;
	}

	@Override
	public Void visitFreturnDir(PreprocessorParser.FreturnDirContext ctx) {
		if (ctx.primary() != null) {
			emitLine("MOVE F0, " + ctx.primary().getText(), substituteInsideDirectives);
			emitLine("JUMP " + funcName + "$_RETURN", false);
		}
		return null;
	}

	@Override
	public Void visitDefFuncDir(PreprocessorParser.DefFuncDirContext ctx) {
		// Default behavior:
		//  - strip the #def_func / #end_func markers from output,
		//  - emit the body (directives/code) after preprocessing.
		// If you’d rather keep the markers, replace with reflowTokens(ctx) + emit.

		// Open a new scope for arguments and local vars.
		funcName = "";
		for (ParseTree child : ctx.children) {
			if (child == ctx.PP_DEF_FUNC()) {
				svarSet.clear();
				fvarSet.clear();
				varSet.clear();
				pushScope();
				funcName = ctx.IDENT().getText().toUpperCase();
				emitLine(funcName + ":", false);
				if (ctx.paramList() != null) {
					int i = 3;
					for (var arg : ctx.paramList().IDENT()) {
						addVar(arg.getText(), "SF[" + i + "]");
						++i;
					}
				}
				emitLine(".BLOCK " + funcName, false);
			} else if (child == ctx.PP_END_FUNC()) {
				emitLineBeginDirective(filename, lineNum);
				emitLine(funcName + "$_RETURN:", false);
				if (fvarSet.size() > 1)
					emitLine("restore f" + (31 - fvarSet.size() + 1) + ", f31", false);
				else if (fvarSet.size() == 1)
					emitLine("pop f31", false);
				if (varSet.size() > 1)
					emitLine("restore r" + (28 - varSet.size() + 1) + ", r28", false);
				else if (varSet.size() == 1)
					emitLine("pop r28", false);
				if (svarSet.size() > 0)
					emitLine("add sp, " + (svarSet.size()), false);
				emitLine("return", false);
				emitLine(".BLOCK_END " + ctx.IDENT().getText().toUpperCase(), false);
				emitLineEndDirective(filename, lineNum);
				popScope();
				funcName = "";
			} else {
				child.accept(this);
			}
		}
		return null;
	}

	private String macroDefName;
	private String macroDefText;
	Map<String, Pair<List<String>, String>> macros = new HashMap<>();

	@Override
	public Void visitDefMacroDir(PreprocessorParser.DefMacroDirContext ctx) {
		// Name + params
		macroDefName = ctx.IDENT().getText().toUpperCase();
		List<String> params = new ArrayList<>();
		if (ctx.paramList() != null) {
			for (var id : ctx.paramList().IDENT()) {
				params.add(id.getText().toUpperCase());
			}
		}
		if (ctx.paramList().ELLIPSIS() != null) {
			params.add("..."); // varargs
		}

		// Accumulate directives and codelines
		StringBuilder body = new StringBuilder();
		for (PreprocessorParser.CodeLineOrDirectiveContext lineCtx : ctx.codeLineOrDirective()) {
			body.append(reflowTokens(lineCtx)); // see note below about whitespace
		}

		macros.put(macroDefName, Pair.of(params, body.toString()));
		return null;
	}

	@Override
	public Void visitMacroDir(PreprocessorParser.MacroDirContext ctx) {
		var def = macros.get(ctx.IDENT().getText().toUpperCase());
		if (def != null) {
			Map<String, String> formalParams = new HashMap<String, String>();
			ArrayList<String> varArgs = new ArrayList<String>();
			for (int i = 0; i < def.getLeft().size(); ++i) {
				if (def.getLeft().get(i).equals("...")) {
					// collect varargs
					for (int j = i; j < ctx.argList().callArg().size(); ++j) {
						varArgs.add(ctx.argList().callArg(j).getText());
					}
					formalParams.put("...", String.join(", ", varArgs));
					break;
				}
				formalParams.put(def.getLeft().get(i).toUpperCase(), ctx.argList().callArg(i).getText());
			}
			String replacement = applyPlaceholders(def.getRight(), formalParams);
			emitLineBeginDirective(filename, lineNum);
			++pauseLineSync;
			if (pauseLineSync > 10)
				throw new PreprocessorException("Macro nesting exceeded 10 levels!");
			replacement = preprocessText(filename, lineNum, replacement, includeLoader, defines, macros, previouslyIncluded, true, pauseLineSync);
			--pauseLineSync;
			emitLine(replacement, true);
			emitLineEndDirective(filename, lineNum);
		} else {
			throw new PreprocessorException("Undefined macro: " + ctx.IDENT().getText());
		}
		return null;
	}
		/* ----- #if / #elseif / #else / #endif ----- */

	@Override
	public Void visitIfBlock(PreprocessorParser.IfBlockContext ctx) {
		// #if arm
		if (evalExpr(ctx.expr()) && ctx.block() != null) {
			visit(ctx.block());   // emit only this block
			return null;
		}

		// #elseif arms (in order)
		for (PreprocessorParser.ElseifClauseContext elif : ctx.elseifClause()) {
			if (evalExpr(elif.expr()) && elif.block() != null) {
				visit(elif.block());
				return null;
			}
		}

		// #else arm (optional)
		if (ctx.elseClause() != null && ctx.elseClause().block() != null) {
			visit(ctx.elseClause().block());
		}
		return null;
	}

	@Override
	public Void visitIfDefBlock(PreprocessorParser.IfDefBlockContext ctx) {
		if (ctx.primary() != null) {
			boolean b = ctx.primary().IDENT() != null ?
				defines.containsKey(ctx.primary().IDENT().getText().toUpperCase()) :
				true;
			// #ifdef arm
			if (b) {
				visit(ctx.block());   // emit only this block
				return null;
			}

			// #else arm (optional)
			if (ctx.elseClause() != null && ctx.elseClause().block() != null) {
				visit(ctx.elseClause().block());
			}
		}
		return null;
	}

	@Override
	public Void visitIfNDefBlock(PreprocessorParser.IfNDefBlockContext ctx) {
		if (ctx.primary() != null) {
			boolean b = ctx.primary().IDENT() != null ?
				defines.containsKey(ctx.primary().IDENT().getText().toUpperCase()) :
				true;
			// #ifdef arm
			if (!b) {
				visit(ctx.block());   // emit only this block
				return null;
			}

			// #else arm (optional)
			if (ctx.elseClause() != null && ctx.elseClause().block() != null) {
				visit(ctx.elseClause().block());
			}
		}
		return null;
	}

	/* ----- #for / #while / #do_while / #IFCOND ----- */
	private String getConditionCode(String s) {
		String result = null;
		switch (s.toUpperCase()) {
			case "==":
			case "EQ":
				result = "z";
				break;
			case "!=":
			case "NE":
				result = "nz";
				break;
			case "<":
			case "LT":
				result = "n";
				break;
			case "<=":
			case "LE":
				result = "np";
				break;
			case ">":
			case "GT":
				result = "p";
				break;
			case ">=":
			case "GE":
				result = "nn";
				break;
			default:
				throw new PreprocessorException("Illegal loop expression");
		}
		return result;
	}

	private String getNotConditionCode(String s) {
		String result = null;
		switch (s.toUpperCase()) {
			case "==":
			case "EQ":
			case "Z":
				result = "nz";
				break;
			case "!=":
			case "NE":
			case "NZ":
				result = "z";
				break;
			case "<":
			case "LT":
			case "N":
				result = "nn";
				break;
			case "<=":
			case "LE":
			case "NP":
				result = "p";
				break;
			case ">":
			case "GT":
			case "P":
				result = "np";
				break;
			case ">=":
			case "GE":
			case "NN":
				result = "n";
				break;
			case "PE":
				result = "po";
				break;
			case "PO":
				result = "pe";
				break;
			case "O":
				result = "no";
				break;
			case "NO":
				result = "o";
				break;
			default:
				throw new PreprocessorException("Illegal loop/cond expression");
		}
		return result;
	}

	@Override
	public Void visitForBlock(PreprocessorParser.ForBlockContext ctx) {
		if (ctx.cond != null && ctx.block() != null) {
			String blockName = "FOR_{}";
			emitLineBeginDirective(filename, lineNum);
			emitLine(".BLOCK " + blockName, false);

			String loopVar = ctx.cond.primary(0).getText();
			if (ctx.init != null) {
				emitLine("move " + loopVar + ", " + ctx.init.getText(), true);
			}
			emitLine("jump $_LOOP_TEST", true);
			emitLine("$_LOOP_BEGIN:", true);
			emitLineEndDirective(filename, lineNum);

			visit(ctx.block());

			emitLineBeginDirective(filename, lineNum);
			emitLine("$_LOOP_NEXT:", true);
			if (ctx.incr != null) {
				emitLine("add " + loopVar + ", " + ctx.incr.getText(), true);
			}
			emitLine("$_LOOP_TEST:", true);
			emitLine("cmp " + loopVar + ", " + ctx.cond.primary(1).getText(), true);
			String conditionOp = getConditionCode(ctx.cond.cmpOp().getText());
			emitLine("jump " + conditionOp + ", $_LOOP_BEGIN", false);
			emitLine("$_LOOP_END:", true);
			emitLine(".BLOCK_END", false);
			emitLineEndDirective(filename, lineNum);
		} else
			throw new PreprocessorException("For loop needs an expression and a block!");
		return null;
	}

	@Override
	public Void visitWhileBlock(PreprocessorParser.WhileBlockContext ctx) {
		if (ctx.cond != null && ctx.block() != null) {
			String blockName = "WHILE_{}";
			emitLineBeginDirective(filename, lineNum);
			emitLine(".BLOCK " + blockName, false);

			String loopVar = ctx.cond.primary(0).getText();
			emitLine("jump $_LOOP_TEST", true);
			emitLine("$_LOOP_BEGIN:", true);
			emitLineEndDirective(filename, lineNum);

			visit(ctx.block());

			emitLineBeginDirective(filename, lineNum);
			emitLine("$_LOOP_NEXT:", true);
			emitLine("$_LOOP_TEST:", true);
			if (ctx.cond.primary().size() == 1) {
				loopVar = applyDefines(loopVar);
				try {
					long i = Long.parseLong(loopVar);
					if (i != 0)
						emitLine("jump $_LOOP_BEGIN", false);
				} catch (NumberFormatException e) {
					emitLine("test " + loopVar, false);
					emitLine("jump nz, $_LOOP_BEGIN", false);
				}
			} else {
				emitLine("cmp " + loopVar + ", " + ctx.cond.primary(1).getText(), true);
				String conditionOp = getConditionCode(ctx.cond.cmpOp().getText());
				emitLine("jump " + conditionOp + ", $_LOOP_BEGIN", false);
			}
			emitLine("$_LOOP_END:", true);
			emitLine(".BLOCK_END", false);
			emitLineEndDirective(filename, lineNum);
		} else
			throw new PreprocessorException("For loop needs an expression and a block!");
		return null;
	}

	@Override
	public Void visitDoWhileBlock(PreprocessorParser.DoWhileBlockContext ctx) {
		if (ctx.cond != null && ctx.block() != null) {
			String blockName = "DO_WHILE_{}";
			emitLineBeginDirective(filename, lineNum);
			emitLine(".BLOCK " + blockName, false);

			String loopVar = ctx.cond.primary(0).getText();
			emitLine("$_LOOP_BEGIN:", true);
			emitLineEndDirective(filename, lineNum);

			visit(ctx.block());

			emitLineBeginDirective(filename, lineNum);
			emitLine("$_LOOP_NEXT:", true);
			emitLine("$_LOOP_TEST:", true);
			if (ctx.cond.primary().size() == 1) {
				loopVar = applyDefines(loopVar);
				try {
					long i = Long.parseLong(loopVar);
					if (i != 0)
						emitLine("jump $_LOOP_BEGIN", false);
				} catch (NumberFormatException e) {
					emitLine("test " + loopVar, false);
					emitLine("jump nz, $_LOOP_BEGIN", false);
				}
			} else {
				emitLine("cmp " + loopVar + ", " + ctx.cond.primary(1).getText(), true);
				String conditionOp = getConditionCode(ctx.cond.cmpOp().getText());
				emitLine("jump " + conditionOp + ", $_LOOP_BEGIN", false);
			}
			emitLine("$_LOOP_END:", true);
			emitLine(".BLOCK_END", false);
			emitLineEndDirective(filename, lineNum);
		} else
			throw new PreprocessorException("For loop needs an expression and a block!");
		return null;
	}

	@Override
	public Void visitBreakDir(PreprocessorParser.BreakDirContext ctx) {
		// $$ refers to the loop scope
		emitLine("JUMP $$_LOOP_END", true);
		return null;
	}

	@Override
	public Void visitContinueDir(PreprocessorParser.ContinueDirContext ctx) {
		// $$ refers to the loop scope
		emitLine("JUMP $$_LOOP_NEXT", true);
		return null;
	}

	@Override
	public Void visitIfCondBlock(PreprocessorParser.IfCondBlockContext ctx) {
		if (ctx.cond != null && ctx.block() != null) {
			String blockName = "COND_{}";
			emitLineBeginDirective(filename, lineNum);
			emitLine(".BLOCK " + blockName, false);

			// If Cond expr
			String leftVal = ctx.cond.primary(0).getText();
			String rightVal;
			String conditionOp;
			if (ctx.cond.primary().size() == 2) {
				rightVal = ctx.cond.primary(1).getText();
				conditionOp = getNotConditionCode(ctx.cond.cmpOp().getText());
				emitLine("cmp " + leftVal + ", " + rightVal, true);
				emitLine("jump " + conditionOp + ", $_SKIP", false);
			} else {
				leftVal = applyDefines(leftVal);
				try {
					long v = Long.parseLong(leftVal);
					if (v == 0)
						emitLine("jump $_SKIP", false);
				} catch (NumberFormatException e) {
					emitLine("test " + leftVal, false);
					emitLine("jump z, $_SKIP", false);
				}
			}
			emitLineEndDirective(filename, lineNum);

			visit(ctx.block());

			emitLineBeginDirective(filename, lineNum);
			if (ctx.elseCondClause() != null ||
				(ctx.elseifCondClause() != null && ctx.elseifCondClause().size() > 0))
				emitLine("jump $_COND_END", false);
			emitLine("$_SKIP:", false);
			emitLineEndDirective(filename, lineNum);

			// else if cond
			if (ctx.elseifCondClause() != null) {
				for (int i = 0; i < ctx.elseifCondClause().size(); ++i) {
					var ectx = ctx.elseifCondClause(i);
					leftVal = ectx.cond.primary(0).getText();
					if (ctx.cond.primary().size() == 2) {
						rightVal = ectx.cond.primary(1).getText();
						conditionOp = getNotConditionCode(ectx.cond.cmpOp().getText());
						emitLineBeginDirective(filename, lineNum);
						emitLine("cmp " + leftVal + ", " + rightVal, true);
						emitLine("jump " + conditionOp + ", $_SKIP_" + (i + 1), false);
					} else {
						leftVal = applyDefines(leftVal);
						try {
							long v = Long.parseLong(leftVal);
							if (v == 0)
								emitLine("jump $_SKIP_" + (i + 1), false);
						} catch (NumberFormatException e) {
							emitLine("test " + leftVal, false);
							emitLine("jump z, $_SKIP_" + (i + 1), false);
						}
					}
					emitLineEndDirective(filename, lineNum);

					visit(ectx.block());

					emitLineBeginDirective(filename, lineNum);
					if (ctx.elseCondClause() != null || i < ctx.elseifCondClause().size() - 1)
						emitLine("jump $_COND_END", false);
					emitLine("$_SKIP_" + (i + 1) + ":", false);
					emitLineEndDirective(filename, lineNum);
				}
			}

			// else cond
			if (ctx.elseCondClause() != null) {
				visit(ctx.elseCondClause().block());
			}
			emitLineBeginDirective(filename, lineNum);
			emitLine("$_COND_END:", false);
			emitLine(".BLOCK_END", false);
			emitLineEndDirective(filename, lineNum);
		} else
			throw new PreprocessorException("If condition needs an expression and a block!");
		return null;
	}

	@Override
	public Void visitIfCondSRBlock(PreprocessorParser.IfCondSRBlockContext ctx) {
		if (ctx.IDENT() != null && ctx.block() != null) {
			String blockName = "CONDSR_{}";
			emitLineBeginDirective(filename, lineNum);
			emitLine(".BLOCK " + blockName, false);

			// If Cond SR IDENT
			// IDENT is z, nz, n, p, nn, np, o or no
			String conditionOp = ctx.IDENT().getText().toUpperCase();
			if (!conditionOp.matches("Z|NZ|N|P|NN|NP|PE|PO|O|NO")) {
				throw new PreprocessorException("If condition SR IDENT must be one of: z, nz, n, p, nn, np, pe, po, o, no");
			}
			emitLine("jump " + getNotConditionCode(conditionOp) + ", $_SKIP", false);
			emitLineEndDirective(filename, lineNum);

			visit(ctx.block());

			emitLineBeginDirective(filename, lineNum);
			if (ctx.elseCondClause() != null)
				emitLine("jump $_COND_END", false);
			emitLine("$_SKIP:", false);
			emitLineEndDirective(filename, lineNum);

			// else cond
			if (ctx.elseCondClause() != null) {
				visit(ctx.elseCondClause().block());
			}

			emitLineBeginDirective(filename, lineNum);
			emitLine("$_COND_END:", false);
			emitLine(".BLOCK_END", false);
			emitLineEndDirective(filename, lineNum);
		} else
			throw new PreprocessorException("If condition SR needs an SR code and a block!");
		return null;
	}

	@Override
	public Void visitSyncBlock(PreprocessorParser.SyncBlockContext ctx) {
		if (ctx.IDENT() != null && ctx.block() != null) {
			String blockName = "SYNC_{}";
			String mutex = ctx.IDENT().getText().toUpperCase();
			emitLineBeginDirective(filename, lineNum);
			emitLine(".BLOCK " + blockName, false);
			emitLine("push " + mutex, true);
			emitLine("call acquireMutex", false);
			emitLine("add sp, 1", false);
			emitLineEndDirective(filename, lineNum);

			visit(ctx.block());

			emitLineBeginDirective(filename, lineNum);
			emitLine("push " + mutex, true);
			emitLine("call releaseMutex", false);
			emitLine("add sp, 1", false);
			emitLine(".BLOCK_END", false);
			emitLineEndDirective(filename, lineNum);
		} else
			throw new PreprocessorException("Sync block needs a mutex and a block!");
		return null;
	}

	@Override
	public Void visitBlock(PreprocessorParser.BlockContext ctx) {
		if (ctx.children == null) return null;
		for (var child : ctx.children) {
			child.accept(this);   // directives and codeLines recurse naturally
		}
		return null;
	}

	/* =========================================================
       Expression evaluation for #if
       ========================================================= */

	private boolean evalExpr(PreprocessorParser.ExprContext expr) {
		PreprocessorParser.PrimaryContext p0 = expr.primary(0);
		if (expr.cmpOp() == null) {
			// Truthiness of a single primary
			return truthy(p0);
		}
		PreprocessorParser.PrimaryContext p1 = expr.primary(1);
		String op = expr.cmpOp().getText();
		return compare(p0, op, p1);
	}

	private boolean truthy(PreprocessorParser.PrimaryContext p) {
		if (p.IDENT() != null) {
			String name = p.IDENT().getText();
			DefVal dv = defines.get(name);
			if (dv == null) return false;
			switch (dv.kind) {
				case INT:    return parseLongSafe(dv.text) != 0;
				case FLOAT:  return Double.compare(parseDoubleSafe(dv.text), 0.0) != 0;
				case CHAR:   return charValue(dv.text) != 0;
				case STRING: return stripQuotes(dv.text).length() != 0;
				case SYMBOL: return true;
			}
		} else if (p.literal() != null) {
			return literalTruthy(p.literal());
		}
		return false;
	}

	private boolean compare(PreprocessorParser.PrimaryContext a, String op, PreprocessorParser.PrimaryContext b) {
		// Try numeric comparison first if both are numeric-ish; otherwise string compare
		if (isNumericLike(a) && isNumericLike(b)) {
			double da = asDouble(a);
			double db = asDouble(b);
			switch (op) {
				case "==": return da == db;
				case "!=": return da != db;
				case "<":  return da <  db;
				case "<=": return da <= db;
				case ">":  return da >  db;
				case ">=": return da >= db;
			}
		} else {
			String sa = asString(a);
			String sb = asString(b);
			int cmp = sa.compareTo(sb);
			switch (op) {
				case "==": return cmp == 0;
				case "!=": return cmp != 0;
				case "<":  return cmp <  0;
				case "<=": return cmp <= 0;
				case ">":  return cmp >  0;
				case ">=": return cmp >= 0;
			}
		}
		throw new PreprocessorException("Unsupported compare op: " + op);
	}

	private boolean isNumericLike(PreprocessorParser.PrimaryContext p) {
		if (p.literal() != null) {
			return p.literal().INT()!=null || p.literal().FLOAT()!=null || p.literal().CHAR()!=null;
		}
		if (p.IDENT() != null) {
			DefVal dv = defines.get(p.IDENT().getText());
			if (dv == null) return false;
			return dv.kind==DefVal.Kind.INT || dv.kind==DefVal.Kind.FLOAT || dv.kind==DefVal.Kind.CHAR;
		}
		return false;
	}

	private double asDouble(PreprocessorParser.PrimaryContext p) {
		if (p.literal()!=null) {
			if (p.literal().INT()!=null)   return (double) parseLongSafe(p.literal().INT().getText());
			if (p.literal().FLOAT()!=null) return parseDoubleSafe(p.literal().FLOAT().getText());
			if (p.literal().CHAR()!=null)  return (double) charValue(p.literal().CHAR().getText());
		}
		if (p.IDENT()!=null) {
			DefVal dv = defines.get(p.IDENT().getText());
			if (dv==null) return 0.0;
			switch (dv.kind) {
				case INT:   return (double) parseLongSafe(dv.text);
				case FLOAT: return parseDoubleSafe(dv.text);
				case CHAR:  return (double) charValue(dv.text);
				default:    return 0.0;
			}
		}
		return 0.0;
	}

	private String asString(PreprocessorParser.PrimaryContext p) {
		if (p.literal()!=null) {
			if (p.literal().STRING()!=null) return stripQuotes(p.literal().STRING().getText());
			if (p.literal().CHAR()!=null)   return new String(new char[]{ (char) charValue(p.literal().CHAR().getText()) });
			return p.literal().getText();
		}
		if (p.IDENT()!=null) {
			DefVal dv = defines.get(p.IDENT().getText());
			return dv==null ? "" : dv.text;
		}
		return "";
	}

	private boolean literalTruthy(PreprocessorParser.LiteralContext lit) {
		if (lit.INT()!=null)   return parseLongSafe(lit.INT().getText()) != 0;
		if (lit.FLOAT()!=null) return Double.compare(parseDoubleSafe(lit.FLOAT().getText()), 0.0) != 0;
		if (lit.CHAR()!=null)  return charValue(lit.CHAR().getText()) != 0;
		if (lit.STRING()!=null) return stripQuotes(lit.STRING().getText()).length() != 0;
		return false;
	}

    /* =========================================================
       Helpers
       ========================================================= */

	private void emitLine(String s, boolean doSubst) {
		if (!lineDirectives.isEmpty()) {
			out.append(lineDirectives.pop());
			lineDirectives.clear();
		}
		if (doSubst) {
			out.append(applyDefines(s));
		} else {
			out.append(s);
		}
		if (s.length() == 0 || s.charAt(s.length() - 1) != '\n')
			out.append('\n');
		if (pauseLineSync <= 0 ) ++lineNum;
	}

	private void emitLineDirective(String filename, int line) {
		if (pauseLineSync <= 0) {
			lineDirectives.push(String.format(".LINE \u00ab%s\u00bb, %d%n", filename, line));
			lineNum = line;
		}
	}

	private void emitLineBeginDirective(String filename, int line) {
		lineDirectives.clear();
		++pauseLineSync;
		if (pauseLineSync <= 1)
			emitLine(String.format(".LINE_BEGIN \u00ab%s\u00bb, %d", filename, lineNum), false);
	}

	private void emitLineEndDirective(String filename, int line) {
		--pauseLineSync;
		if (pauseLineSync <= 0)
			emitLine(".LINE_END", false);
	}

	/** Token-aware substitution (whole identifiers only). */
	private String applyDefines(String line) {
		if (line.isEmpty()) return line;
		Matcher m = TOKEN.matcher(line);
		StringBuffer sb = new StringBuffer(line.length());
		while (m.find()) {
			// Check if the token is inside quotes before substituting
			int start = m.start();
			if (isInsideQuotes(line, start)) {
				// leave this token unchanged
				m.appendReplacement(sb, m.group());
				continue;
			}
			String ident = m.group().toUpperCase();
			String replacement = null;
			DefVal dv = defines.get(ident);
			if (dv != null) {
				// Use stored original text (so strings keep quotes etc.)
				replacement = dv.text;
			} else {
				replacement = lookupVar(ident);
				// No replacement
				if (replacement == null) {
					if (ident.equals("__FILE__"))
						replacement = '"' + filename + '"';
					else if (ident.equals("__LINE__"))
						replacement = String.valueOf(lineNum);
					else
						continue;
				}
			}
			// Escape backslashes and dollars for appendReplacement
			replacement = replacement.replace("\\", "\\\\").replace("$", "\\$");
			m.appendReplacement(sb, replacement);
		}
		m.appendTail(sb);
		return sb.toString();
	}

	/** Utility: return true if position `pos` lies inside a quoted string. */
	private static boolean isInsideQuotes(String s, int pos) {
		boolean inSingle = false;
		boolean inDouble = false;
		boolean escaped = false;
		for (int i = 0; i < pos; i++) {
			char c = s.charAt(i);
			if (escaped) { escaped = false; continue; }
			if (c == '\\') { escaped = true; continue; }
			if (c == '\'' && !inDouble) inSingle = !inSingle;
			else if (c == '"' && !inSingle) inDouble = !inDouble;
		}
		return inSingle || inDouble;
	}

	/** Token-aware substitution (whole identifiers only). */
	private String applyPlaceholders(String line, Map<String, String> formalArgs) {
		if (line.isEmpty()) return line;
		Matcher m = PLACEHOLDER.matcher(line);
		StringBuffer sb = new StringBuffer(line.length());
		while (m.find()) {
			String ident = m.group(1).toUpperCase();
			String replacement = formalArgs.get(ident);
			if (replacement == null) continue;
			// Escape backslashes and dollars for appendReplacement
			replacement = replacement.replace("\\", "\\\\").replace("$", "\\$");
			m.appendReplacement(sb, replacement);
		}
		m.appendTail(sb);
		return sb.toString();
	}

	public DefVal getDefine(String symbol) {
		DefVal dv = defines.get(symbol);
		return dv;
	}

	private static long parseLongSafe(String s) {
		try { return Long.decode(s); } catch (Exception e) { return 0L; }
	}

	private static double parseDoubleSafe(String s) {
		try { return Double.parseDouble(s); } catch (Exception e) { return 0.0; }
	}

	/** Parse a char literal like `'c'` or `'\n'` or `'\u00A9'`. */
	private static int charValue(String charToken) {
		// charToken includes quotes
		if (charToken.length() >= 3 && charToken.charAt(0)=='\'' && charToken.charAt(charToken.length()-1)=='\'') {
			String body = charToken.substring(1, charToken.length()-1);
			if (body.startsWith("\\u") && body.length()==6) {
				return Integer.parseInt(body.substring(2), 16);
			}
			if (body.startsWith("\\")) {
				switch (body) {
					case "\\n": return '\n';
					case "\\r": return '\r';
					case "\\t": return '\t';
					case "\\f": return '\f';
					case "\\b": return '\b';
					case "\\'": return '\'';
					case "\\\"": return '\"';
					case "\\\\": return '\\';
					default: return body.charAt(body.length()-1); // fallback last char
				}
			}
			return body.isEmpty() ? 0 : body.charAt(0);
		}
		return 0;
	}

	/** Strip surrounding double quotes (") if present. */
	private static String stripQuotes(String s) {
		if (s!=null && s.length()>=2 && s.charAt(0)=='"' && s.charAt(s.length()-1)=='"') {
			return s.substring(1, s.length()-1);
		}
		return s;
	}

	/** Normalize ANGLE_PATH or quoted string to just the inner path. */
	private static String normalizeIncludeTarget(String raw) {
		if (raw == null) return null;
		if (raw.startsWith("<") && raw.endsWith(">")) {
			return raw.substring(1, raw.length()-1).trim();
		}
		return raw; // already stripped quotes by caller
	}

	/** Rebuild a directive line with spaces, rather than ctx.getText(). */
	private static String reflowTokens(ParserRuleContext ctx) {
		Token start = ctx.getStart();
		Token stop  = ctx.getStop();
		if (start == null || stop == null) return "";
		// CharStream slice from the original input
		return start.getInputStream().getText(Interval.of(start.getStartIndex(), stop.getStopIndex()));
	}

    /* =========================================================
       Public entry points
       ========================================================= */

	/**
	 * Convenience: preprocess a source string with a given IncludeLoader.
	 * This one starts with a fresh, empty define table.
	 */
	public static String preprocessText(String filename, String source, IncludeLoader loader) {
		return preprocessText(filename, 1, source, loader, null, null, null, true, 0);
	}

	// Sets elements of args to null when they are used.
	public static String preprocessText(String filename, String source, IncludeLoader loader, String[] args) {
		HashMap<String, PreprocessorVisitor.DefVal> definitions = new HashMap<>();
		for (int i = 0; i < args.length; ++i) {
			String arg = args[i];
			if (arg.charAt(0) == '-') {
				if (arg.equals("--DEBUG")) {
					definitions.put("__DEBUG__", new PreprocessorVisitor.DefVal(PreprocessorVisitor.DefVal.Kind.SYMBOL, "1"));
					args[i] = null;
				} else if (arg.startsWith("-D")) {
					var def = arg.substring(2).split("=", 2);
					if (def.length == 2) {
						definitions.put(def[0], new PreprocessorVisitor.DefVal(PreprocessorVisitor.DefVal.Kind.STRING, def[1]));
					} else {
						definitions.put(def[0], new PreprocessorVisitor.DefVal(PreprocessorVisitor.DefVal.Kind.SYMBOL, "1"));
					}
					args[i] = null;
				}
			}
		}
		return preprocessText(filename, 1, source, loader, definitions, null, null, true, 0);
	}

	/**
	 * Same as above, but allows reusing an existing define map (e.g., across #include).
	 */
	public static String preprocessText(String filename,
										int lineNum,
										String source,
										IncludeLoader loader,
										Map<String, DefVal> seedDefines,
										Map<String, Pair<List<String>, String>> seedMacros,
										Set<String> seedIncludes,
										boolean substituteInsideDirectives,
										int pauseLineSync) {
		CharStream input = CharStreams.fromString(source);
		PreprocessorLexer lexer = new PreprocessorLexer(input);
		CommonTokenStream tokens = new CommonTokenStream(lexer);
		PreprocessorParser parser = new PreprocessorParser(tokens);
		parser.removeErrorListeners();
		parser.addErrorListener(new DiagnosticErrorListener());

		PreprocessorVisitor v = new PreprocessorVisitor(filename, lineNum, loader, substituteInsideDirectives, pauseLineSync);
		v.defines = seedDefines != null ? seedDefines : new HashMap<>();
		v.macros = seedMacros != null ? seedMacros : new HashMap<>();
		v.previouslyIncluded = seedIncludes != null ? seedIncludes : new HashSet<>();
		v.visit(parser.preproc());
		return v.getOutput();
	}
}
