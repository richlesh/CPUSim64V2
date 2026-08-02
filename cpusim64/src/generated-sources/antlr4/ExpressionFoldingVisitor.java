// Generated from ExpressionFolding.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link ExpressionFoldingParser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface ExpressionFoldingVisitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link ExpressionFoldingParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitExpr(ExpressionFoldingParser.ExprContext ctx);
	/**
	 * Visit a parse tree produced by {@link ExpressionFoldingParser#addExpr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAddExpr(ExpressionFoldingParser.AddExprContext ctx);
	/**
	 * Visit a parse tree produced by {@link ExpressionFoldingParser#mulExpr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMulExpr(ExpressionFoldingParser.MulExprContext ctx);
	/**
	 * Visit a parse tree produced by {@link ExpressionFoldingParser#unaryExpr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitUnaryExpr(ExpressionFoldingParser.UnaryExprContext ctx);
	/**
	 * Visit a parse tree produced by {@link ExpressionFoldingParser#primary}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPrimary(ExpressionFoldingParser.PrimaryContext ctx);
}