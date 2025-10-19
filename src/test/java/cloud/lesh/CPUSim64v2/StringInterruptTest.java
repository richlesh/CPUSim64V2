package cloud.lesh.CPUSim64v2;

import org.apache.commons.lang3.tuple.Triple;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class StringInterruptTest {
	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src) {
		String[] args = {"test"};
		return runProgram(src, args);
	}

	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src, String[] args) {
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);

		LiteralRewriter rewriter = new LiteralRewriter();
		String rewritten = rewriter.rewrite(preprocessed);

		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(rewritten);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> prog = asm.result();
		Simulator sim = new Simulator(1000, args);
		sim.clearCPUState();
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		int result = sim.run();
		var diff = new SimStateDiff(sim, startState);
		return Triple.of(result, sim, diff);
	}

	@Test
	void testNumberFormat() {
		String src = """
			START:
				#include <system/string.def>
				move	R0, 3261963
				int		iFMT_DEC
				move	R28, R0
				move	R1, 16
				move	R0, 3261963
				int		iFMT_HEX
				move	R27, R0
				move	R0, 7
				load	F0, 3.1415926
				int		iFMT_FLOAT
				move	R26, R0
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		assertEquals("3261963", sim.convertString(sim.getR(28)));
		assertEquals("000000000031C60B", sim.convertString(sim.getR(27)));
		assertEquals("3.1415926", sim.convertString(sim.getR(26)));
	}

	@Test
	void testParse() {
		String src = """
			START:
				#include <system/string.def>
				move	r0, "3261963"
				int		iPARSE_INT
				move	r28, r0
				move	r0, "0x123456"
				int		iPARSE_INT
				move	r27, r0
				move	r0, "01234"
				int		iPARSE_INT
				move	r26, r0
				move	r0, "123456"
				int		iPARSE_DEC
				move	r25, r0
				move	r0, "123456"
				int		iPARSE_HEX
				move	r24, r0
				move	r0, "3.1415926"
				int		iPARSE_FLOAT
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(9, diff.size());
		diff.assertDiff(28, 3261963);
		diff.assertDiff(27, 1193046);
		diff.assertDiff(26, 668);
		diff.assertDiff(25, 123456);
		diff.assertDiff(24, 1193046);
		diff.assertDiff(0, 3.1415926);
	}

	@Test
	void testFormat() {
		String src = """
			START:
				#include <system/string.def>				
				move	r1, 326
				move	r2, "Héllø"
				load	f0, 3.1415926
				move	r3, '😀'
				#call	sprintf("%d %x %s %.2f %c", r1, r1, r2, f0, r3)
				move	r28, r0
				
				move	r3, "Rich"
				move	r4, "!"
				move	r5, ", "
				#call	format("<{}{}{}{}>", r2, r5, r3, r4)
				move	r27, r0
				#call	format("<{0}{3}{1}{2}>", r2, r3, r4, r5)
				move	r26, r0
				STOP
				#def_func	sprintf(fmt, values)
				 	int		iSPRINTF
				 #end_func sprintf
				#def_func	format(fmt, values)
				 	int		iFORMAT
				 #end_func format
				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(12, diff.size());
		assertEquals("326 146 Héllø 3.14 😀", sim.convertString(sim.getR(28)));
		assertEquals("<Héllø, Rich!>", sim.convertString(sim.getR(26)));
		assertEquals("<Héllø, Rich!>", sim.convertString(sim.getR(27)));
	}

	@Test
	void testConvertCase() {
		String src = """
			START:
				#include <system/string.def>
				move	r1, 'A';
				move	r2, 'a';
				move	r3, "AbCdEf123!"
				
				move	r0, r1
				int		iTO_LOWER
				move	r28, r0
				move	r0, r1
				int		iTO_UPPER
				move	r27, r0
				
				move	r0, r2
				int		iTO_LOWER
				move	r26, r0
				move	r0, r2
				int		iTO_UPPER
				move	r25, r0
				
				move	r0, r3
				int		iTO_LOWER_STR
				move	r24, r0
				move	r0, r3
				int		iTO_UPPER_STR
				move	r23, r0
				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(12, diff.size());
		diff.assertDiff(28, 'a');
		diff.assertDiff(27, 'A');
		diff.assertDiff(26, 'a');
		diff.assertDiff(25, 'A');
		assertEquals("abcdef123!", sim.convertString(sim.getR(24)));
		assertEquals("ABCDEF123!", sim.convertString(sim.getR(23)));
	}

	@Test
	void testSTRCMP() {
		String src = """
			START:
				#include <system/string.def>
				move	r28, "ABC"
				move	r27, "abc"

				move	r0, r28
				move	r1, r28
				int		iSTRCMP
				move	r26, r0
				
				move	r0, r28
				move	r1, r27
				int		iSTRCMP
				move	r25, r0
				
				move	r0, r27
				move	r1, r28
				int		iSTRCMP
				move	r24, r0
				
				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		assertTrue(sim.getR(26) == 0);
		assertTrue(sim.getR(25) < 0);
		assertTrue(sim.getR(24) > 0);
	}

	@Test
	void testSTRICMP() {
		String src = """
			START:
				#include <system/string.def>
				move	r28, "DEF"
				move	r27, "def"
				move	r26, "abc"

				move	r0, r28
				move	r1, r28
				int		iSTRICMP
				move	r25, r0
				
				move	r0, r28
				move	r1, r27
				int		iSTRICMP
				move	r24, r0
				
				move	r0, r27
				move	r1, r28
				int		iSTRICMP
				move	r23, r0
				
				move	r0, r26
				move	r1, r28
				int		iSTRICMP
				move	r22, r0
				
				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		assertTrue(sim.getR(25) == 0);
		assertTrue(sim.getR(24) == 0);
		assertTrue(sim.getR(23) == 0);
		assertTrue(sim.getR(22) < 0);
	}

	@Test
	void testSubstring() {
		String src = """
			START:
				#include <system/string.def>
				move	r28, "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
				move	r0, r28
				move	r1, 5
				move	r2, 6
				int		iSUBSTRING
				move	r27, r0
				move	r0, r28
				move	r1, 24
				move	r2, 10
				int		iSUBSTRING
				move	r26, r0
				
				move	r0, r28
				move	r1, 5
				int		iPREFIX
				move	r25, r0
				
				move	r0, R28
				move	r1, 4
				int		iSUFFIX
				move	r24, r0
				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(10, diff.size());
		assertEquals("FGHIJK", sim.convertString(sim.getR(27)));
		assertEquals("YZ", sim.convertString(sim.getR(26)));
		assertEquals("ABCDE", sim.convertString(sim.getR(25)));
		assertEquals("WXYZ", sim.convertString(sim.getR(24)));
	}

	@Test
	void testCharSearch() {
		String src = """
			START:
				#include <system/string.def>
				move	r28, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
				move	r0, r28
				move	r1, 'J'
				move	r2, 0
				int		iCHAR_SEARCH
				move	r27, r0
				move	r0, r28
				move	r2, 20
				int		iCHAR_SEARCH
				move	r26, r0
				move	r0, r28
				move	r2, 60
				int		iCHAR_SEARCH
				move	r25, r0
				move	r0, r28
				move	r1, '1'
				move	r2, 0
				int		iCHAR_SEARCH
				move	r24, r0

				move	r0, r28
				move	r1, 'J'
				move	r2, 100
				int		iLAST_CHAR_SEARCH
				move	r23, r0
				move	r0, r28
				move	r2, 20
				int		iLAST_CHAR_SEARCH
				move	r22, r0
				move	r0, r28
				move	r2, 5
				int		iLAST_CHAR_SEARCH
				move	r21, r0
				move	r0, r28
				move	r1, '1'
				move	r2, 100
				int		iLAST_CHAR_SEARCH
				move	r20, r0

				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(14, diff.size());
		diff.assertDiff(27, 9);
		diff.assertDiff(26, 26 + 9);
		diff.assertDiff(25, -1);
		diff.assertDiff(24, -1);
		diff.assertDiff(23, 26 + 9);
		diff.assertDiff(22, 9);
		diff.assertDiff(21, -1);
		diff.assertDiff(20, -1);
	}

	@Test
	void testSubstringSearch() {
		String src = """
			START:
				#include <system/string.def>
				move	r28, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
				move	r0, r28
				move	r1, "JKL"
				move	r2, 0
				int		iSUBSTRING_SEARCH
				move	r27, r0
				move	r0, r28
				move	r2, 20
				int		iSUBSTRING_SEARCH
				move	r26, r0
				move	r0, r28
				move	r2, 60
				int		iSUBSTRING_SEARCH
				move	r25, r0
				move	r0, r28
				move	r1, "jkl"
				move	r2, 0
				int		iSUBSTRING_SEARCH
				move	r24, r0

				move	r0, r28
				move	r1, "JKL"
				move	r2, 100
				int		iLAST_SUBSTRING_SEARCH
				move	r23, r0
				move	r0, r28
				move	r2, 20
				int		iLAST_SUBSTRING_SEARCH
				move	r22, r0
				move	r0, r28
				move	r2, 5
				int		iLAST_SUBSTRING_SEARCH
				move	r21, r0
				move	r0, r28
				move	r1, "jkl"
				move	r2, 100
				int		iLAST_SUBSTRING_SEARCH
				move	r20, r0

				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(14, diff.size());
		diff.assertDiff(27, 9);
		diff.assertDiff(26, 26 + 9);
		diff.assertDiff(25, -1);
		diff.assertDiff(24, -1);
		diff.assertDiff(23, 26 + 9);
		diff.assertDiff(22, 9);
		diff.assertDiff(21, -1);
		diff.assertDiff(20, -1);
	}

	@Test
	void testRegex() {
		String src = """
			START:
				#include <system/string.def>
				move	r28, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ"
				move	r0, r28
				move	r1, "A\\\\w+Q"
				int		iMATCHES
				move	r27, r0
				move	r0, r28
				move	r1, "\\\\d+"
				int		iMATCHES
				move	r26, r0

				move	r0, r28
				move	r1, "L[MNOP]+Q"
				move	r2, "xxx"
				int		iREPLACE_FIRST
				move	r25, r0
				move	r0, r28
				move	r1, "\\\\d+"
				int		iREPLACE_FIRST
				move	r24, r0
			
				move	r0, r28
				move	r1, "L[MNOP]+Q"
				move	r2, "xxx"
				int		iREPLACE_ALL
				move	r23, r0
				move	r0, r28
				move	r1, "\\\\d+"
				int		iREPLACE_ALL
				move	r22, r0
			
				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(11, diff.size());
		diff.assertDiff(27, -1);
		diff.assertDiff(26, 0);
		assertEquals("ABCDEFGHIJKxxxRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ", sim.convertString(sim.getR(25)));
		assertEquals("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ", sim.convertString(sim.getR(24)));
		assertEquals("ABCDEFGHIJKxxxRSTUVWXYZABCDEFGHIJKxxxRSTUVWXYZ", sim.convertString(sim.getR(23)));
		assertEquals("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ", sim.convertString(sim.getR(22)));
	}

	@Test
	void testSplitJoin() {
		String src = """
			START:
				#include <system/system.def>
				#include <system/string.def>
				move	r28, "ABC DEF GHIJK LMNOPQ RST U VWXYZ"
				move	r0, r28
				move	r1, "\\\\s+"
				move	r2, -1
				int		iSPLIT
				move	r27, r0
				
				move	r0, r27
				move	r1, ","
				int		iJOIN
				move	r26, r0

				move	r0, "ABCD"
				move	r1, "EFGH"
				move	r2, 0
				int		iSTRCAT
				move	r25, r0
				
				// STRCAT into the string allocate in r0
				move	r1, "I"
				move	r2, r0
				int		iSTRCAT
				move	r24, r0
				
				move	r0, 10
				int		iALLOC

				// STRCAT into the string allocate in r0 will realloc
				move	r0, r24
				move	r1, "JKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ"
				move	r2, r0
				int		iSTRCAT
				move	r23, r0
				
				STOP
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(11, diff.size());
		int a = (int)sim.getR(27);
		int i = 0;
		assertEquals(7, sim.memRead(a + i++));
		assertEquals("ABC", sim.convertString(sim.memRead(a + i++)));
		assertEquals("DEF", sim.convertString(sim.memRead(a + i++)));
		assertEquals("GHIJK", sim.convertString(sim.memRead(a + i++)));
		assertEquals("LMNOPQ", sim.convertString(sim.memRead(a + i++)));
		assertEquals("RST", sim.convertString(sim.memRead(a + i++)));
		assertEquals("U", sim.convertString(sim.memRead(a + i++)));
		assertEquals("VWXYZ", sim.convertString(sim.memRead(a + i++)));
		assertEquals("ABC,DEF,GHIJK,LMNOPQ,RST,U,VWXYZ", sim.convertString((int)sim.getR(26)));
		assertEquals("ABCDEFGHI", sim.convertString((int)sim.getR(25)));
		assertEquals("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ", sim.convertString((int)sim.getR(23)));
	}
}