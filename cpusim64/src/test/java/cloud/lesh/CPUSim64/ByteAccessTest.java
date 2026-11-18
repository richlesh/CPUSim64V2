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

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/** Tests for the byte-granular LDB (39) and STB (40) instructions. */
public class ByteAccessTest extends BaseTest {

	@Test
	void testLdbByteOrder() {
		String src = """
			START:
				move	r10, WORD		// .dcw emits a count word first
				add		r10, 1
				lshift	r10, 3
				ldb		r1, r10
				ldb		r2, r10[1]
				ldb		r3, 1[r10]
				ldb		r4, r10[3]
				ldb		r5, r10[4]
				ldb		r6, r10[5]
				ldb		r7, r10[6]
				ldb		r8, r10[7]
				move	r11, r10
				add		r11, 16			// skip WORD2's count word
				ldb		r9, r11
				ldb		r12, r11[3]
				stop
				stop
			WORD:	.dcw 0x0102030405060708
			WORD2:	.dcw 0xFFEEDDCCBBAA9988
			""";
		var tuple = runProgram(src);
		var sim = tuple.getMiddle();
		assertEquals(0x01, sim.R[1]);
		assertEquals(0x02, sim.R[2]);
		assertEquals(0x02, sim.R[3]);
		assertEquals(0x04, sim.R[4]);
		assertEquals(0x05, sim.R[5]);
		assertEquals(0x06, sim.R[6]);
		assertEquals(0x07, sim.R[7]);
		assertEquals(0x08, sim.R[8]);
		assertEquals(0xFF, sim.R[9]);          // zero-extended, not sign-extended
		assertEquals(0xCC, sim.R[12]);
	}

	@Test
	void testStbLeavesNeighboursIntact() {
		String src = """
			START:
				move	r10, __HEAP_START__
				add		r10, 16
				move	r11, r10
				lshift	r11, 3
				load	r5, INIT[1]
				store	r5, r10
				stb		0xAA, r11[3]
				load	r1, r10
				stb		0x1FF, r11		// only the low 8 bits are stored
				load	r2, r10
				move	r4, 0x1234
				stb		r4, r11[7]
				load	r3, r10
				stop
				stop
			INIT:	.dcw 0x0102030405060708
			""";
		var tuple = runProgram(src);
		var sim = tuple.getMiddle();
		assertEquals(0x010203AA05060708L, sim.R[1]);
		assertEquals(0xFF0203AA05060708L, sim.R[2]);
		assertEquals(0xFF0203AA05060734L, sim.R[3]);
	}

	@Test
	void testStringRoundTrip() {
		// .dcs lays out a length word followed by UTF-8 bytes packed big-endian.
		String src = """
			START:
				move	r10, STR
				lshift	r10, 3
				load	r9, STR			// length word
				add		r10, 8			// first byte of the text
				move	r11, __HEAP_START__
				add		r11, 16
				lshift	r11, 3			// byte address of scratch
				move	r12, 0
			LOOP:
				cmp		r12, r9
				jump	ge, DONE
				ldb		r1, r10[r12]
				stb		r1, r11[r12]
				add		r12, 1
				jump	LOOP
			DONE:
				move	r13, __HEAP_START__
				add		r13, 16
				load	r2, r13
				load	r3, r13[1]
				stop
				stop
			STR:	.dcs "Hello, World!"
			""";
		var tuple = runProgram(src);
		var sim = tuple.getMiddle();
		assertEquals(13, sim.R[9]);
		assertEquals(0x48656C6C6F2C2057L, sim.R[2]);   // "Hello, W"
		assertEquals(0x6F726C6421000000L, sim.R[3]);   // "orld!"
	}

	@Test
	void testReadonlyViolation() {
		List<Long> prog = new ArrayList<>();
		prog.add(Simulator.encT1(Opcode.READONLY.code, 4));
		// STB R0, [16]  => byte address 16 -> word 2, byte 0 (inside the protected region)
		prog.add(Simulator.encT2(Opcode.STB.code, 2, 0, 16));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		Simulator sim = new Simulator(1000, new String[] {"test"});
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var ex = assertThrows(Simulator.CPUException.class, () -> sim.run(0L));
		assertTrue(ex.getMessage().contains("Write access violation of 00000002"), ex.getMessage());
	}

	@Test
	void testBoundsErrors() {
		Simulator sim = new Simulator(1000, new String[] {"test"});
		sim.loadProgram(new ArrayList<>(List.of(
				Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0))), 0L);
		// Past the end of memory.
		assertThrows(Simulator.CPUException.class, () -> sim.memWriteByte(8L * 100000, 1));
		// Negative (shared) range: word address floor(EA/8) is not a valid shared-memory index.
		assertThrows(Simulator.CPUException.class, () -> sim.memWriteByte(-8L, 1));
	}

	@Test
	void testStbEffectiveAddressShift() {
		Simulator sim = new Simulator(0x2000, 0x1000, 0x500, new String[] {"test"});
		sim.loadProgram(new ArrayList<>(List.of(
				Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0))), 0L);
		long w = sim.heapStart + 16;
		sim.memWriteByte(w * 8, 0x11);
		sim.memWriteByte(w * 8 + 7, 0x22);
		sim.memWriteByte((w + 1) * 8 + 4, 0x33);
		assertEquals(0x1100000000000022L, sim.memRead(w));
		assertEquals(0x0000000033000000L, sim.memRead(w + 1));
	}

	/** Two threads each store their own 4 bytes of the same word; no update may be lost. */
	@Test
	void testAdjacentByteStoresAreAtomic() throws Exception {
		Simulator sim = new Simulator(0x2000, 0x1000, 0x500, new String[] {"test"});
		sim.loadProgram(new ArrayList<>(List.of(
				Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0))), 0L);
		final long w = sim.heapStart + 16;
		final int iterations = 200_000;
		var failure = new java.util.concurrent.atomic.AtomicReference<Throwable>();
		Thread[] threads = new Thread[2];
		for (int t = 0; t < 2; ++t) {
			final int base = t * 4;
			threads[t] = new Thread(() -> {
				try {
					for (int i = 0; i < iterations; ++i)
						for (int k = 0; k < 4; ++k)
							sim.memWriteByte(w * 8 + base + k, (i + base + k) & 0xFF);
				} catch (Throwable ex) {
					failure.set(ex);
				}
			});
		}
		for (var th : threads) th.start();
		for (var th : threads) th.join();
		assertNull(failure.get());
		long expected = 0;
		for (int b = 0; b < 8; ++b)
			expected = (expected << 8) | ((iterations - 1 + b) & 0xFF);
		assertEquals(expected, sim.memRead(w));
	}

	@Test
	void testDisassembly() {
		long ldb = Simulator.encT2(Opcode.LDB.code, 2, 1, 16);
		long stb = Simulator.encT2(Opcode.STB.code, 2, 1, 16);
		assertTrue(Simulator.Decoded.decode(ldb).disassemble(new java.util.HashMap<>()).startsWith("LDB"));
		assertTrue(Simulator.Decoded.decode(stb).disassemble(new java.util.HashMap<>()).startsWith("STB"));
	}
}
