package cloud.lesh.CPUSim64;

import org.junit.jupiter.api.Test;

import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

class PreprocessorTest {
	@Test
	void testIncludeLocal() {
		String src = """
			#include "src/test/resources/include_1.asm"
			#include "src/test/resources/include_2.asm"
			""";
		String expected = """
include_1$asm:
.LINE «include_1.asm», 2
MOV R0, 1
MOV R1, 2
include_2$asm:
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
include_1$asm:
.LINE «include_1.asm», 2
MOV R0, 1
MOV R1, 2
include_2$asm:
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
include_3$asm:
include_1$asm:
.LINE «include_1.asm», 2
MOV R0, 1
MOV R1, 2
include_2$asm:
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
			#end_if
			#undef USE_FP
			#if USE_FP
			MOV F0, 3.14
			#end_if
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
			MOV F0, 3.1415
			#else
			MOV F0, 3.1415
			#endif
			""";
		String expected = """
			.LINE «Test.asm», 3
			MOV F0, 3.14159
			.LINE «Test.asm», 11
			MOV F0, 3.1415
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
			#else_if SIZE == 2
			MOV F0, 3.1
			#else_if SIZE == 3
			MOV F0, 3.14
			#end_if
			#define SIZE 2
			#if SIZE == 1
			MOV F0, 3
			#else_if SIZE == 2
			MOV F0, 3.1
			#else_if SIZE == 3
			MOV F0, 3.14
			#end_if
			#define SIZE 3
			#if SIZE == 1
			MOV F0, 3
			#else_if SIZE == 2
			MOV F0, 3.1
			#else_if SIZE == 3
			MOV F0, 3.14
			#end_if
			#undef SIZE
			#if SIZE == 1
			MOV F0, 3
			#else_if SIZE == 2
			MOV F0, 3.1
			#else_if SIZE == 3
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
	void testIfDef() {
		String src = """
			#define USE_FP
			#ifdef USE_FP
			MOV F0, 3.14159
			#endif
			#undef USE_FP
			#ifdef USE_FP
			MOV F0, 3.14
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
	void testIfDefElse() {
		String src = """
			#define USE_FP
			#ifdef USE_FP
			MOV F0, 3.14159
			#else
			MOV F0, 3.14
			#end_if
			#undef USE_FP
			#ifdef USE_FP
			MOV F0, 3.141
			#else
			MOV F0, 3.1415
			#end_if
			""";
		String expected = """
			.LINE «Test.asm», 3
			MOV F0, 3.14159
			.LINE «Test.asm», 11
			MOV F0, 3.1415
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIfNDef() {
		String src = """
			#define USE_FP
			#ifndef USE_FP
			MOV F0, 3.14159
			#endif
			#undef USE_FP
			#ifndef USE_FP
			MOV F0, 3.14
			#endif
			""";
		String expected = """
			.LINE «Test.asm», 7
			MOV F0, 3.14
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIfNDefElse() {
		String src = """
			#define USE_FP
			#ifndef USE_FP
			MOV F0, 3.14159
			#else
			MOV F0, 3.14
			#endif
			#undef USE_FP
			#ifndef USE_FP
			MOV F0, 3.1415
			#else
			MOV F0, 3.141
			#endif
			""";
		String expected = """
			.LINE «Test.asm», 5
			MOV F0, 3.14
			.LINE «Test.asm», 9
			MOV F0, 3.1415
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
			JUMP $_RETURN
			.LINE «TEST.ASM», 2
			MOVE F0, 3.14
			JUMP $_RETURN
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
.LINE «TEST.ASM», 1
$BEGIN:
MOVE R0, 42
MYFUNC:
.BLOCK MYFUNC
.LINE «TEST.ASM», 4
ADD SP, -2
SAVE R26, R28
SAVE F30, F31
$LOOP:
LOAD R1, SF[3]
LOAD R2, SF[-1]
ADD R3, R28, R27
ADD F3, F31, F30
STORE R26, SF[5]
JUMP $LOOP
MOVE R0, R3
JUMP MYFUNC$_RETURN
.LINE_BEGIN «TEST.ASM», 16
MYFUNC$_RETURN:
RESTORE F30, F31
RESTORE R26, R28
ADD SP, 2
RETURN
.BLOCK_END MYFUNC
.LINE_END
.LINE «TEST.ASM», 16
JUMP $BEGIN
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected.toUpperCase(), preprocessed.toUpperCase());
	}

	@Test
	void testGlobal() {
		String src = """
			#global	GLOBAL_INT:	.dci 326			// comment
			#global	GLOBAL_FP:	.dcf 3.14
			#global	GLOBAL_ARRAY:	.dca 10			/* Comment */
			#global	GLOBAL_BYTES:	.dcb 1,2,3,4,5
			move	r0, r1
			move	r1, "Hello, world!"
			move	f0, 3.14159267
			stop
			stop
			#global GLOBAL_WORDS:	.dcw 0x01, 0x02, 0x03
			#global GLOBAL_FLOATS:	.dcw 1.2, 2.3, 3.4	// Comment
			""";
		String expected = """
__CODE__:
READONLY __CODE_END__
.LINE «TEST.ASM», 5
MOVE	R0, R1
MOVE	R1, __STR_1
MOVE	F0, __FP_1
STOP
STOP

__FP_1: .DCF 3.14159267
__STR_1: .DCS "HELLO, WORLD!"

__CODE_END__:
__DATA__:
GLOBAL_INT:	.DCI 326
GLOBAL_FP:	.DCF 3.14
GLOBAL_ARRAY:	.DCA 10
GLOBAL_BYTES:	.DCB 1,2,3,4,5
GLOBAL_WORDS:	.DCW 0X01, 0X02, 0X03
GLOBAL_FLOATS:	.DCW 1.2, 2.3, 3.4
__DATA_END__:
__HEAP_START__:
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		LiteralRewriter rw = new LiteralRewriter();
		preprocessed = rw.rewrite(preprocessed);
		preprocessed = PreprocessorVisitor.addGlobals(preprocessed);
		assertEquals(expected, preprocessed.toUpperCase());
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
			MYFUNC$_RETURN:
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
			MYFUNC$_RETURN:
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
			MYFUNC$_RETURN:
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
				#macro put_dec(3)
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
.LINE_BEGIN «TEST.ASM», 21
MOVE R1, 3
MOVE R0, 1
INT 202
.LINE_END
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
STOP
STOP
FINIS:
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testMacroVarArg() {
		String src = """
			#def_macro	DEBUG(...)
				#call	debug(STDOUT, ${...})
			#end_macro
			#macro DEBUG("Test message", 1, 2, 3)
			#macro DEBUG("Another message")
			""";
		String expected = """
			.LINE_BEGIN «TEST.ASM», 4
			PUSH 3
			PUSH 2
			PUSH 1
			PUSH "TEST MESSAGE"
			PUSH STDOUT
			CALL DEBUG
			ADD SP, 5
			.LINE_END
			.LINE_BEGIN «TEST.ASM», 5
			PUSH "ANOTHER MESSAGE"
			PUSH STDOUT
			CALL DEBUG
			ADD SP, 2
			.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCond() {
		String src = """
			#var x, y
			move x = 0
			#if_cond x < 10
				move y, 10
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
MOVE R28 = 0
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
MOVE R27, 10
.LINE_BEGIN «TEST.ASM», 5
$_SKIP:
.LINE_END
.LINE_BEGIN «TEST.ASM», 6
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 6
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondElse() {
		String src = """
			#var x, y
			move x = 0
			#if_cond x < 10
				move y, 10
			#else_cond
				move y, 20
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
MOVE R28 = 0
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
MOVE R27, 10
.LINE_BEGIN «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_END
MOVE R27, 20
.LINE_BEGIN «TEST.ASM», 7
$_COND_END:
.BLOCK_END
.LINE_END
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondElseIf() {
		String src = """
			#var x, y
			move x = 0
			#if_cond x < 10
				move y, 10
			#else_if_cond x < 20
				move y, 20
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
MOVE R28 = 0
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
MOVE R27, 10
.LINE_BEGIN «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_END
.LINE_BEGIN «TEST.ASM», 6
CMP R28, 20
JUMP NN, $_SKIP_1
.LINE_END
.LINE «TEST.ASM», 6
MOVE R27, 20
.LINE_BEGIN «TEST.ASM», 7
$_SKIP_1:
.LINE_END
.LINE_BEGIN «TEST.ASM», 8
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 8
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondElseIf2() {
		String src = """
			#var x, y
			move x = 0
			#if_cond x < 10
				move y, 10
			#else_if_cond x < 20
				move y, 20
			#else_if_cond x < 30
				move y, 30
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
MOVE R28 = 0
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
MOVE R27, 10
.LINE_BEGIN «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_END
.LINE_BEGIN «TEST.ASM», 6
CMP R28, 20
JUMP NN, $_SKIP_1
.LINE_END
.LINE «TEST.ASM», 6
MOVE R27, 20
.LINE_BEGIN «TEST.ASM», 7
JUMP $_COND_END
$_SKIP_1:
.LINE_END
.LINE_BEGIN «TEST.ASM», 8
CMP R28, 30
JUMP NN, $_SKIP_2
.LINE_END
.LINE «TEST.ASM», 8
MOVE R27, 30
.LINE_BEGIN «TEST.ASM», 9
$_SKIP_2:
.LINE_END
.LINE_BEGIN «TEST.ASM», 10
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 10
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}
	@Test
	void testIfCondElseIfElse() {
		String src = """
			#var x, y
			move x = 0
			#if_cond x < 10
				move y, 10
			#else_if_cond x < 20
				move y, 20
			#else_if_cond x < 30
				move y, 30
			#else_cond
				move y, 40
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
MOVE R28 = 0
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
MOVE R27, 10
.LINE_BEGIN «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_END
.LINE_BEGIN «TEST.ASM», 6
CMP R28, 20
JUMP NN, $_SKIP_1
.LINE_END
.LINE «TEST.ASM», 6
MOVE R27, 20
.LINE_BEGIN «TEST.ASM», 7
JUMP $_COND_END
$_SKIP_1:
.LINE_END
.LINE_BEGIN «TEST.ASM», 8
CMP R28, 30
JUMP NN, $_SKIP_2
.LINE_END
.LINE «TEST.ASM», 8
MOVE R27, 30
.LINE_BEGIN «TEST.ASM», 9
JUMP $_COND_END
$_SKIP_2:
.LINE_END
MOVE R27, 40
.LINE_BEGIN «TEST.ASM», 11
$_COND_END:
.BLOCK_END
.LINE_END
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondSR() {
		String src = """
			#var x, y
			move x = 0
			#if_cond_SR nz
				move y, 10
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
MOVE R28 = 0
.LINE_BEGIN «TEST.ASM», 3
.BLOCK CONDSR_{}
JUMP Z, $_SKIP
.LINE_END
MOVE R27, 10
.LINE_BEGIN «TEST.ASM», 5
$_SKIP:
.LINE_END
.LINE_BEGIN «TEST.ASM», 6
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 6
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondSRElse() {
		String src = """
			#var x, y
			move x = 0
			#if_cond_SR z
				move y, 10
			#else_cond
				move y, 20
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
MOVE R28 = 0
.LINE_BEGIN «TEST.ASM», 3
.BLOCK CONDSR_{}
JUMP NZ, $_SKIP
.LINE_END
MOVE R27, 10
.LINE_BEGIN «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_END
MOVE R27, 20
.LINE_BEGIN «TEST.ASM», 7
$_COND_END:
.BLOCK_END
.LINE_END
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);
		assertEquals(expected, preprocessed.toUpperCase());
	}
}