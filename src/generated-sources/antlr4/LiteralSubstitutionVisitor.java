// Generated from LiteralSubstitution.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64v2;
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link LiteralSubstitutionParser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface LiteralSubstitutionVisitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link LiteralSubstitutionParser#file}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFile(LiteralSubstitutionParser.FileContext ctx);
	/**
	 * Visit a parse tree produced by {@link LiteralSubstitutionParser#piece}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPiece(LiteralSubstitutionParser.PieceContext ctx);
}