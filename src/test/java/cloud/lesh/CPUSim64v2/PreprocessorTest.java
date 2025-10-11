package cloud.lesh.CPUSim64v2;

import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.Test;

class PreprocessorTest {
	@Test
	void testIncludeLocal() {
		String src = """
			#include "src/test/resources/include_1.asm"
			#include "src/test/resources/include_2.asm"
			""";
		String expected = """
			.LINE «include_1.asm», 2
			MOV R0, 1
			MOV R1, 2
			.LINE «include_2.asm», 2
			ADD R0, R0, R1
			SUB R1, R1, 1
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIncludeSystem() {
		String src = """
			#include <include_1.asm>
			#include <include_2.asm>
			""";
		String expected = """
			.LINE «include_1.asm», 2
			MOV R0, 1
			MOV R1, 2
			.LINE «include_2.asm», 2
			ADD R0, R0, R1
			SUB R1, R1, 1
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIncludeBad() {
		String src = """
			#include "src/test/resources/include_bad.asm"
			""";
		String expected = """
			.LINE «TEST.ASM», 1
			MOV R0, 1
			MOV R1, 2
			ADD R0, R0, R1
			SUB R1, R1, 1
			""";
		var loader = new IncludeLoader(Path.of("."));
		assertThrows(PreprocessorVisitor.PreprocessorException.class, () -> PreprocessorVisitor.preprocessText("Test.asm", src, loader));
	}

	@Test
	void testIncludeBad2() {
		String src = """
			#include 'src/test/resources/include_bad.asm'
			""";
		var loader = new IncludeLoader(Path.of("."));
		assertThrows(PreprocessorVisitor.PreprocessorException.class, () -> PreprocessorVisitor.preprocessText("Test.asm", src, loader));
	}

	@Test
	void testIncludeNested() {
		String src = """
			#include <include_3.asm>
			""";
		String expected = """
			.LINE «include_1.asm», 2
			MOV R0, 1
			MOV R1, 2
			.LINE «include_2.asm», 2
			ADD R0, R0, R1
			SUB R1, R1, 1
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testDefine() {
		String src = """
			#define USE_FP
			#define INT1 326
			#define INT2 0x7f
			#define ADDR1 0X1000
			#define PI 3.14159
			#define E10 2.20264657948067e4
			#define CHAR1 'A'
			#define STR1 "Hello, World!"
			MOV R0, USE_FP
			MOV R0, INT1
			MOV R1, INT2
			MOV R2, ADDR1
			MOV F0, PI
			MOV F1, E10
			MOV R2, CHAR1
			MOV R3, STR1
			""";
		String expected = """
			.LINE «Test.asm», 9
			MOV R0, 1
			MOV R0, 326
			MOV R1, 0x7f
			MOV R2, 0X1000
			MOV F0, 3.14159
			MOV F1, 2.20264657948067e4
			MOV R2, 'A'
			MOV R3, "Hello, World!"
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIf() {
		String src = """
			#define USE_FP
			#if USE_FP
			MOV F0, 3.14159
			#endif
			#undef USE_FP
			#if USE_FP
			MOV F1, 3.14159
			#endif
			""";
		String expected = """
			.LINE «Test.asm», 3
			MOV F0, 3.14159
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIfElse() {
		String src = """
			#define USE_FP
			#if USE_FP
			MOV F0, 3.14159
			#else
			MOV F0, 3.14
			#endif
			#undef USE_FP
			#if USE_FP
			MOV F0, 3.14159
			#else
			MOV F0, 3.14
			#endif
			""";
		String expected = """
			.LINE «Test.asm», 3
			MOV F0, 3.14159
			.LINE «Test.asm», 11
			MOV F0, 3.14
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIfElseIf() {
		String src = """
			#define SIZE 1
			#if SIZE == 1
			MOV F0, 3
			#elseif SIZE == 2
			MOV F0, 3.1
			#elseif SIZE == 3
			MOV F0, 3.14
			#endif
			#define SIZE 2
			#if SIZE == 1
			MOV F0, 3
			#elseif SIZE == 2
			MOV F0, 3.1
			#elseif SIZE == 3
			MOV F0, 3.14
			#endif
			#define SIZE 3
			#if SIZE == 1
			MOV F0, 3
			#elseif SIZE == 2
			MOV F0, 3.1
			#elseif SIZE == 3
			MOV F0, 3.14
			#endif
			#undef SIZE
			#if SIZE == 1
			MOV F0, 3
			#elseif SIZE == 2
			MOV F0, 3.1
			#elseif SIZE == 3
			MOV F0, 3.14
			#endif
			""";
		String expected = """
			.LINE «Test.asm», 3
			MOV F0, 3
			.LINE «Test.asm», 13
			MOV F0, 3.1
			.LINE «Test.asm», 23
			MOV F0, 3.14
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testReturn() {
		String src = """
			#return INT1	// This is a comment
			#freturn 3.14	// This is a comment
			""";
		String expected = """
			.LINE «TEST.ASM», 1
			MOVE R0, INT1
			MOVE F0, 3.14
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testCall() {
		String src = """
			#call bubblegum()
			#call myFunc(1, 2, 3)
			""";
		String expected = """
			.LINE «TEST.ASM», 1
			CALL BUBBLEGUM
			.LINE_BEGIN «TEST.ASM», 2
			PUSH 3
			PUSH 2
			PUSH 1
			CALL MYFUNC
			ADD SP, 3
			.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testDefFunc() {
		String src = """
			$BEGIN:
			MOVE R0, 42
			#def_func myFunc(a, b, c)
			#svar d, e
			#var f, g, h
			#fvar i, j
			$LOOP:
			LOAD r1, a
			LOAD r2, e
			ADD r3, f, g
			ADD f3, i, j
			STORE h, c
			JUMP $LOOP
			#return r3
			#end_func
			JUMP $BEGIN
			""";
		String expected = """
			.LINE «Test.asm», 1
			$BEGIN:
			MOVE R0, 42
			MYFUNC:
			.BLOCK MYFUNC
			.LINE «Test.asm», 4
			add sp, -2
			save r26, r28
			save f30, f31
			$LOOP:
			LOAD r1, SF[3]
			LOAD r2, sf[-1]
			ADD r3, R28, R27
			ADD f3, F31, F30
			STORE R26, SF[5]
			JUMP $LOOP
			MOVE r0, r3
			.LINE_BEGIN «Test.asm», 15
			restore f30, f31
			restore r26, r28
			add sp, 2
			return
			.BLOCK_END MYFUNC
			.LINE_END
			.LINE «Test.asm», 16
			JUMP $BEGIN
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected.toUpperCase(), preprocessed.toUpperCase());
	}

	@Test
	void testSVar() {
		String src = """
			#def_func myFunc()
			#svar a
			#svar b, c, d
			#end_func
			""";
		String expected = """
			.LINE «TEST.ASM», 1
			MYFUNC:
			.BLOCK MYFUNC
			.LINE «TEST.ASM», 2
			PUSH 0
			ADD SP, -3
			.LINE_BEGIN «TEST.ASM», 4
			ADD SP, 4
			RETURN
			.BLOCK_END MYFUNC
			.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testVar() {
		String src = """
			#def_func myFunc()
			#var a
			#var b, c, d
			#end_func
			""";
		String expected = """
			.LINE «TEST.ASM», 1
			MYFUNC:
			.BLOCK MYFUNC
			.LINE «TEST.ASM», 2
			PUSH R28
			SAVE R26, R28
			.LINE_BEGIN «TEST.ASM», 4
			RESTORE R25, R28
			RETURN
			.BLOCK_END MYFUNC
			.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testFVar() {
		String src = """
			#def_func myFunc()
			#fvar a
			#fvar b, c, d
			#end_func
			""";
		String expected = """
			.LINE «TEST.ASM», 1
			MYFUNC:
			.BLOCK MYFUNC
			.LINE «TEST.ASM», 2
			PUSH F31
			SAVE F29, F31
			.LINE_BEGIN «TEST.ASM», 4
			RESTORE F28, F31
			RETURN
			.BLOCK_END MYFUNC
			.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testMacro() {
		String src = """
			START:
			#define iPUT_DEC 202
			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
				#macro putter()
			#end_macro
			#def_macro print3(i)
				#macro put_dec(${i})
				move r0, ${i}
				add r0, 1
				#macro put_dec(r0)
				move r0, ${i}
				add r0, 2
				#macro put_dec(r0)
			#end_macro
			#def_macro putter()
				int iPUT_DEC
			#end_macro
			MAIN:
				#macro print_dec(3)
				#macro print3(4)
				stop
				stop
			FINIS:
		""";
		String expected = """
			.LINE «TEST.ASM», 1
			START:
			.LINE «TEST.ASM», 20
			MAIN:
			.LINE_BEGIN «TEST.ASM», 22
			MOVE R1, 4
			MOVE R0, 1
			INT 202
			MOVE R0, 4
			ADD R0, 1
			MOVE R1, R0
			MOVE R0, 1
			INT 202
			MOVE R0, 4
			ADD R0, 2
			MOVE R1, R0
			MOVE R0, 1
			INT 202
			.LINE_END
			.LINE «TEST.ASM», 23
			STOP
			STOP
			FINIS:
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

}