package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class AssemblerVisitorTest {
	@Test
	void testNOP() {
		String src = """
			START:
			NOP
			DEBUG R1
			DEBUG F2
			DEBUG PC
			DEBUG r1,r2,r3,r4
			DEBUG f1,f2,f3,f4
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(6, words.size());
		assertEquals(0x0000000000000000L, words.get(0));
		assertEquals(0x0080001000000000L, words.get(1));
		assertEquals(0x00C0002000000000L, words.get(2));
		assertEquals(0x008001F000000000L, words.get(3));
		assertEquals(0x00AA001002003004L, words.get(4));
		assertEquals(0x00FF001002003004L, words.get(5));
		assertEquals(6, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testCLEAR() {
		String src = """
			START:
			CLEAR
			CLEAR R1
			CLEAR F2
			CLEAR R28
			CLEAR F31
			CLEAR r1,r2,r3,r4
			CLEAR f1,f2,f3,f4
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(7, words.size());
		assertEquals(0x0100000000000000L, words.get(0));
		assertEquals(0x0180001000000000L, words.get(1));
		assertEquals(0x01C0002000000000L, words.get(2));
		assertEquals(0x018001C000000000L, words.get(3));
		assertEquals(0x01C001F000000000L, words.get(4));
		assertEquals(0x01AA001002003004L, words.get(5));
		assertEquals(0x01FF001002003004L, words.get(6));
		assertEquals(7, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testMOVE() {
		String src = """
			START:
			MOVE R1, R2
			MOVE F1, F2
			MOVE R1, 0x1234
			MOVE F1, 4660
			MOVE R1, R2, R3
			MOVE R1, R2, 0x1234
			MOVE R1, 0x1234, R2
			MOVE Z, R1, R2, R3
			MOVE NN, F1, F2, F3
			MOVE NZ, R1, 1, 2
			MOVE R2, 'a'
			MOVE R3, FINIS
			FINIS:
			""";
		LiteralRewriter rw = new LiteralRewriter();
		String preprocessed = rw.rewrite(src);
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(preprocessed);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(12, words.size());
		int i = 0;
		assertEquals(0x02A0001002000000L, words.get(i++));
		assertEquals(0x02F0001002000000L, words.get(i++));
		assertEquals(0x8280040000001234L, words.get(i++));
		assertEquals(0x82C0040000001234L, words.get(i++));
		assertEquals(0x02A8001002003000L, words.get(i++));
		assertEquals(0xC2A0010020001234L, words.get(i++));
		assertEquals(0xC2A0010020001234L, words.get(i++));
		assertEquals(0x026A001001002003L, words.get(i++));
		assertEquals(0x027F005001002003L, words.get(i++));
		assertEquals(0x0265002001001002L, words.get(i++));
		assertEquals(0x8280080000000061L, words.get(i++));
		assertEquals(0x82800C000000000CL, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testLOAD() {
		String src = """
			START:
			LOAD R1, 0x1234[]
			LOAD F1, 0x1234[]
			LOAD R1, R2[]
			LOAD F1, R2[]
			LOAD R1,R2[0x1234]
			LOAD F1,R2[0x1234]
			LOAD R1,0x1234[R2]
			LOAD F1,0x1234[R2]
			LOAD R1,0x1234[8]
			LOAD F1,0x1234[8]
			LOAD R1,R2[R3]
			LOAD F1,R2[R3]
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(12, words.size());
		int i = 0;
		assertEquals(0x8380040000001234L, words.get(i++));
		assertEquals(0x83C0040000001234L, words.get(i++));
		assertEquals(0x03A0001002000000L, words.get(i++));
		assertEquals(0x03E0001002000000L, words.get(i++));
		assertEquals(0xC3A0010020001234L, words.get(i++));
		assertEquals(0xC3E0010020001234L, words.get(i++));
		assertEquals(0xC3A0010020001234L, words.get(i++));
		assertEquals(0xC3E0010020001234L, words.get(i++));
		assertEquals(0xC390010080001234L, words.get(i++));
		assertEquals(0xC3D0010080001234L, words.get(i++));
		assertEquals(0x03A8001002003000L, words.get(i++));
		assertEquals(0x03E8001002003000L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testSTORE() {
		String src = """
			START:
			STORE R1, 0x1234[]
			STORE F1, 0x1234[]
			STORE 3, 0x1234[]
		
			STORE R1, R2[]
			STORE F1, R2[]
			STORE 3, R2[]

			STORE R1,R2[0x1234]
			STORE F1,R2[0x1234]
			STORE 3,R2[0x1234]
			
			STORE R1,0x1234[R2]
			STORE F1,0x1234[R2]
			STORE 3,0x1234[R2]
			
			STORE R1,0x1234[8]
			STORE F1,0x1234[8]
			STORE 3,0x1234[8]
			
			STORE R1,R2[R3]
			STORE F1,R2[R3]
			STORE 3,R2[R3]
 			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(18, words.size());
		int i = 0;
		assertEquals(0x8480040000001234L, words.get(i++));
		assertEquals(0x84C0040000001234L, words.get(i++));
		assertEquals(0x84400C0000001234L, words.get(i++));

		assertEquals(0x04A0001002000000L, words.get(i++));
		assertEquals(0x04E0001002000000L, words.get(i++));
		assertEquals(0x0460003002000000L, words.get(i++));

		assertEquals(0xC4A0010020001234L, words.get(i++));
		assertEquals(0xC4E0010020001234L, words.get(i++));
		assertEquals(0xC460030020001234L, words.get(i++));

		assertEquals(0xC4A0010020001234L, words.get(i++));
		assertEquals(0xC4E0010020001234L, words.get(i++));
		assertEquals(0xC460030020001234L, words.get(i++));

		assertEquals(0xC490010080001234L, words.get(i++));
		assertEquals(0xC4D0010080001234L, words.get(i++));
		assertEquals(0xC450030080001234L, words.get(i++));

		assertEquals(0x04A8001002003000L, words.get(i++));
		assertEquals(0x04E8001002003000L, words.get(i++));
		assertEquals(0x0468003002003000L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testPOP() {
		String src = """
			START:
			POP
			POP R1
			POP F2
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(3, words.size());
		assertEquals(0x0500000000000000L, words.get(0));
		assertEquals(0x0580001000000000L, words.get(1));
		assertEquals(0x05C0002000000000L, words.get(2));
		assertEquals(3, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}


	@Test
	void testPUSH() {
		String src = """
			START:
			PUSH R1
			PUSH F2
			PUSH 0x1234
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(3, words.size());
		assertEquals(0x0680001000000000L, words.get(0));
		assertEquals(0x06C0002000000000L, words.get(1));
		assertEquals(0x4600000000001234L, words.get(2));
		assertEquals(3, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testJUMP() {
		String src = """
			START:
			JUMP R1
			JUMP 0x1234
			JUMP NZ, R2
			JUMP NP, 0x1234
			JUMP NN, R1 + 0x1234
			JUMP Z, 0x1234 + R1
			JUMP N, 0x1234 + 3
			LOOP:
			JUMP P, R1 + R2
			.BLOCK B1
			$LOOP:
			JUMP $LOOP
			JUMP B1$LOOP
			JUMP LOOP
			.BLOCK_END
			JUMP START
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(12, words.size());
		int i = 0;
		assertEquals(0x0780001000000000L, words.get(i++));
		assertEquals(0x4700000000001234L, words.get(i++));
		assertEquals(0x0760002002000000L, words.get(i++));
		assertEquals(0x8740180000001234L, words.get(i++));
		assertEquals(0xC760050010001234L, words.get(i++));
		assertEquals(0xC760010010001234L, words.get(i++));
		assertEquals(0xC750030030001234L, words.get(i++));
		assertEquals(0x0768004001002000L, words.get(i++));
		assertEquals(0x4700000000000008L, words.get(i++));
		assertEquals(0x4700000000000008L, words.get(i++));
		assertEquals(0x4700000000000007L, words.get(i++));
		assertEquals(0x4700000000000000L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testCALL() {
		String src = """
			START:
			CALL R1
			CALL 0x1234
			CALL NZ, R2
			CALL NP, 0x1234
			CALL NN, R1 + 0x1234
			CALL Z, 0x1234 + R1
			CALL N, 0x1234 + 3
			CALL P, R1 + R2
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(8, words.size());
		int i = 0;
		assertEquals(0x0880001000000000L, words.get(i++));
		assertEquals(0x4800000000001234L, words.get(i++));
		assertEquals(0x0860002002000000L, words.get(i++));
		assertEquals(0x8840180000001234L, words.get(i++));
		assertEquals(0xC860050010001234L, words.get(i++));
		assertEquals(0xC860010010001234L, words.get(i++));
		assertEquals(0xC850030030001234L, words.get(i++));
		assertEquals(0x0868004001002000L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testRETURN() {
		String src = """
			START:
			RETURN
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(1, words.size());
		assertEquals(0x0900000000000000L, words.get(0));
		assertEquals(1, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testINTERRUPT() {
		String src = """
			START:
			INT R1
			INT 0xff
			INT 0x12345678
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(3, words.size());
		assertEquals(0x0A80001000000000L, words.get(0));
		assertEquals(0x0A400FF000000000L, words.get(1));
		assertEquals(0x4A00000012345678L, words.get(2));
		assertEquals(3, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testSTOP() {
		String src = """
			START:
			STOP
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(1, words.size());
		assertEquals(0x0B00000000000000L, words.get(0));
		assertEquals(1, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testNEG() {
		String src = """
			START:
			NEG R1
			NEG F2
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(2, words.size());
		assertEquals(0x0C80001000000000L, words.get(0));
		assertEquals(0x0CC0002000000000L, words.get(1));
		assertEquals(2, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testADD() {
		String src = """
			START:
			ADD R1,R2
			ADD F1,F2
			ADD R1,0x1234
			ADD F1,0x1234
			ADD R1,R2,R3
			ADD F1,F2,F3
			ADD R1,R2,0x1234
			ADD R1,0x1234,R2
			ADD F1,F2,0x1234
			ADD F1,0x1234,F2
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(10, words.size());
		int i = 0;
		assertEquals(0x0DA0001002000000L, words.get(i++));
		assertEquals(0x0DF0001002000000L, words.get(i++));
		assertEquals(0x8D80040000001234L, words.get(i++));
		assertEquals(0x8DC0040000001234L, words.get(i++));
		assertEquals(0x0DA8001002003000L, words.get(i++));
		assertEquals(0x0DFC001002003000L, words.get(i++));
		assertEquals(0xCDA0010020001234L, words.get(i++));
		assertEquals(0xCDA0010020001234L, words.get(i++));
		assertEquals(0xCDF0010020001234L, words.get(i++));
		assertEquals(0xCDF0010020001234L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testSUB() {
		String src = """
			START:
			SUB R1,R2
			SUB F1,F2
			SUB R1,0x1234
			SUB F1,0x1234
			SUB R1,R2,R3
			SUB F1,F2,F3
			SUB R1,R2,0x1234
			SUB R1,0x1234,R2
			SUB F1,F2,0x1234
			SUB F1,0x1234,F2
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(10, words.size());
		int i = 0;
		assertEquals(0x0EA0001002000000L, words.get(i++));
		assertEquals(0x0EF0001002000000L, words.get(i++));
		assertEquals(0x8E80040000001234L, words.get(i++));
		assertEquals(0x8EC0040000001234L, words.get(i++));
		assertEquals(0x0EA8001002003000L, words.get(i++));
		assertEquals(0x0EFC001002003000L, words.get(i++));
		assertEquals(0xCEA0010020001234L, words.get(i++));
		assertEquals(0xCEA0010020001234L, words.get(i++));
		assertEquals(0xCEF0010020001234L, words.get(i++));
		assertEquals(0xCEF0010020001234L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testMULT() {
		String src = """
			START:
			MULT R1,R2
			MULT F1,F2
			MULT R1,0x1234
			MULT F1,0x1234
			MULT R1,R2,R3
			MULT F1,F2,F3
			MULT R1,R2,0x1234
			MULT R1,0x1234,R2
			MULT F1,F2,0x1234
			MULT F1,0x1234,F2
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(10, words.size());
		int i = 0;
		assertEquals(0x0FA0001002000000L, words.get(i++));
		assertEquals(0x0FF0001002000000L, words.get(i++));
		assertEquals(0x8F80040000001234L, words.get(i++));
		assertEquals(0x8FC0040000001234L, words.get(i++));
		assertEquals(0x0FA8001002003000L, words.get(i++));
		assertEquals(0x0FFC001002003000L, words.get(i++));
		assertEquals(0xCFA0010020001234L, words.get(i++));
		assertEquals(0xCFA0010020001234L, words.get(i++));
		assertEquals(0xCFF0010020001234L, words.get(i++));
		assertEquals(0xCFF0010020001234L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testDIV() {
		String src = """
			START:
			DIV R1,R2
			DIV F1,F2
			DIV R1,0x1234
			DIV F1,0x1234
			DIV R1,R2,R3
			DIV F1,F2,F3
			DIV R1,R2,0x1234
			DIV R1,0x1234,R2
			DIV F1,F2,0x1234
			DIV F1,0x1234,F2
			DIV R1,R2,R3,R4
			DIV R1,R2,R3,4
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(12, words.size());
		int i = 0;
		assertEquals(0x10A0001002000000L, words.get(i++));
		assertEquals(0x10F0001002000000L, words.get(i++));
		assertEquals(0x9080040000001234L, words.get(i++));
		assertEquals(0x90C0040000001234L, words.get(i++));
		assertEquals(0x10A8001002003000L, words.get(i++));
		assertEquals(0x10FC001002003000L, words.get(i++));
		assertEquals(0xD0A0010020001234L, words.get(i++));
		assertEquals(0xD0A0010020001234L, words.get(i++));
		assertEquals(0xD0F0010020001234L, words.get(i++));
		assertEquals(0xD0F0010020001234L, words.get(i++));
		assertEquals(0x10AA001002003004L, words.get(i++));
		assertEquals(0x10A9001002003004L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testRECIP() {
		String src = """
			START:
			RECIP F1
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(1, words.size());
		assertEquals(0x10C0001000000000L, words.get(0));
		assertEquals(1, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testCOMPL() {
		String src = """
			START:
			COMPL R1
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(1, words.size());
		assertEquals(0x1180001000000000L, words.get(0));
		assertEquals(1, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testAND() {
		String src = """
			START:
			AND R1, R2
			AND R1, 3
			AND R1, R2, R3
			AND R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x12A0001002000000L, words.get(0));
		assertEquals(0x9280040000000003L, words.get(1));
		assertEquals(0x12A8001002003000L, words.get(2));
		assertEquals(0xD2A0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testOR() {
		String src = """
			START:
			OR R1, R2
			OR R1, 3
			OR R1, R2, R3
			OR R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x13A0001002000000L, words.get(0));
		assertEquals(0x9380040000000003L, words.get(1));
		assertEquals(0x13A8001002003000L, words.get(2));
		assertEquals(0xD3A0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testXOR() {
		String src = """
			START:
			XOR R1, R2
			XOR R1, 3
			XOR R1, R2, R3
			XOR R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x14A0001002000000L, words.get(0));
		assertEquals(0x9480040000000003L, words.get(1));
		assertEquals(0x14A8001002003000L, words.get(2));
		assertEquals(0xD4A0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testTEST() {
		String src = """
			START:
			TEST R1
			TEST F1
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(2, words.size());
		assertEquals(0x1580001000000000L, words.get(0));
		assertEquals(0x15C0001000000000L, words.get(1));
		assertEquals(2, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testCMP() {
		String src = """
			START:
			CMP R1,R2
			CMP F1,F2
			CMP R1,0x1234
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(3, words.size());
		assertEquals(0x16A0001002000000L, words.get(0));
		assertEquals(0x16F0001002000000L, words.get(1));
		assertEquals(0x9680040000001234L, words.get(2));
		assertEquals(3, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testLSHIFT() {
		String src = """
			START:
			LSHIFT R1, R2
			LSHIFT R1, 3
			LSHIFT R1, R2, R3
			LSHIFT R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x17A0001002000000L, words.get(0));
		assertEquals(0x9780040000000003L, words.get(1));
		assertEquals(0x17A8001002003000L, words.get(2));
		assertEquals(0xD7A0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testRSHIFT() {
		String src = """
			START:
			RSHIFT R1, R2
			RSHIFT R1, 3
			RSHIFT R1, R2, R3
			RSHIFT R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x18A0001002000000L, words.get(0));
		assertEquals(0x9880040000000003L, words.get(1));
		assertEquals(0x18A8001002003000L, words.get(2));
		assertEquals(0xD8A0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testARSHIFT() {
		String src = """
			START:
			ARSHIFT R1, R2
			ARSHIFT R1, 3
			ARSHIFT R1, R2, R3
			ARSHIFT R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x19A0001002000000L, words.get(0));
		assertEquals(0x9980040000000003L, words.get(1));
		assertEquals(0x19A8001002003000L, words.get(2));
		assertEquals(0xD9A0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testLROTATE() {
		String src = """
			START:
			LROTATE R1, R2
			LROTATE R1, 3
			LROTATE R1, R2, R3
			LROTATE R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x1AA0001002000000L, words.get(0));
		assertEquals(0x9A80040000000003L, words.get(1));
		assertEquals(0x1AA8001002003000L, words.get(2));
		assertEquals(0xDAA0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testRROTATE() {
		String src = """
			START:
			RROTATE R1, R2
			RROTATE R1, 3
			RROTATE R1, R2, R3
			RROTATE R1, R2, 3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x1BA0001002000000L, words.get(0));
		assertEquals(0x9B80040000000003L, words.get(1));
		assertEquals(0x1BA8001002003000L, words.get(2));
		assertEquals(0xDBA0010020000003L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testIN() {
		String src = """
			START:
			IN R1, R2, R3
			IN R1, 2, 3
			IN R1, R2, 3
			IN R1, 2, R3
			IN F1, R2, R3
			IN F1, 2, 3
			IN F1, R2, 3
			IN F1, 2, R3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(8, words.size());
		assertEquals(0x1CA8001002003000L, words.get(0));
		assertEquals(0x1C94001002003000L, words.get(1));
		assertEquals(0x1CA4001002003000L, words.get(2));
		assertEquals(0x1C98001002003000L, words.get(3));
		assertEquals(0x1CE8001002003000L, words.get(4));
		assertEquals(0x1CD4001002003000L, words.get(5));
		assertEquals(0x1CE4001002003000L, words.get(6));
		assertEquals(0x1CD8001002003000L, words.get(7));
		assertEquals(8, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testOUT() {
		String src = """
			START:
			OUT R1, R2, R3
			OUT R1, 2, 3
			OUT R1, R2, 3
			OUT R1, 2, R3
			OUT F1, R2, R3
			OUT F1, 2, 3
			OUT F1, R2, 3
			OUT F1, 2, R3
			OUT 1, R2, R3
			OUT 1, 2, 3
			OUT 1, R2, 3
			OUT 1, 2, R3
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(12, words.size());
		int i = 0;
		assertEquals(0x1DA8001002003000L, words.get(i++));
		assertEquals(0x1D94001002003000L, words.get(i++));
		assertEquals(0x1DA4001002003000L, words.get(i++));
		assertEquals(0x1D98001002003000L, words.get(i++));
		assertEquals(0x1DE8001002003000L, words.get(i++));
		assertEquals(0x1DD4001002003000L, words.get(i++));
		assertEquals(0x1DE4001002003000L, words.get(i++));
		assertEquals(0x1DD8001002003000L, words.get(i++));
		assertEquals(0x1D68001002003000L, words.get(i++));
		assertEquals(0x1D54001002003000L, words.get(i++));
		assertEquals(0x1D64001002003000L, words.get(i++));
		assertEquals(0x1D58001002003000L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testPACK() {
		String src = """
			START:
			PACK R1, R2
			PACK R1, R2, R3, R4
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(2, words.size());
		assertEquals(0x1EA0001002000000L, words.get(0));
		assertEquals(0x1EAA001002003004L, words.get(1));
		assertEquals(2, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testPACK64() {
		String src = """
			START:
			PACK64 R1, R2
			PACK64 R1, R2, R3, R4
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(2, words.size());
		assertEquals(0x1FA0001002000000L, words.get(0));
		assertEquals(0x1FAA001002003004L, words.get(1));
		assertEquals(2, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testUNPACK() {
		String src = """
			START:
			UNPACK R1, R2
			UNPACK R1, R2, R3, R4
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(2, words.size());
		assertEquals(0x20A0001002000000L, words.get(0));
		assertEquals(0x20AA001002003004L, words.get(1));
		assertEquals(2, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testUNPACK64() {
		String src = """
			START:
			UNPACK64 R1, R2
			UNPACK64 R1, R2, R3, R4
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(2, words.size());
		assertEquals(0x21A0001002000000L, words.get(0));
		assertEquals(0x21AA001002003004L, words.get(1));
		assertEquals(2, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testCAS() {
		String src = """
			START:
			CAS R1, R2, R3, R4
			CAS R1, R2, R3, 0x123
			CAS 1, 2, R3, R4
			CAS 1, 2, R3, 0x123
			CAS R1, 2, R3, R4
			CAS R1, 2, R3, 0x123
			CAS 1, R2, R3, R4
			CAS 1, R2, R3, 0x123
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(8, words.size());
		int i = 0;
		assertEquals(0x22AA001002003004L, words.get(i++));
		assertEquals(0x22A9001002003123L, words.get(i++));
		assertEquals(0x225A001002003004L, words.get(i++));
		assertEquals(0x2259001002003123L, words.get(i++));
		assertEquals(0x229A001002003004L, words.get(i++));
		assertEquals(0x2299001002003123L, words.get(i++));
		assertEquals(0x226A001002003004L, words.get(i++));
		assertEquals(0x2269001002003123L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testENDIAN() {
		String src = """
			START:
			ENDIAN R1, R2
			ENDIAN R1, 2
			ENDIAN 1, R2
			ENDIAN 1, 2
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x23A0001002000000L, words.get(0));
		assertEquals(0x2390001002000000L, words.get(1));
		assertEquals(0x2360001002000000L, words.get(2));
		assertEquals(0x2350001002000000L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testSAVE_RESTORE() {
		String src = """
			START:
			SAVE R10
			SAVE R20,R22
			RESTORE F10
			RESTORE F20,F22
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x24A000A01C000000L, words.get(0));
		assertEquals(0x24A0014016000000L, words.get(1));
		assertEquals(0x25F000A01F000000L, words.get(2));
		assertEquals(0x25F0014016000000L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testDCI() {
		String src = """
			START:
			.DCI 0x1234567890ABCDEF
			.DCI -1
			.DCI -0x123
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(3, words.size());
		assertEquals(0x1234567890ABCDEFL, words.get(0));
		assertEquals(0xFFFFFFFFFFFFFFFFL, words.get(1));
		assertEquals(0xFFFFFFFFFFFFFEDDL, words.get(2));
		assertEquals(3, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testDCF() {
		String src = """
			START:
			.DCF 3.1415926
			.DCF -2.7182818
			.DCF 0.0
			.DCF 1.0
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(4, words.size());
		assertEquals(0x400921FB4D12D84AL, words.get(0));
		assertEquals(0xC005BF0A87427F01L, words.get(1));
		assertEquals(0x0000000000000000L, words.get(2));
		assertEquals(0x3FF0000000000000L, words.get(3));
		assertEquals(4, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testDCA() {
		String src = """
			START:
			.DCA 5
			.DCA 10
			.DCA 100
			.DCA 1
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(120, words.size());
		int i = 0;
		assertEquals(5, words.get(i));
		i += 6;
		assertEquals(10, words.get(i));
		i += 11;
		assertEquals(100, words.get(i));
		i += 101;
		assertEquals(1, words.get(i));
	}

	@Test
	void testDCB() {
		String src = """
			START:
			.DCB 1, 2, 3, 4
			.DCB 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8
			.DCB 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
			.DCB -1, -2, -3, 4, 5, 6, 7, 8, 9, '\\n', 'a', 'b', 'c', 'd', 'é', '\\u1234'
			FINIS:
			""";
		LiteralRewriter rw = new LiteralRewriter();
		String preprocessed = rw.rewrite(src);
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(preprocessed);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(10, words.size());
		int i = 0;
		assertEquals(4, words.get(i++));
		assertEquals(0x0102030400000000L, words.get(i++));
		assertEquals(8, words.get(i++));
		assertEquals(0x0102030405060708L, words.get(i++));
		assertEquals(12, words.get(i++));
		assertEquals(0x0102030405060708L, words.get(i++));
		assertEquals(0x090A0B0C00000000L, words.get(i++));
		assertEquals(16, words.get(i++));
		assertEquals(0xFFFEFD0405060708L, words.get(i++));
		assertEquals(0x090A61626364E934L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testDCW() {
		String src = """
			START:
			.DCW -326
			.DCW 0x1234, 0x5678, 0x9ABC, 0xDEF0
			.DCW 3.1415926, -2.7182818, 0.
			.DCW 'a', '\\n', 'é', '\\u1234'
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(16, words.size());
		int i = 0;
		assertEquals(1, words.get(i++));
		assertEquals(-326, words.get(i++));
		assertEquals(4, words.get(i++));
		assertEquals(0x1234, words.get(i++));
		assertEquals(0x5678, words.get(i++));
		assertEquals(0x9ABC, words.get(i++));
		assertEquals(0xDEF0, words.get(i++));
		assertEquals(3, words.get(i++));
		assertEquals(0x400921FB4D12D84AL, words.get(i++));
		assertEquals(0xC005BF0A87427F01L, words.get(i++));
		assertEquals(0, words.get(i++));
		assertEquals(4, words.get(i++));
	}

	@Test
	void testDCS() {
		String src = """
			START:
			.DCS ""
			.DCS "Hello, World!"
			.DCS "\\'\\"\\\\\\n\\b\\u1234"
			FINIS:
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(6, words.size());
		int i = 0;
		assertEquals(0x0000000000000000L, words.get(i++));
		assertEquals(13, words.get(i++));
		assertEquals(0x48656C6C6F2C2057L, words.get(i++));
		assertEquals(0x6F726C6421000000L, words.get(i++));
		assertEquals(8, words.get(i++));
		assertEquals(0x27225C0A08E188B4L, words.get(i++));
		assertEquals(i, labelVisitor.getLabelMap().get("FINIS") - labelVisitor.getLabelMap().get("START"));
	}

	@Test
	void testBlock() {
		String src = """
			START:
			#DEF_MACRO block(x)
			.block block_label_{}
				move	r0, 10
			$LABEL:
				sub		r0, 1
				cmp		r0, ${x}
				jump	z, $LABEL
			.block_end
			#END_MACRO
			#MACRO block(4)
			#MACRO block(8)
			#MACRO block(2)
			FINIS:
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(preprocessed);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> words = asm.result();
		assertEquals(12, words.size());
		assertEquals(0x8740040000000001L, words.get(3));
		assertEquals(0x8740040000000005L, words.get(7));
		assertEquals(0x8740040000000009L, words.get(11));
		assertEquals(1, labelVisitor.getLabelMap().get("BLOCK_LABEL_1$LABEL"));
		assertEquals(5, labelVisitor.getLabelMap().get("BLOCK_LABEL_2$LABEL"));
		assertEquals(9, labelVisitor.getLabelMap().get("BLOCK_LABEL_3$LABEL"));
	}
}
