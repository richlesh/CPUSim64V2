// Generated from CPUSim64v2.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64v2;
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link CPUSim64v2Parser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface CPUSim64v2Visitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#program}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitProgram(CPUSim64v2Parser.ProgramContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#line}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLine(CPUSim64v2Parser.LineContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#labelDef}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLabelDef(CPUSim64v2Parser.LabelDefContext ctx);
	/**
	 * Visit a parse tree produced by the {@code Data_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitData_Directive(CPUSim64v2Parser.Data_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code ORG_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitORG_Directive(CPUSim64v2Parser.ORG_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code LINE_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLINE_Directive(CPUSim64v2Parser.LINE_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code LINE_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLINE_BEGIN_Directive(CPUSim64v2Parser.LINE_BEGIN_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code LINE_END_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLINE_END_Directive(CPUSim64v2Parser.LINE_END_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code BLOCK_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBLOCK_BEGIN_Directive(CPUSim64v2Parser.BLOCK_BEGIN_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by the {@code BLOCK_END_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBLOCK_END_Directive(CPUSim64v2Parser.BLOCK_END_DirectiveContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#dataDirective}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitDataDirective(CPUSim64v2Parser.DataDirectiveContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#intList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIntList(CPUSim64v2Parser.IntListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#floatList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFloatList(CPUSim64v2Parser.FloatListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#charList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCharList(CPUSim64v2Parser.CharListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#byteList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitByteList(CPUSim64v2Parser.ByteListContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instruction}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstruction(CPUSim64v2Parser.InstructionContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrNOP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrNOP(CPUSim64v2Parser.InstrNOPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrDEBUG}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrDEBUG(CPUSim64v2Parser.InstrDEBUGContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrCLEAR}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCLEAR(CPUSim64v2Parser.InstrCLEARContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrMOVE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrMOVE(CPUSim64v2Parser.InstrMOVEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrLOAD}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrLOAD(CPUSim64v2Parser.InstrLOADContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrSTORE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSTORE(CPUSim64v2Parser.InstrSTOREContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrPOP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPOP(CPUSim64v2Parser.InstrPOPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrPUSH}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPUSH(CPUSim64v2Parser.InstrPUSHContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#branchModes}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBranchModes(CPUSim64v2Parser.BranchModesContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrJUMP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrJUMP(CPUSim64v2Parser.InstrJUMPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrCALL}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCALL(CPUSim64v2Parser.InstrCALLContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrRETURN}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRETURN(CPUSim64v2Parser.InstrRETURNContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrINTERRUPT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrINTERRUPT(CPUSim64v2Parser.InstrINTERRUPTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrSTOP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSTOP(CPUSim64v2Parser.InstrSTOPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrNEG}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrNEG(CPUSim64v2Parser.InstrNEGContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#arithmeticModes}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitArithmeticModes(CPUSim64v2Parser.ArithmeticModesContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrADD}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrADD(CPUSim64v2Parser.InstrADDContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrSUB}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSUB(CPUSim64v2Parser.InstrSUBContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrMULT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrMULT(CPUSim64v2Parser.InstrMULTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrDIV}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrDIV(CPUSim64v2Parser.InstrDIVContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrRECIP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRECIP(CPUSim64v2Parser.InstrRECIPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrCOMPL}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCOMPL(CPUSim64v2Parser.InstrCOMPLContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#logicModes}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLogicModes(CPUSim64v2Parser.LogicModesContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrAND}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrAND(CPUSim64v2Parser.InstrANDContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrOR}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrOR(CPUSim64v2Parser.InstrORContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrXOR}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrXOR(CPUSim64v2Parser.InstrXORContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrTEST}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrTEST(CPUSim64v2Parser.InstrTESTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrCMP}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCMP(CPUSim64v2Parser.InstrCMPContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrLSHIFT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrLSHIFT(CPUSim64v2Parser.InstrLSHIFTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrRSHIFT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRSHIFT(CPUSim64v2Parser.InstrRSHIFTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrARSHIFT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrARSHIFT(CPUSim64v2Parser.InstrARSHIFTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrLROTATE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrLROTATE(CPUSim64v2Parser.InstrLROTATEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrRROTATE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRROTATE(CPUSim64v2Parser.InstrRROTATEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrIN}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrIN(CPUSim64v2Parser.InstrINContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrOUT}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrOUT(CPUSim64v2Parser.InstrOUTContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrPACK}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPACK(CPUSim64v2Parser.InstrPACKContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrPACK64}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrPACK64(CPUSim64v2Parser.InstrPACK64Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrUNPACK}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrUNPACK(CPUSim64v2Parser.InstrUNPACKContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrUNPACK64}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrUNPACK64(CPUSim64v2Parser.InstrUNPACK64Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrCAS}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrCAS(CPUSim64v2Parser.InstrCASContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrENDIAN}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrENDIAN(CPUSim64v2Parser.InstrENDIANContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrSAVE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrSAVE(CPUSim64v2Parser.InstrSAVEContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#instrRESTORE}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInstrRESTORE(CPUSim64v2Parser.InstrRESTOREContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#operand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitOperand(CPUSim64v2Parser.OperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#rOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitROperand(CPUSim64v2Parser.ROperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#fOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFOperand(CPUSim64v2Parser.FOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#aOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAOperand(CPUSim64v2Parser.AOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#xOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitXOperand(CPUSim64v2Parser.XOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#yOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitYOperand(CPUSim64v2Parser.YOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#oOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitOOperand(CPUSim64v2Parser.OOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#qOperand}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitQOperand(CPUSim64v2Parser.QOperandContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#x1to4}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitX1to4(CPUSim64v2Parser.X1to4Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#y1to4}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitY1to4(CPUSim64v2Parser.Y1to4Context ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#bLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBLiteral(CPUSim64v2Parser.BLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#aLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitALiteral(CPUSim64v2Parser.ALiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#cLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCLiteral(CPUSim64v2Parser.CLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#kLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitKLiteral(CPUSim64v2Parser.KLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#eLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitELiteral(CPUSim64v2Parser.ELiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#pLiteral}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitPLiteral(CPUSim64v2Parser.PLiteralContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#zPort}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitZPort(CPUSim64v2Parser.ZPortContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#zCond}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitZCond(CPUSim64v2Parser.ZCondContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#memRef}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemRef(CPUSim64v2Parser.MemRefContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#memC}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemC(CPUSim64v2Parser.MemCContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#memA}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemA(CPUSim64v2Parser.MemAContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#memAplusC}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemAplusC(CPUSim64v2Parser.MemAplusCContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#memCplusA}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemCplusA(CPUSim64v2Parser.MemCplusAContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#memCplusC}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemCplusC(CPUSim64v2Parser.MemCplusCContext ctx);
	/**
	 * Visit a parse tree produced by {@link CPUSim64v2Parser#memAplusR}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMemAplusR(CPUSim64v2Parser.MemAplusRContext ctx);
}