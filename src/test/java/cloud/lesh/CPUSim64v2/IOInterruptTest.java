package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class IOInterruptTest extends InterruptTest {
	@Test
	void testSTDOUT() {
		String src = """
			START:
				#include <system/io.def>
				#include <system/math.def>
				MOVE R0, STDOUT
				MOVE R1, 326
				MOVE R2, 10
				INT iPUT_INT
				INT iPUT_NL
				MOVE R2, 16
				INT iPUT_INT
				INT iPUT_NL
				MOVE R2, 2
				INT iPUT_INT
				INT iPUT_NL
				INT iPUT_DEC
				INT iPUT_NL
				INT iPUT_HEX
				INT iPUT_NL
				INT iPI
				MOVE R1, 8
				INT iPUT_FP
				INT iPUT_NL
				MOVE R1, "HÉLLØ😀🇺🇸"
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
		assertEquals(6, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testSTDERR() {
		String src = """
			START:
				#include <system/io.def>
				#include <system/math.def>
				MOVE R0, STDERR
				MOVE R1, 326
				MOVE R2, 10
				INT iPUT_INT
				INT iPUT_NL
				MOVE R2, 16
				INT iPUT_INT
				INT iPUT_NL
				MOVE R2, 2
				INT iPUT_INT
				INT iPUT_NL
				INT iPUT_DEC
				INT iPUT_NL
				INT iPUT_HEX
				INT iPUT_NL
				INT iPI
				MOVE R1, 8
				INT iPUT_FP
				INT iPUT_NL
				MOVE R1, "HÉLLØ😀🇺🇸"
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
		assertEquals(6, diff.size());
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
				move	r0, 3
				move	r1, "test.bin"
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
				move	r0, 3
				int		iCLOSE_FILE

			// Open file for appending in binary mode
				move	r0, 3
				move	r1, "test.bin"
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
				move	r0, 3
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
		assertEquals(6, diff.size());
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
				move	r0, 3
				move	r1, "test.bin"
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
				move	r0, 3
				int		iCLOSE_FILE

			// Open file for appending in binary mode
				move	r0, 3
				move	r1, "test.bin"
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
				move	r0, 3
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
		assertEquals(6, diff.size());
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
				move	r0, 3
				move	r1, "src/test/resources/test.big.bin"
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
			
				move	r1, -1
			
			// Close the file
				move	r0, 3
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
		diff.assertDiff(1, -1);
	}

	@Test
	void testRawFileRead_LittleEndian() throws IOException {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			
			MAIN:
			#var i, val
			// Open file for reading in binary mode
				move	r0, 3
				move	r1, "src/test/resources/test.little.bin"
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
			
				move	r1, -1
			
			// Close the file
				move	r0, 3
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
		diff.assertDiff(1, -1);
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
				move	r0, 3
				move	r1, "test.txt"
				int		iOPEN_FILE_WRITE
		
			// Write 256 ints in base 10, 16, 2
			// for i=0; i<256; ++i
				move	i, 0
			LOOP:
				move	r0, 3
				move	r1, i
				move	r2, 10
				int		iPUT_INT
				int		iPUT_NL
				move	r2, 16
				int		iPUT_INT
				int		iPUT_NL
				move	r2, 2
				int		iPUT_INT
				int		iPUT_NL
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP
		
			// Close the file
				move	r0, 3
				int		iCLOSE_FILE
		
			// Open file for appending in text mode
				move	r0, 3
				move	r1, "test.txt"
				int		iOPEN_FILE_APPEND
		
			// Write 256 words
			// for i=0; i<256; ++i
				move	i, 0
			LOOP2:
				move	r0, 3
				move	r1, i
				int		iPUT_DEC
				int		iPUT_NL
				move	r2, 8
				int		iPUT_HEX
				int		iPUT_NL
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP2
		
			// Print FP
				load	f0, DATA
				move	r1, 7
				int		iPUT_FP
				int		iPUT_NL
		
			// Print String
				move	r1, STR
				int		iPUTS
			// Close the file
				move	r0, 3
				int		iCLOSE_FILE
		
			// Open file for reading in text mode
				move	r0, 3
				move	r1, "test.txt"
				int		iOPEN_FILE_READ
		
			// read 256 ints in base 10, 16, 2
			// for i=0; i<256; ++i
				move	i, 0
			LOOP3:
				move	r0, 3
				move	r1, 10
				int		iGET_INT
				cmp		r0, i
				jump	ne, END
				move	r0, 3
				move	r1, 16
				int		iGET_INT
				cmp		r0, i
				jump	ne, END
				move	r0, 3
				move	r1, 2
				int		iGET_INT
				cmp		r0, i
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP3
		
				move	i, 0
			LOOP4:
				move	r0, 3
				int		iGET_DEC
				cmp		r0, i
				jump	ne, END
				move	r0, 3
				int		iGET_HEX
				cmp		r0, i
				jump	ne, END
				add		i, 1
				cmp		i, 256
				jump	ne, LOOP4
		
			// Read FP
				load	f1, DATA
				move	r0, 3
				int		iGET_FP
				cmp		f0, f1
				jump	ne, END
		
			// Read String
				move	r0, 3
				move	r1, 0
				int		iGETS
				move	r0, STR
				int		iSTRCMP
				jump	ne, END
				int		iFREE
		
			// Close the file
				move	r0, 3
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
		assertEquals(9, diff.size());
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
				move	r0, 3
				move	r1, "src/test/resources/usconst_lf.txt"
				int		iOPEN_FILE_READ
		
				move	buffer, 0
				move	count, 0
			LOOP:
				move	r0, 3
				move	r1, buffer
				int		iGET_LINE
				move	buffer, r1
				cmp		r0, -1
				jump	eq, LOOP_END
				load	r0, buffer[0]
				add		count, r0
				add		count, 1
				jump	LOOP
			LOOP_END:
			
			// Close the file
				move	r0, 3
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
		assertEquals(7, diff.size());
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
				move	r0, 3
				move	r1, "src/test/resources/usconst_crlf.txt"
				int		iOPEN_FILE_READ
		
				move	buffer, 0
				move	count, 0
			LOOP:
				move	r0, 3
				move	r1, buffer
				int		iGET_LINE
				move	buffer, r1
				cmp		r0, -1
				jump	eq, LOOP_END
				load	r0, buffer[0]
				add		count, r0
				add		count, 1
				jump	LOOP
			LOOP_END:
			
			// Close the file
				move	r0, 3
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
		assertEquals(7, diff.size());
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
				move	r0, 3
				move	r1, "src/test/resources/usconst_lf.txt"
				int		iOPEN_FILE_READ
			// Open file for writing in text mode
				move	r0, 4
				move	r1, "usconst.txt"
				int		iOPEN_FILE_WRITE
	
				move	buffer, 0
				move	count, 0
			LOOP:
				move	r0, 3
				move	r1, buffer
				int		iGET_LINE
				move	buffer, r1
				cmp		r0, -1
				jump	eq, LOOP_END
				load	r0, buffer[0]
				move	r0, 4
				move	r1, buffer
				int		iPUT_LINE
				add		count, 1
				jump	LOOP
			LOOP_END:
			
			// Close the file
				move	r0, 3
				int		iCLOSE_FILE
				move	r0, 4
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
		assertEquals(7, diff.size());
		assertTrue(Files.mismatch(Path.of("usconst.txt"), Path.of("src/test/resources/usconst_lf.txt")) == -1);
		diff.assertDiff(0, 978);
	}

	@Test
	void testFileDelete() throws IOException {
		String src = """
			#include <system/io.def>
		
			MAIN:
				#var	FILENAME
				move	r0, "TEMP_"
				move	r1, ".TXT"
				int		iTEMP_FILE
				move	FILENAME, r0

			// Open file for writing in text mode
				move	r0, 3
				move	r1, FILENAME
				int		iOPEN_FILE_WRITE
				jump	z, END

			// Close the file
				move	r0, 3
				int		iCLOSE_FILE
				
			// Check to see if file exists
				move	r0, FILENAME
				int		iFILE_EXISTS
				jump	z, END
				
			// Check to see if file is a file
				move	r0, FILENAME
				int		iIS_FILE
				jump	z, END
				
			// Delete the file
				move	r0, FILENAME
				int		iDELETE_FILE
				move	r1, r0
				jump	z, END
				
			// Delete the file again
				move	r0, FILENAME
				int		iDELETE_FILE
				jump	nz, END

			// Check to see if file exists again
				move	r0, FILENAME
				int		iFILE_EXISTS
				jump	nz, END
				
			// Check to see if file is a file
				move	r0, FILENAME
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
		assertEquals(6, diff.size());
		diff.assertDiff(0, -2);
	}

	@Test
	void testFileDirectoryOps() throws IOException {
		String src = """
			#include <system/io.def>
		
			MAIN:
				#var	DIRNAME
			// Get Temp dir name
				move	r0, "TEST_"
				int		iTEMP_DIR
				move	DIRNAME, r0

			// Check dir exists
				move	r0, DIRNAME
				int		iIS_DIR
				jump	z, END
				
			// Make dir
				move	r0, DIRNAME2
				int		iMAKE_DIR
				jump	z, END
				
			// Make dir again
				move	r0, DIRNAME2
				int		iMAKE_DIR
				jump	nz, END
				
			// Check dir exists
				move	r0, DIRNAME2
				int		iIS_DIR
				jump	z, END
				
			// Remove dir
				move	r0, DIRNAME2
				int		iDELETE_DIR
				jump	z, END
				
			// Remove dir again
				move	r0, DIRNAME2
				int		iDELETE_DIR
				jump	nz, END
				
			// Check dir exists
				move	r0, DIRNAME2
				int		iIS_DIR
				jump	nz, END
					
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
		assertEquals(5, diff.size());
		diff.assertDiff(0, -2);
	}
}
