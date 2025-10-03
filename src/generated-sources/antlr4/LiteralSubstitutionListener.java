// Generated from LiteralSubstitution.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64v2;
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link LiteralSubstitutionParser}.
 */
public interface LiteralSubstitutionListener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link LiteralSubstitutionParser#file}.
	 * @param ctx the parse tree
	 */
	void enterFile(LiteralSubstitutionParser.FileContext ctx);
	/**
	 * Exit a parse tree produced by {@link LiteralSubstitutionParser#file}.
	 * @param ctx the parse tree
	 */
	void exitFile(LiteralSubstitutionParser.FileContext ctx);
	/**
	 * Enter a parse tree produced by {@link LiteralSubstitutionParser#piece}.
	 * @param ctx the parse tree
	 */
	void enterPiece(LiteralSubstitutionParser.PieceContext ctx);
	/**
	 * Exit a parse tree produced by {@link LiteralSubstitutionParser#piece}.
	 * @param ctx the parse tree
	 */
	void exitPiece(LiteralSubstitutionParser.PieceContext ctx);
}