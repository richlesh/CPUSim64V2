package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class IOLibTest extends BaseTest {
	@Test
	void testDebugMsg() {
		String src = """
			START:
			#include <system/debug.asm>

			STOP
			STOP
			FINIS:
			""";
		String expected = """
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(1, diff.size());
		assertEquals(expected, output);
	}
}