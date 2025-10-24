package cloud.lesh.CPUSim64v2;

import org.apache.commons.lang3.tuple.Triple;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class DebugLibTest {
	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src) {
		String[] args = {"test"};
		return runProgram(src, args);
	}

	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src, String[] args) {
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader, args);

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

	@Disabled
	@Test
	void testBoilerplate() {
		String src = """
			START:
				#include <system/system.def>
				#macro SLEEP(500)		// 5 seconds
				MOVE R1, 500
				#macro sleep(r1)
				STOP
			FINIS:
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(4, diff.size());
	}

	@Test
	void testDebugMsg() {
		String src = """
			START:
			#include <system/debug.asm>
			#macro DEBUG_MSG("Hello, World!")
			#macro DEBUG_MSG("Hello, %s!", "Rich")
			#macro DEBUG_MSG("Monolith: %d %d %d", 1, 4, 9)
			#macro DEBUG_MSG("Hex: %x", 255)
			#macro DEBUG_MSG("Char: %c", 65)
			load f0, 12.3456
			#macro DEBUG_MSG("Percent: %.2f%%", f0)
			STOP
			STOP
			FINIS:
			""";
		String expected = """
DEBUG: Hello, World!
DEBUG: Hello, Rich!
DEBUG: Monolith: 1 4 9
DEBUG: Hex: ff
DEBUG: Char: A
DEBUG: Percent: 12.35%
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testCondDebugMsg() {
		String src = """
			START:
			#include <system/debug.asm>
			#macro COND_DEBUG_MSG(0, "Bad, World!")
			#macro COND_DEBUG_MSG(1, "Hello, World!")
			#macro COND_DEBUG_MSG(0, "Bad, %s!", "Rich")
			#macro COND_DEBUG_MSG(1, "Hello, %s!", "Rich")
			#macro COND_DEBUG_MSG(0, "Bad: %d %d %d", 1, 4, 9)
			#macro COND_DEBUG_MSG(1, "Monolith: %d %d %d", 1, 4, 9)
			#macro COND_DEBUG_MSG(0, "Bad: %x", 255)
			#macro COND_DEBUG_MSG(1, "Hex: %x", 255)
			#macro COND_DEBUG_MSG(0, "Bad: %c", 65)
			#macro COND_DEBUG_MSG(1, "Char: %c", 65)
			load f0, 12.3456
			#macro COND_DEBUG_MSG(0, "Bad: %.2f%%", f0)
			#macro COND_DEBUG_MSG(1, "Percent: %.2f%%", f0)
			STOP
			STOP
			FINIS:
			""";
		String expected = """
DEBUG: Hello, World!
DEBUG: Hello, Rich!
DEBUG: Monolith: 1 4 9
DEBUG: Hex: ff
DEBUG: Char: A
DEBUG: Percent: 12.35%
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testDebugMsg_NoDebug() {
		String src = """
			START:
			#include <system/debug.asm>
			#macro DEBUG_MSG("Hello, World!")
			#macro DEBUG_MSG("Hello, %s!", "Rich")
			#macro DEBUG_MSG("Monolith: %d %d %d", 1, 4, 9)
			#macro DEBUG_MSG("Hex: %x", 255)
			#macro DEBUG_MSG("Char: %c", 65)
			load f0, 12.3456
			#macro DEBUG_MSG("Percent: %.2f%%", f0)
			STOP
			STOP
			FINIS:
			""";
		String expected = "";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testCondDebugMsg_NoDebug() {
		String src = """
			START:
			#include <system/debug.asm>
			#macro COND_DEBUG_MSG(0, "Bad, World!")
			#macro COND_DEBUG_MSG(1, "Hello, World!")
			#macro COND_DEBUG_MSG(0, "Bad, %s!", "Rich")
			#macro COND_DEBUG_MSG(1, "Hello, %s!", "Rich")
			#macro COND_DEBUG_MSG(0, "Bad: %d %d %d", 1, 4, 9)
			#macro COND_DEBUG_MSG(1, "Monolith: %d %d %d", 1, 4, 9)
			#macro COND_DEBUG_MSG(0, "Bad: %x", 255)
			#macro COND_DEBUG_MSG(1, "Hex: %x", 255)
			#macro COND_DEBUG_MSG(0, "Bad: %c", 65)
			#macro COND_DEBUG_MSG(1, "Char: %c", 65)
			load f0, 12.3456
			#macro COND_DEBUG_MSG(0, "Bad: %.2f%%", f0)
			#macro COND_DEBUG_MSG(1, "Percent: %.2f%%", f0)
			STOP
			STOP
			FINIS:
			""";
		String expected = "";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testAssertPass() {
		String src = """
			START:
			#include <system/debug.asm>
			#macro SET_EXIT_ON_ASSERT_FAILURE(0)
			#macro assert_true(1, "true")
			#macro assert_false(0, "false")
			#macro assert_eq(1, 1, "eq")
			#macro assert_ne(1, 2, "ne")
			#macro assert_lt(1, 2, "lt")
			#macro assert_gt(2, 1, "gt")
			#macro assert_le(1, 2, "le")
			#macro assert_ge(2, 1, "ge")
			#macro assert_le(1, 1, "le")
			#macro assert_ge(2, 2, "ge")
			move f1, 1.0
			move f2, 2.0
			#macro assert_eq_fp(f1, f1, "eq")
			#macro assert_ne_fp(f1, f2, "ne")
			#macro assert_lt_fp(f1, f2, "lt")
			#macro assert_gt_fp(f2, f1, "gt")
			#macro assert_le_fp(f1, f2, "le")
			#macro assert_ge_fp(f2, f1, "ge")
			#macro assert_le_fp(f1, f1, "le")
			#macro assert_ge_fp(f2, f2, "ge")
			STOP
			STOP
			FINIS:
			""";
		String expected = "";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(4, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testAssertFail() {
		String src = """
			START:
			#include <system/debug.asm>
			#macro SET_EXIT_ON_ASSERT_FAILURE(0)
			#macro assert_true(0, "true")
			#macro assert_false(1, "false")
			#macro assert_eq(1, 2, "eq")
			#macro assert_ne(1, 1, "ne")
			#macro assert_lt(2, 1, "lt")
			#macro assert_gt(1, 2, "gt")
			#macro assert_lt(1, 1, "lt")
			#macro assert_gt(2, 2, "gt")
			#macro assert_le(2, 1, "le")
			#macro assert_ge(1, 2, "ge")
			move f1, 1.0
			move f2, 2.0
			#macro assert_eq_fp(f1, f2, "eq")
			#macro assert_ne_fp(f1, f1, "ne")
			#macro assert_lt_fp(f2, f1, "lt")
			#macro assert_gt_fp(f1, f2, "gt")
			#macro assert_lt_fp(f1, f1, "lt")
			#macro assert_gt_fp(f2, f2, "gt")
			#macro assert_le_fp(f2, f1, "le")
			#macro assert_ge_fp(f1, f2, "ge")
			STOP
			STOP
			FINIS:
			""";
		String expected = """
Assertion Failed (Test.asm:4) true
Assertion Failed (Test.asm:5) false
Assertion Failed (Test.asm:6) eq 1=2
Assertion Failed (Test.asm:7) ne 1≠1
Assertion Failed (Test.asm:8) lt 2<1
Assertion Failed (Test.asm:9) gt 1>2
Assertion Failed (Test.asm:10) lt 1<1
Assertion Failed (Test.asm:11) gt 2>2
Assertion Failed (Test.asm:12) le 2≤1
Assertion Failed (Test.asm:13) ge 1≥2
Assertion Failed (Test.asm:16) eq 755.0000000000000000=756.0000000000000000
Assertion Failed (Test.asm:17) ne 755.0000000000000000≠755.0000000000000000
Assertion Failed (Test.asm:18) lt 756.0000000000000000<755.0000000000000000
Assertion Failed (Test.asm:19) gt 755.0000000000000000>756.0000000000000000
Assertion Failed (Test.asm:20) lt 755.0000000000000000<755.0000000000000000
Assertion Failed (Test.asm:21) gt 756.0000000000000000>756.0000000000000000
Assertion Failed (Test.asm:22) le 756.0000000000000000≤755.0000000000000000
Assertion Failed (Test.asm:23) ge 755.0000000000000000≥756.0000000000000000
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(6, diff.size());
		assertEquals(expected, output);
	}
}