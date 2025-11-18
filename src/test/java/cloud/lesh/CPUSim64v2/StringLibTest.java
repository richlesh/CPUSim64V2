package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class StringLibTest extends BaseTest {
	@Test
	void testStringArray() {
		String src = """
			START:
			#include <system/system.def>
			#include <system/string.asm>
			#var 	i, len, array
			move	len, 10
			move	r0, len
			add		r0, 1
			int		iAlloc
			move	array, r0
			store	len, array[0]
			#for 	1, i <= len, 1
				#macro	FMT_DEC(i)
				store	r0, array[i]
			#end_for
			#call	printStrArray(array)
			#call	freeStrArray(array)
			int		iALLOC_COUNT
			STOP
			STOP
			FINIS:
			""";
		String expected = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		diff.assertDiff(0, 0);
		assertEquals(expected, output);
	}
}