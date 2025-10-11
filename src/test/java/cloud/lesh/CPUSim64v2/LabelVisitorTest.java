package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;
import java.util.stream.Collectors;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class LabelVisitorTest {
	@Test
	void testPlain() {
		String src = """
			MOV R0, 42
			ADD R2, R0, R1
			""";
		String expected = """
			MOV R0, 42
			ADD R2, R0, R1
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		assertEquals(expected, noLabels);
		var labelMap = labelVisitor.getLabelMap();
		assertEquals(2, labelMap.size());
		assertEquals(2, labelMap.get("__CODE_END__"));
		assertEquals(2, labelMap.get("__HEAP_START__"));
		var errors = labelVisitor.getErrors();
		assertTrue(errors.isEmpty());
	}

	@Test
	void testSimple() {
		String src = """
			MOV R0, 42
			LABEL1: ADD R2, R0, R1
			SUB R1, R1, 1
			LABEL2:
				NOP
			STOP
			STOP
			""";
		String expected = """
			MOV R0, 42
			ADD R2, R0, R1
			SUB R1, R1, 1
			NOP
			STOP
			STOP
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		assertEquals(expected, noLabels);
		var labelMap = labelVisitor.getLabelMap();
		assertEquals(4, labelMap.size());
		assertEquals(1, labelMap.get("LABEL1"));
		assertEquals(3, labelMap.get("LABEL2"));
		assertEquals(6, labelMap.get("__CODE_END__"));
		assertEquals(6, labelMap.get("__HEAP_START__"));
		var errors = labelVisitor.getErrors();
		assertTrue(errors.isEmpty());
	}

	@Test
	void testData() {
		String src = """
			MOV R0, 42
			LABEL1: ADD R2, R0, R1
			
			SUB R1, R1, 1
			
			LABEL2:
				NOP
			DATA1: .DCI 123
			DATA2: .DCF 3.14
			DATA3: .DCS "Hello, World!"
			DATA4: .DCB 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'
			DATA5: .DCW 1, 2, 3, 4, 5
			DATA6: .DCW 1.2, 3.4, 5.6
			DATA7: .DCW 'x', 'y', 'z', '\\0'
			END:
			STOP
			STOP
			""";
		String expected = """
			MOV R0, 42
			ADD R2, R0, R1
			SUB R1, R1, 1
			NOP
			.DCI 123
			.DCF 3.14
			.DCS "Hello, World!"
			.DCB 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'
			.DCW 1, 2, 3, 4, 5
			.DCW 1.2, 3.4, 5.6
			.DCW 'x', 'y', 'z', '\\0'
			STOP
			STOP
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		assertEquals(expected, noLabels);
		var labelMap = labelVisitor.getLabelMap();
		assertEquals(12, labelMap.size());
		assertEquals(1, labelMap.get("LABEL1"));
		assertEquals(3, labelMap.get("LABEL2"));
		assertEquals(4, labelMap.get("DATA1"));
		assertEquals(5, labelMap.get("DATA2") );
		assertEquals(6, labelMap.get("DATA3"));
		assertEquals(9, labelMap.get("DATA4"));
		assertEquals(12, labelMap.get("DATA5"));
		assertEquals(18, labelMap.get("DATA6"));
		assertEquals(22, labelMap.get("DATA7"));
		assertEquals(27, labelMap.get("END"));
		assertEquals(29, labelMap.get("__CODE_END__"));
		assertEquals(29, labelMap.get("__HEAP_START__"));
		var errors = labelVisitor.getErrors();
		assertTrue(errors.isEmpty());
	}

	@Test
	void testOrg() {
		String src = """
			MOV R0, 42
			LABEL1: ADD R2, R0, R1
			SUB R1, R1, 1
			LABEL2: .ORG 100
				NOP
			LABEL3: SUB R3, R3, R4
			.ORG 200
			END:
			STOP
			STOP
			""";
		String expected = """
			MOV R0, 42
			ADD R2, R0, R1
			SUB R1, R1, 1
			.ORG 100
			NOP
			SUB R3, R3, R4
			.ORG 200
			STOP
			STOP
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		assertEquals(expected, noLabels);
		var labelMap = labelVisitor.getLabelMap();
		assertEquals(6, labelMap.size());
		assertEquals(1, labelMap.get("LABEL1"));
		assertEquals(3, labelMap.get("LABEL2"));
		assertEquals(101, labelMap.get("LABEL3"));
		assertEquals(200, labelMap.get("END"));
		assertEquals(202, labelMap.get("__CODE_END__"));
		assertEquals(202, labelMap.get("__HEAP_START__"));
		var errors = labelVisitor.getErrors();
		assertTrue(errors.isEmpty());
	}

	@Disabled
	@Test
	void testBlock() {
		String src = """
			MOV R0, 42
			$LABEL1: ADD R2, R0, R1
			SUB R1, R1, 1
			.BLOCK BLOCK1
			$LABEL2: NOP
			.BLOCK BLOCK2
			$LABEL3: SUB R3, R3, R4
			.BLOCK_END
			$LABEL4: NOP
			.BLOCK_END
			$LABEL5: NOP
			END:
			STOP
			STOP
			""";
		String expected = """
			MOV R0, 42
			ADD R2, R0, R1
			SUB R1, R1, 1
			.BLOCK BLOCK1
			NOP
			.BLOCK BLOCK2
			SUB R3, R3, R4
			.BLOCK_END
			NOP
			.BLOCK_END
			NOP
			STOP
			STOP
			""";
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(src);
		assertEquals(expected, noLabels);
		var labelMap = labelVisitor.getLabelMap();
		assertEquals(8, labelMap.size());
		assertEquals(1, labelMap.get("$LABEL1"));
		assertEquals(3, labelMap.get("BLOCK1$LABEL2"));
		assertEquals(4, labelMap.get("BLOCK1$BLOCK2$LABEL3"));
		assertEquals(5, labelMap.get("BLOCK1$LABEL4"));
		assertEquals(6, labelMap.get("$LABEL5"));
		assertEquals(7, labelMap.get("END"));
		assertEquals(9, labelMap.get("__CODE_END__"));
		assertEquals(9, labelMap.get("__HEAP_START__"));
		var errors = labelVisitor.getErrors();
		assertTrue(errors.isEmpty());
	}

	@Test
	void testLexerErrors() {
		String src = """
			// Comment			
			/* Comment
			Comment
			*/
			#define PI 3.1415926
				START:
				move	r2, 123456789012345
				buckwheat
				.DCW -326
				spanky
				.DCW 0x1234, 0x5678, 0x9ABC, 0xDEF0
				alfalfa
				.DCW 3.1415926, -2.7182818, 0.
				.DCB 'a', '\\n', 'é', '\\u1234'
			#CALL func(r1, r2, r3)
				MOVE	r2, 123
				darla
				FINIS:
				""";
		String errorsString = """
				Line «test.asm»:8:9 missing ':' at '\\n' (offending: \\n)
				Line «test.asm»:10:6 missing ':' at '\\n' (offending: \\n)
				Line «test.asm»:12:7 missing ':' at '\\n' (offending: \\n)
				Line «test.asm»:17:5 missing ':' at '\\n' (offending: \\n)
				""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("test.asm", src, loader);
		LiteralRewriter rw = new LiteralRewriter();
		preprocessed = rw.rewrite(preprocessed);
		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(preprocessed);
//		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
//		asm.assemble(noLabels);
//		List<Long> words = asm.result();
		List<String> errors = labelVisitor.getErrors();
		String actualErrors = errors.stream().collect(Collectors.joining(System.lineSeparator())) + System.lineSeparator();
		assertEquals(errorsString, actualErrors);
	}
}
