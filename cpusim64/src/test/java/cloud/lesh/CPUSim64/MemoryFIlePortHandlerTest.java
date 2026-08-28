// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package cloud.lesh.CPUSim64;

import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class MemoryFIlePortHandlerTest {
	@Test
	void testReadWriteBigEndian() {
		Simulator sim = new Simulator(0x2000, new String[] {});

		// Test Write
		var ph = new MemoryFilePortHandler(sim, 1, null);
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
		byte[] bytes = ph.toBytes();
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
		ph = new MemoryFilePortHandler(sim, 0, bytes);
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
	}

	@Test
	void testReadWriteLittleEndian() {
		Simulator sim = new Simulator(0x2000, new String[] {});

		// Test write
		var ph = new MemoryFilePortHandler(sim, 1, null);
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
		byte[] bytes = ph.toBytes();
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
		ph = new MemoryFilePortHandler(sim, 0, bytes);
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
		ph.close();
	}

	@Test
	void testReadChar() {
		Simulator sim = new Simulator(0x2000, new String[] {});
		String s = "Bye🙂¡€🇺🇸";
		var ph = new MemoryFilePortHandler(sim, 0, s.getBytes(StandardCharsets.UTF_8));
		StringBuilder sb = new StringBuilder();
		for (var cp = ph.readChar(); cp >= 0; cp = ph.readChar()) {
			sb.appendCodePoint(cp);
		}
		assertEquals(s, sb.toString());
		ph.close();
	}

	@Test
	void testWriteChar() {		Simulator sim = new Simulator(0x2000, new String[] {});
		var ph = new MemoryFilePortHandler(sim, 1, null);
		String s = "Bye🙂¡€🇺🇸";
		for (var c : s.codePoints().toArray()) {
			ph.writeChar(c);
		}
		ph.flush();
		ph.close();
		byte[] bytes = ph.toBytes();
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
		assertEquals(s, ph.toString());
	}

	// A short (partial) word read at EOF must return -1 and leave the bytes it
	// already consumed available for subsequent byte-mode reads. This mirrors
	// the hashcode program: read 8 bytes at a time, then fall back to reading
	// one byte at a time for the < 8-byte tail.
	@Test
	void testShortWordReadPushesBackTailBigEndian() {
		Simulator sim = new Simulator(0x2000, new String[] {});
		// 11 bytes: one full 8-byte word (0..7) plus a 3-byte tail (8, 9, 10).
		byte[] data = new byte[] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		var ph = new MemoryFilePortHandler(sim, 0, data);
		ph.setLittleEndian(false);

		// First 8-byte word read succeeds: big-endian 0x0001020304050607.
		assertEquals(0x0001020304050607L, ph.read(8));
		assertEquals(false, ph.isEOF());

		// Second 8-byte word read hits EOF after only 3 bytes: returns -1 and
		// pushes those 3 bytes back (in original order) instead of discarding.
		assertEquals(-1L, ph.read(8));
		assertEquals(true, ph.isEOF());

		// Byte-mode tail reads now recover the pushed-back bytes in order.
		assertEquals(8L, ph.read(1));
		assertEquals(9L, ph.read(1));
		assertEquals(10L, ph.read(1));
		// Stream is now truly exhausted.
		assertEquals(-1L, ph.read(1));
		ph.close();
	}

	@Test
	void testShortWordReadPushesBackTailLittleEndian() {
		Simulator sim = new Simulator(0x2000, new String[] {});
		byte[] data = new byte[] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		var ph = new MemoryFilePortHandler(sim, 0, data);
		ph.setLittleEndian(true);

		// First 8-byte word, little-endian: 0x0706050403020100.
		assertEquals(0x0706050403020100L, ph.read(8));
		// Short read of the 3-byte tail returns -1 and pushes bytes back.
		assertEquals(-1L, ph.read(8));
		// Recover tail in original stream order.
		assertEquals(8L, ph.read(1));
		assertEquals(9L, ph.read(1));
		assertEquals(10L, ph.read(1));
		assertEquals(-1L, ph.read(1));
		ph.close();
	}

	// Partial 2-byte and 4-byte reads must also push back their consumed bytes.
	@Test
	void testShortMultiByteReadsPushBack() {
		Simulator sim = new Simulator(0x2000, new String[] {});

		// 4-byte read with only 3 bytes available -> -1, then recover 3 bytes.
		byte[] three = new byte[] {0x11, 0x22, 0x33};
		var ph = new MemoryFilePortHandler(sim, 0, three);
		ph.setLittleEndian(false);
		assertEquals(-1L, ph.read(4));
		assertEquals(0x11L, ph.read(1));
		assertEquals(0x22L, ph.read(1));
		assertEquals(0x33L, ph.read(1));
		assertEquals(-1L, ph.read(1));
		ph.close();

		// 2-byte read with only 1 byte available -> -1, then recover it.
		byte[] one = new byte[] {0x55};
		ph = new MemoryFilePortHandler(sim, 0, one);
		ph.setLittleEndian(false);
		assertEquals(-1L, ph.read(2));
		assertEquals(0x55L, ph.read(1));
		assertEquals(-1L, ph.read(1));
		ph.close();

		// A short 2-byte read followed by another 2-byte read: the second read
		// draws the pushed-back byte first, then hits EOF -> still -1, and the
		// byte remains recoverable one more time.
		byte[] oneB = new byte[] {0x77};
		ph = new MemoryFilePortHandler(sim, 0, oneB);
		ph.setLittleEndian(false);
		assertEquals(-1L, ph.read(2));   // consumes 0x77, pushes it back
		assertEquals(-1L, ph.read(2));   // re-reads 0x77 from pushback, EOF again
		assertEquals(0x77L, ph.read(1)); // byte still recoverable
		assertEquals(-1L, ph.read(1));
		ph.close();
	}

}
