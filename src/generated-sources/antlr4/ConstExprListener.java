// Generated from ConstExpr.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link ConstExprParser}.
 */
public interface ConstExprListener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link ConstExprParser#line}.
	 * @param ctx the parse tree
	 */
	void enterLine(ConstExprParser.LineContext ctx);
	/**
	 * Exit a parse tree produced by {@link ConstExprParser#line}.
	 * @param ctx the parse tree
	 */
	void exitLine(ConstExprParser.LineContext ctx);
	/**
	 * Enter a parse tree produced by the {@code PartExpr}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void enterPartExpr(ConstExprParser.PartExprContext ctx);
	/**
	 * Exit a parse tree produced by the {@code PartExpr}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void exitPartExpr(ConstExprParser.PartExprContext ctx);
	/**
	 * Enter a parse tree produced by the {@code PartChar}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void enterPartChar(ConstExprParser.PartCharContext ctx);
	/**
	 * Exit a parse tree produced by the {@code PartChar}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void exitPartChar(ConstExprParser.PartCharContext ctx);
	/**
	 * Enter a parse tree produced by the {@code PartString}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void enterPartString(ConstExprParser.PartStringContext ctx);
	/**
	 * Exit a parse tree produced by the {@code PartString}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void exitPartString(ConstExprParser.PartStringContext ctx);
	/**
	 * Enter a parse tree produced by the {@code PartOther}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void enterPartOther(ConstExprParser.PartOtherContext ctx);
	/**
	 * Exit a parse tree produced by the {@code PartOther}
	 * labeled alternative in {@link ConstExprParser#part}.
	 * @param ctx the parse tree
	 */
	void exitPartOther(ConstExprParser.PartOtherContext ctx);
	/**
	 * Enter a parse tree produced by {@link ConstExprParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterExpr(ConstExprParser.ExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ConstExprParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitExpr(ConstExprParser.ExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ConstExprParser#addExpr}.
	 * @param ctx the parse tree
	 */
	void enterAddExpr(ConstExprParser.AddExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ConstExprParser#addExpr}.
	 * @param ctx the parse tree
	 */
	void exitAddExpr(ConstExprParser.AddExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ConstExprParser#mulExpr}.
	 * @param ctx the parse tree
	 */
	void enterMulExpr(ConstExprParser.MulExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ConstExprParser#mulExpr}.
	 * @param ctx the parse tree
	 */
	void exitMulExpr(ConstExprParser.MulExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ConstExprParser#unaryExpr}.
	 * @param ctx the parse tree
	 */
	void enterUnaryExpr(ConstExprParser.UnaryExprContext ctx);
	/**
	 * Exit a parse tree produced by {@link ConstExprParser#unaryExpr}.
	 * @param ctx the parse tree
	 */
	void exitUnaryExpr(ConstExprParser.UnaryExprContext ctx);
	/**
	 * Enter a parse tree produced by {@link ConstExprParser#primary}.
	 * @param ctx the parse tree
	 */
	void enterPrimary(ConstExprParser.PrimaryContext ctx);
	/**
	 * Exit a parse tree produced by {@link ConstExprParser#primary}.
	 * @param ctx the parse tree
	 */
	void exitPrimary(ConstExprParser.PrimaryContext ctx);
}