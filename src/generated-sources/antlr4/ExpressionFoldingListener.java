// Generated from ExpressionFolding.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link ExpressionFoldingParser}.
 */
public interface ExpressionFoldingListener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link ExpressionFoldingParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterExpr(ExpressionFoldingParser.ExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ExpressionFoldingParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitExpr(ExpressionFoldingParser.ExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ExpressionFoldingParser#addExpr}.
	 * @param ctx the parse tree
	 */
	void enterAddExpr(ExpressionFoldingParser.AddExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ExpressionFoldingParser#addExpr}.
	 * @param ctx the parse tree
	 */
	void exitAddExpr(ExpressionFoldingParser.AddExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ExpressionFoldingParser#mulExpr}.
	 * @param ctx the parse tree
	 */
	void enterMulExpr(ExpressionFoldingParser.MulExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ExpressionFoldingParser#mulExpr}.
	 * @param ctx the parse tree
	 */
	void exitMulExpr(ExpressionFoldingParser.MulExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ExpressionFoldingParser#unaryExpr}.
	 * @param ctx the parse tree
	 */
	void enterUnaryExpr(ExpressionFoldingParser.UnaryExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ExpressionFoldingParser#unaryExpr}.
	 * @param ctx the parse tree
	 */
	void exitUnaryExpr(ExpressionFoldingParser.UnaryExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ExpressionFoldingParser#primary}.
	 * @param ctx the parse tree
	 */
	void enterPrimary(ExpressionFoldingParser.PrimaryContext ctx);
	/**
	 * Exit a parse tree produced by {@link ExpressionFoldingParser#primary}.
	 * @param ctx the parse tree
	 */
	void exitPrimary(ExpressionFoldingParser.PrimaryContext ctx);
}