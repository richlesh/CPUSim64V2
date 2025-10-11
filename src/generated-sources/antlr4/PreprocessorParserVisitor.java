// Generated from PreprocessorParser.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64v2;
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link PreprocessorParser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface PreprocessorParserVisitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#preproc}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPreproc(PreprocessorParser.PreprocContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#codeLine}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCodeLine(PreprocessorParser.CodeLineContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitDirective(PreprocessorParser.DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#includeDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIncludeDir(PreprocessorParser.IncludeDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#defineDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitDefineDir(PreprocessorParser.DefineDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#undefDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitUndefDir(PreprocessorParser.UndefDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#callDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCallDir(PreprocessorParser.CallDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#macroDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMacroDir(PreprocessorParser.MacroDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#argList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitArgList(PreprocessorParser.ArgListContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#callArg}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCallArg(PreprocessorParser.CallArgContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#defFuncDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitDefFuncDir(PreprocessorParser.DefFuncDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#defMacroDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitDefMacroDir(PreprocessorParser.DefMacroDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#paramList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitParamList(PreprocessorParser.ParamListContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#codeLineOrDirective}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCodeLineOrDirective(PreprocessorParser.CodeLineOrDirectiveContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#svarDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitSvarDir(PreprocessorParser.SvarDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#varDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitVarDir(PreprocessorParser.VarDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#fvarDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFvarDir(PreprocessorParser.FvarDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#identList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIdentList(PreprocessorParser.IdentListContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#returnDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitReturnDir(PreprocessorParser.ReturnDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#freturnDir}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFreturnDir(PreprocessorParser.FreturnDirContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#ifBlock}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIfBlock(PreprocessorParser.IfBlockContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#elseifClause}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitElseifClause(PreprocessorParser.ElseifClauseContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#elseClause}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitElseClause(PreprocessorParser.ElseClauseContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#block}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBlock(PreprocessorParser.BlockContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitExpr(PreprocessorParser.ExprContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#primary}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPrimary(PreprocessorParser.PrimaryContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#cmpOp}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCmpOp(PreprocessorParser.CmpOpContext ctx);
	/**
	 * Visit a parse tree produced by {@link PreprocessorParser#literal}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLiteral(PreprocessorParser.LiteralContext ctx);
}