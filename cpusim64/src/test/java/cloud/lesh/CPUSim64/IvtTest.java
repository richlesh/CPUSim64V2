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

import static org.junit.jupiter.api.Assertions.*;

/**
 * Tests for vectored interrupts: the interrupt vector table, LDCR/STCR (41, 42),
 * IRET (43), INT -n host services, and the exit hook.
 *
 * The simulator under test has 0x2000 words of memory; the tests put the vector
 * table at 0x1800 (word address), so entry n lives at 0x1800 + n.
 */
public class IvtTest extends BaseTest {
	private static final long SR_S = 1L << 2;

	@Test
	void testVectoredCallAndReturnPreservesState() {
		String src = """
			START:
				move	r1, 0x1800
				move	r2, H5
				store	r2, 0x1805
				stcr	0, r1
				move	r20, sp
				move	r21, sf
				move	r3, 7
				move	r5, -1				// sets S, clears Z
				int		5
				stop
				stop
			H5:	add		r3, 1				// clobbers SR (clears S)
				move	r6, 0				// sets Z
				iret
			""";
		var sim = runProgram(src).getMiddle();
		assertEquals(8, sim.R[3]);
		assertEquals(sim.R[20], sim.R[Simulator.R_SP], "SP restored");
		assertEquals(sim.R[21], sim.R[Simulator.R_SF], "SF restored");
		assertEquals(SR_S, sim.getSR(), "SR restored to the value before INT");
	}

	@Test
	void testHandlerSeesFrameAndRegisters() {
		// Inside the handler SF = SP, the saved SF/SR/PC sit just above, and r0..r28 are untouched.
		String src = """
			START:
				move	r1, 0x1800
				move	r2, H5
				store	r2, 0x1805
				stcr	0, r1
				move	r7, 77
				move	r21, sf
				move	r22, sp
				int		5
				stop
				stop
			H5:	move	r8, sf
				move	r9, r7				// registers are not saved or cleared
				load	r10, sf[1]			// saved SF
				load	r11, sf[3]			// return PC
				iret
			""";
		var sim = runProgram(src).getMiddle();
		assertEquals(77, sim.R[9]);
		assertEquals(sim.R[21], sim.R[10]);
		assertEquals(sim.R[22] - 3, sim.R[8], "SF = SP after pushing 3 words");
		assertTrue(sim.R[11] > 0, "return PC is the instruction after INT");
	}

	@Test
	void testNegativeNumberRunsHostServiceAndMatchesPositive() {
		String src = """
			START:
			#include <system/system.def>
				int		iGET_PID
				move	r1, r0
				int		-34
				move	r2, r0
				move	r3, iGET_PID
				int		r3
				move	r4, r0
				neg		r3
				int		r3
				move	r5, r0
				stop
				stop
			""";
		var sim = runProgram(src).getMiddle();
		assertTrue(sim.R[1] > 0);
		assertEquals(sim.R[1], sim.R[2]);
		assertEquals(sim.R[1], sim.R[4]);
		assertEquals(sim.R[1], sim.R[5]);
	}

	@Test
	void testEmptyVectorFallsThroughToHostService() {
		String src = """
			START:
			#include <system/system.def>
				move	r1, 0x1800
				stcr	0, r1
				int		iGET_PID
				move	r2, r0
				int		-34
				move	r3, r0
				stop
				stop
			""";
		var sim = runProgram(src).getMiddle();
		assertTrue(sim.R[2] > 0);
		assertEquals(sim.R[2], sim.R[3]);
	}

	@Test
	void testInstalledVectorOverridesHostServiceAndCanWrapIt() {
		String src = """
			START:
			#include <system/system.def>
				move	r1, 0x1800
				move	r2, HGET
				store	r2, 0x1822			// vector 34 = iGET_PID
				stcr	0, r1
				int		-34					// built-in, never consults the table
				move	r10, r0
				int		34					// vectored: wraps the built-in
				move	r11, r0
				stop
				stop
			HGET:	int		-34
				add		r0, 1000
				iret
			""";
		var sim = runProgram(src).getMiddle();
		assertTrue(sim.R[10] > 0 && sim.R[10] < 1000);
		assertEquals(sim.R[10] + 1000, sim.R[11]);
	}

	@Test
	void testNestedInterrupts() {
		String src = """
			START:
				move	r1, 0x1800
				move	r2, H5
				store	r2, 0x1805
				move	r2, H6
				store	r2, 0x1806
				stcr	0, r1
				move	r20, sp
				int		5
				move	r21, sp
				stop
				stop
			H5:	add		r3, 1
				int		6
				add		r3, 10
				iret
			H6:	add		r3, 100
				iret
			""";
		var sim = runProgram(src).getMiddle();
		assertEquals(111, sim.R[3]);
		assertEquals(sim.R[20], sim.R[21]);
	}

	@Test
	void testConditionalInterrupt() {
		// The assembler has no conditional INT syntax, so build the words directly.
		// Condition codes: 1 = z, 2 = nz. MOVE R1,0x1800 leaves Z clear.
		java.util.List<Long> prog = new java.util.ArrayList<>();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 0x1800));       // 0: MOVE R1, 0x1800
		prog.add(Simulator.encT0(Opcode.STCR.code, 1, 2, 0, 0, 0, 1, 0, 0)); // 1: STCR 0, R1
		prog.add(Simulator.encT2(Opcode.INTERRUPT.code, 1, 1, 5));       // 2: INT z, 5   (not taken)
		prog.add(Simulator.encT2(Opcode.INTERRUPT.code, 1, 2, 5));       // 3: INT nz, 5  (taken)
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));  // 4
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));  // 5
		prog.add(Simulator.encT2(Opcode.ADD.code, 2, 3, 1));             // 6: H5: ADD R3, 1
		prog.add(Simulator.encT0(Opcode.IRET.code, 0, 0, 0, 0, 0, 0, 0, 0));  // 7
		Simulator sim = new Simulator(0x2000, 0x1000, 0x500, new String[] {"test"});
		sim.loadProgram(prog, 0L);
		sim.memWrite(0x1805, 6);
		sim.run(0L);
		assertEquals(1, sim.R[3], "handler ran once: only the taken conditional INT");
	}

	@Test
	void testIntZeroIsNoOp() {
		String src = """
			START:
				move	r1, 0x1800
				move	r2, HEXIT
				store	r2, 0x1800
				stcr	0, r1
				int		0
				move	r3, 5
				stop
				stop
			HEXIT:	move	r3, 99
				stop
			""";
		var sim = runProgram(src).getMiddle();
		assertEquals(5, sim.R[3]);
	}

	@Test
	void testControlRegisterReadBack() {
		String src = """
			START:
				ldcr	r1, 0
				move	r2, 0x1800
				stcr	0, r2
				ldcr	r3, 0
				move	r4, 0
				stcr	0, r4
				ldcr	r5, 0
				stop
				stop
			""";
		var sim = runProgram(src).getMiddle();
		assertEquals(0, sim.R[1]);
		assertEquals(0x1800, sim.R[3]);
		assertEquals(0, sim.R[5]);
		assertEquals(0, sim.getIvtBase());
	}

	@Test
	void testBadControlRegisterAndBase() {
		var ex = assertThrows(Simulator.CPUException.class, () -> runProgram("""
			START:
				ldcr	r1, 7
				stop
				stop
			"""));
		assertTrue(ex.getMessage().contains("Illegal control register 7"), ex.getMessage());

		ex = assertThrows(Simulator.CPUException.class, () -> runProgram("""
			START:
				move	r1, -1
				stcr	0, r1
				stop
				stop
			"""));
		assertTrue(ex.getMessage().contains("Illegal interrupt vector table base"), ex.getMessage());

		ex = assertThrows(Simulator.CPUException.class, () -> runProgram("""
			START:
				move	r1, 0x1F01			// base + 255 would pass the end of memory
				stcr	0, r1
				stop
				stop
			"""));
		assertTrue(ex.getMessage().contains("Illegal interrupt vector table base"), ex.getMessage());
	}

	@Test
	void testExitHookReceivesExitCode() {
		String src = """
			START:
			#include <system/system.def>
				move	r1, 0x1800
				move	r2, HEXIT
				store	r2, 0x1800
				stcr	0, r1
				move	r1, 3
				int		-32					// iEXIT: the hook runs instead of terminating
				move	r9, 111				// never reached
				stop
				stop
			HEXIT:	move	r9, r0
				stop
			""";
		var tuple = runProgram(src);
		assertEquals(3, tuple.getMiddle().R[9]);
	}

	@Test
	void testExitHookOnReturnToNegativePc() {
		String src = """
			START:
				move	r1, 0x1800
				move	r2, HEXIT
				store	r2, 0x1800
				stcr	0, r1
				push	-5
				push	0
				move	sf, sp
				return
				stop
				stop
			HEXIT:	move	r9, r0
				stop
			""";
		var tuple = runProgram(src);
		assertEquals(-5, tuple.getMiddle().R[9]);
	}

	@Test
	void testExitHookCanBeDisarmedToTerminate() {
		String src = """
			START:
			#include <system/system.def>
				move	r1, 0x1800
				move	r2, HEXIT
				store	r2, 0x1800
				stcr	0, r1
				move	r1, 4
				int		-32
				stop
				stop
			HEXIT:	add		r8, 1				// count hook entries
				move	r6, 0
				store	r6, 0x1800			// clear vector 0
				move	r1, r0
				int		-32					// now terminates for real
				move	r9, 111
				stop
			""";
		var tuple = runProgram(src);
		var sim = tuple.getMiddle();
		assertEquals(1, sim.R[8]);
		assertEquals(0, sim.R[9]);
		assertEquals(4, tuple.getLeft());
	}

	@Test
	void testThreadInheritsTableAndDoesNotRunExitHook() {
		String src = """
			START:
			#include <system/system.def>
				move	r1, 0x1800
				move	r2, H5
				store	r2, 0x1805
				move	r2, HEXIT
				store	r2, 0x1800
				stcr	0, r1
				move	r1, WORKER
				move	r2, 0
				int		-40					// iTHREAD
				move	r1, r0
				int		-41					// iJOIN_THREAD
				load	r4, 0x1900			// bumped by the handler, called from the thread
				load	r5, 0x1901			// bumped only if the exit hook ran
				stop
				stop
			WORKER:	int		5				// the thread uses the inherited table
				return							// ends the thread; the exit hook is main-thread only
			H5:		load	r3, 0x1900
				add		r3, 1
				store	r3, 0x1900
				iret
			HEXIT:	load	r3, 0x1901
				add		r3, 1
				store	r3, 0x1901
				stop
			""";
		var sim = runProgram(src).getMiddle();
		assertEquals(1, sim.R[4]);
		assertEquals(0, sim.R[5]);
	}

	@Test
	void testNoHookWithoutTable() {
		String src = """
			START:
			#include <system/system.def>
				move	r1, 6
				int		iEXIT
				move	r9, 111
				stop
				stop
			""";
		var tuple = runProgram(src);
		assertEquals(0, tuple.getMiddle().R[9]);
		assertEquals(6, tuple.getLeft());
	}

	@Test
	void testCycleCosts() {
		String base = """
			START:
			#include <system/system.def>
				move	r1, 0x1800
				move	r2, H5
				store	r2, 0x1805
				stcr	0, r1
				%s
				stop
				stop
			H5:	iret
			""";
		long nop = runProgram(base.formatted("nop")).getMiddle().cycles;
		long host = runProgram(base.formatted("int -10")).getMiddle().cycles;   // iCYCLES
		long vectored = runProgram(base.formatted("int 5")).getMiddle().cycles;
		assertEquals(6, host - nop, "host service: 3 pushes + 3 pops");
		assertEquals(7, vectored - nop, "vectored: 3 pushes + IRET fetch + 3 pops");
	}

	@Test
	void testSaveRestoreStillWorkAsHostServices() {
		String src = """
			START:
			#include <system/system.def>
				move	r1, 11
				move	r2, 22
				move	r20, sp
				int		-12				// iSAVE
				move	r1, 0
				move	r2, 0
				int		-14				// iRESTORE
				move	r21, sp
				stop
				stop
			""";
		var sim = runProgram(src).getMiddle();
		assertEquals(11, sim.R[1]);
		assertEquals(22, sim.R[2]);
		assertEquals(sim.R[20], sim.R[21]);
	}

	@Test
	void testDisassembly() {
		var d = Simulator.Decoded.decode(Simulator.encT0(Opcode.LDCR.code, 2, 1, 0, 0, 3, 0, 0, 0));
		assertTrue(d.disassemble(null).startsWith("LDCR"), d.disassemble(null));
		d = Simulator.Decoded.decode(Simulator.encT0(Opcode.STCR.code, 1, 2, 0, 0, 0, 4, 0, 0));
		assertTrue(d.disassemble(null).startsWith("STCR"), d.disassemble(null));
		d = Simulator.Decoded.decode(Simulator.encT0(Opcode.IRET.code, 0, 0, 0, 0, 0, 0, 0, 0));
		assertTrue(d.disassemble(null).startsWith("IRET"), d.disassemble(null));
	}
}
