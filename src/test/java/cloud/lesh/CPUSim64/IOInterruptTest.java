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

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class IOInterruptTest extends BaseTest {
	@Test
	void testSTDOUT() {
		String src = """
			START:
				#include <system/io.def>
				#include <system/math.def>
				MOVE R1, STDOUT
				MOVE R2, 326
				MOVE R3, 10
				INT iPUT_INT
				INT iPUT_NL
				MOVE R3, 16
				INT iPUT_INT
				INT iPUT_NL
				MOVE R3, 2
				INT iPUT_INT
				INT iPUT_NL
				INT iPUT_DEC
				INT iPUT_NL
				INT iPUT_HEX
				INT iPUT_NL
				INT iPI
				MOVE F1, F0
				MOVE R2, 8
				INT iPUT_FP
				INT iPUT_NL
				MOVE R2, "HÉLLØ😀🇺🇸"
				INT iPUTS
				INT iPUT_NL
				STOP
			FINIS:
			""";
		String expected = """
326
146
101000110
326
146
3.14159265
HÉLLØ😀🇺🇸
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testSTDERR() {
		String src = """
			START:
				#include <system/io.def>
				#include <system/math.def>
				MOVE R1, STDERR
				MOVE R2, 326
				MOVE R3, 10
				INT iPUT_INT
				INT iPUT_NL
				MOVE R3, 16
				INT iPUT_INT
				INT iPUT_NL
				MOVE R3, 2
				INT iPUT_INT
				INT iPUT_NL
				INT iPUT_DEC
				INT iPUT_NL
				INT iPUT_HEX
				INT iPUT_NL
				INT iPI
				MOVE F1, F0
				MOVE R2, 8
				INT iPUT_FP
				INT iPUT_NL
				MOVE R2, "HÉLLØ😀🇺🇸"
				INT iPUTS
				INT iPUT_NL
				STOP
			FINIS:
			""";
		String expected = """
326
146
101000110
326
146
3.14159265
HÉLLØ😀🇺🇸
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testRawFileWrite_BigEndian() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			
			MAIN:
			#var i
			// Open file for writing in binary mode
				move	r1, 3
				move	r2, "test.bin"
				int		iOPEN_RAW_FILE_WRITE
				endian	3, 0
			
			// Write 256 bytes
			// for i=0; i<256; ++i
				move	i, 0
			LOOP:
				out		i, 1, 3
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE

			// Open file for appending in binary mode
				move	r1, 3
				move	r2, "test.bin"
				int		iOPEN_RAW_FILE_APPEND
				endian	3, 0

			// Write 256 words
			// for i=0; i<256; ++i
				move	i, 0
			LOOP2:
				out		i, 8, 3
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP2
			
				load	i, DATA
				out		i, 8, 3
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
			
			END:
				stop
				stop
			DATA: .DCI	0x1234567890ABCDEF
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		assertTrue(Files.mismatch(Path.of("test.bin"), Path.of("src/test/resources/test.big.bin")) == -1);
	}

	@Test
	void testRawFileWrite_LittleEndian() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			
			MAIN:
			#var i
			// Open file for writing in binary mode
				move	r1, 3
				move	r2, "test.bin"
				int		iOPEN_RAW_FILE_WRITE
				endian	3, 1
			
			// Write 256 bytes
			// for i=0; i<256; ++i
				move	i, 0
			LOOP:
				out		i, 1, 3
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE

			// Open file for appending in binary mode
				move	r1, 3
				move	r2, "test.bin"
				int		iOPEN_RAW_FILE_APPEND
				endian	3, 1
			// Write 256 words
			// for i=0; i<256; ++i
				move	i, 0
			LOOP2:
				out		i, 8, 3
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP2
			
				load	i, DATA
				out		i, 8, 3
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
			
			END:
				stop
				stop
			DATA: .DCI	0x1234567890ABCDEF
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		assertTrue(Files.mismatch(Path.of("test.bin"), Path.of("src/test/resources/test.little.bin")) == -1);
	}

	@Test
	void testRawFileRead_BigEndian() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			
			MAIN:
			#var i, val
			// Open file for reading in binary mode
				move	r1, 3
				move	r2, "src/test/resources/test.big.bin"
				int		iOPEN_RAW_FILE_READ
				endian	3, 0
			
			// Read 256 bytes
			// for i=0; i<256; ++i
				move	i, 0
			LOOP:
				in		val, 1, 3
				cmp		i, val
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP
			
			// Read 256 words
			// for i=0; i<256; ++i
				move	i, 0
			LOOP2:
				in		val, 8, 3
				cmp		i, val
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP2
			
				load	i, DATA
				in		val, 8, 3
				cmp		i, val
				jump	ne, END
			
				move	r0, -2
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
			
			END:
				stop
				stop
			DATA: .DCI	0x1234567890ABCDEF
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, -2);
	}

	@Test
	void testRawFileRead_LittleEndian() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			
			MAIN:
			#var i, val
			// Open file for reading in binary mode
				move	r1, 3
				move	r2, "src/test/resources/test.little.bin"
				int		iOPEN_RAW_FILE_READ
				endian	3, 1
			
			// Read 256 bytes
			// for i=0; i<256; ++i
				move	i, 0
			LOOP:
				in		val, 1, 3
				cmp		i, val
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP
			
			// Read 256 words
			// for i=0; i<256; ++i
				move	i, 0
			LOOP2:
				in		val, 8, 3
				cmp		i, val
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP2
			
				load	i, DATA
				in		val, 8, 3
				cmp		i, val
				jump	ne, END
			
				move	r0, -2
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
			
			END:
				stop
				stop
			DATA: .DCI	0x1234567890ABCDEF
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, -2);
	}

	@Test
	void testFileWriteAppendRead() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			#include <system/string.def>
		
			MAIN:
			#var i
			// Open file for writing in text mode
				move	r1, 3
				move	r2, "test.txt"
				int		iOPEN_FILE_WRITE
		
			// Write 256 ints in base 10, 16, 2
			// for i=0; i<256; ++i
				move	i, 0
			LOOP:
				move	r1, 3
				move	r2, i
				move	r3, 10
				int		iPUT_INT
				int		iPUT_NL
				move	r3, 16
				int		iPUT_INT
				int		iPUT_NL
				move	r3, 2
				int		iPUT_INT
				int		iPUT_NL
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP
		
			// Flush the file
				move	r1, 3
				int		iFLUSH

			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
		
			// Open file for appending in text mode
				move	r1, 3
				move	r2, "test.txt"
				int		iOPEN_FILE_APPEND
		
			// Write 256 words
			// for i=0; i<256; ++i
				move	i, 0
			LOOP2:
				move	r1, 3
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
				move	r3, 8
				int		iPUT_HEX
				int		iPUT_NL
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP2
		
			// Print FP
				load	f1, DATA
				move	r2, 7
				int		iPUT_FP
				int		iPUT_NL
		
			// Print String
				move	r2, STR
				int		iPUTS
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
		
			// Open file for reading in text mode
				move	r1, 3
				move	r2, "test.txt"
				int		iOPEN_FILE_READ
		
			// read 256 ints in base 10, 16, 2
			// for i=0; i<256; ++i
				move	i, 0
			LOOP3:
				move	r1, 3
				move	r2, 10
				int		iGET_INT
				cmp		r0, i
				jump	ne, END
				move	r1, 3
				move	r2, 16
				int		iGET_INT
				cmp		r0, i
				jump	ne, END
				move	r1, 3
				move	r2, 2
				int		iGET_INT
				cmp		r0, i
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP3
		
				move	i, 0
			LOOP4:
				move	r1, 3
				int		iGET_DEC
				cmp		r0, i
				jump	ne, END
				move	r1, 3
				int		iGET_HEX
				cmp		r0, i
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP4
		
			// Read FP
				load	f1, DATA
				move	r1, 3
				int		iGET_FP
				cmp		f0, f1
				jump	ne, END
		
			// Read String
				move	r1, 3
				move	r2, 0
				int		iGETS
				move	r1, STR
				int		iSTRCMP
				jump	ne, END
				move	r1, r0
				int		iFREE
		
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
		
				move	r0, -1
			END:
				stop
				stop
			DATA: .DCF	3.1415926
			STR:  .DCS	"HÉLLØ😀🇺🇸"
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		diff.assertDiff(0, -1);
	}

	@Test
	void testFileGetLineLF() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
		
			MAIN:
			#var i, buffer, count
			// Open file for reading in text mode
				move	r1, 3
				move	r2, "src/test/resources/usconst_lf.txt"
				int		iOPEN_FILE_READ
		
				move	buffer, 0
				move	count, 0
			LOOP:
				move	r1, 3
				move	r2, buffer
				int		iGET_LINE
				move	buffer, r2
				cmp		r0, -1
				jump	eq, LOOP_END
				load	r0, buffer[0]
				add		count, r0
				add		count, 1
				jump	LOOP
			LOOP_END:
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
		
				move	r0, count
			END:
				stop
				stop
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, 48301);
	}

	@Test
	void testFileGetLineCRLF() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
		
			MAIN:
			#var i, buffer, count
			// Open file for reading in text mode
				move	r1, 3
				move	r2, "src/test/resources/usconst_crlf.txt"
				int		iOPEN_FILE_READ
		
				move	buffer, 0
				move	count, 0
			LOOP:
				move	r1, 3
				move	r2, buffer
				int		iGET_LINE
				move	buffer, r2
				cmp		r0, -1
				jump	eq, LOOP_END
				load	r0, buffer[0]
				add		count, r0
				add		count, 1
				jump	LOOP
			LOOP_END:
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
		
				move	r0, count
			END:
				stop
				stop
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, 48301);
	}

	@Test
	void testFilePutLine() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
		
			MAIN:
			#var i, buffer, count
			// Open file for reading in text mode
				move	r1, 3
				move	r2, "src/test/resources/usconst_lf.txt"
				int		iOPEN_FILE_READ
			// Open file for writing in text mode
				move	r1, 4
				move	r2, "usconst.txt"
				int		iOPEN_FILE_WRITE
	
				move	buffer, 0
				move	count, 0
			LOOP:
				move	r1, 3
				move	r2, buffer
				int		iGET_LINE
				move	buffer, r2
				cmp		r0, -1
				jump	eq, LOOP_END
				move	r1, 4
				move	r2, buffer
				int		iPUT_LINE
				add		count, 1
				jump	LOOP
			LOOP_END:
			
			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
				move	r1, 4
				int		iCLOSE_FILE
		
				move	r0, count
			END:
				stop
				stop
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		assertTrue(Files.mismatch(Path.of("usconst.txt"), Path.of("src/test/resources/usconst_lf.txt")) == -1);
		diff.assertDiff(0, 978);
	}

	@Test
	void testFileDelete() throws IOException {
		String src = """
			#include <system/io.def>
		
			MAIN:
				#var	FILENAME
				move	r1, "TEMP_"
				move	r2, ".TXT"
				int		iTEMP_FILE
				move	FILENAME, r0

			// Open file for writing in text mode
				move	r1, 3
				move	r2, FILENAME
				int		iOPEN_FILE_WRITE
				jump	z, END

			// Close the file
				move	r1, 3
				int		iCLOSE_FILE
				
			// Check to see if file exists
				move	r1, FILENAME
				int		iFILE_EXISTS
				jump	z, END
				
			// Check to see if file is a file
				move	r1, FILENAME
				int		iIS_FILE
				jump	z, END
				
			// Delete the file
				move	r1, FILENAME
				int		iDELETE_FILE
				move	r1, r0
				jump	z, END
				
			// Delete the file again
				move	r1, FILENAME
				int		iDELETE_FILE
				jump	nz, END

			// Check to see if file exists again
				move	r1, FILENAME
				int		iFILE_EXISTS
				jump	nz, END
				
			// Check to see if file is a file
				move	r1, FILENAME
				int		iIS_FILE
				jump	nz, END
				
				move	r0, -2
			END:
				stop
				stop
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		diff.assertDiff(0, -2);
	}

	@Test
	void testFileDirectoryOps() throws IOException {
		String src = """
			#include <system/io.def>
			#include <system/string.asm>
		
			MAIN:
				#var	dirName, fileList
			// Get Temp dir name
				move	r1, "TEST_"
				int		iTEMP_DIR
				move	dirName, r0

			// Check dir exists
				move	r1, dirName
				int		iIS_DIR
				jump	z, END
				
			// Make dir
				move	r1, DIRNAME2
				int		iMAKE_DIR
				jump	z, END
				
			// Make dir again
				move	r1, DIRNAME2
				int		iMAKE_DIR
				jump	nz, END
				
			// Check dir exists
				move	r1, DIRNAME2
				int		iIS_DIR
				jump	z, END
				
			// Remove dir
				move	r1, DIRNAME2
				int		iDELETE_DIR
				jump	z, END
				
			// Remove dir again
				move	r1, DIRNAME2
				int		iDELETE_DIR
				jump	nz, END
				
			// Check dir exists
				move	r1, DIRNAME2
				int		iIS_DIR
				jump	nz, END
					
			// List files
				move	r1, "."
				int		iFILES
				move	fileList, r0
				#call	printStrArray(fileList)
				load	r0, fileList[0]
				test	r0
				jump	z, END
				
				move	r0, -2			
			END:
				stop
				stop
			DIRNAME2: .DCS "test"
		""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, -2);
	}
}
