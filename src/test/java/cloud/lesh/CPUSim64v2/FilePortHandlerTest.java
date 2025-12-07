package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Test;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class FilePortHandlerTest {
	@Test
	void testReadWriteBigEndian() throws InvalidPathException, IOException {
		Simulator sim = new Simulator(0x2000, new String[] {});
		Path path = Path.of("./test.bin");

		// Test Write
		var ph = new FilePortHandler(sim, 1, path.toAbsolutePath().toString());
		ph.setLittleEndian(false);
		for (int i = 0; i < 256; ++i) {								// [0, 255]
			ph.write((byte)i);
		}
		for (int i = 1000; i < 2000; ++i) {							// [0, 65_535]
			ph.write((short)i);
		}
		for (int i = 70000; i < 71000; ++i) {						// [0, 4_294_967_296]
			ph.write((int)i);
		}
		for (long i = 0x1234567890ABCDEFL; i < 0x1234567890ABCDEFL + 1000; ++i) {
			ph.write(i);
		}
		ph.flush();
		ph.close();
		byte[] bytes = Files.readAllBytes(path);
		assertEquals(256 + 2000 + 4000 + 8000, bytes.length);
		assertEquals(3, bytes[256]);								// 0x3E8
		assertEquals(-24, bytes[257]);
		assertEquals(0, bytes[256 + 2000]);							// 0x00011170
		assertEquals(1, bytes[256 + 2001]);
		assertEquals(0x11, bytes[256 + 2002]);
		assertEquals(0x70, bytes[256 + 2003]);
		assertEquals(0x12, bytes[256 + 2000 + 4000]);				// 0x1234567890ABCDEF
		assertEquals(0x34, bytes[256 + 2000 + 4001]);
		assertEquals(0x56, bytes[256 + 2000 + 4002]);
		assertEquals(0x78, bytes[256 + 2000 + 4003]);
		assertEquals(0x90 - 256, bytes[256 + 2000 + 4004]);
		assertEquals(0xAB - 256, bytes[256 + 2000 + 4005]);
		assertEquals(0xCD - 256, bytes[256 + 2000 + 4006]);
		assertEquals(0xEF - 256, bytes[256 + 2000 + 4007]);

		// Test read
		ph = new FilePortHandler(sim, 0, path.toAbsolutePath().toString());
		ph.setLittleEndian(false);
		for (int i = 0; i < 256; ++i) {								// [0, 255]
			assertEquals(i, ph.read(1));
		}
		for (int i = 1000; i < 2000; ++i) {							// [0, 65_535]
			assertEquals(i, ph.read(2));
		}
		for (int i = 70000; i < 71000; ++i) {						// [0, 4_294_967_296]
			assertEquals(i, ph.read(4));
		}
		for (long i = 0x1234567890ABCDEFL; i < 0x1234567890ABCDEFL + 1000; ++i) {
			assertEquals(i, ph.read(8));
		}
		ph.close();
		Files.deleteIfExists(path);
	}

	@Test
	void testReadWriteLittleEndian() throws InvalidPathException, IOException {
		Simulator sim = new Simulator(0x2000, new String[] {});
		Path path = Path.of("./test2.bin");

		// Test write
		var ph = new FilePortHandler(sim, 1, path.toAbsolutePath().toString());
		ph.setLittleEndian(true);
		for (int i = 0; i < 256; ++i) {								// [0, 255]
			ph.write((byte)i);
		}
		for (int i = 1000; i < 2000; ++i) {							// [0, 65_535]
			ph.write((short)i);
		}
		for (int i = 70000; i < 71000; ++i) {						// [0, 4_294_967_296]
			ph.write((int)i);
		}
		for (long i = 0x1234567890ABCDEFL; i < 0x1234567890ABCDEFL + 1000; ++i) {
			ph.write(i);
		}
		ph.flush();
		ph.close();
		byte[] bytes = Files.readAllBytes(path);
		assertEquals(256 + 2000 + 4000 + 8000, bytes.length);
		assertEquals(-24, bytes[256]);								// 0x3E8
		assertEquals(3, bytes[257]);
		assertEquals(0x70, bytes[256 + 2000]);						// 0x00011170
		assertEquals(0x11, bytes[256 + 2001]);
		assertEquals(1, bytes[256 + 2002]);
		assertEquals(0, bytes[256 + 2003]);
		assertEquals(0xEF - 256, bytes[256 + 2000 + 4000]);			// 0x1234567890ABCDEF
		assertEquals(0xCD - 256, bytes[256 + 2000 + 4001]);
		assertEquals(0xAB - 256, bytes[256 + 2000 + 4002]);
		assertEquals(0x90 - 256, bytes[256 + 2000 + 4003]);
		assertEquals(0x78, bytes[256 + 2000 + 4004]);
		assertEquals(0x56, bytes[256 + 2000 + 4005]);
		assertEquals(0x34, bytes[256 + 2000 + 4006]);
		assertEquals(0x12, bytes[256 + 2000 + 4007]);

		// Test read
		ph = new FilePortHandler(sim, 0, path.toAbsolutePath().toString());
		ph.setLittleEndian(true);
		for (int i = 0; i < 256; ++i) {								// [0, 255]
			assertEquals(i, ph.read(1));
		}
		for (int i = 1000; i < 2000; ++i) {							// [0, 65_535]
			assertEquals(i, ph.read(2));
		}
		for (int i = 70000; i < 71000; ++i) {						// [0, 4_294_967_296]
			assertEquals(i, ph.read(4));
		}
		for (long i = 0x1234567890ABCDEFL; i < 0x1234567890ABCDEFL + 1000; ++i) {
			assertEquals(i, ph.read(8));
		}
		Files.deleteIfExists(path);
	}

	@Test
	void testReadWriteChar() throws InvalidPathException, IOException {
		Simulator sim = new Simulator(0x2000, new String[] {});
		Path path = Path.of("./test.txt");

		// Test write
		var ph = new FilePortHandler(sim, 1, path.toAbsolutePath().toString());
		String s = "Bye🙂¡€🇺🇸";
		for (var c : s.codePoints().toArray()) {
			ph.writeChar(c);
		}
		ph.flush();
		ph.close();
		byte[] bytes = Files.readAllBytes(path);
		assertEquals(20, bytes.length);
		assertEquals('B', bytes[0]);
		assertEquals('y', bytes[1]);
		assertEquals('e', bytes[2]);
		assertEquals(0xF0 - 256, bytes[3]);
		assertEquals(0x9F - 256, bytes[4]);
		assertEquals(0x99 - 256, bytes[5]);
		assertEquals(0x82 - 256, bytes[6]);
		assertEquals(0xC2 - 256, bytes[7]);
		assertEquals(0xA1 - 256, bytes[8]);
		assertEquals(0xE2 - 256, bytes[9]);
		assertEquals(0x82 - 256, bytes[10]);
		assertEquals(0xAC - 256, bytes[11]);
		assertEquals(0xF0 - 256, bytes[12]);
		assertEquals(0x9F - 256, bytes[13]);
		assertEquals(0x87 - 256, bytes[14]);
		assertEquals(0xBA - 256, bytes[15]);
		assertEquals(0xF0 - 256, bytes[16]);
		assertEquals(0x9F - 256, bytes[17]);
		assertEquals(0x87 - 256, bytes[18]);
		assertEquals(0xB8 - 256, bytes[19]);

		// Test read
		ph = new FilePortHandler(sim, 0, path.toAbsolutePath().toString());
		StringBuilder sb = new StringBuilder();
		for (var cp = ph.readChar(); cp >= 0; cp = ph.readChar()) {
			sb.appendCodePoint(cp);
		}
		assertEquals(s, sb.toString());
		ph.close();
		Files.deleteIfExists(path);
	}
}
