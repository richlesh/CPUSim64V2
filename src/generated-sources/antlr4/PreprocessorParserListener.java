// Generated from PreprocessorParser.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64v2;
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link PreprocessorParser}.
 */
public interface PreprocessorParserListener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#preproc}.
	 * @param ctx the parse tree
	 */
	void enterPreproc(PreprocessorParser.PreprocContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#preproc}.
	 * @param ctx the parse tree
	 */
	void exitPreproc(PreprocessorParser.PreprocContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#codeLine}.
	 * @param ctx the parse tree
	 */
	void enterCodeLine(PreprocessorParser.CodeLineContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#codeLine}.
	 * @param ctx the parse tree
	 */
	void exitCodeLine(PreprocessorParser.CodeLineContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#directive}.
	 * @param ctx the parse tree
	 */
	void enterDirective(PreprocessorParser.DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#directive}.
	 * @param ctx the parse tree
	 */
	void exitDirective(PreprocessorParser.DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#includeDir}.
	 * @param ctx the parse tree
	 */
	void enterIncludeDir(PreprocessorParser.IncludeDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#includeDir}.
	 * @param ctx the parse tree
	 */
	void exitIncludeDir(PreprocessorParser.IncludeDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#defineDir}.
	 * @param ctx the parse tree
	 */
	void enterDefineDir(PreprocessorParser.DefineDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#defineDir}.
	 * @param ctx the parse tree
	 */
	void exitDefineDir(PreprocessorParser.DefineDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#undefDir}.
	 * @param ctx the parse tree
	 */
	void enterUndefDir(PreprocessorParser.UndefDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#undefDir}.
	 * @param ctx the parse tree
	 */
	void exitUndefDir(PreprocessorParser.UndefDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#callDir}.
	 * @param ctx the parse tree
	 */
	void enterCallDir(PreprocessorParser.CallDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#callDir}.
	 * @param ctx the parse tree
	 */
	void exitCallDir(PreprocessorParser.CallDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#macroDir}.
	 * @param ctx the parse tree
	 */
	void enterMacroDir(PreprocessorParser.MacroDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#macroDir}.
	 * @param ctx the parse tree
	 */
	void exitMacroDir(PreprocessorParser.MacroDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#argList}.
	 * @param ctx the parse tree
	 */
	void enterArgList(PreprocessorParser.ArgListContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#argList}.
	 * @param ctx the parse tree
	 */
	void exitArgList(PreprocessorParser.ArgListContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#callArg}.
	 * @param ctx the parse tree
	 */
	void enterCallArg(PreprocessorParser.CallArgContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#callArg}.
	 * @param ctx the parse tree
	 */
	void exitCallArg(PreprocessorParser.CallArgContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#defFuncDir}.
	 * @param ctx the parse tree
	 */
	void enterDefFuncDir(PreprocessorParser.DefFuncDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#defFuncDir}.
	 * @param ctx the parse tree
	 */
	void exitDefFuncDir(PreprocessorParser.DefFuncDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#defMacroDir}.
	 * @param ctx the parse tree
	 */
	void enterDefMacroDir(PreprocessorParser.DefMacroDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#defMacroDir}.
	 * @param ctx the parse tree
	 */
	void exitDefMacroDir(PreprocessorParser.DefMacroDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#paramList}.
	 * @param ctx the parse tree
	 */
	void enterParamList(PreprocessorParser.ParamListContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#paramList}.
	 * @param ctx the parse tree
	 */
	void exitParamList(PreprocessorParser.ParamListContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#codeLineOrDirective}.
	 * @param ctx the parse tree
	 */
	void enterCodeLineOrDirective(PreprocessorParser.CodeLineOrDirectiveContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#codeLineOrDirective}.
	 * @param ctx the parse tree
	 */
	void exitCodeLineOrDirective(PreprocessorParser.CodeLineOrDirectiveContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#svarDir}.
	 * @param ctx the parse tree
	 */
	void enterSvarDir(PreprocessorParser.SvarDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#svarDir}.
	 * @param ctx the parse tree
	 */
	void exitSvarDir(PreprocessorParser.SvarDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#varDir}.
	 * @param ctx the parse tree
	 */
	void enterVarDir(PreprocessorParser.VarDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#varDir}.
	 * @param ctx the parse tree
	 */
	void exitVarDir(PreprocessorParser.VarDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#fvarDir}.
	 * @param ctx the parse tree
	 */
	void enterFvarDir(PreprocessorParser.FvarDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#fvarDir}.
	 * @param ctx the parse tree
	 */
	void exitFvarDir(PreprocessorParser.FvarDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#identList}.
	 * @param ctx the parse tree
	 */
	void enterIdentList(PreprocessorParser.IdentListContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#identList}.
	 * @param ctx the parse tree
	 */
	void exitIdentList(PreprocessorParser.IdentListContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#returnDir}.
	 * @param ctx the parse tree
	 */
	void enterReturnDir(PreprocessorParser.ReturnDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#returnDir}.
	 * @param ctx the parse tree
	 */
	void exitReturnDir(PreprocessorParser.ReturnDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#freturnDir}.
	 * @param ctx the parse tree
	 */
	void enterFreturnDir(PreprocessorParser.FreturnDirContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#freturnDir}.
	 * @param ctx the parse tree
	 */
	void exitFreturnDir(PreprocessorParser.FreturnDirContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#ifBlock}.
	 * @param ctx the parse tree
	 */
	void enterIfBlock(PreprocessorParser.IfBlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#ifBlock}.
	 * @param ctx the parse tree
	 */
	void exitIfBlock(PreprocessorParser.IfBlockContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#elseifClause}.
	 * @param ctx the parse tree
	 */
	void enterElseifClause(PreprocessorParser.ElseifClauseContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#elseifClause}.
	 * @param ctx the parse tree
	 */
	void exitElseifClause(PreprocessorParser.ElseifClauseContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#elseClause}.
	 * @param ctx the parse tree
	 */
	void enterElseClause(PreprocessorParser.ElseClauseContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#elseClause}.
	 * @param ctx the parse tree
	 */
	void exitElseClause(PreprocessorParser.ElseClauseContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#forBlock}.
	 * @param ctx the parse tree
	 */
	void enterForBlock(PreprocessorParser.ForBlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#forBlock}.
	 * @param ctx the parse tree
	 */
	void exitForBlock(PreprocessorParser.ForBlockContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#whileBlock}.
	 * @param ctx the parse tree
	 */
	void enterWhileBlock(PreprocessorParser.WhileBlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#whileBlock}.
	 * @param ctx the parse tree
	 */
	void exitWhileBlock(PreprocessorParser.WhileBlockContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#doWhileBlock}.
	 * @param ctx the parse tree
	 */
	void enterDoWhileBlock(PreprocessorParser.DoWhileBlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#doWhileBlock}.
	 * @param ctx the parse tree
	 */
	void exitDoWhileBlock(PreprocessorParser.DoWhileBlockContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#ifCondBlock}.
	 * @param ctx the parse tree
	 */
	void enterIfCondBlock(PreprocessorParser.IfCondBlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#ifCondBlock}.
	 * @param ctx the parse tree
	 */
	void exitIfCondBlock(PreprocessorParser.IfCondBlockContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#elseifCondClause}.
	 * @param ctx the parse tree
	 */
	void enterElseifCondClause(PreprocessorParser.ElseifCondClauseContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#elseifCondClause}.
	 * @param ctx the parse tree
	 */
	void exitElseifCondClause(PreprocessorParser.ElseifCondClauseContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#elseCondClause}.
	 * @param ctx the parse tree
	 */
	void enterElseCondClause(PreprocessorParser.ElseCondClauseContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#elseCondClause}.
	 * @param ctx the parse tree
	 */
	void exitElseCondClause(PreprocessorParser.ElseCondClauseContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#block}.
	 * @param ctx the parse tree
	 */
	void enterBlock(PreprocessorParser.BlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#block}.
	 * @param ctx the parse tree
	 */
	void exitBlock(PreprocessorParser.BlockContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterExpr(PreprocessorParser.ExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitExpr(PreprocessorParser.ExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#primary}.
	 * @param ctx the parse tree
	 */
	void enterPrimary(PreprocessorParser.PrimaryContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#primary}.
	 * @param ctx the parse tree
	 */
	void exitPrimary(PreprocessorParser.PrimaryContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#cmpOp}.
	 * @param ctx the parse tree
	 */
	void enterCmpOp(PreprocessorParser.CmpOpContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#cmpOp}.
	 * @param ctx the parse tree
	 */
	void exitCmpOp(PreprocessorParser.CmpOpContext ctx);
	/**
	 * Enter a parse tree produced by {@link PreprocessorParser#literal}.
	 * @param ctx the parse tree
	 */
	void enterLiteral(PreprocessorParser.LiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link PreprocessorParser#literal}.
	 * @param ctx the parse tree
	 */
	void exitLiteral(PreprocessorParser.LiteralContext ctx);
}