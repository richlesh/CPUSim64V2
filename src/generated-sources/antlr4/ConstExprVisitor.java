// Generated from ConstExpr.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link ConstExprParser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface ConstExprVisitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link ConstExprParser#line}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLine(ConstExprParser.LineContext ctx);
	/**
	 * Visit a parse tree produced by the {@code PartExpr}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPartExpr(ConstExprParser.PartExprContext ctx);
	/**
	 * Visit a parse tree produced by the {@code PartChar}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPartChar(ConstExprParser.PartCharContext ctx);
	/**
	 * Visit a parse tree produced by the {@code PartString}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPartString(ConstExprParser.PartStringContext ctx);
	/**
	 * Visit a parse tree produced by the {@code PartIdent}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPartIdent(ConstExprParser.PartIdentContext ctx);
	/**
	 * Visit a parse tree produced by the {@code PartOther}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPartOther(ConstExprParser.PartOtherContext ctx);
	/**
	 * Visit a parse tree produced by the {@code unaryExpr}
	 * labeled alternative in {@link ConstExprParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitUnaryExpr(ConstExprParser.UnaryExprContext ctx);
	/**
	 * Visit a parse tree produced by the {@code primaryExpr}
	 * labeled alternative in {@link ConstExprParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPrimaryExpr(ConstExprParser.PrimaryExprContext ctx);
	/**
	 * Visit a parse tree produced by the {@code addExpr}
	 * labeled alternative in {@link ConstExprParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAddExpr(ConstExprParser.AddExprContext ctx);
	/**
	 * Visit a parse tree produced by the {@code mulExpr}
	 * labeled alternative in {@link ConstExprParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMulExpr(ConstExprParser.MulExprContext ctx);
	/**
	 * Visit a parse tree produced by the {@code parensExpr}
	 * labeled alternative in {@link ConstExprParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitParensExpr(ConstExprParser.ParensExprContext ctx);
	/**
	 * Visit a parse tree produced by {@link ConstExprParser#primary}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPrimary(ConstExprParser.PrimaryContext ctx);
}