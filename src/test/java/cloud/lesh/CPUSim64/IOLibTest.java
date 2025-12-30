package cloud.lesh.CPUSim64;

import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class IOLibTest extends BaseTest {
	@Test
	void testPuts() {
		String src = """
			START:
			#include <system/io.asm>
			#call	puts("Hello")
			#call	puts(", ")
			#call	puts("World")
			#call	puts("! 😀\\n")
			
			#call	fputs(STDERR, "Goodbye!\\n")
			
			#call	putline("Hello!")
			#call	fputline(STDERR, "Goodbye!")
			STOP
			STOP
			FINIS:
			""";
		String expected = "Hello, World! 😀\nHello!\n";
		String expected2 = "Goodbye!\nGoodbye!\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals(expected, output);
		assertEquals(expected2, output2);
	}

	@Test
	void testPutc() {
		String src = """
			START:
			#include <system/io.asm>
			#call	putc('H')
			#call	putc('e')
			#call	putc('l')
			#call	putc('l')
			#call	putc('o')
			#call	putc(',')
			#call	putc('\\u{1F642}')		// Smiley 4-byte UTF-8
			#call	putc('\\u{a1}')			// Inverted Exclamation Mark 2-byte UTF-8
			#call	putc('\\u{20AC}')		// Euro 3-byte UTF-8
			#call	putc('\\u{1F1FA}')		// REGIONAL INDICATOR U
			#call	putc('\\u{1F1F8}')		// REGIONAL INDICATOR S
			#call	putc('\\n')

			#call	fputc(STDERR, 'B')
			#call	fputc(STDERR, 'y')
			#call	fputc(STDERR, 'e')
			#call	fputc(STDERR, '\\u{1F642}')		// Smiley 4-byte UTF-8
			#call	fputc(STDERR, '\\u{a1}')		// Inverted Exclamation Mark 2-byte UTF-8
			#call	fputc(STDERR, '\\u{20AC}')		// Euro 3-byte UTF-8
			#call	fputc(STDERR, '\\u{1F1FA}')		// REGIONAL INDICATOR U
			#call	fputc(STDERR, '\\u{1F1F8}')		// REGIONAL INDICATOR S
			#call	fputc(STDERR, '\\n')
			STOP
			STOP
			FINIS:
			""";
		String expected = "Hello,🙂¡€🇺🇸\n";
		String expected2 = "Bye🙂¡€\uD83C\uDDFA\uD83C\uDDF8\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals(expected, output);
		assertEquals(expected2, output2);
	}

	@Test
	void testPutInt() {
		String src = """
			START:
			#include <system/io.asm>
			#var	i
			#for 	0, i < 256, 1
				#call	put_int(i, 8)
				#call	put_nl()
			#end_for

			#for 	0, i < 256, 1
				#call	fput_int(STDERR, i, 8)
				#call	fput_nl(STDERR)
			#end_for
			STOP
			STOP
			FINIS:
			""";
		StringBuffer expected = new StringBuffer();
		for (int i = 0; i <= 255; i++) {
			expected.append(Integer.toOctalString(i));
			expected.append('\n');
		}
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		assertEquals(expected.toString(), output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testPutDec() {
		String src = """
			START:
			#include <system/io.asm>
			#var	i
			#for 	0, i < 256, 1
				#call	put_dec(i)
				#call	put_nl()
			#end_for

			#for 	0, i < 256, 1
				#call	fput_dec(STDERR, i)
				#call	fput_nl(STDERR)
			#end_for
			STOP
			STOP
			FINIS:
			""";
		StringBuffer expected = new StringBuffer();
		for (int i = 0; i <= 255; i++) {
			expected.append(Integer.toString(i));
			expected.append('\n');
		}
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		assertEquals(expected.toString(), output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testPutHex() {
		String src = """
			START:
			#include <system/io.asm>
			#var	i
			#for 	0, i < 256, 1
				#call	put_hex(i)
				#call	put_nl()
			#end_for

			#for 	0, i < 256, 1
				#call	fput_hex(STDERR, i)
				#call	fput_nl(STDERR)
			#end_for
			STOP
			STOP
			FINIS:
			""";
		StringBuffer expected = new StringBuffer();
		for (int i = 0; i <= 255; i++) {
			expected.append(Integer.toHexString(i).toUpperCase());
			expected.append('\n');
		}
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		assertEquals(expected.toString(), output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testPutHexSize() {
		String src = """
			START:
			#include <system/io.asm>
			#var	i
			#for 	0, i < 256, 1
				#call	put_hex_size(i, 2)
				#call	put_nl()
			#end_for

			#for 	0, i < 256, 1
				#call	fput_hex_size(STDERR, i, 2)
				#call	fput_nl(STDERR)
			#end_for
			STOP
			STOP
			FINIS:
			""";
		StringBuffer expected = new StringBuffer();
		for (int i = 0; i <= 255; i++) {
			expected.append(String.format("%02X", i));
			expected.append('\n');
		}
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		assertEquals(expected.toString(), output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testPutFP() {
		String src = """
			START:
			#include <system/io.asm>
			#fvar	i, end, incr
			move	end, 1
			load	incr, 0.01
			#for 	0, i < end, incr
				#call	put_fp(i, 2)
				#call	put_nl()
			#end_for

			move	end, 1
			load	incr, 0.01
			#for 	0, i < end, incr
				#call	fput_fp(STDERR, i, 2)
				#call	fput_nl(STDERR)
			#end_for
			STOP
			STOP
			FINIS:
			""";
		StringBuffer expected = new StringBuffer();
		for (double i = 0; i <= 1; i += 0.01) {
			expected.append(String.format("%.2f", i));
			expected.append('\n');
		}
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		assertEquals(expected.toString(), output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testPrintf() {
		String src = """
			START:
			#include <system/io.asm>
			#var	i, str
			#fvar	x
			load	x, 3.141592653
			move	i, 326
			move	str, "HELLO"
			#call	printf("%d %x %f %s\\n", i, i, x, str)
			#call	cond_fprintf(TRUE, STDOUT, "%d\\n", i)
			#call	cond_fprintf(FALSE, STDOUT, "%x\\n", i)

			#call	fprintf(STDERR, "%d %x %f %s\\n", i, i, x, str)
			#call	cond_fprintf(TRUE, STDERR, "%d\\n", i)
			#call	cond_fprintf(FALSE, STDERR, "%x\\n", i)
			STOP
			STOP
			FINIS:
			""";
		String expected = "326 146 3.141593 HELLO\n326\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		assertEquals(expected.toString(), output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testFatal() {
		String src = """
			START:
			#include <system/io.asm>
			#var	i, str
			#fvar	x
			load	x, 3.141592653
			move	i, 326
			move	str, "HELLO"
			#call	cond_fatal(FALSE, "%d %x %f %s\\n", i, i, x, str)
			#call	fatal("%d %x %f %s\\n", i, i, x, str)
			STOP
			STOP
			FINIS:
			""";
		String expected = "\nFATAL: 326 146 3.141593 HELLO\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(9, diff.size());
		assertEquals(result, 1);
		assertEquals("", output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testCondFatal() {
		String src = """
			START:
			#include <system/io.asm>
			#var	i, str
			#fvar	x
			load	x, 3.141592653
			move	i, 326
			move	str, "HELLO"
			#call	cond_fatal(TRUE, "%d %x %f %s\\n", i, i, x, str)
			STOP
			STOP
			FINIS:
			""";
		String expected = "\nFATAL: 326 146 3.141593 HELLO\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		ConsoleOutputCapturer capturer2 = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		capturer2.start(ConsoleOutputCapturer.StdStream.STDERR);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		String output2 = capturer2.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(9, diff.size());
		assertEquals(result, 1);
		assertEquals("", output);
		assertEquals(expected.toString(), output2);
	}

	@Test
	void testGetLine() {
		String src = """
			START:
			#include <system/io.asm>
			#var	buffer
			move	r1, 10
			int		iAlloc
			move	buffer, r0
			#call	fgetline(STDIN, buffer)
			#call	putline(buffer)
			#call	fgetline(STDIN, buffer)
			#call	putline(buffer)
			#call	fgetline(STDIN, buffer)
			#call	putline(buffer)
			#call	fgetline(STDIN, buffer)
			#call	putline(buffer)
			STOP
			STOP
			FINIS:
			""";
		String expected = "1\n22\n333\n🙂¡€\uD83C\uDDFA\uD83C\uDDF8\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"}, "1\n22\n333\n🙂¡€\uD83C\uDDFA\uD83C\uDDF8\n".getBytes(StandardCharsets.UTF_8));
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		assertEquals(expected.toString(), output);
	}
}