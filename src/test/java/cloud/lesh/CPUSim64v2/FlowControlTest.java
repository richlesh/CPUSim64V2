package cloud.lesh.CPUSim64v2;

import org.apache.commons.lang3.tuple.Triple;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class FlowControlTest {
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
	void testForBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i
			#FOR 0, i < 10, 1
				move	r0, STDOUT
				move	r1, i
				int		iPUT_DEC
				int		iPUT_NL
			#ENDFOR
			move	r0, STDOUT
			move	r1, "FINIS"
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
				move	r0, STDOUT
				move	r1, i
				int		iPUT_DEC
				int		iPUT_NL
				sub		i, 1
			#ENDWHILE
			#WHILE i > 0
				move	r0, STDOUT
				move	r1, i
				int		iPUT_DEC
				int		iPUT_NL
				sub		i, 1
			#ENDWHILE
			move	r0, STDOUT
			move	r1, "FINIS"
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
				move	r0, STDOUT
				move	r1, i
				int		iPUT_DEC
				int		iPUT_NL
				add		i, 2
			#END_DO_WHILE i < 10
			#DO_WHILE
				move	r0, STDOUT
				move	r1, i
				int		iPUT_DEC
				int		iPUT_NL
				add		i, 2
			#END_DO_WHILE i < 10
			move	r0, STDOUT
			move	r1, "FINIS"
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
	void testNestedLoopBlock() {
		String src = """
			#include <system/io.def>
			START:
			#VAR i,j
			#FOR 0, i < 10, 1
				move	r0, STDOUT
				move	r1, i
				int		iPUT_DEC
				move	r1, ","
				int		iPUTS
				#FOR 0, j < 4, 1
					move	r0, STDOUT
					move	r1, j
					int		iPUT_DEC
					move	r1, ","
					int		iPUTS
				#ENDFOR
				int		iPUT_NL
			#ENDFOR
			move	r0, STDOUT
			move	r1, "FINIS"
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
				move	r0, STDOUT
				move	r1, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			#IF_COND i == 9
				move	r0, STDOUT
				move	r1, "BADWOLF"
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
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r0, STDOUT
				move	r1, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			#IF_COND i >= 10
				move	r0, STDOUT
				move	r1, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r0, STDOUT
				move	r1, "BAD WOLF"
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
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 8
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r0, STDOUT
				move	r1, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#END_COND

			move	i, 9
			#IF_COND i > 10
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 8
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r0, STDOUT
				move	r1, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			move	i, 8
			#IF_COND i > 10
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 8
				move	r0, STDOUT
				move	r1, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#END_COND
			move	i, 7
			#IF_COND i > 10
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i <= 7
				move	r0, STDOUT
				move	r1, "GOOD"
				int		iPUTS
				int		iPUT_NL
			#ELSE_IF_COND i == j
				move	r0, STDOUT
				move	r1, "BAD WOLF"
				int		iPUTS
				int		iPUT_NL
			#ELSE_COND
				move	r0, STDOUT
				move	r1, "BAD WOLF"
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