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

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

import org.junit.jupiter.api.Test;

/**
 * End-to-end tests for iOPEN_RANDOM_ACCESS_FILE, iSEEK and iTELL.
 */
public class RandomAccessInterruptTest extends BaseTest {

	private static String tempPath(String name) throws Exception {
		File f = File.createTempFile(name, ".bin");
		f.delete();
		f.deleteOnExit();
		return f.getAbsolutePath();
	}

	@Test
	void testWriteSeekReadTell() throws Exception {
		String path = tempPath("ra_basic");
		String src = """
			#include <system/system.def>
			#include <system/io.def>

			MAIN:
				move   r1, 10
				move   r2, "%s"
				int    iOPEN_RANDOM_ACCESS_FILE
				move   r5, r0              // r5 = TRUE (opened)

				// write 0x0102030405060708 then 'A' 'B'
				load   r20, BIGW
				out    r20, LONG, 10
				move   r20, 'A'
				out    r20, BYTE, 10
				move   r20, 'B'
				out    r20, BYTE, 10
				#macro TELL_PORT(10)
				move   r6, r0              // r6 = 10

				// read the word back from the beginning
				#macro SEEK_PORT(10, SEEK_BEGIN)
				move   r7, r0              // r7 = 0
				in     r8, LONG, 10        // r8 = 0x0102030405060708

				// read the last byte (position -1 = last byte)
				#macro SEEK_PORT(10, -1)
				move   r9, r0              // r9 = 9
				in     r10, BYTE, 10       // r10 = 'B'
				in     r11, BYTE, 10       // r11 = -1 (EOF)
				#macro FEOF(10)
				move   r12, r0             // r12 = TRUE

				// overwrite byte 2 in place, then check the neighbours
				#macro SEEK_PORT(10, 2)
				move   r20, 0xEE
				out    r20, BYTE, 10
				#macro TELL_PORT(10)
				move   r13, r0             // r13 = 3
				#macro SEEK_PORT(10, 1)
				in     r14, BYTE, 10       // r14 = 0x02
				in     r15, BYTE, 10       // r15 = 0xEE
				in     r16, BYTE, 10       // r16 = 0x04

				// SEEK_END returns the length; positions past the end clamp
				#macro SEEK_PORT(10, SEEK_END)
				move   r17, r0             // r17 = 10
				#macro SEEK_PORT(10, 1000)
				move   r18, r0             // r18 = 10
				#macro SEEK_PORT(10, -11)
				move   r19, r0             // r19 = -1 (before the start)
				#macro TELL_PORT(10)
				move   r21, r0             // r21 = 10 (unchanged by the failed seek)

				// append at the end
				#macro SEEK_PORT(10, SEEK_END)
				move   r20, 'C'
				out    r20, BYTE, 10
				move   r1, 10
				int    iCLOSE_FILE

				// reopen: the file is not truncated
				move   r1, 11
				move   r2, "%s"
				int    iOPEN_RANDOM_ACCESS_FILE
				#macro SEEK_PORT(11, SEEK_END)
				move   r22, r0             // r22 = 11
				#macro SEEK_PORT(11, -1)
				in     r23, BYTE, 11       // r23 = 'C'
				move   r1, 11
				int    iCLOSE_FILE

				// TELL / SEEK on a non-random-access port report -1
				#macro TELL_PORT(STDOUT)
				move   r24, r0             // r24 = -1
				#macro SEEK_PORT(STDOUT, 0)
				move   r25, r0             // r25 = -1
				stop
			BIGW: .dci 0x0102030405060708
				stop
			""".formatted(path, path);

		var tuple = runProgram(src, new String[] {"test"});
		var diff = tuple.getRight();
		diff.assertDiff(5, -1);
		diff.assertDiff(6, 10);
		diff.assertDiff(7, 0);
		diff.assertDiff(8, 0x0102030405060708L);
		diff.assertDiff(9, 9);
		diff.assertDiff(10, 'B');
		diff.assertDiff(11, -1);
		diff.assertDiff(12, -1);
		diff.assertDiff(13, 3);
		diff.assertDiff(14, 0x02);
		diff.assertDiff(15, 0xEE);
		diff.assertDiff(16, 0x04);
		diff.assertDiff(17, 10);
		diff.assertDiff(18, 10);
		diff.assertDiff(19, -1);
		diff.assertDiff(21, 10);
		diff.assertDiff(22, 11);
		diff.assertDiff(23, 'C');
		diff.assertDiff(24, -1);
		diff.assertDiff(25, -1);
		byte[] expect = {1, 2, (byte) 0xEE, 4, 5, 6, 7, 8, 'A', 'B', 'C'};
		assertArrayEquals(expect, Files.readAllBytes(new File(path).toPath()));
	}

	@Test
	void testCharactersAndEndian() throws Exception {
		String path = tempPath("ra_char");
		String src = """
			#include <system/system.def>
			#include <system/io.def>

			MAIN:
				move   r1, 12
				move   r2, "%s"
				int    iOPEN_RANDOM_ACCESS_FILE
				// UTF-8 characters: 'h' (1 byte), U+00E9 (2), U+4E16 (3), U+1F600 (4)
				move   r20, 'h'
				out    r20, CHAR, 12
				move   r20, 0xE9
				out    r20, CHAR, 12
				move   r20, 0x4E16
				out    r20, CHAR, 12
				move   r20, 0x1F600
				out    r20, CHAR, 12
				#macro TELL_PORT(12)
				move   r5, r0              // r5 = 10 bytes
				#macro SEEK_PORT(12, 0)
				in     r6, CHAR, 12        // 'h'
				in     r7, CHAR, 12        // 0xE9
				#macro TELL_PORT(12)
				move   r8, r0              // r8 = 3
				in     r9, CHAR, 12        // 0x4E16
				in     r10, CHAR, 12       // 0x1F600
				in     r11, CHAR, 12       // -1
				// little-endian word at the end
				endian 12, TRUE
				#macro SEEK_PORT(12, SEEK_END)
				move   r20, 0x0A0B
				out    r20, SHORT, 12
				#macro SEEK_PORT(12, -2)
				in     r12, BYTE, 12       // 0x0B (low byte first)
				#macro SEEK_PORT(12, -2)
				in     r13, SHORT, 12      // 0x0A0B
				move   r1, 12
				int    iCLOSE_FILE
				stop
				stop
			""".formatted(path);

		var tuple = runProgram(src, new String[] {"test"});
		var diff = tuple.getRight();
		diff.assertDiff(5, 10);
		diff.assertDiff(6, 'h');
		diff.assertDiff(7, 0xE9);
		diff.assertDiff(8, 3);
		diff.assertDiff(9, 0x4E16);
		diff.assertDiff(10, 0x1F600);
		diff.assertDiff(11, -1);
		diff.assertDiff(12, 0x0B);
		diff.assertDiff(13, 0x0A0B);
		byte[] bytes = Files.readAllBytes(new File(path).toPath());
		assertEquals(12, bytes.length);
		assertEquals("h\u00e9\u4e16\uD83D\uDE00", new String(bytes, 0, 10, StandardCharsets.UTF_8));
	}

	@Test
	void testOpenFailureAndLibraryFunctions() throws Exception {
		String path = tempPath("ra_lib");
		String src = """
			#include <system/io.asm>
			#include <system/system.def>

			MAIN:
				// a directory cannot be opened
				move   r1, 13
				move   r2, "/"
				int    iOPEN_RANDOM_ACCESS_FILE
				move   r5, r0              // r5 = FALSE

				#call  openRandomAccessFile("%s")
				move   r6, r0              // r6 = port >= 3
				move   r20, 7
				out    r20, BYTE, r6
				out    r20, BYTE, r6
				#call  tell(r6)
				move   r7, r0              // r7 = 2
				#call  seek(r6, SEEK_BEGIN)
				move   r8, r0              // r8 = 0
				#call  closeFile(r6)
				stop
				stop
			""".formatted(path);

		var tuple = runProgram(src, new String[] {"test"});
		var diff = tuple.getRight();
		diff.assertDiff(5, 0);
		assertTrue(tuple.getMiddle().getR(6) >= 3);
		diff.assertDiff(7, 2);
		diff.assertDiff(8, 0);
		assertEquals(2, new File(path).length());
	}
}
