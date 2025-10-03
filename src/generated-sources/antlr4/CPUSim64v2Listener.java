// Generated from CPUSim64v2.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64v2;
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link CPUSim64v2Parser}.
 */
public interface CPUSim64v2Listener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#program}.
	 * @param ctx the parse tree
	 */
	void enterProgram(CPUSim64v2Parser.ProgramContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#program}.
	 * @param ctx the parse tree
	 */
	void exitProgram(CPUSim64v2Parser.ProgramContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#line}.
	 * @param ctx the parse tree
	 */
	void enterLine(CPUSim64v2Parser.LineContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#line}.
	 * @param ctx the parse tree
	 */
	void exitLine(CPUSim64v2Parser.LineContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#labelDef}.
	 * @param ctx the parse tree
	 */
	void enterLabelDef(CPUSim64v2Parser.LabelDefContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#labelDef}.
	 * @param ctx the parse tree
	 */
	void exitLabelDef(CPUSim64v2Parser.LabelDefContext ctx);
	/**
	 * Enter a parse tree produced by the {@code Data_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterData_Directive(CPUSim64v2Parser.Data_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code Data_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitData_Directive(CPUSim64v2Parser.Data_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code ORG_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterORG_Directive(CPUSim64v2Parser.ORG_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code ORG_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitORG_Directive(CPUSim64v2Parser.ORG_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code LINE_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterLINE_Directive(CPUSim64v2Parser.LINE_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code LINE_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitLINE_Directive(CPUSim64v2Parser.LINE_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code LINE_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterLINE_BEGIN_Directive(CPUSim64v2Parser.LINE_BEGIN_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code LINE_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitLINE_BEGIN_Directive(CPUSim64v2Parser.LINE_BEGIN_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code LINE_END_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterLINE_END_Directive(CPUSim64v2Parser.LINE_END_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code LINE_END_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitLINE_END_Directive(CPUSim64v2Parser.LINE_END_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code BLOCK_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterBLOCK_BEGIN_Directive(CPUSim64v2Parser.BLOCK_BEGIN_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code BLOCK_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitBLOCK_BEGIN_Directive(CPUSim64v2Parser.BLOCK_BEGIN_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code BLOCK_END_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterBLOCK_END_Directive(CPUSim64v2Parser.BLOCK_END_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code BLOCK_END_Directive}
	 * labeled alternative in {@link CPUSim64v2Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitBLOCK_END_Directive(CPUSim64v2Parser.BLOCK_END_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#dataDirective}.
	 * @param ctx the parse tree
	 */
	void enterDataDirective(CPUSim64v2Parser.DataDirectiveContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#dataDirective}.
	 * @param ctx the parse tree
	 */
	void exitDataDirective(CPUSim64v2Parser.DataDirectiveContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#intList}.
	 * @param ctx the parse tree
	 */
	void enterIntList(CPUSim64v2Parser.IntListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#intList}.
	 * @param ctx the parse tree
	 */
	void exitIntList(CPUSim64v2Parser.IntListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#floatList}.
	 * @param ctx the parse tree
	 */
	void enterFloatList(CPUSim64v2Parser.FloatListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#floatList}.
	 * @param ctx the parse tree
	 */
	void exitFloatList(CPUSim64v2Parser.FloatListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#charList}.
	 * @param ctx the parse tree
	 */
	void enterCharList(CPUSim64v2Parser.CharListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#charList}.
	 * @param ctx the parse tree
	 */
	void exitCharList(CPUSim64v2Parser.CharListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#byteList}.
	 * @param ctx the parse tree
	 */
	void enterByteList(CPUSim64v2Parser.ByteListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#byteList}.
	 * @param ctx the parse tree
	 */
	void exitByteList(CPUSim64v2Parser.ByteListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instruction}.
	 * @param ctx the parse tree
	 */
	void enterInstruction(CPUSim64v2Parser.InstructionContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instruction}.
	 * @param ctx the parse tree
	 */
	void exitInstruction(CPUSim64v2Parser.InstructionContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrNOP}.
	 * @param ctx the parse tree
	 */
	void enterInstrNOP(CPUSim64v2Parser.InstrNOPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrNOP}.
	 * @param ctx the parse tree
	 */
	void exitInstrNOP(CPUSim64v2Parser.InstrNOPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrDEBUG}.
	 * @param ctx the parse tree
	 */
	void enterInstrDEBUG(CPUSim64v2Parser.InstrDEBUGContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrDEBUG}.
	 * @param ctx the parse tree
	 */
	void exitInstrDEBUG(CPUSim64v2Parser.InstrDEBUGContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrCLEAR}.
	 * @param ctx the parse tree
	 */
	void enterInstrCLEAR(CPUSim64v2Parser.InstrCLEARContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrCLEAR}.
	 * @param ctx the parse tree
	 */
	void exitInstrCLEAR(CPUSim64v2Parser.InstrCLEARContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrMOVE}.
	 * @param ctx the parse tree
	 */
	void enterInstrMOVE(CPUSim64v2Parser.InstrMOVEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrMOVE}.
	 * @param ctx the parse tree
	 */
	void exitInstrMOVE(CPUSim64v2Parser.InstrMOVEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrLOAD}.
	 * @param ctx the parse tree
	 */
	void enterInstrLOAD(CPUSim64v2Parser.InstrLOADContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrLOAD}.
	 * @param ctx the parse tree
	 */
	void exitInstrLOAD(CPUSim64v2Parser.InstrLOADContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrSTORE}.
	 * @param ctx the parse tree
	 */
	void enterInstrSTORE(CPUSim64v2Parser.InstrSTOREContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrSTORE}.
	 * @param ctx the parse tree
	 */
	void exitInstrSTORE(CPUSim64v2Parser.InstrSTOREContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrPOP}.
	 * @param ctx the parse tree
	 */
	void enterInstrPOP(CPUSim64v2Parser.InstrPOPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrPOP}.
	 * @param ctx the parse tree
	 */
	void exitInstrPOP(CPUSim64v2Parser.InstrPOPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrPUSH}.
	 * @param ctx the parse tree
	 */
	void enterInstrPUSH(CPUSim64v2Parser.InstrPUSHContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrPUSH}.
	 * @param ctx the parse tree
	 */
	void exitInstrPUSH(CPUSim64v2Parser.InstrPUSHContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#branchModes}.
	 * @param ctx the parse tree
	 */
	void enterBranchModes(CPUSim64v2Parser.BranchModesContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#branchModes}.
	 * @param ctx the parse tree
	 */
	void exitBranchModes(CPUSim64v2Parser.BranchModesContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrJUMP}.
	 * @param ctx the parse tree
	 */
	void enterInstrJUMP(CPUSim64v2Parser.InstrJUMPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrJUMP}.
	 * @param ctx the parse tree
	 */
	void exitInstrJUMP(CPUSim64v2Parser.InstrJUMPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrCALL}.
	 * @param ctx the parse tree
	 */
	void enterInstrCALL(CPUSim64v2Parser.InstrCALLContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrCALL}.
	 * @param ctx the parse tree
	 */
	void exitInstrCALL(CPUSim64v2Parser.InstrCALLContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrRETURN}.
	 * @param ctx the parse tree
	 */
	void enterInstrRETURN(CPUSim64v2Parser.InstrRETURNContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrRETURN}.
	 * @param ctx the parse tree
	 */
	void exitInstrRETURN(CPUSim64v2Parser.InstrRETURNContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrINTERRUPT}.
	 * @param ctx the parse tree
	 */
	void enterInstrINTERRUPT(CPUSim64v2Parser.InstrINTERRUPTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrINTERRUPT}.
	 * @param ctx the parse tree
	 */
	void exitInstrINTERRUPT(CPUSim64v2Parser.InstrINTERRUPTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrSTOP}.
	 * @param ctx the parse tree
	 */
	void enterInstrSTOP(CPUSim64v2Parser.InstrSTOPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrSTOP}.
	 * @param ctx the parse tree
	 */
	void exitInstrSTOP(CPUSim64v2Parser.InstrSTOPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrNEG}.
	 * @param ctx the parse tree
	 */
	void enterInstrNEG(CPUSim64v2Parser.InstrNEGContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrNEG}.
	 * @param ctx the parse tree
	 */
	void exitInstrNEG(CPUSim64v2Parser.InstrNEGContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#arithmeticModes}.
	 * @param ctx the parse tree
	 */
	void enterArithmeticModes(CPUSim64v2Parser.ArithmeticModesContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#arithmeticModes}.
	 * @param ctx the parse tree
	 */
	void exitArithmeticModes(CPUSim64v2Parser.ArithmeticModesContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrADD}.
	 * @param ctx the parse tree
	 */
	void enterInstrADD(CPUSim64v2Parser.InstrADDContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrADD}.
	 * @param ctx the parse tree
	 */
	void exitInstrADD(CPUSim64v2Parser.InstrADDContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrSUB}.
	 * @param ctx the parse tree
	 */
	void enterInstrSUB(CPUSim64v2Parser.InstrSUBContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrSUB}.
	 * @param ctx the parse tree
	 */
	void exitInstrSUB(CPUSim64v2Parser.InstrSUBContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrMULT}.
	 * @param ctx the parse tree
	 */
	void enterInstrMULT(CPUSim64v2Parser.InstrMULTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrMULT}.
	 * @param ctx the parse tree
	 */
	void exitInstrMULT(CPUSim64v2Parser.InstrMULTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrDIV}.
	 * @param ctx the parse tree
	 */
	void enterInstrDIV(CPUSim64v2Parser.InstrDIVContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrDIV}.
	 * @param ctx the parse tree
	 */
	void exitInstrDIV(CPUSim64v2Parser.InstrDIVContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrRECIP}.
	 * @param ctx the parse tree
	 */
	void enterInstrRECIP(CPUSim64v2Parser.InstrRECIPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrRECIP}.
	 * @param ctx the parse tree
	 */
	void exitInstrRECIP(CPUSim64v2Parser.InstrRECIPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrCOMPL}.
	 * @param ctx the parse tree
	 */
	void enterInstrCOMPL(CPUSim64v2Parser.InstrCOMPLContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrCOMPL}.
	 * @param ctx the parse tree
	 */
	void exitInstrCOMPL(CPUSim64v2Parser.InstrCOMPLContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#logicModes}.
	 * @param ctx the parse tree
	 */
	void enterLogicModes(CPUSim64v2Parser.LogicModesContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#logicModes}.
	 * @param ctx the parse tree
	 */
	void exitLogicModes(CPUSim64v2Parser.LogicModesContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrAND}.
	 * @param ctx the parse tree
	 */
	void enterInstrAND(CPUSim64v2Parser.InstrANDContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrAND}.
	 * @param ctx the parse tree
	 */
	void exitInstrAND(CPUSim64v2Parser.InstrANDContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrOR}.
	 * @param ctx the parse tree
	 */
	void enterInstrOR(CPUSim64v2Parser.InstrORContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrOR}.
	 * @param ctx the parse tree
	 */
	void exitInstrOR(CPUSim64v2Parser.InstrORContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrXOR}.
	 * @param ctx the parse tree
	 */
	void enterInstrXOR(CPUSim64v2Parser.InstrXORContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrXOR}.
	 * @param ctx the parse tree
	 */
	void exitInstrXOR(CPUSim64v2Parser.InstrXORContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrTEST}.
	 * @param ctx the parse tree
	 */
	void enterInstrTEST(CPUSim64v2Parser.InstrTESTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrTEST}.
	 * @param ctx the parse tree
	 */
	void exitInstrTEST(CPUSim64v2Parser.InstrTESTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrCMP}.
	 * @param ctx the parse tree
	 */
	void enterInstrCMP(CPUSim64v2Parser.InstrCMPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrCMP}.
	 * @param ctx the parse tree
	 */
	void exitInstrCMP(CPUSim64v2Parser.InstrCMPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrLSHIFT}.
	 * @param ctx the parse tree
	 */
	void enterInstrLSHIFT(CPUSim64v2Parser.InstrLSHIFTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrLSHIFT}.
	 * @param ctx the parse tree
	 */
	void exitInstrLSHIFT(CPUSim64v2Parser.InstrLSHIFTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrRSHIFT}.
	 * @param ctx the parse tree
	 */
	void enterInstrRSHIFT(CPUSim64v2Parser.InstrRSHIFTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrRSHIFT}.
	 * @param ctx the parse tree
	 */
	void exitInstrRSHIFT(CPUSim64v2Parser.InstrRSHIFTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrARSHIFT}.
	 * @param ctx the parse tree
	 */
	void enterInstrARSHIFT(CPUSim64v2Parser.InstrARSHIFTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrARSHIFT}.
	 * @param ctx the parse tree
	 */
	void exitInstrARSHIFT(CPUSim64v2Parser.InstrARSHIFTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrLROTATE}.
	 * @param ctx the parse tree
	 */
	void enterInstrLROTATE(CPUSim64v2Parser.InstrLROTATEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrLROTATE}.
	 * @param ctx the parse tree
	 */
	void exitInstrLROTATE(CPUSim64v2Parser.InstrLROTATEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrRROTATE}.
	 * @param ctx the parse tree
	 */
	void enterInstrRROTATE(CPUSim64v2Parser.InstrRROTATEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrRROTATE}.
	 * @param ctx the parse tree
	 */
	void exitInstrRROTATE(CPUSim64v2Parser.InstrRROTATEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrIN}.
	 * @param ctx the parse tree
	 */
	void enterInstrIN(CPUSim64v2Parser.InstrINContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrIN}.
	 * @param ctx the parse tree
	 */
	void exitInstrIN(CPUSim64v2Parser.InstrINContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrOUT}.
	 * @param ctx the parse tree
	 */
	void enterInstrOUT(CPUSim64v2Parser.InstrOUTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrOUT}.
	 * @param ctx the parse tree
	 */
	void exitInstrOUT(CPUSim64v2Parser.InstrOUTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrPACK}.
	 * @param ctx the parse tree
	 */
	void enterInstrPACK(CPUSim64v2Parser.InstrPACKContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrPACK}.
	 * @param ctx the parse tree
	 */
	void exitInstrPACK(CPUSim64v2Parser.InstrPACKContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrPACK64}.
	 * @param ctx the parse tree
	 */
	void enterInstrPACK64(CPUSim64v2Parser.InstrPACK64Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrPACK64}.
	 * @param ctx the parse tree
	 */
	void exitInstrPACK64(CPUSim64v2Parser.InstrPACK64Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrUNPACK}.
	 * @param ctx the parse tree
	 */
	void enterInstrUNPACK(CPUSim64v2Parser.InstrUNPACKContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrUNPACK}.
	 * @param ctx the parse tree
	 */
	void exitInstrUNPACK(CPUSim64v2Parser.InstrUNPACKContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrUNPACK64}.
	 * @param ctx the parse tree
	 */
	void enterInstrUNPACK64(CPUSim64v2Parser.InstrUNPACK64Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrUNPACK64}.
	 * @param ctx the parse tree
	 */
	void exitInstrUNPACK64(CPUSim64v2Parser.InstrUNPACK64Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrCAS}.
	 * @param ctx the parse tree
	 */
	void enterInstrCAS(CPUSim64v2Parser.InstrCASContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrCAS}.
	 * @param ctx the parse tree
	 */
	void exitInstrCAS(CPUSim64v2Parser.InstrCASContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrENDIAN}.
	 * @param ctx the parse tree
	 */
	void enterInstrENDIAN(CPUSim64v2Parser.InstrENDIANContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrENDIAN}.
	 * @param ctx the parse tree
	 */
	void exitInstrENDIAN(CPUSim64v2Parser.InstrENDIANContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrSAVE}.
	 * @param ctx the parse tree
	 */
	void enterInstrSAVE(CPUSim64v2Parser.InstrSAVEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrSAVE}.
	 * @param ctx the parse tree
	 */
	void exitInstrSAVE(CPUSim64v2Parser.InstrSAVEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#instrRESTORE}.
	 * @param ctx the parse tree
	 */
	void enterInstrRESTORE(CPUSim64v2Parser.InstrRESTOREContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#instrRESTORE}.
	 * @param ctx the parse tree
	 */
	void exitInstrRESTORE(CPUSim64v2Parser.InstrRESTOREContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#operand}.
	 * @param ctx the parse tree
	 */
	void enterOperand(CPUSim64v2Parser.OperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#operand}.
	 * @param ctx the parse tree
	 */
	void exitOperand(CPUSim64v2Parser.OperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#rOperand}.
	 * @param ctx the parse tree
	 */
	void enterROperand(CPUSim64v2Parser.ROperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#rOperand}.
	 * @param ctx the parse tree
	 */
	void exitROperand(CPUSim64v2Parser.ROperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#fOperand}.
	 * @param ctx the parse tree
	 */
	void enterFOperand(CPUSim64v2Parser.FOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#fOperand}.
	 * @param ctx the parse tree
	 */
	void exitFOperand(CPUSim64v2Parser.FOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#aOperand}.
	 * @param ctx the parse tree
	 */
	void enterAOperand(CPUSim64v2Parser.AOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#aOperand}.
	 * @param ctx the parse tree
	 */
	void exitAOperand(CPUSim64v2Parser.AOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#xOperand}.
	 * @param ctx the parse tree
	 */
	void enterXOperand(CPUSim64v2Parser.XOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#xOperand}.
	 * @param ctx the parse tree
	 */
	void exitXOperand(CPUSim64v2Parser.XOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#yOperand}.
	 * @param ctx the parse tree
	 */
	void enterYOperand(CPUSim64v2Parser.YOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#yOperand}.
	 * @param ctx the parse tree
	 */
	void exitYOperand(CPUSim64v2Parser.YOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#oOperand}.
	 * @param ctx the parse tree
	 */
	void enterOOperand(CPUSim64v2Parser.OOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#oOperand}.
	 * @param ctx the parse tree
	 */
	void exitOOperand(CPUSim64v2Parser.OOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#qOperand}.
	 * @param ctx the parse tree
	 */
	void enterQOperand(CPUSim64v2Parser.QOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#qOperand}.
	 * @param ctx the parse tree
	 */
	void exitQOperand(CPUSim64v2Parser.QOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#x1to4}.
	 * @param ctx the parse tree
	 */
	void enterX1to4(CPUSim64v2Parser.X1to4Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#x1to4}.
	 * @param ctx the parse tree
	 */
	void exitX1to4(CPUSim64v2Parser.X1to4Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#y1to4}.
	 * @param ctx the parse tree
	 */
	void enterY1to4(CPUSim64v2Parser.Y1to4Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#y1to4}.
	 * @param ctx the parse tree
	 */
	void exitY1to4(CPUSim64v2Parser.Y1to4Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#bLiteral}.
	 * @param ctx the parse tree
	 */
	void enterBLiteral(CPUSim64v2Parser.BLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#bLiteral}.
	 * @param ctx the parse tree
	 */
	void exitBLiteral(CPUSim64v2Parser.BLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#aLiteral}.
	 * @param ctx the parse tree
	 */
	void enterALiteral(CPUSim64v2Parser.ALiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#aLiteral}.
	 * @param ctx the parse tree
	 */
	void exitALiteral(CPUSim64v2Parser.ALiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#cLiteral}.
	 * @param ctx the parse tree
	 */
	void enterCLiteral(CPUSim64v2Parser.CLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#cLiteral}.
	 * @param ctx the parse tree
	 */
	void exitCLiteral(CPUSim64v2Parser.CLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#kLiteral}.
	 * @param ctx the parse tree
	 */
	void enterKLiteral(CPUSim64v2Parser.KLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#kLiteral}.
	 * @param ctx the parse tree
	 */
	void exitKLiteral(CPUSim64v2Parser.KLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#eLiteral}.
	 * @param ctx the parse tree
	 */
	void enterELiteral(CPUSim64v2Parser.ELiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#eLiteral}.
	 * @param ctx the parse tree
	 */
	void exitELiteral(CPUSim64v2Parser.ELiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#pLiteral}.
	 * @param ctx the parse tree
	 */
	void enterPLiteral(CPUSim64v2Parser.PLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#pLiteral}.
	 * @param ctx the parse tree
	 */
	void exitPLiteral(CPUSim64v2Parser.PLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#zPort}.
	 * @param ctx the parse tree
	 */
	void enterZPort(CPUSim64v2Parser.ZPortContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#zPort}.
	 * @param ctx the parse tree
	 */
	void exitZPort(CPUSim64v2Parser.ZPortContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#zCond}.
	 * @param ctx the parse tree
	 */
	void enterZCond(CPUSim64v2Parser.ZCondContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#zCond}.
	 * @param ctx the parse tree
	 */
	void exitZCond(CPUSim64v2Parser.ZCondContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#memRef}.
	 * @param ctx the parse tree
	 */
	void enterMemRef(CPUSim64v2Parser.MemRefContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#memRef}.
	 * @param ctx the parse tree
	 */
	void exitMemRef(CPUSim64v2Parser.MemRefContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#memC}.
	 * @param ctx the parse tree
	 */
	void enterMemC(CPUSim64v2Parser.MemCContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#memC}.
	 * @param ctx the parse tree
	 */
	void exitMemC(CPUSim64v2Parser.MemCContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#memA}.
	 * @param ctx the parse tree
	 */
	void enterMemA(CPUSim64v2Parser.MemAContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#memA}.
	 * @param ctx the parse tree
	 */
	void exitMemA(CPUSim64v2Parser.MemAContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#memAplusC}.
	 * @param ctx the parse tree
	 */
	void enterMemAplusC(CPUSim64v2Parser.MemAplusCContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#memAplusC}.
	 * @param ctx the parse tree
	 */
	void exitMemAplusC(CPUSim64v2Parser.MemAplusCContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#memCplusA}.
	 * @param ctx the parse tree
	 */
	void enterMemCplusA(CPUSim64v2Parser.MemCplusAContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#memCplusA}.
	 * @param ctx the parse tree
	 */
	void exitMemCplusA(CPUSim64v2Parser.MemCplusAContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#memCplusC}.
	 * @param ctx the parse tree
	 */
	void enterMemCplusC(CPUSim64v2Parser.MemCplusCContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#memCplusC}.
	 * @param ctx the parse tree
	 */
	void exitMemCplusC(CPUSim64v2Parser.MemCplusCContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64v2Parser#memAplusR}.
	 * @param ctx the parse tree
	 */
	void enterMemAplusR(CPUSim64v2Parser.MemAplusRContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64v2Parser#memAplusR}.
	 * @param ctx the parse tree
	 */
	void exitMemAplusR(CPUSim64v2Parser.MemAplusRContext ctx);
}