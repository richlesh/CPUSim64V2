package cloud.lesh.CPUSim64;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class FlowControlTest extends BaseTest {
	@Test
	void testForBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			#FOR 0, i < 10, 1
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
			#END_FOR
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
0
1
2
3
4
5
6
7
8
9
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 10);
		assertEquals(expected, output);
	}

	@Test
	void testWhileBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			move	i, 10
			#WHILE i > 0
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
				sub		i, 1
			#END_WHILE
			#WHILE i > 0
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
				sub		i, 1
			#END_WHILE
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
10
9
8
7
6
5
4
3
2
1
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 0);
		assertEquals(expected, output);
	}

	@Test
	void testDoWhileBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			move	i, 0
			#DO_WHILE
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
				add		i, 2
			#END_DO_WHILE i < 10
			#DO_WHILE
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
				add		i, 2
			#END_DO_WHILE i < 10
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
0
2
4
6
8
10
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 12);
		assertEquals(expected, output);
	}

	@Test
	void testForContinue() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i, j
			#FOR 0, i < 10, 1
				div		r2, j, i, 2
				#if_cond	j == 1
					#continue
				#end_cond
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
			#END_FOR
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
0
2
4
6
8
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 10);
		assertEquals(expected, output);
	}

	@Test
	void testWhileContinue() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i, j
			move	i, 11
			#WHILE i > 1
				sub		i, 1
				div		r2, j, i, 2
				#if_cond	j == 1
					#continue
				#end_cond
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
			#END_WHILE
			#WHILE i > 1
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
				sub		i, 1
			#END_WHILE
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
10
8
6
4
2
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 1);
		assertEquals(expected, output);
	}

	@Test
	void testDoWhileContinue() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i, j
			move	i, -1
			#DO_WHILE
				add		i, 1
				div		r2, j, i, 2
				#if_cond	j == 1
					#continue
				#end_cond
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
			#END_DO_WHILE i < 10
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
0
2
4
6
8
10
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 10);
		assertEquals(expected, output);
	}

	@Test
	void testForBreak() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			#FOR 0, i < 10, 1
				#if_cond	i == 5
					#break
				#end_cond
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
			#END_FOR
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
0
1
2
3
4
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 5);
		assertEquals(expected, output);
	}

	@Test
	void testWhileBreak() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			move	i, 11
			#WHILE i > 1
				sub		i, 1
				#if_cond	i == 5
					#break
				#end_cond
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
			#END_WHILE
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
10
9
8
7
6
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 5);
		assertEquals(expected, output);
	}

	@Test
	void testDoWhileBreak() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			move	i, -1
			#DO_WHILE
				add		i, 1
				#if_cond	i == 5
					#break
				#end_cond
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				int		iPUT_NL
			#END_DO_WHILE i < 10
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
0
1
2
3
4
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 5);
		assertEquals(expected, output);
	}

	@Test
	void testNestedLoopBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i,j
			#FOR 0, i < 10, 1
				move	r1, STDOUT
				move	r2, i
				int		iPUT_DEC
				move	r2, ","
				int		iPUTS
				#FOR 0, j < 4, 1
					move	r1, STDOUT
					move	r2, j
					int		iPUT_DEC
					move	r2, ","
					int		iPUTS
				#END_FOR
				int		iPUT_NL
			#END_FOR
			move	r1, STDOUT
			move	r2, "FINIS"
			int		iPUTS
			int		iPUT_NL
			stop
			stop
			FINIS:
			""";
		String expected = """
0,0,1,2,3,
1,0,1,2,3,
2,0,1,2,3,
3,0,1,2,3,
4,0,1,2,3,
5,0,1,2,3,
6,0,1,2,3,
7,0,1,2,3,
8,0,1,2,3,
9,0,1,2,3,
FINIS
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 10);
		diff.assertDiff(27, 4);
		assertEquals(expected, output);
	}

	@Test
	void testCondBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			move	i, 10
			#IF_COND i == 10
				move	r1, STDOUT
				move	r2, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			#IF_COND i == 9
				move	r1, STDOUT
				move	r2, "BADWOLF"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			stop
			stop
			FINIS:
			""";
		String expected = """
GOOD
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 10);
		assertEquals(expected, output);
	}

	@Test
	void testCondElseBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			move	i, 10
			#IF_COND i < 10
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r1, STDOUT
				move	r2, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			#IF_COND i >= 10
				move	r1, STDOUT
				move	r2, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			stop
			stop
			FINIS:
			""";
		String expected = """
GOOD
GOOD
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		diff.assertDiff(28, 10);
		assertEquals(expected, output);
	}

	@Test
	void testCondElseIFBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i, j
			move	i, 10
			move	j, 9
			#IF_COND i > 10
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 8
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r1, STDOUT
				move	r2, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#END_COND

			move	i, 9
			#IF_COND i > 10
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 8
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r1, STDOUT
				move	r2, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			move	i, 8
			#IF_COND i > 10
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 8
				move	r1, STDOUT
				move	r2, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			move	i, 7
			#IF_COND i > 10
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 7
				move	r1, STDOUT
				move	r2, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r1, STDOUT
				move	r2, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			stop
			stop
			FINIS:
			""";
		String expected = """
GOOD
GOOD
GOOD
GOOD
		""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(expected, output);
	}
}