// Generated from CPUSim64.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link CPUSim64Parser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface CPUSim64Visitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#program}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitProgram(CPUSim64Parser.ProgramContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#line}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLine(CPUSim64Parser.LineContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#labelDef}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLabelDef(CPUSim64Parser.LabelDefContext ctx);
	/**
	 * Visit a parse tree produced by the {@code Data_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitData_Directive(CPUSim64Parser.Data_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code ORG_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitORG_Directive(CPUSim64Parser.ORG_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code LINE_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLINE_Directive(CPUSim64Parser.LINE_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code LINE_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLINE_BEGIN_Directive(CPUSim64Parser.LINE_BEGIN_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code LINE_END_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLINE_END_Directive(CPUSim64Parser.LINE_END_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code BLOCK_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBLOCK_BEGIN_Directive(CPUSim64Parser.BLOCK_BEGIN_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code BLOCK_END_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBLOCK_END_Directive(CPUSim64Parser.BLOCK_END_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#dataDirective}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitDataDirective(CPUSim64Parser.DataDirectiveContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#intList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIntList(CPUSim64Parser.IntListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#floatList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFloatList(CPUSim64Parser.FloatListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#charList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCharList(CPUSim64Parser.CharListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#byteList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitByteList(CPUSim64Parser.ByteListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instruction}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstruction(CPUSim64Parser.InstructionContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrNOP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrNOP(CPUSim64Parser.InstrNOPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrDEBUG}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrDEBUG(CPUSim64Parser.InstrDEBUGContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrCLEAR}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCLEAR(CPUSim64Parser.InstrCLEARContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrMOVE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrMOVE(CPUSim64Parser.InstrMOVEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrLOAD}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrLOAD(CPUSim64Parser.InstrLOADContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrSTORE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSTORE(CPUSim64Parser.InstrSTOREContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrPOP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPOP(CPUSim64Parser.InstrPOPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrPUSH}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPUSH(CPUSim64Parser.InstrPUSHContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#branchModes}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBranchModes(CPUSim64Parser.BranchModesContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrJUMP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrJUMP(CPUSim64Parser.InstrJUMPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrCALL}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCALL(CPUSim64Parser.InstrCALLContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrRETURN}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRETURN(CPUSim64Parser.InstrRETURNContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrINTERRUPT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrINTERRUPT(CPUSim64Parser.InstrINTERRUPTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrSTOP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSTOP(CPUSim64Parser.InstrSTOPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrNEG}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrNEG(CPUSim64Parser.InstrNEGContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#arithmeticModes}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitArithmeticModes(CPUSim64Parser.ArithmeticModesContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrADD}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrADD(CPUSim64Parser.InstrADDContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrSUB}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSUB(CPUSim64Parser.InstrSUBContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrMULT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrMULT(CPUSim64Parser.InstrMULTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrDIV}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrDIV(CPUSim64Parser.InstrDIVContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrRECIP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRECIP(CPUSim64Parser.InstrRECIPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrCOMPL}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCOMPL(CPUSim64Parser.InstrCOMPLContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#logicModes}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLogicModes(CPUSim64Parser.LogicModesContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrAND}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrAND(CPUSim64Parser.InstrANDContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrOR}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrOR(CPUSim64Parser.InstrORContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrXOR}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrXOR(CPUSim64Parser.InstrXORContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrTEST}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrTEST(CPUSim64Parser.InstrTESTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrCMP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCMP(CPUSim64Parser.InstrCMPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrLSHIFT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrLSHIFT(CPUSim64Parser.InstrLSHIFTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrRSHIFT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRSHIFT(CPUSim64Parser.InstrRSHIFTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrARSHIFT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrARSHIFT(CPUSim64Parser.InstrARSHIFTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrLROTATE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrLROTATE(CPUSim64Parser.InstrLROTATEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrRROTATE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRROTATE(CPUSim64Parser.InstrRROTATEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrIN}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrIN(CPUSim64Parser.InstrINContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrOUT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrOUT(CPUSim64Parser.InstrOUTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrPACK}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPACK(CPUSim64Parser.InstrPACKContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrPACK64}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPACK64(CPUSim64Parser.InstrPACK64Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrUNPACK}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrUNPACK(CPUSim64Parser.InstrUNPACKContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrUNPACK64}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrUNPACK64(CPUSim64Parser.InstrUNPACK64Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrCAS}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCAS(CPUSim64Parser.InstrCASContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrENDIAN}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrENDIAN(CPUSim64Parser.InstrENDIANContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrSAVE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSAVE(CPUSim64Parser.InstrSAVEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrRESTORE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRESTORE(CPUSim64Parser.InstrRESTOREContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#instrREADONLY}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrREADONLY(CPUSim64Parser.InstrREADONLYContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#operand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitOperand(CPUSim64Parser.OperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#rOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitROperand(CPUSim64Parser.ROperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#fOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFOperand(CPUSim64Parser.FOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#aOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAOperand(CPUSim64Parser.AOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#xOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitXOperand(CPUSim64Parser.XOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#yOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitYOperand(CPUSim64Parser.YOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#oOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitOOperand(CPUSim64Parser.OOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#pOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPOperand(CPUSim64Parser.POperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#qOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitQOperand(CPUSim64Parser.QOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#x1to4}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitX1to4(CPUSim64Parser.X1to4Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#y1to4}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitY1to4(CPUSim64Parser.Y1to4Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#bLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBLiteral(CPUSim64Parser.BLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#aLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitALiteral(CPUSim64Parser.ALiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#cLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCLiteral(CPUSim64Parser.CLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#kLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitKLiteral(CPUSim64Parser.KLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#eLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitELiteral(CPUSim64Parser.ELiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#pLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPLiteral(CPUSim64Parser.PLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#zPort}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitZPort(CPUSim64Parser.ZPortContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#zCond}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitZCond(CPUSim64Parser.ZCondContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64Parser#memRef}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemRef(CPUSim64Parser.MemRefContext ctx);
}