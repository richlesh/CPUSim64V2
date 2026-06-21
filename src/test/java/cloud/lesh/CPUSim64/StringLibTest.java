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

import static org.junit.jupiter.api.Assertions.assertEquals;

public class StringLibTest extends BaseTest {
	@Test
	void testStartsWith() {
		String src = """
			START:
			#include <system/system.def>
			#include <system/string.def>
			#var	str, prefix1, prefix2, prefix3
			move	str, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			move	prefix1, "ABCD"
			move	prefix2, "HIJK"
			move	prefix3, "1234"
			#macro	STARTS_WITH(str, prefix1)
			move	r4, r0
			#macro	STARTS_WITH(str, prefix2)
			move	r5, r0
			#macro	STARTS_WITH(str, prefix3)
			move	r6, r0
			STOP
			STOP
			FINIS:
			""";
		String expected = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		diff.assertDiff(4, -1);
		diff.assertDiff(5, 0);
		diff.assertDiff(6, 0);
	}

	@Test
	void testEndsWith() {
		String src = """
			START:
			#include <system/system.def>
			#include <system/string.def>
			#var	str, prefix1, prefix2, prefix3
			move	str, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			move	prefix1, "WXYZ"
			move	prefix2, "HIJK"
			move	prefix3, "1234"
			#macro	ENDS_WITH(str, prefix1)
			move	r6, r0
			#macro	ENDS_WITH(str, prefix2)
			move	r7, r0
			#macro	ENDS_WITH(str, prefix3)
			move	r8, r0
			STOP
			STOP
			FINIS:
			""";
		String expected = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(11, diff.size());
		diff.assertDiff(6, -1);
		diff.assertDiff(7, 0);
		diff.assertDiff(8, 0);
	}

	@Test
	void testSPrintF() {
		String src = """
			START:
			#include <system/system.def>
			#include <system/string.asm>
			#include <system/io.asm>
			#var	fmt, i, str
			#fvar	pi
			move	i, 326
			load	pi, 3.1415926
			move	str, "Rich"
			move	fmt, "Hello %s: %d %f\\n"
			#call	sprintf(fmt, str, i, pi)
			#call	puts(r0)
			STOP
			STOP
			FINIS:
			""";
		String expected = "Hello Rich: 326 3.141593\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testFormat() {
		String src = """
			START:
			#include <system/system.def>
			#include <system/string.asm>
			#include <system/io.asm>
			#var	fmt, i, str, pi
			move	i, "326"
			move	pi, "3.1415926"
			move	str, "Rich"
			move	fmt, "Hello {}: {} {}\\n"
			#call	format(fmt, str, i, pi)
			#call	puts(r0)
			STOP
			STOP
			FINIS:
			""";
		String expected = "Hello Rich: 326 3.1415926\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testStringArray() {
		String src = """
			START:
			#include <system/io.asm>
			#include <system/string.asm>
			#var 	i, len, array
			move	len, 10
			move	r1, len
			add		r1, 1
			int		iAlloc
			move	array, r0
			store	len, array[0]
			#for 	1, i <= len, 1
				#macro	FMT_DEC(i)
				store	r0, array[i]
			#end_for
			#call	printStringArray(array)
			#call	freeStrArray(array)
			int		iALLOC_COUNT
			STOP
			STOP
			FINIS:
			""";
		String expected = "1,2,3,4,5,6,7,8,9,10";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, 0);
		assertEquals(expected, output);
	}
}