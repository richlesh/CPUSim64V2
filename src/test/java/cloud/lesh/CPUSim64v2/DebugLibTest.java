package cloud.lesh.CPUSim64v2;

import org.apache.commons.lang3.tuple.Triple;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class DebugLibTest extends BaseTest {
	@Test
	void testDebugMsg() {
		String src = """
			START:
			#include <system/debug.def>
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
			#include <system/debug.def>
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
	}

	@Test
	void testDebugMsg_NoDebug() {
		String src = """
			START:
			#include <system/debug.def>
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
			#include <system/debug.def>
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
			#include <system/debug.def>
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
		assertEquals(5, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testAssertFail() {
		String src = """
			START:
			#include <system/debug.def>
			#include <system/debug.asm>
			#macro SET_EXIT_ON_ASSERT_FAILURE(0)
			#macro ASSERT_TRUE(0, "true")
			#macro ASSERT_FALSE(1, "false")
			#macro ASSERT_EQ(1, 2, "eq")
			#macro ASSERT_NE(1, 1, "ne")
			#macro ASSERT_LT(2, 1, "lt")
			#macro ASSERT_GT(1, 2, "gt")
			#macro ASSERT_LT(1, 1, "lt")
			#macro ASSERT_GT(2, 2, "gt")
			#macro ASSERT_LE(2, 1, "le")
			#macro ASSERT_GE(1, 2, "ge")
			load f1, 1.0
			load f2, 2.0
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
Assertion Failed (Test.asm:5) true
Assertion Failed (Test.asm:6) false
Assertion Failed (Test.asm:7) eq 1=2
Assertion Failed (Test.asm:8) ne 1≠1
Assertion Failed (Test.asm:9) lt 2<1
Assertion Failed (Test.asm:10) gt 1>2
Assertion Failed (Test.asm:11) lt 1<1
Assertion Failed (Test.asm:12) gt 2>2
Assertion Failed (Test.asm:13) le 2≤1
Assertion Failed (Test.asm:14) ge 1≥2
Assertion Failed (Test.asm:17) eq 1.0000000000000000=2.0000000000000000
Assertion Failed (Test.asm:18) ne 1.0000000000000000≠1.0000000000000000
Assertion Failed (Test.asm:19) lt 2.0000000000000000<1.0000000000000000
Assertion Failed (Test.asm:20) gt 1.0000000000000000>2.0000000000000000
Assertion Failed (Test.asm:21) lt 1.0000000000000000<1.0000000000000000
Assertion Failed (Test.asm:22) gt 2.0000000000000000>2.0000000000000000
Assertion Failed (Test.asm:23) le 2.0000000000000000≤1.0000000000000000
Assertion Failed (Test.asm:24) ge 1.0000000000000000≥2.0000000000000000
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