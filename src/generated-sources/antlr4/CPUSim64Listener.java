// Generated from CPUSim64.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link CPUSim64Parser}.
 */
public interface CPUSim64Listener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#program}.
	 * @param ctx the parse tree
	 */
	void enterProgram(CPUSim64Parser.ProgramContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#program}.
	 * @param ctx the parse tree
	 */
	void exitProgram(CPUSim64Parser.ProgramContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#line}.
	 * @param ctx the parse tree
	 */
	void enterLine(CPUSim64Parser.LineContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#line}.
	 * @param ctx the parse tree
	 */
	void exitLine(CPUSim64Parser.LineContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#labelDef}.
	 * @param ctx the parse tree
	 */
	void enterLabelDef(CPUSim64Parser.LabelDefContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#labelDef}.
	 * @param ctx the parse tree
	 */
	void exitLabelDef(CPUSim64Parser.LabelDefContext ctx);
	/**
	 * Enter a parse tree produced by the {@code Data_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterData_Directive(CPUSim64Parser.Data_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code Data_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitData_Directive(CPUSim64Parser.Data_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code ORG_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterORG_Directive(CPUSim64Parser.ORG_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code ORG_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitORG_Directive(CPUSim64Parser.ORG_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code LINE_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterLINE_Directive(CPUSim64Parser.LINE_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code LINE_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitLINE_Directive(CPUSim64Parser.LINE_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code LINE_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterLINE_BEGIN_Directive(CPUSim64Parser.LINE_BEGIN_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code LINE_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitLINE_BEGIN_Directive(CPUSim64Parser.LINE_BEGIN_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code LINE_END_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterLINE_END_Directive(CPUSim64Parser.LINE_END_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code LINE_END_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitLINE_END_Directive(CPUSim64Parser.LINE_END_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code BLOCK_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterBLOCK_BEGIN_Directive(CPUSim64Parser.BLOCK_BEGIN_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code BLOCK_BEGIN_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitBLOCK_BEGIN_Directive(CPUSim64Parser.BLOCK_BEGIN_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by the {@code BLOCK_END_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void enterBLOCK_END_Directive(CPUSim64Parser.BLOCK_END_DirectiveContext ctx);
	/**
	 * Exit a parse tree produced by the {@code BLOCK_END_Directive}
	 * labeled alternative in {@link CPUSim64Parser#directive}.
	 * @param ctx the parse tree
	 */
	void exitBLOCK_END_Directive(CPUSim64Parser.BLOCK_END_DirectiveContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#dataDirective}.
	 * @param ctx the parse tree
	 */
	void enterDataDirective(CPUSim64Parser.DataDirectiveContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#dataDirective}.
	 * @param ctx the parse tree
	 */
	void exitDataDirective(CPUSim64Parser.DataDirectiveContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#intList}.
	 * @param ctx the parse tree
	 */
	void enterIntList(CPUSim64Parser.IntListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#intList}.
	 * @param ctx the parse tree
	 */
	void exitIntList(CPUSim64Parser.IntListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#floatList}.
	 * @param ctx the parse tree
	 */
	void enterFloatList(CPUSim64Parser.FloatListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#floatList}.
	 * @param ctx the parse tree
	 */
	void exitFloatList(CPUSim64Parser.FloatListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#charList}.
	 * @param ctx the parse tree
	 */
	void enterCharList(CPUSim64Parser.CharListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#charList}.
	 * @param ctx the parse tree
	 */
	void exitCharList(CPUSim64Parser.CharListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#byteList}.
	 * @param ctx the parse tree
	 */
	void enterByteList(CPUSim64Parser.ByteListContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#byteList}.
	 * @param ctx the parse tree
	 */
	void exitByteList(CPUSim64Parser.ByteListContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instruction}.
	 * @param ctx the parse tree
	 */
	void enterInstruction(CPUSim64Parser.InstructionContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instruction}.
	 * @param ctx the parse tree
	 */
	void exitInstruction(CPUSim64Parser.InstructionContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrNOP}.
	 * @param ctx the parse tree
	 */
	void enterInstrNOP(CPUSim64Parser.InstrNOPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrNOP}.
	 * @param ctx the parse tree
	 */
	void exitInstrNOP(CPUSim64Parser.InstrNOPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrDEBUG}.
	 * @param ctx the parse tree
	 */
	void enterInstrDEBUG(CPUSim64Parser.InstrDEBUGContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrDEBUG}.
	 * @param ctx the parse tree
	 */
	void exitInstrDEBUG(CPUSim64Parser.InstrDEBUGContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrCLEAR}.
	 * @param ctx the parse tree
	 */
	void enterInstrCLEAR(CPUSim64Parser.InstrCLEARContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrCLEAR}.
	 * @param ctx the parse tree
	 */
	void exitInstrCLEAR(CPUSim64Parser.InstrCLEARContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrMOVE}.
	 * @param ctx the parse tree
	 */
	void enterInstrMOVE(CPUSim64Parser.InstrMOVEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrMOVE}.
	 * @param ctx the parse tree
	 */
	void exitInstrMOVE(CPUSim64Parser.InstrMOVEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrLOAD}.
	 * @param ctx the parse tree
	 */
	void enterInstrLOAD(CPUSim64Parser.InstrLOADContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrLOAD}.
	 * @param ctx the parse tree
	 */
	void exitInstrLOAD(CPUSim64Parser.InstrLOADContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrSTORE}.
	 * @param ctx the parse tree
	 */
	void enterInstrSTORE(CPUSim64Parser.InstrSTOREContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrSTORE}.
	 * @param ctx the parse tree
	 */
	void exitInstrSTORE(CPUSim64Parser.InstrSTOREContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrPOP}.
	 * @param ctx the parse tree
	 */
	void enterInstrPOP(CPUSim64Parser.InstrPOPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrPOP}.
	 * @param ctx the parse tree
	 */
	void exitInstrPOP(CPUSim64Parser.InstrPOPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrPUSH}.
	 * @param ctx the parse tree
	 */
	void enterInstrPUSH(CPUSim64Parser.InstrPUSHContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrPUSH}.
	 * @param ctx the parse tree
	 */
	void exitInstrPUSH(CPUSim64Parser.InstrPUSHContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#branchModes}.
	 * @param ctx the parse tree
	 */
	void enterBranchModes(CPUSim64Parser.BranchModesContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#branchModes}.
	 * @param ctx the parse tree
	 */
	void exitBranchModes(CPUSim64Parser.BranchModesContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrJUMP}.
	 * @param ctx the parse tree
	 */
	void enterInstrJUMP(CPUSim64Parser.InstrJUMPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrJUMP}.
	 * @param ctx the parse tree
	 */
	void exitInstrJUMP(CPUSim64Parser.InstrJUMPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrCALL}.
	 * @param ctx the parse tree
	 */
	void enterInstrCALL(CPUSim64Parser.InstrCALLContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrCALL}.
	 * @param ctx the parse tree
	 */
	void exitInstrCALL(CPUSim64Parser.InstrCALLContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrRETURN}.
	 * @param ctx the parse tree
	 */
	void enterInstrRETURN(CPUSim64Parser.InstrRETURNContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrRETURN}.
	 * @param ctx the parse tree
	 */
	void exitInstrRETURN(CPUSim64Parser.InstrRETURNContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrINTERRUPT}.
	 * @param ctx the parse tree
	 */
	void enterInstrINTERRUPT(CPUSim64Parser.InstrINTERRUPTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrINTERRUPT}.
	 * @param ctx the parse tree
	 */
	void exitInstrINTERRUPT(CPUSim64Parser.InstrINTERRUPTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrSTOP}.
	 * @param ctx the parse tree
	 */
	void enterInstrSTOP(CPUSim64Parser.InstrSTOPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrSTOP}.
	 * @param ctx the parse tree
	 */
	void exitInstrSTOP(CPUSim64Parser.InstrSTOPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrNEG}.
	 * @param ctx the parse tree
	 */
	void enterInstrNEG(CPUSim64Parser.InstrNEGContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrNEG}.
	 * @param ctx the parse tree
	 */
	void exitInstrNEG(CPUSim64Parser.InstrNEGContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#arithmeticModes}.
	 * @param ctx the parse tree
	 */
	void enterArithmeticModes(CPUSim64Parser.ArithmeticModesContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#arithmeticModes}.
	 * @param ctx the parse tree
	 */
	void exitArithmeticModes(CPUSim64Parser.ArithmeticModesContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrADD}.
	 * @param ctx the parse tree
	 */
	void enterInstrADD(CPUSim64Parser.InstrADDContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrADD}.
	 * @param ctx the parse tree
	 */
	void exitInstrADD(CPUSim64Parser.InstrADDContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrSUB}.
	 * @param ctx the parse tree
	 */
	void enterInstrSUB(CPUSim64Parser.InstrSUBContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrSUB}.
	 * @param ctx the parse tree
	 */
	void exitInstrSUB(CPUSim64Parser.InstrSUBContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrMULT}.
	 * @param ctx the parse tree
	 */
	void enterInstrMULT(CPUSim64Parser.InstrMULTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrMULT}.
	 * @param ctx the parse tree
	 */
	void exitInstrMULT(CPUSim64Parser.InstrMULTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrDIV}.
	 * @param ctx the parse tree
	 */
	void enterInstrDIV(CPUSim64Parser.InstrDIVContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrDIV}.
	 * @param ctx the parse tree
	 */
	void exitInstrDIV(CPUSim64Parser.InstrDIVContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrRECIP}.
	 * @param ctx the parse tree
	 */
	void enterInstrRECIP(CPUSim64Parser.InstrRECIPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrRECIP}.
	 * @param ctx the parse tree
	 */
	void exitInstrRECIP(CPUSim64Parser.InstrRECIPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrCOMPL}.
	 * @param ctx the parse tree
	 */
	void enterInstrCOMPL(CPUSim64Parser.InstrCOMPLContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrCOMPL}.
	 * @param ctx the parse tree
	 */
	void exitInstrCOMPL(CPUSim64Parser.InstrCOMPLContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#logicModes}.
	 * @param ctx the parse tree
	 */
	void enterLogicModes(CPUSim64Parser.LogicModesContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#logicModes}.
	 * @param ctx the parse tree
	 */
	void exitLogicModes(CPUSim64Parser.LogicModesContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrAND}.
	 * @param ctx the parse tree
	 */
	void enterInstrAND(CPUSim64Parser.InstrANDContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrAND}.
	 * @param ctx the parse tree
	 */
	void exitInstrAND(CPUSim64Parser.InstrANDContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrOR}.
	 * @param ctx the parse tree
	 */
	void enterInstrOR(CPUSim64Parser.InstrORContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrOR}.
	 * @param ctx the parse tree
	 */
	void exitInstrOR(CPUSim64Parser.InstrORContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrXOR}.
	 * @param ctx the parse tree
	 */
	void enterInstrXOR(CPUSim64Parser.InstrXORContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrXOR}.
	 * @param ctx the parse tree
	 */
	void exitInstrXOR(CPUSim64Parser.InstrXORContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrTEST}.
	 * @param ctx the parse tree
	 */
	void enterInstrTEST(CPUSim64Parser.InstrTESTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrTEST}.
	 * @param ctx the parse tree
	 */
	void exitInstrTEST(CPUSim64Parser.InstrTESTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrCMP}.
	 * @param ctx the parse tree
	 */
	void enterInstrCMP(CPUSim64Parser.InstrCMPContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrCMP}.
	 * @param ctx the parse tree
	 */
	void exitInstrCMP(CPUSim64Parser.InstrCMPContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrLSHIFT}.
	 * @param ctx the parse tree
	 */
	void enterInstrLSHIFT(CPUSim64Parser.InstrLSHIFTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrLSHIFT}.
	 * @param ctx the parse tree
	 */
	void exitInstrLSHIFT(CPUSim64Parser.InstrLSHIFTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrRSHIFT}.
	 * @param ctx the parse tree
	 */
	void enterInstrRSHIFT(CPUSim64Parser.InstrRSHIFTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrRSHIFT}.
	 * @param ctx the parse tree
	 */
	void exitInstrRSHIFT(CPUSim64Parser.InstrRSHIFTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrARSHIFT}.
	 * @param ctx the parse tree
	 */
	void enterInstrARSHIFT(CPUSim64Parser.InstrARSHIFTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrARSHIFT}.
	 * @param ctx the parse tree
	 */
	void exitInstrARSHIFT(CPUSim64Parser.InstrARSHIFTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrLROTATE}.
	 * @param ctx the parse tree
	 */
	void enterInstrLROTATE(CPUSim64Parser.InstrLROTATEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrLROTATE}.
	 * @param ctx the parse tree
	 */
	void exitInstrLROTATE(CPUSim64Parser.InstrLROTATEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrRROTATE}.
	 * @param ctx the parse tree
	 */
	void enterInstrRROTATE(CPUSim64Parser.InstrRROTATEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrRROTATE}.
	 * @param ctx the parse tree
	 */
	void exitInstrRROTATE(CPUSim64Parser.InstrRROTATEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrIN}.
	 * @param ctx the parse tree
	 */
	void enterInstrIN(CPUSim64Parser.InstrINContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrIN}.
	 * @param ctx the parse tree
	 */
	void exitInstrIN(CPUSim64Parser.InstrINContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrOUT}.
	 * @param ctx the parse tree
	 */
	void enterInstrOUT(CPUSim64Parser.InstrOUTContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrOUT}.
	 * @param ctx the parse tree
	 */
	void exitInstrOUT(CPUSim64Parser.InstrOUTContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrPACK}.
	 * @param ctx the parse tree
	 */
	void enterInstrPACK(CPUSim64Parser.InstrPACKContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrPACK}.
	 * @param ctx the parse tree
	 */
	void exitInstrPACK(CPUSim64Parser.InstrPACKContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrPACK64}.
	 * @param ctx the parse tree
	 */
	void enterInstrPACK64(CPUSim64Parser.InstrPACK64Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrPACK64}.
	 * @param ctx the parse tree
	 */
	void exitInstrPACK64(CPUSim64Parser.InstrPACK64Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrUNPACK}.
	 * @param ctx the parse tree
	 */
	void enterInstrUNPACK(CPUSim64Parser.InstrUNPACKContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrUNPACK}.
	 * @param ctx the parse tree
	 */
	void exitInstrUNPACK(CPUSim64Parser.InstrUNPACKContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrUNPACK64}.
	 * @param ctx the parse tree
	 */
	void enterInstrUNPACK64(CPUSim64Parser.InstrUNPACK64Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrUNPACK64}.
	 * @param ctx the parse tree
	 */
	void exitInstrUNPACK64(CPUSim64Parser.InstrUNPACK64Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrCAS}.
	 * @param ctx the parse tree
	 */
	void enterInstrCAS(CPUSim64Parser.InstrCASContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrCAS}.
	 * @param ctx the parse tree
	 */
	void exitInstrCAS(CPUSim64Parser.InstrCASContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrENDIAN}.
	 * @param ctx the parse tree
	 */
	void enterInstrENDIAN(CPUSim64Parser.InstrENDIANContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrENDIAN}.
	 * @param ctx the parse tree
	 */
	void exitInstrENDIAN(CPUSim64Parser.InstrENDIANContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrSAVE}.
	 * @param ctx the parse tree
	 */
	void enterInstrSAVE(CPUSim64Parser.InstrSAVEContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrSAVE}.
	 * @param ctx the parse tree
	 */
	void exitInstrSAVE(CPUSim64Parser.InstrSAVEContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrRESTORE}.
	 * @param ctx the parse tree
	 */
	void enterInstrRESTORE(CPUSim64Parser.InstrRESTOREContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrRESTORE}.
	 * @param ctx the parse tree
	 */
	void exitInstrRESTORE(CPUSim64Parser.InstrRESTOREContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#instrREADONLY}.
	 * @param ctx the parse tree
	 */
	void enterInstrREADONLY(CPUSim64Parser.InstrREADONLYContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#instrREADONLY}.
	 * @param ctx the parse tree
	 */
	void exitInstrREADONLY(CPUSim64Parser.InstrREADONLYContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#operand}.
	 * @param ctx the parse tree
	 */
	void enterOperand(CPUSim64Parser.OperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#operand}.
	 * @param ctx the parse tree
	 */
	void exitOperand(CPUSim64Parser.OperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#rOperand}.
	 * @param ctx the parse tree
	 */
	void enterROperand(CPUSim64Parser.ROperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#rOperand}.
	 * @param ctx the parse tree
	 */
	void exitROperand(CPUSim64Parser.ROperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#fOperand}.
	 * @param ctx the parse tree
	 */
	void enterFOperand(CPUSim64Parser.FOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#fOperand}.
	 * @param ctx the parse tree
	 */
	void exitFOperand(CPUSim64Parser.FOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#aOperand}.
	 * @param ctx the parse tree
	 */
	void enterAOperand(CPUSim64Parser.AOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#aOperand}.
	 * @param ctx the parse tree
	 */
	void exitAOperand(CPUSim64Parser.AOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#xOperand}.
	 * @param ctx the parse tree
	 */
	void enterXOperand(CPUSim64Parser.XOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#xOperand}.
	 * @param ctx the parse tree
	 */
	void exitXOperand(CPUSim64Parser.XOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#yOperand}.
	 * @param ctx the parse tree
	 */
	void enterYOperand(CPUSim64Parser.YOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#yOperand}.
	 * @param ctx the parse tree
	 */
	void exitYOperand(CPUSim64Parser.YOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#oOperand}.
	 * @param ctx the parse tree
	 */
	void enterOOperand(CPUSim64Parser.OOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#oOperand}.
	 * @param ctx the parse tree
	 */
	void exitOOperand(CPUSim64Parser.OOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#pOperand}.
	 * @param ctx the parse tree
	 */
	void enterPOperand(CPUSim64Parser.POperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#pOperand}.
	 * @param ctx the parse tree
	 */
	void exitPOperand(CPUSim64Parser.POperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#qOperand}.
	 * @param ctx the parse tree
	 */
	void enterQOperand(CPUSim64Parser.QOperandContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#qOperand}.
	 * @param ctx the parse tree
	 */
	void exitQOperand(CPUSim64Parser.QOperandContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#x1to4}.
	 * @param ctx the parse tree
	 */
	void enterX1to4(CPUSim64Parser.X1to4Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#x1to4}.
	 * @param ctx the parse tree
	 */
	void exitX1to4(CPUSim64Parser.X1to4Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#y1to4}.
	 * @param ctx the parse tree
	 */
	void enterY1to4(CPUSim64Parser.Y1to4Context ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#y1to4}.
	 * @param ctx the parse tree
	 */
	void exitY1to4(CPUSim64Parser.Y1to4Context ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#bLiteral}.
	 * @param ctx the parse tree
	 */
	void enterBLiteral(CPUSim64Parser.BLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#bLiteral}.
	 * @param ctx the parse tree
	 */
	void exitBLiteral(CPUSim64Parser.BLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#aLiteral}.
	 * @param ctx the parse tree
	 */
	void enterALiteral(CPUSim64Parser.ALiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#aLiteral}.
	 * @param ctx the parse tree
	 */
	void exitALiteral(CPUSim64Parser.ALiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#cLiteral}.
	 * @param ctx the parse tree
	 */
	void enterCLiteral(CPUSim64Parser.CLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#cLiteral}.
	 * @param ctx the parse tree
	 */
	void exitCLiteral(CPUSim64Parser.CLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#kLiteral}.
	 * @param ctx the parse tree
	 */
	void enterKLiteral(CPUSim64Parser.KLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#kLiteral}.
	 * @param ctx the parse tree
	 */
	void exitKLiteral(CPUSim64Parser.KLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#eLiteral}.
	 * @param ctx the parse tree
	 */
	void enterELiteral(CPUSim64Parser.ELiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#eLiteral}.
	 * @param ctx the parse tree
	 */
	void exitELiteral(CPUSim64Parser.ELiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#pLiteral}.
	 * @param ctx the parse tree
	 */
	void enterPLiteral(CPUSim64Parser.PLiteralContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#pLiteral}.
	 * @param ctx the parse tree
	 */
	void exitPLiteral(CPUSim64Parser.PLiteralContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#zPort}.
	 * @param ctx the parse tree
	 */
	void enterZPort(CPUSim64Parser.ZPortContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#zPort}.
	 * @param ctx the parse tree
	 */
	void exitZPort(CPUSim64Parser.ZPortContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#zCond}.
	 * @param ctx the parse tree
	 */
	void enterZCond(CPUSim64Parser.ZCondContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#zCond}.
	 * @param ctx the parse tree
	 */
	void exitZCond(CPUSim64Parser.ZCondContext ctx);
	/**
	 * Enter a parse tree produced by {@link CPUSim64Parser#memRef}.
	 * @param ctx the parse tree
	 */
	void enterMemRef(CPUSim64Parser.MemRefContext ctx);
	/**
	 * Exit a parse tree produced by {@link CPUSim64Parser#memRef}.
	 * @param ctx the parse tree
	 */
	void exitMemRef(CPUSim64Parser.MemRefContext ctx);
}