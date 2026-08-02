// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
.LINE «Test.asm», 1
include_1$asm:
.LINE «include_1.asm», 2
MOV R0, 1
.LINE «include_1.asm», 3
MOV R1, 2
.LINE «Test.asm», 2
include_2$asm:
.LINE «include_2.asm», 2
ADD R0, R0, R1
.LINE «include_2.asm», 3
SUB R1, R1, 1
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed);
	}

	@Test
	void testIncludeSystem() {
		String src = """
			#include <include_1.asm>
			#include <include_2.asm>
			""";
		String expected = """
.LINE «Test.asm», 1
include_1$asm:
.LINE «include_1.asm», 2
MOV R0, 1
.LINE «include_1.asm», 3
MOV R1, 2
.LINE «Test.asm», 2
include_2$asm:
.LINE «include_2.asm», 2
ADD R0, R0, R1
.LINE «include_2.asm», 3
SUB R1, R1, 1
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		assertThrows(PreprocessorVisitor.PreprocessorException.class, () -> pp.preprocessText(src));
	}

	@Test
	void testIncludeBad2() {
		String src = """
			#include 'src/test/resources/include_bad.asm'
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		assertThrows(PreprocessorVisitor.PreprocessorException.class, () -> pp.preprocessText(src));
	}

	@Test
	void testIncludeNested() {
		String src = """
			#include <include_3.asm>
			""";
		String expected = """
.LINE «Test.asm», 1
include_3$asm:
.LINE «include_3.asm», 1
include_1$asm:
.LINE «include_1.asm», 2
MOV R0, 1
.LINE «include_1.asm», 3
MOV R1, 2
.LINE «include_3.asm», 2
include_2$asm:
.LINE «include_2.asm», 2
ADD R0, R0, R1
.LINE «include_2.asm», 3
SUB R1, R1, 1
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
.LINE «Test.asm», 10
MOV R0, 326
.LINE «Test.asm», 11
MOV R1, 127
.LINE «Test.asm», 12
MOV R2, 4096
.LINE «Test.asm», 13
MOV F0, 3.14159
.LINE «Test.asm», 14
MOV F1, 22026.4657948067
.LINE «Test.asm», 15
MOV R2, 'A'
.LINE «Test.asm», 16
MOV R3, "Hello, World!"
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
			#undef SIZE
			#define SIZE 2
			#if SIZE == 1
			MOV F0, 3
			#else_if SIZE == 2
			MOV F0, 3.1
			#else_if SIZE == 3
			MOV F0, 3.14
			#end_if
			#undef SIZE
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
.LINE «Test.asm», 14
MOV F0, 3.1
.LINE «Test.asm», 25
MOV F0, 3.14
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
.LINE «TEST.ASM», 2
MOVE R0, 42
.LINE «TEST.ASM», 3
MYFUNC:
.BLOCK _MYFUNC
.LINE «TEST.ASM», 4
ADD SP, -2
.LINE «TEST.ASM», 5
SAVE R26, R28
.LINE «TEST.ASM», 6
SAVE F30, F31
.LINE «TEST.ASM», 7
$LOOP:
.LINE «TEST.ASM», 8
LOAD R1, SF[3]
.LINE «TEST.ASM», 9
LOAD R2, SF[-1]
.LINE «TEST.ASM», 10
ADD R3, R28, R27
.LINE «TEST.ASM», 11
ADD F3, F31, F30
.LINE «TEST.ASM», 12
STORE R26, SF[5]
.LINE «TEST.ASM», 13
JUMP $LOOP
.LINE «TEST.ASM», 14
MOVE R0, R3
JUMP MYFUNC$_RETURN
.LINE_BEGIN «TEST.ASM», 15
MYFUNC$_RETURN:
RESTORE F30, F31
RESTORE R26, R28
ADD SP, 2
RETURN
.BLOCK_END _MYFUNC
.LINE_END
.LINE «TEST.ASM», 16
JUMP $BEGIN
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
.ORG 1
__CODE__: READONLY __CODE_END__
.LINE «TEST.ASM», 5
MOVE R0, R1
.LINE «TEST.ASM», 6
MOVE R1, __STR_1
.LINE «TEST.ASM», 7
MOVE F0, __FP_1
.LINE «TEST.ASM», 8
STOP
.LINE «TEST.ASM», 9
STOP

__FP_1: .DCF 3.14159267
__STR_1: .DCS "HELLO, WORLD!"

__CODE_END__:
__DATA__:
.LINE «TEST.ASM», 1
GLOBAL_INT: .DCI 326
.LINE «TEST.ASM», 2
GLOBAL_FP: .DCF 3.14
.LINE «TEST.ASM», 3
GLOBAL_ARRAY: .DCA 10
.LINE «TEST.ASM», 4
GLOBAL_BYTES: .DCB 1, 2, 3, 4, 5
.LINE «TEST.ASM», 10
GLOBAL_WORDS: .DCW 1, 2, 3
.LINE «TEST.ASM», 11
GLOBAL_FLOATS: .DCW 1.2, 2.3, 3.4
__DATA_END__:
__HEAP_START__:
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		LiteralRewriter rw = new LiteralRewriter();
		preprocessed = rw.rewrite(preprocessed, pp.getSourceLocations());
		preprocessed = pp.addGlobals(preprocessed, false);
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
.BLOCK _MYFUNC
.LINE «TEST.ASM», 2
PUSH 0
.LINE «TEST.ASM», 3
ADD SP, -3
.LINE_BEGIN «TEST.ASM», 4
MYFUNC$_RETURN:
ADD SP, 4
RETURN
.BLOCK_END _MYFUNC
.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testVar() {
		String src = """
			#def_func myFunc()
			#var a, \\
			 	b, c, d
			#end_func
			""";
		String expected = """
.LINE «TEST.ASM», 1
MYFUNC:
.BLOCK _MYFUNC
.LINE «TEST.ASM», 2
SAVE R25, R28
.LINE_BEGIN «TEST.ASM», 4
MYFUNC$_RETURN:
RESTORE R25, R28
RETURN
.BLOCK_END _MYFUNC
.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
.BLOCK _MYFUNC
.LINE «TEST.ASM», 2
PUSH F31
.LINE «TEST.ASM», 3
SAVE F29, F31
.LINE_BEGIN «TEST.ASM», 4
MYFUNC$_RETURN:
RESTORE F28, F31
RETURN
.BLOCK_END _MYFUNC
.LINE_END
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testMacro() {
		String src = """
			START:
			#define iPUT_DEC 202
			#def_macro put_dec(i)
				move R2, ${i}
				move R1, 1
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
MOVE R2, 3
MOVE R1, 1
INT 202
.LINE_END
.LINE_BEGIN «TEST.ASM», 22
MOVE R2, 4
MOVE R1, 1
INT 202
MOVE R0, 4
ADD R0, 1
MOVE R2, R0
MOVE R1, 1
INT 202
MOVE R0, 4
ADD R0, 2
MOVE R2, R0
MOVE R1, 1
INT 202
.LINE_END
.LINE «TEST.ASM», 23
STOP
.LINE «TEST.ASM», 24
STOP
.LINE «TEST.ASM», 25
FINIS:
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
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
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCond() {
		String src = """
			#var x, y
			clear x
			#if_cond x < 10
				move y, 10
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
.LINE «TEST.ASM», 2
CLEAR R28
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
.LINE «TEST.ASM», 4
MOVE R27, 10
$_SKIP:
.LINE_BEGIN «TEST.ASM», 5
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 6
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondElse() {
		String src = """
			#var x, y
			clear x
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
.LINE «TEST.ASM», 2
CLEAR R28
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
.LINE «TEST.ASM», 4
MOVE R27, 10
.LINE «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE «TEST.ASM», 6
MOVE R27, 20
.LINE_BEGIN «TEST.ASM», 7
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 8
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondElseIf() {
		String src = """
			#var x, y
			clear x
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
.LINE «TEST.ASM», 2
CLEAR R28
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
.LINE «TEST.ASM», 4
MOVE R27, 10
.LINE «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_BEGIN «TEST.ASM», 5
CMP R28, 20
JUMP NN, $_SKIP_1
.LINE_END
.LINE «TEST.ASM», 6
MOVE R27, 20
$_SKIP_1:
.LINE_BEGIN «TEST.ASM», 7
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 8
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondElseIf2() {
		String src = """
			#var x, y
			clear x
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
.LINE «TEST.ASM», 2
CLEAR R28
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
.LINE «TEST.ASM», 4
MOVE R27, 10
.LINE «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_BEGIN «TEST.ASM», 5
CMP R28, 20
JUMP NN, $_SKIP_1
.LINE_END
.LINE «TEST.ASM», 6
MOVE R27, 20
.LINE «TEST.ASM», 7
JUMP $_COND_END
$_SKIP_1:
.LINE_BEGIN «TEST.ASM», 7
CMP R28, 30
JUMP NN, $_SKIP_2
.LINE_END
.LINE «TEST.ASM», 8
MOVE R27, 30
$_SKIP_2:
.LINE_BEGIN «TEST.ASM», 9
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 10
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}
	@Test
	void testIfCondElseIfElse() {
		String src = """
			#var x, y
			clear x
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
.LINE «TEST.ASM», 2
CLEAR R28
.LINE_BEGIN «TEST.ASM», 3
.BLOCK COND_{}
CMP R28, 10
JUMP NN, $_SKIP
.LINE_END
.LINE «TEST.ASM», 4
MOVE R27, 10
.LINE «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE_BEGIN «TEST.ASM», 5
CMP R28, 20
JUMP NN, $_SKIP_1
.LINE_END
.LINE «TEST.ASM», 6
MOVE R27, 20
.LINE «TEST.ASM», 7
JUMP $_COND_END
$_SKIP_1:
.LINE_BEGIN «TEST.ASM», 7
CMP R28, 30
JUMP NN, $_SKIP_2
.LINE_END
.LINE «TEST.ASM», 8
MOVE R27, 30
.LINE «TEST.ASM», 9
JUMP $_COND_END
$_SKIP_2:
.LINE «TEST.ASM», 10
MOVE R27, 40
.LINE_BEGIN «TEST.ASM», 11
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 12
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondSR() {
		String src = """
			#var x, y
			clear x
			#if_cond_SR nz
				move y, 10
			#end_cond
			move x, 0
			""";
		String expected = """
.LINE «TEST.ASM», 1
SAVE R27, R28
.LINE «TEST.ASM», 2
CLEAR R28
.LINE_BEGIN «TEST.ASM», 3
.BLOCK CONDSR_{}
JUMP Z, $_SKIP
.LINE_END
.LINE «TEST.ASM», 4
MOVE R27, 10
$_SKIP:
.LINE_BEGIN «TEST.ASM», 5
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 6
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testIfCondSRElse() {
		String src = """
			#var x, y
			clear x
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
.LINE «TEST.ASM», 2
CLEAR R28
.LINE_BEGIN «TEST.ASM», 3
.BLOCK CONDSR_{}
JUMP NZ, $_SKIP
.LINE_END
.LINE «TEST.ASM», 4
MOVE R27, 10
.LINE «TEST.ASM», 5
JUMP $_COND_END
$_SKIP:
.LINE «TEST.ASM», 6
MOVE R27, 20
.LINE_BEGIN «TEST.ASM», 7
$_COND_END:
.BLOCK_END
.LINE_END
.LINE «TEST.ASM», 8
MOVE R28, 0
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}

	@Test
	void testExpressionFolding() {
		String src = """
			move	r3, 1+1
			move	r3, 1-1
			move	r3, -1-1
			move	r3, -1--1
			move	r3, -1-(-1)
			move	r3, 1 + 2 + 3 + 4
			move	r3, 1 + 2 * 3 + 4
			move	r3, (1 + 2) * (3 + 4)
			move	r3, 1 * 2 + 3 * 4
			move	r3, 10 / 5
			load	f0, 2.2 + 3.3
			load	f0, 2.2 * 2
			load	f0, 2.2 / 2
			load	f0, 1.5 + 1.1 * 3 + 1.3
			load	f0, (1.5 + 1.1) * (3 + 1.3)
			load	f0, 1.5 * 1.1 + 3 * 1.3
			load	f0, -1.5 + -1.1 * 3 + -1.3
			load	f0, (-1.5 - 1.1) * -(3 + 1.3)
			load	f0, -1.5 * 1.1 - 3 * 1.3
			""";
		String expected = """
.LINE «TEST.ASM», 1
MOVE R3, 2
.LINE «TEST.ASM», 2
MOVE R3, 0
.LINE «TEST.ASM», 3
MOVE R3, -2
.LINE «TEST.ASM», 4
MOVE R3, 0
.LINE «TEST.ASM», 5
MOVE R3, 0
.LINE «TEST.ASM», 6
MOVE R3, 10
.LINE «TEST.ASM», 7
MOVE R3, 11
.LINE «TEST.ASM», 8
MOVE R3, 21
.LINE «TEST.ASM», 9
MOVE R3, 14
.LINE «TEST.ASM», 10
MOVE R3, 2
.LINE «TEST.ASM», 11
LOAD F0, 5.5
.LINE «TEST.ASM», 12
LOAD F0, 4.4
.LINE «TEST.ASM», 13
LOAD F0, 1.1
.LINE «TEST.ASM», 14
LOAD F0, 6.1000000000000005
.LINE «TEST.ASM», 15
LOAD F0, 11.18
.LINE «TEST.ASM», 16
LOAD F0, 5.550000000000001
.LINE «TEST.ASM», 17
LOAD F0, -6.1000000000000005
.LINE «TEST.ASM», 18
LOAD F0, 11.18
.LINE «TEST.ASM», 19
LOAD F0, -5.550000000000001
			""";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src);
		assertEquals(expected, preprocessed.toUpperCase());
	}
}