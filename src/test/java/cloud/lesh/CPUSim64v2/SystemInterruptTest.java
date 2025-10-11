package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.Comparator;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class SystemInterruptTest extends InterruptTest {
	@Test
	void testNumLimits() {
		String src = """
			START:
			#include <system/system.def>
				INT		iINT_MIN
				MOVE 	R1, R0
				INT		iINT_MAX
				MOVE 	R2, R0
				INT		iFLOAT_MIN
				MOVE 	F1, F0
				INT		iFLOAT_LOWEST
				MOVE 	F2, F0
				INT		iFLOAT_MAX
				MOVE 	F3, F0
				INT		iNEGATIVE_INFINITY
				MOVE 	F4, F0
				INT		iPOSITIVE_INFINITY
				MOVE 	F5, F0
				INT		iNAN
				MOVE 	F6, F0
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(12, diff.size());
		diff.assertDiff(1, Long.MIN_VALUE);
		diff.assertDiff(2, Long.MAX_VALUE);
		diff.assertDiff(1, -Double.MAX_VALUE);
		diff.assertDiff(2, Double.MIN_VALUE);
		diff.assertDiff(3, Double.MAX_VALUE);
		diff.assertDiff(4, Double.NEGATIVE_INFINITY);
		diff.assertDiff(5, Double.POSITIVE_INFINITY);
		diff.assertDiff(6, Double.NaN);
	}

	@Test
	void testSave() {
		String src = """
			START:
				#include <system/system.def>
				#DEF_MACRO SET(r,x)
					MOVE ${r}${x}, ${x}
				#END_MACRO
				#MACRO SET(R, 1)
				#MACRO SET(R, 2)
				#MACRO SET(R, 3)
				#MACRO SET(R, 4)
				#MACRO SET(R, 5)
				#MACRO SET(R, 6)
				#MACRO SET(R, 7)
				#MACRO SET(R, 8)
				#MACRO SET(R, 9)
				#MACRO SET(R, 10)
				#MACRO SET(R, 11)
				#MACRO SET(R, 12)
				#MACRO SET(R, 13)
				#MACRO SET(R, 14)
				#MACRO SET(R, 15)
				#MACRO SET(R, 16)
				#MACRO SET(R, 17)
				#MACRO SET(R, 18)
				#MACRO SET(R, 19)
				#MACRO SET(R, 20)
				#MACRO SET(R, 21)
				#MACRO SET(R, 22)
				#MACRO SET(R, 23)
				#MACRO SET(R, 24)
				#MACRO SET(R, 25)
				#MACRO SET(R, 26)
				#MACRO SET(R, 27)
				#MACRO SET(R, 28)
				#MACRO SET(F, 1)
				#MACRO SET(F, 2)
				#MACRO SET(F, 3)
				#MACRO SET(F, 4)
				#MACRO SET(F, 5)
				#MACRO SET(F, 6)
				#MACRO SET(F, 7)
				#MACRO SET(F, 8)
				#MACRO SET(F, 9)
				#MACRO SET(F, 10)
				#MACRO SET(F, 11)
				#MACRO SET(F, 12)
				#MACRO SET(F, 13)
				#MACRO SET(F, 14)
				#MACRO SET(F, 15)
				#MACRO SET(F, 16)
				#MACRO SET(F, 17)
				#MACRO SET(F, 18)
				#MACRO SET(F, 19)
				#MACRO SET(F, 20)
				#MACRO SET(F, 21)
				#MACRO SET(F, 22)
				#MACRO SET(F, 23)
				#MACRO SET(F, 24)
				#MACRO SET(F, 25)
				#MACRO SET(F, 26)
				#MACRO SET(F, 27)
				#MACRO SET(F, 28)
				#MACRO SET(F, 29)
				#MACRO SET(F, 30)
				#MACRO SET(F, 31)
				INT iSAVE
				INT iSAVE_FP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(62, diff.size());
		diff.assertDiff(1, 1);
		diff.assertDiff(2, 2);
		diff.assertDiff(3, 3);
		diff.assertDiff(4, 4);
		diff.assertDiff(5, 5);
		diff.assertDiff(6, 6);
		diff.assertDiff(7, 7);
		diff.assertDiff(8, 8);
		diff.assertDiff(9, 9);
		diff.assertDiff(10, 10);
		diff.assertDiff(11, 11);
		diff.assertDiff(12, 12);
		diff.assertDiff(13, 13);
		diff.assertDiff(14, 14);
		diff.assertDiff(15, 15);
		diff.assertDiff(16, 16);
		diff.assertDiff(17, 17);
		diff.assertDiff(18, 18);
		diff.assertDiff(19, 19);
		diff.assertDiff(20, 20);
		diff.assertDiff(21, 21);
		diff.assertDiff(22, 22);
		diff.assertDiff(23, 23);
		diff.assertDiff(24, 24);
		diff.assertDiff(25, 25);
		diff.assertDiff(26, 26);
		diff.assertDiff(27, 27);
		diff.assertDiff(28, 28);
		diff.assertDiff(1, 1.0);
		diff.assertDiff(2, 2.0);
		diff.assertDiff(3, 3.0);
		diff.assertDiff(4, 4.0);
		diff.assertDiff(5, 5.0);
		diff.assertDiff(6, 6.0);
		diff.assertDiff(7, 7.0);
		diff.assertDiff(8, 8.0);
		diff.assertDiff(9, 9.0);
		diff.assertDiff(10, 10.0);
		diff.assertDiff(11, 11.0);
		diff.assertDiff(12, 12.0);
		diff.assertDiff(13, 13.0);
		diff.assertDiff(14, 14.0);
		diff.assertDiff(15, 15.0);
		diff.assertDiff(16, 16.0);
		diff.assertDiff(17, 17.0);
		diff.assertDiff(18, 18.0);
		diff.assertDiff(19, 19.0);
		diff.assertDiff(20, 20.0);
		diff.assertDiff(21, 21.0);
		diff.assertDiff(22, 22.0);
		diff.assertDiff(23, 23.0);
		diff.assertDiff(24, 24.0);
		diff.assertDiff(25, 25.0);
		diff.assertDiff(26, 26.0);
		diff.assertDiff(27, 27.0);
		diff.assertDiff(28, 28.0);
		diff.assertDiff(29, 29.0);
		diff.assertDiff(30, 30.0);
		diff.assertDiff(31, 31.0);
		long p = sim.stackBase - 1;
		diff.assertMem(p--, 0);
		diff.assertMem(p--, 1);
		diff.assertMem(p--, 2);
		diff.assertMem(p--, 3);
		diff.assertMem(p--, 4);
		diff.assertMem(p--, 5);
		diff.assertMem(p--, 6);
		diff.assertMem(p--, 7);
		diff.assertMem(p--, 8);
		diff.assertMem(p--, 9);
		diff.assertMem(p--, 10);
		diff.assertMem(p--, 11);
		diff.assertMem(p--, 12);
		diff.assertMem(p--, 13);
		diff.assertMem(p--, 14);
		diff.assertMem(p--, 15);
		diff.assertMem(p--, 16);
		diff.assertMem(p--, 17);
		diff.assertMem(p--, 18);
		diff.assertMem(p--, 19);
		diff.assertMem(p--, 20);
		diff.assertMem(p--, 21);
		diff.assertMem(p--, 22);
		diff.assertMem(p--, 23);
		diff.assertMem(p--, 24);
		diff.assertMem(p--, 25);
		diff.assertMem(p--, 26);
		diff.assertMem(p--, 27);
		diff.assertMem(p--, 28);
		diff.assertMem(p--, 0.0);
		diff.assertMem(p--, 1.0);
		diff.assertMem(p--, 2.0);
		diff.assertMem(p--, 3.0);
		diff.assertMem(p--, 4.0);
		diff.assertMem(p--, 5.0);
		diff.assertMem(p--, 6.0);
		diff.assertMem(p--, 7.0);
		diff.assertMem(p--, 8.0);
		diff.assertMem(p--, 9.0);
		diff.assertMem(p--, 10.0);
		diff.assertMem(p--, 11.0);
		diff.assertMem(p--, 12.0);
		diff.assertMem(p--, 13.0);
		diff.assertMem(p--, 14.0);
		diff.assertMem(p--, 15.0);
		diff.assertMem(p--, 16.0);
		diff.assertMem(p--, 17.0);
		diff.assertMem(p--, 18.0);
		diff.assertMem(p--, 19.0);
		diff.assertMem(p--, 20.0);
		diff.assertMem(p--, 21.0);
		diff.assertMem(p--, 22.0);
		diff.assertMem(p--, 23.0);
		diff.assertMem(p--, 24.0);
		diff.assertMem(p--, 25.0);
		diff.assertMem(p--, 26.0);
		diff.assertMem(p--, 27.0);
		diff.assertMem(p--, 28.0);
		diff.assertMem(p--, 29.0);
		diff.assertMem(p--, 30.0);
		diff.assertMem(p--, 31.0);
	}

	@Test
	void testRestore() {
		String src = """
			START:
				#include <system/system.def>
				#DEF_MACRO SET(r,x)
					MOVE ${r}${x}, ${x}
				#END_MACRO
				#DEF_MACRO TEST(reg)
					TEST ${reg}
					JUMP NZ, FINIS
				#END_MACRO
				#MACRO SET(R, 1)
				#MACRO SET(R, 2)
				#MACRO SET(R, 3)
				#MACRO SET(R, 4)
				#MACRO SET(R, 5)
				#MACRO SET(R, 6)
				#MACRO SET(R, 7)
				#MACRO SET(R, 8)
				#MACRO SET(R, 9)
				#MACRO SET(R, 10)
				#MACRO SET(R, 11)
				#MACRO SET(R, 12)
				#MACRO SET(R, 13)
				#MACRO SET(R, 14)
				#MACRO SET(R, 15)
				#MACRO SET(R, 16)
				#MACRO SET(R, 17)
				#MACRO SET(R, 18)
				#MACRO SET(R, 19)
				#MACRO SET(R, 20)
				#MACRO SET(R, 21)
				#MACRO SET(R, 22)
				#MACRO SET(R, 23)
				#MACRO SET(R, 24)
				#MACRO SET(R, 25)
				#MACRO SET(R, 26)
				#MACRO SET(R, 27)
				#MACRO SET(R, 28)
				#MACRO SET(F, 1)
				#MACRO SET(F, 2)
				#MACRO SET(F, 3)
				#MACRO SET(F, 4)
				#MACRO SET(F, 5)
				#MACRO SET(F, 6)
				#MACRO SET(F, 7)
				#MACRO SET(F, 8)
				#MACRO SET(F, 9)
				#MACRO SET(F, 10)
				#MACRO SET(F, 11)
				#MACRO SET(F, 12)
				#MACRO SET(F, 13)
				#MACRO SET(F, 14)
				#MACRO SET(F, 15)
				#MACRO SET(F, 16)
				#MACRO SET(F, 17)
				#MACRO SET(F, 18)
				#MACRO SET(F, 19)
				#MACRO SET(F, 20)
				#MACRO SET(F, 21)
				#MACRO SET(F, 22)
				#MACRO SET(F, 23)
				#MACRO SET(F, 24)
				#MACRO SET(F, 25)
				#MACRO SET(F, 26)
				#MACRO SET(F, 27)
				#MACRO SET(F, 28)
				#MACRO SET(F, 29)
				#MACRO SET(F, 30)
				#MACRO SET(F, 31)
				INT iSAVE
				INT iSAVE_FP
				clear
				#MACRO TEST(R0)
				#MACRO TEST(R1)
				#MACRO TEST(R2)
				#MACRO TEST(R3)
				#MACRO TEST(R4)
				#MACRO TEST(R5)
				#MACRO TEST(R6)
				#MACRO TEST(R7)
				#MACRO TEST(R8)
				#MACRO TEST(R9)
				#MACRO TEST(R10)
				#MACRO TEST(R11)
				#MACRO TEST(R12)
				#MACRO TEST(R13)
				#MACRO TEST(R14)
				#MACRO TEST(R15)
				#MACRO TEST(R16)
				#MACRO TEST(R17)
				#MACRO TEST(R18)
				#MACRO TEST(R19)
				#MACRO TEST(R20)
				#MACRO TEST(R21)
				#MACRO TEST(R22)
				#MACRO TEST(R23)
				#MACRO TEST(R24)
				#MACRO TEST(R25)
				#MACRO TEST(R26)
				#MACRO TEST(R27)
				#MACRO TEST(R28)
				#MACRO TEST(F0)
				#MACRO TEST(F1)
				#MACRO TEST(F2)
				#MACRO TEST(F3)
				#MACRO TEST(F4)
				#MACRO TEST(F5)
				#MACRO TEST(F6)
				#MACRO TEST(F7)
				#MACRO TEST(F8)
				#MACRO TEST(F9)
				#MACRO TEST(F10)
				#MACRO TEST(F11)
				#MACRO TEST(F12)
				#MACRO TEST(F13)
				#MACRO TEST(F14)
				#MACRO TEST(F15)
				#MACRO TEST(F16)
				#MACRO TEST(F17)
				#MACRO TEST(F18)
				#MACRO TEST(F19)
				#MACRO TEST(F20)
				#MACRO TEST(F21)
				#MACRO TEST(F22)
				#MACRO TEST(F23)
				#MACRO TEST(F24)
				#MACRO TEST(F25)
				#MACRO TEST(F26)
				#MACRO TEST(F27)
				#MACRO TEST(F28)
				#MACRO TEST(F29)
				#MACRO TEST(F30)
				#MACRO TEST(F31)
				INT iRESTORE_FP
				INT iRESTORE
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(61, diff.size());
		diff.assertDiff(1, 1);
		diff.assertDiff(2, 2);
		diff.assertDiff(3, 3);
		diff.assertDiff(4, 4);
		diff.assertDiff(5, 5);
		diff.assertDiff(6, 6);
		diff.assertDiff(7, 7);
		diff.assertDiff(8, 8);
		diff.assertDiff(9, 9);
		diff.assertDiff(10, 10);
		diff.assertDiff(11, 11);
		diff.assertDiff(12, 12);
		diff.assertDiff(13, 13);
		diff.assertDiff(14, 14);
		diff.assertDiff(15, 15);
		diff.assertDiff(16, 16);
		diff.assertDiff(17, 17);
		diff.assertDiff(18, 18);
		diff.assertDiff(19, 19);
		diff.assertDiff(20, 20);
		diff.assertDiff(21, 21);
		diff.assertDiff(22, 22);
		diff.assertDiff(23, 23);
		diff.assertDiff(24, 24);
		diff.assertDiff(25, 25);
		diff.assertDiff(26, 26);
		diff.assertDiff(27, 27);
		diff.assertDiff(28, 28);
		diff.assertDiff(1, 1.0);
		diff.assertDiff(2, 2.0);
		diff.assertDiff(3, 3.0);
		diff.assertDiff(4, 4.0);
		diff.assertDiff(5, 5.0);
		diff.assertDiff(6, 6.0);
		diff.assertDiff(7, 7.0);
		diff.assertDiff(8, 8.0);
		diff.assertDiff(9, 9.0);
		diff.assertDiff(10, 10.0);
		diff.assertDiff(11, 11.0);
		diff.assertDiff(12, 12.0);
		diff.assertDiff(13, 13.0);
		diff.assertDiff(14, 14.0);
		diff.assertDiff(15, 15.0);
		diff.assertDiff(16, 16.0);
		diff.assertDiff(17, 17.0);
		diff.assertDiff(18, 18.0);
		diff.assertDiff(19, 19.0);
		diff.assertDiff(20, 20.0);
		diff.assertDiff(21, 21.0);
		diff.assertDiff(22, 22.0);
		diff.assertDiff(23, 23.0);
		diff.assertDiff(24, 24.0);
		diff.assertDiff(25, 25.0);
		diff.assertDiff(26, 26.0);
		diff.assertDiff(27, 27.0);
		diff.assertDiff(28, 28.0);
		diff.assertDiff(29, 29.0);
		diff.assertDiff(30, 30.0);
		diff.assertDiff(31, 31.0);
		assertEquals(sim.stackBase - 1, sim.getR(sim.R_SP));
	}

	@Test
	void testSystem() {
		String src = """
			START:
				#include <system/system.def>
				INT iARGC
				MOVE R28, R0
				MOVE R0, 2
				INT iARGS
				MOVE R27, R0
				MOVE R0, 1
				INT iARGS
				MOVE R26, R0 
				INT iGET_PID
				MOVE R1, R0
				INT iGET_NUM_CORES
				MOVE R2, R0
				INT	 iCYCLES
				MOVE R3, R0
				MOVE R0, 500
				INT iSLEEP
				INT	iCLOCK
				MOVE R4, R0
				MOVE R0, "src/test/resources/sum.sh 4 5"
				INT iSYSTEM
				INT iEXIT
				STOP
			FINIS:
			""";
		String[] args = {"test", "abc", "defghijklmn"};
		var tuple = runProgram(src, args);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		// assertEquals(9, diff.size());   // can be 9 or 10 depending on if PID is 0
		assertEquals(9, result);
		diff.assertDiff(0, 9);
		diff.assertDiff(28, 3);
		assertEquals("defghijklmn", sim.convertString(sim.getR(27)));
		assertEquals("abc", sim.convertString(sim.getR(26)));

		// diff.assertDiff(1, sim.getPID());
		diff.assertDiff(2, Runtime.getRuntime().availableProcessors());
		diff.assertDiff(3, 53);
		assertTrue(500000000 < diff.getReg(4));
	}

	@Test
	void testPrintCPUState() {
		String src = """
			START:
				#include <system/system.def>
				#DEF_MACRO SET(r,x)
					MOVE ${r}${x}, ${x}
				#END_MACRO
				#MACRO SET(R, 1)
				#MACRO SET(R, 2)
				#MACRO SET(R, 3)
				#MACRO SET(R, 4)
				#MACRO SET(R, 5)
				#MACRO SET(R, 6)
				#MACRO SET(R, 7)
				#MACRO SET(R, 8)
				#MACRO SET(R, 9)
				#MACRO SET(R, 10)
				#MACRO SET(R, 11)
				#MACRO SET(R, 12)
				#MACRO SET(R, 13)
				#MACRO SET(R, 14)
				#MACRO SET(R, 15)
				#MACRO SET(R, 16)
				#MACRO SET(R, 17)
				#MACRO SET(R, 18)
				#MACRO SET(R, 19)
				#MACRO SET(R, 20)
				#MACRO SET(R, 21)
				#MACRO SET(R, 22)
				#MACRO SET(R, 23)
				#MACRO SET(R, 24)
				#MACRO SET(R, 25)
				#MACRO SET(R, 26)
				#MACRO SET(R, 27)
				#MACRO SET(R, 28)
				#MACRO SET(F, 1)
				#MACRO SET(F, 2)
				#MACRO SET(F, 3)
				#MACRO SET(F, 4)
				#MACRO SET(F, 5)
				#MACRO SET(F, 6)
				#MACRO SET(F, 7)
				#MACRO SET(F, 8)
				#MACRO SET(F, 9)
				#MACRO SET(F, 10)
				#MACRO SET(F, 11)
				#MACRO SET(F, 12)
				#MACRO SET(F, 13)
				#MACRO SET(F, 14)
				#MACRO SET(F, 15)
				#MACRO SET(F, 16)
				#MACRO SET(F, 17)
				#MACRO SET(F, 18)
				#MACRO SET(F, 19)
				#MACRO SET(F, 20)
				#MACRO SET(F, 21)
				#MACRO SET(F, 22)
				#MACRO SET(F, 23)
				#MACRO SET(F, 24)
				#MACRO SET(F, 25)
				#MACRO SET(F, 26)
				#MACRO SET(F, 27)
				#MACRO SET(F, 28)
				#MACRO SET(F, 29)
				#MACRO SET(F, 30)
				#MACRO SET(F, 31)
				PUSH R20
				PUSH F20
				INT iPrintCPUState
				STOP
			FINIS:
			""";
		String expected = """

CPU State    SR: oszP
  R               hex                   dec  FP                    float   Stack               hex                   dec                   float
 R0: 0000000000000000                     0  F0:       0.000000000000000 =>SP+02: 0000000000000014                    20  9.900000000000000e-323
 R1: 0000000000000001                     1  F1:       1.000000000000000   SP+01: 4034000000000000   4626322717216342016       20.00000000000000
 R2: 0000000000000002                     2  F2:       2.000000000000000
 R3: 0000000000000003                     3  F3:       3.000000000000000
 R4: 0000000000000004                     4  F4:       4.000000000000000
 R5: 0000000000000005                     5  F5:       5.000000000000000
 R6: 0000000000000006                     6  F6:       6.000000000000000
 R7: 0000000000000007                     7  F7:       7.000000000000000
 R8: 0000000000000008                     8  F8:       8.000000000000000
 R9: 0000000000000009                     9  F9:       9.000000000000000
R10: 000000000000000a                    10 F10:       10.00000000000000
R11: 000000000000000b                    11 F11:       11.00000000000000
R12: 000000000000000c                    12 F12:       12.00000000000000
R13: 000000000000000d                    13 F13:       13.00000000000000
R14: 000000000000000e                    14 F14:       14.00000000000000
R15: 000000000000000f                    15 F15:       15.00000000000000
R16: 0000000000000010                    16 F16:       16.00000000000000
R17: 0000000000000011                    17 F17:       17.00000000000000
R18: 0000000000000012                    18 F18:       18.00000000000000
R19: 0000000000000013                    19 F19:       19.00000000000000
R20: 0000000000000014                    20 F20:       20.00000000000000
R21: 0000000000000015                    21 F21:       21.00000000000000
R22: 0000000000000016                    22 F22:       22.00000000000000
R23: 0000000000000017                    23 F23:       23.00000000000000
R24: 0000000000000018                    24 F24:       24.00000000000000
R25: 0000000000000019                    25 F25:       25.00000000000000
R26: 000000000000001a                    26 F26:       26.00000000000000
R27: 000000000000001b                    27 F27:       27.00000000000000
R28: 000000000000001c                    28 F28:       28.00000000000000
 SF: 000000000000044b                       F29:       29.00000000000000
 SP: 0000000000000449                       F30:       30.00000000000000
 PC: 000000000000003e                       F31:       31.00000000000000
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(62, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testAllocFree0() {
		String src = """
			START:
				#include <system/system.def>
				MOVE R0, 100
				INT iALLOC
				MOVE R1, R0
				MOVE R0, 200
				INT iALLOC
				MOVE R2, R0
				MOVE R0, 300
				INT iALLOC
				MOVE R3, R0
				MOVE R0, 400
				INT iALLOC
				INT iALLOC_COUNT
				MOVE R6, R0
				INT iFREE_COUNT
				MOVE R7, R0
				INT iALLOC_SIZE
				MOVE R8, R0
				INT iFREE_SIZE
				MOVE R9, R0
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		// diff.assertDiff(0, 0);
		diff.assertDiff(1, sim.heapStart + 3);
		diff.assertDiff(2, sim.heapStart + 144 + 3);
		diff.assertDiff(3, sim.heapStart + 233 + 144 + 3);
		diff.assertDiff(6, 3);		// Alloc count
		diff.assertDiff(7, 1);		// Free count
		diff.assertDiff(8, 754);	// Alloc size
		diff.assertDiff(9, sim.heapLimit - sim.heapStart - sim.getR(8));	// Free size
	}

	// Three allocs, free, then alloc same size
	@Test
	void testAllocFree1() {
		String src = """
			START:
				#include <system/system.def>
				MOVE R0, 100
				INT iALLOC
				MOVE R1, R0
				MOVE R0, 200
				INT iALLOC
				MOVE R2, R0
				MOVE R0, 300
				INT iALLOC
				MOVE R3, R0
				MOVE R0, R2
				INT iFREE
				MOVE R0, 200
				INT iALLOC
				MOVE R2, R0
				INT iALLOC_COUNT
				MOVE R6, R0
				INT iFREE_COUNT
				MOVE R7, R0
				INT iALLOC_SIZE
				MOVE R8, R0
				INT iFREE_SIZE
				MOVE R9, R0
			STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		// diff.assertDiff(0, 0);
		diff.assertDiff(1, sim.heapStart + 3);
		diff.assertDiff(2, sim.heapStart + 144 + 3);
		diff.assertDiff(3, sim.heapStart + 233 + 144 + 3);
		diff.assertDiff(6, 3);		// Alloc count
		diff.assertDiff(7, 1);		// Free count
		diff.assertDiff(8, 754);	// Alloc size
		diff.assertDiff(9, sim.heapLimit - sim.heapStart - sim.getR(8));	// Free size
	}

	// Five allocs, five frees
	@Test
	void testAllocFree2() {
		String src = """
			START:
				#include <system/system.def>
				MOVE R0, 15
				INT iALLOC
				MOVE R1, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R2, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R3, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R4, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R5, R0
				MOVE R0, R3
				INT iFREE
				MOVE R0, R2
				INT iFREE
				MOVE R0, R4
				INT iFREE
				MOVE R0, R1
				INT iFREE
				MOVE R0, R5
				INT iFREE
				INT iALLOC_COUNT
				MOVE R6, R0
				INT iFREE_COUNT
				MOVE R7, R0
				INT iALLOC_SIZE
				MOVE R8, R0
				INT iFREE_SIZE
				MOVE R9, R0
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		// diff.assertDiff(0, 0);
		diff.assertDiff(1, sim.heapStart + 3);
		// diff.assertDiff(6, 0);	// Alloc count
		diff.assertDiff(7, 1);		// Free count
		// diff.assertDiff(8, 0);	// Alloc size
		diff.assertDiff(9, sim.heapLimit - sim.heapStart - sim.getR(8));	// Free size
	}

	// three allocs, realloc same size
	@Test
	void testRealloc0() {
		String src = """
			START:
				#include <system/system.def>
				MOVE R0, 15
				INT iALLOC
				MOVE R1, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R2, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R3, R0

				STORE 1, R2
				STORE 2, R2 + 1
				STORE 3, R2 + 2
				STORE 99, R2 + 14
				MOVE R0, R2
				MOVE R1, 16
				INT iREALLOC
				MOVE R2, R0
				
				INT iALLOC_COUNT
				MOVE R6, R0
				INT iFREE_COUNT
				MOVE R7, R0
				INT iALLOC_SIZE
				MOVE R8, R0
				INT iFREE_SIZE
				MOVE R9, R0
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		// diff.assertDiff(0, 0);
		diff.assertDiff(6, 3);		// Alloc count
		diff.assertDiff(7, 1);		// Free count
		diff.assertDiff(8, 63);		// Alloc size
		diff.assertDiff(9, sim.heapLimit - sim.heapStart - sim.getR(8));	// Free size
		long r2 = sim.getR(2);
		diff.assertMem(r2 - 1, 21);
		diff.assertMem(r2, 1);
		diff.assertMem(r2 + 1, 2);
		diff.assertMem(r2 + 2, 3);
		diff.assertMem(r2 + 14, 99);
	}

	// three allocs, realloc larger size
	@Test
	void testRealloc1() {
		String src = """
			START:
				#include <system/system.def>
				MOVE R0, 15
				INT iALLOC
				MOVE R1, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R2, R0
				MOVE R0, 15
				INT iALLOC
				MOVE R3, R0

				STORE 1, R2
				STORE 2, R2 + 1
				STORE 3, R2 + 2
				STORE 99, R2 + 14
				MOVE R0, R2
				MOVE R1, 100
				INT iREALLOC
				MOVE R2, R0
				
				INT iALLOC_COUNT
				MOVE R6, R0
				INT iFREE_COUNT
				MOVE R7, R0
				INT iALLOC_SIZE
				MOVE R8, R0
				INT iFREE_SIZE
				MOVE R9, R0
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		// diff.assertDiff(0, 0);
		diff.assertDiff(6, 3);		// Alloc count
		diff.assertDiff(7, 2);		// Free count
		diff.assertDiff(8, 186);	// Alloc size
		diff.assertDiff(9, sim.heapLimit - sim.heapStart - sim.getR(8));	// Free size
		long r2 = sim.getR(2);
		diff.assertMem(r2 - 1, 144);
		diff.assertMem(r2, 1);
		diff.assertMem(r2 + 1, 2);
		diff.assertMem(r2 + 2, 3);
		diff.assertMem(r2 + 14, 99);
	}

	// three allocs, realloc smaller size
	@Test
	void testRealloc2() {
		String src = """
			START:
				#include <system/system.def>
				MOVE R0, 30
				INT iALLOC
				MOVE R1, R0
				MOVE R0, 30
				INT iALLOC
				MOVE R2, R0
				MOVE R0, 30
				INT iALLOC
				MOVE R3, R0

				STORE 1, R2
				STORE 2, R2 + 1
				STORE 3, R2 + 2
				STORE 99, R2 + 29
				MOVE R0, R2
				MOVE R1, 10
				INT iREALLOC
				MOVE R2, R0
				
				INT iALLOC_COUNT
				MOVE R6, R0
				INT iFREE_COUNT
				MOVE R7, R0
				INT iALLOC_SIZE
				MOVE R8, R0
				INT iFREE_SIZE
				MOVE R9, R0
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		// diff.assertDiff(0, 0);
		diff.assertDiff(6, 3);		// Alloc count
		diff.assertDiff(7, 2);		// Free count
		diff.assertDiff(8, 81);		// Alloc size
		diff.assertDiff(9, sim.heapLimit - sim.heapStart - sim.getR(8));	// Free size
		long r2 = sim.getR(2);
		diff.assertMem(r2 - 1, 13);
		diff.assertMem(r2, 1);
		diff.assertMem(r2 + 1, 2);
		diff.assertMem(r2 + 2, 3);
	}

	@Test
	void testMemMove() {
		String src = """
			START:
				#include <system/system.def>
				#DEFINE COUNT 100
				MOVE R0, COUNT
				ADD R0, 1
				INT iALLOC				// Allocate COUNT + 1 words
				MOVE R28, R0			// Save src block in R28
				MOVE R0, COUNT
				ADD R0, 1
				INT iALLOC				// Allocate COUNT + 1 words
				MOVE R27, R0			// Save dest block in R27
				
				MOVE R1, R28
				MOVE R0, COUNT
			LOOP_START:
				JUMP Z, LOOP_END		// End we we get to 0
				STORE R0, R1			// Store the loop counter in block
				ADD R1, 1
				SUB R0, 1
				JUMP LOOP_START
			LOOP_END:
				STORE -1, R1			// Add a sentinel of -1 at end of src
				MOVE R0, R27 + COUNT
				STORE -2, R0			// Add a sentinel of -2 at end of dest
				
				MOVE R0, R27
				MOVE R1, R28
				MOVE R2, COUNT
				INT iMEMMOVE			// Move data from src to dest but not sentinel
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		long p = sim.getR(27);
		for (int i = 100; i > 0; --i) {
			diff.assertMem(p, i);
			++p;
		}
		diff.assertMem(p, -2);
	}

	@Test
	void testMemClear() {
		String src = """
			START:
				#include <system/system.def>
				#DEFINE COUNT 100
				MOVE R0, COUNT
				ADD R0, 1
				INT iALLOC				// Allocate COUNT + 1 words
				MOVE R28, R0			// Save block in R28

				MOVE R1, R28			// Pointer into block
				MOVE R0, COUNT			// Loop counter
			LOOP_START:
				JUMP Z, LOOP_END		// End we we get to 0
				STORE R0, R1			// Store the loop counter in block
				ADD R1, 1
				SUB R0, 1
				JUMP LOOP_START
			LOOP_END:
				STORE -1, R1			// Add a sentinel of -1 at end
				
				MOVE R0, R28
				MOVE R1, COUNT
				INT iMEMCLEAR			// Clear the block but not sentinel
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		long p = sim.getR(28);
		for (int i = 100; i > 0; --i) {
			diff.assertMem(p, 0);
			++p;
		}
		diff.assertMem(p, -1);
	}

	@Disabled
	@Test
	void testFork() {
		String src = """
			START:
			#include <system/system.def>
			#include <system/io.def>
			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
				int iPUT_DEC
				int iPUT_NL
			#end_macro
			MAIN:
				#var	child_pid
				int		iFORK
				move	child_pid, r0
				cmp		child_pid, -1
				jump	eq, @FORK_FAILED
				test	child_pid
				jump	z, @CHILD_FORK
				#macro	put_dec(child_pid)
				#macro	SLEEP(1000)
				move	r0, child_pid
				int		iWAIT_PID
				move	r0, child_pid
				neg		r0
				#macro	put_dec(r0)
				jump	@END
			CHILD_FORK:
				#macro	put_dec(1000)
				int		iGET_PID
				#macro	put_dec(r0)
				#macro	SLEEP(1000)
				#macro	put_dec(-1000)
				jump	@END
			FORK_FAILED:
				#macro	put_dec(-999)
			END:
				stop
				stop
			FINIS:
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		Arrays.sort(lines, Comparator.comparingInt(Integer::parseInt));		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(6, diff.size());
		assertTrue("-1000".equals(lines[0]) || "-1000".equals(lines[1]));
		assertTrue(1000 > Integer.parseInt(lines[2]));
		assertEquals(Integer.parseInt(lines[3]), Integer.parseInt(lines[2]));
		assertEquals("1000", lines[4]);
	}

	/* This test will create 7 children
		  f3
		|     \
				f4
				|  \
				f5  f5
				| \  |  \
		 f4
		|   \
			 f5
			 |  \
		f5
		|  \
	 */

	@Test
	void testWait() {
		String src = """
			START:
			#include <system/system.def>
			#include <system/io.def>
			#define MS 000
			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
				int iPUT_DEC
				int iPUT_NL
			#end_macro
			#def_macro fork_child(i)
				int		iFORK
				move	r${i}, r0
				cmp		r${i}, -1
				jump	eq, @FORK_FAILED_${i}
				test	r${i}
				jump	z, @CHILD_FORK_${i}
				jump	@END_MACRO_${i}
			CHILD_FORK_${i}:
				int		iGET_PID
				move	r1, r0
				move	r0, 1
				int		iPUT_DEC
				int		iPUT_NL
				move	r0, ${i}000
				int		iSLEEP
				jump	@END_MACRO_${i}
			FORK_FAILED_${i}:
				move	r1, -999
				move	r1, 1
				int		iPUT_DEC
				int		iPUT_NL
			END_MACRO_${i}:
			#end_macro
			MAIN:
				#macro fork_child(3)
				#macro fork_child(4)
				#macro fork_child(5)
			END:
				int iWAIT
				stop
				stop
			FINIS:
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(6, diff.size());
		assertEquals(7, lines.length);
	}

	@Disabled
	@Test
	void testThread() {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			
			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
				int iPUT_DEC
				int	iPUT_NL
			#end_macro
			
				#call	main()
				move	r0, 0
				int		iEXIT
			
			PIDS: .dca	3
			#def_func	MAIN()
				#var	pid, i
				move	r0, run
				move	r1, 1
				int		iTHREAD
				store	r0, PIDS[1]
				move	r0, run
				move	r1, 2
				int		iTHREAD
				store	r0, PIDS[2]
				move	r0, run
				move	r1, 3
				int		iTHREAD
				store	r0, PIDS[3]
				load	r0, PIDS[1]
				int		iJOIN_THREAD
				move	r0, -1
				#macro	put_dec(r0)
				load	r0, PIDS[2]
				int		iJOIN_THREAD
				move	r0, -2
				#macro	put_dec(r0)
				load	r0, PIDS[3]
				int		iJOIN_THREAD
				move	r0, -3
				#macro	put_dec(r0)
			#end_func
			
			#def_func run(data)
				#var	d
				load	d, data
				#macro	put_dec(d)
				move	r0, d
				mult	r0, 1000				
				#macro	SLEEP(r0)
				move	r0, d
				#macro	put_dec(d)
			#end_func
				stop
				stop
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		Arrays.sort(lines, Comparator.comparingInt(Integer::parseInt));		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals("-3", lines[0]);
		assertEquals("-2", lines[1]);
		assertEquals("-1", lines[2]);
		assertEquals("1", lines[3]);
		assertEquals("1", lines[4]);
		assertEquals("2", lines[5]);
		assertEquals("2", lines[6]);
		assertEquals("3", lines[7]);
		assertEquals("3", lines[8]);
	}
}