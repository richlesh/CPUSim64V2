package cloud.lesh.CPUSim64v2;

import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.Test;

import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;

public class LiteralRewriterTest {
	@Test
	void testRewrite() {
		String src = """
			LOAD R3, "Hello, World!"
			MOV R0, 42
			.LINE "hello.asm", 1
			LOAD F1, 3.14
			LOAD F2, 3.
			LOAD F3, .14
			LOAD F4, -2.71e2
			ADD R2, R0, R1
			LOAD R4, "Hølá, World!"
			LOAD R5, "\\u007a, \\b\\n\\t\\r\\f\\\\\\""
			LOAD R6, "This is not a float 3.14"
			LOAD F5, 1.6E-19
			LOAD F6, 3.14
			LOAD R7, "Hello, World!"
			LOAD R8, 'r'
			LOAD R9, '\\u1234'
			STOP
			STOP
			STR_A: .DCS "This is a string"
			FP_A: .DCF 326.5
			INT_A: .DCI 123456
			CHAR_A: .DCB 'c'
			CHAR_B: .DCB '©'
			CHAR_C: .DCB '\\n', '\t'
			ARRAY: .DCW 1, 2, 3, 4, 5
			ARRAY2: .DCC '©', 'a', '\\b', '\\0'
			ARRAY3: .DCW 1.1, 1.2, 1.3, 1.4
			""";
		String expected = """
			LOAD R3, __STR_1
			MOV R0, 42
			.LINE "hello.asm", 1
			LOAD F1, __FP_1
			LOAD F2, __FP_2
			LOAD F3, __FP_3
			LOAD F4, __FP_4
			ADD R2, R0, R1
			LOAD R4, __STR_2
			LOAD R5, __STR_3
			LOAD R6, __STR_4
			LOAD F5, __FP_5
			LOAD F6, __FP_1
			LOAD R7, __STR_1
			LOAD R8, 0x72
			LOAD R9, 0x1234
			STOP
			STOP
			STR_A: .DCS "This is a string"
			FP_A: .DCF 326.5
			INT_A: .DCI 123456
			CHAR_A: .DCB 0x63
			CHAR_B: .DCB 0xA9
			CHAR_C: .DCB 0xA, 0x9
			ARRAY: .DCW 1, 2, 3, 4, 5
			ARRAY2: .DCC 0xA9, 0x61, 0x8, 0x0
			ARRAY3: .DCW 1.1, 1.2, 1.3, 1.4
		
			__FP_1: .DCF 3.14
			__FP_2: .DCF 3.
			__FP_3: .DCF .14
			__FP_4: .DCF -2.71e2
			__FP_5: .DCF 1.6E-19
			__STR_1: .DCS "Hello, World!"
			__STR_2: .DCS "Hølá, World!"
			__STR_3: .DCS "\\u007a, \\b\\n\\t\\r\\f\\\\\\""
			__STR_4: .DCS "This is not a float 3.14"
			""";
		LiteralRewriter rewriter = new LiteralRewriter();
		String rewritten = rewriter.rewrite(src);
		assertEquals(expected, rewritten);
	}
}
