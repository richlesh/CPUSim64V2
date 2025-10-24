package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

public class SimulatorTest {
	public void assertDiff(List<String> diffs, int val) {
		assertTrue(diffs.contains("SR:" + val));
	}

	public void assertDiff(List<String> diffs, int reg, long val) {
		if (reg == Simulator.R_SF)
			assertTrue(diffs.contains("SF:" + val));
		else if (reg == Simulator.R_SP)
			assertTrue(diffs.contains("SP:" + val));
		else if (reg == Simulator.R_PC)
			assertTrue(diffs.contains("PC:" + val));
		else
			assertTrue(diffs.contains("R" + reg + ":" + val));
	}

	public void assertDiff(List<String> diffs, int reg, double val) {
		assertTrue(diffs.contains("F" + reg + ":" + val));
	}

	public void assertMem(Simulator sim, int addr, long val) {
		assertEquals(val, sim.memRead(addr));
	}

	public void assertMem(Simulator sim, int addr, double val) {
		assertEquals(Double.doubleToRawLongBits(val), sim.mem[addr]);
	}

	@Test
	void testSignExtend() {
		assertEquals(1L, Simulator.signExtend(1L, 4));
		assertEquals(2L, Simulator.signExtend(2L, 4));
		assertEquals(4L, Simulator.signExtend(4L, 4));
		assertEquals(-8L, Simulator.signExtend(8L, 4));
		assertEquals(0L, Simulator.signExtend(16L, 4));
		assertEquals(0L, Simulator.signExtend(0L, 4));
		assertEquals(-1L, Simulator.signExtend(-1L, 4));
		assertEquals(-2L, Simulator.signExtend(-2L, 4));
		assertEquals(-4L, Simulator.signExtend(-4L, 4));
		assertEquals(-8L, Simulator.signExtend(-8L, 4));
		assertEquals(0L, Simulator.signExtend(-16L, 4));
		assertEquals(7L, Simulator.signExtend(0x7, 4));
		assertEquals(-8L, Simulator.signExtend(0x8, 4));
		assertEquals(-7L, Simulator.signExtend(0x9, 4));
		assertEquals(-2L, Simulator.signExtend(0xe, 4));
		assertEquals(-1L, Simulator.signExtend(0xf, 4));
	}

	@Test
	void testDecoder() {
		var t0 = Simulator.Decoded.decode(Simulator.encT0(Opcode.DEBUG.code, 0, 1, 2, 3, 4, 5, 6, 7));
		var t1 = Simulator.Decoded.decode(Simulator.encT1(Opcode.ADD.code, 0x1234_5678_9abc_def0L));
		var t2 = Simulator.Decoded.decode(Simulator.encT2(Opcode.MOVE.code, 1, 2, 0x1234_5678_9abc_def0L));
		var t3 = Simulator.Decoded.decode(Simulator.encT3(Opcode.JUMP.code, 2, 3, 4, 5, 0x1234_5678));
		assertEquals(0, t0.getType());
		assertEquals(1, t1.getType());
		assertEquals(2, t2.getType());
		assertEquals(3, t3.getType());
		assertEquals(Opcode.DEBUG.code, t0.getOpCode());
		assertEquals(Opcode.ADD.code, t1.getOpCode());
		assertEquals(Opcode.MOVE.code, t2.getOpCode());
		assertEquals(Opcode.JUMP.code, t3.getOpCode());
		assertEquals(0, t0.a);
		assertEquals(1, t0.b);
		assertEquals(2, t0.c);
		assertEquals(3, t0.d);
		assertEquals(4, t0.v0);
		assertEquals(5, t0.v1);
		assertEquals(6, t0.v2);
		assertEquals(7, t0.v3);
		assertEquals(0x34_5678_9abc_def0L, t1.c1);
		assertEquals(1, t2.a);
		assertEquals(2, t2.v0);
		assertEquals(0xFFFF_FE78_9abc_def0L, t2.c2);
	}

	@Test
	void testClear() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();

		// Clear all test (N)
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		for (int i = 0; i < sim.GPR_COUNT - 3; ++i)
			sim.R[i] = i + 1;
		for (int i = 0; i < sim.FPR_COUNT; ++i)
			sim.F[i] = i + 1;
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(sim.GPR_COUNT + sim.FPR_COUNT - 3 + 2, diff.size());
		for (int i = 0; i < sim.GPR_COUNT - 3; ++i)
			assertEquals(0, sim.R[i]);
		for (int i = 0; i < sim.FPR_COUNT; ++i)
			assertEquals(0, sim.F[i]);
		assertNotEquals(0, sim.R[sim.R_PC]);
		assertNotEquals(0, sim.R[sim.R_SP]);
		assertNotEquals(0, sim.R[sim.R_SF]);
		assertEquals(sim.SR_Z, sim.SR);

		// Clear one register R/F (Y)
		prog.clear();
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 2, 0, 0, 0, 3, 0, 0, 0));
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 3, 0, 0, 0, 4, 0, 0, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		for (int i = 0; i < sim.GPR_COUNT - 3; ++i)
			sim.setR(2, i, i + 1);
		for (int i = 0; i < sim.FPR_COUNT; ++i)
			sim.setFP(3, i, i + 1);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, 3, 0);
		assertDiff(diff, 4, 0.);
		assertDiff(diff, sim.SR_Z);

		// Clear two register R/F (YY)
		prog.clear();
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 2, 2, 0, 0, 4, 5, 0, 0));
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 3, 3, 0, 0, 5, 6, 0, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 4, 0);
		assertDiff(diff, 5, 0);
		assertDiff(diff, 5, 0.);
		assertDiff(diff, 6, 0.);
		assertDiff(diff, sim.SR_Z);

		// Clear three register R/F (YYY)
		prog.clear();
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 2, 2, 2, 0, 6, 7, 8, 0));
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 3, 3, 3, 0, 7, 8, 9, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(8, diff.size());
		assertDiff(diff, 6, 0);
		assertDiff(diff, 7, 0);
		assertDiff(diff, 8, 0);
		assertDiff(diff, 7, 0.);
		assertDiff(diff, 8, 0.);
		assertDiff(diff, 9, 0.);
		assertDiff(diff, sim.SR_Z);

		// Clear four register R/F (YYYY)
		prog.clear();
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 2, 2, 2, 2, 9, 10, 11, 12));
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 3, 3, 3, 3, 10, 11, 12, 13));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(10, diff.size());
		assertDiff(diff, 9, 0);
		assertDiff(diff, 10, 0);
		assertDiff(diff, 11, 0);
		assertDiff(diff, 12, 0);
		assertDiff(diff, 10, 0.);
		assertDiff(diff, 11, 0.);
		assertDiff(diff, 12, 0.);
		assertDiff(diff, 13, 0.);
		assertDiff(diff, sim.SR_Z);

		// Clear four register mix R/F (YYYY)
		prog.clear();
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 2, 3, 2, 3, 20, 21, 22, 23));
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 3, 2, 3, 2, 24, 25, 26, 27));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(10, diff.size());
		assertDiff(diff, 20, 0);
		assertDiff(diff, 22, 0);
		assertDiff(diff, 25, 0);
		assertDiff(diff, 27, 0);
		assertDiff(diff, 21, 0.);
		assertDiff(diff, 23, 0.);
		assertDiff(diff, 24, 0.);
		assertDiff(diff, 26, 0.);
		assertDiff(diff, sim.SR_Z);
	}

	@Test
	void testMove() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// move YC
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 1234));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 1, 4567));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, 1, 1234);
		assertDiff(diff, 1, 4567.);
		assertDiff(diff, sim.SR_P);

		// move YY  (XX and FF)
		prog.clear();
		prog.add(Simulator.encT0(Opcode.MOVE.code, 2, 2, 0, 0, 2, 1, 0, 0));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3, 3, 0, 0, 2, 1, 0, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, 2, 1234);
		assertDiff(diff, 2, 4567.);
		assertDiff(diff, sim.SR_P);

		// move YY  (XF and FX)
		prog.clear();
		prog.add(Simulator.encT0(Opcode.MOVE.code, 2, 3, 0, 0, 3, 1, 0, 0));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3, 2, 0, 0, 3, 1, 0, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, 3, 4567);
		assertDiff(diff, 3, 1234.);
		assertDiff(diff, sim.SR_P);

		// move AAC/ACA
		prog.clear();
		prog.add(Simulator.encT3(Opcode.MOVE.code, 2, 2, 4, 1, 5));
		prog.add(Simulator.encT3(Opcode.MOVE.code, 2, 2, 5, 3, 6));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, 4, 1234 + 5);
		assertDiff(diff, 5, 4567 + 6);
		assertDiff(diff, 0);

		// move AAR
		prog.clear();
		prog.add(Simulator.encT0(Opcode.MOVE.code, 2, 2, 2, 0, 6, 1, 3, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, 6, 1234 + 4567);
		assertDiff(diff, sim.SR_P);

		// move ZYQQ (ZRCC and ZFCC) with Z True and False
		prog.clear();
		sim.SR = sim.SR_Z;
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 2, 1, 1, Condition.EQ.ordinal(), 7, 326, 623));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 3, 1, 1, Condition.NE.ordinal(), 7, 123, 321));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 2, 1, 1, Condition.LT.ordinal(), 8, 326, 623));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 3, 1, 1, Condition.LT.ordinal(), 8, 123, 321));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 7, 326);
		assertDiff(diff, 7, 123.);
		assertDiff(diff, 8, 623);
		assertDiff(diff, 8, 321.);
		assertDiff(diff, 0);

		// move ZYQQ (ZRRR and ZFFF) with Z True and False
		prog.clear();
		sim.SR = sim.SR_Z;
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 2, 2, 2, Condition.EQ.ordinal(), 9, 1, 3));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 3, 3, 3, Condition.NE.ordinal(), 9, 1, 3));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 2, 2, 2, Condition.LT.ordinal(), 10, 1, 3));
		prog.add(Simulator.encT0(Opcode.MOVE.code, 1, 3, 3, 3, Condition.LT.ordinal(), 10, 1, 3));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 9, 1234);
		assertDiff(diff, 9, 4567.);
		assertDiff(diff, 10, 4567);
		assertDiff(diff, 10, 1234.);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testLoad() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// load YC, YA
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 8));                // MOVE R1,8
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 10));                // MOVE R2,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 3, 7));                // LOAD R3,7
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 3, 9));                // LOAD F3,9
		prog.add(Simulator.encT0(Opcode.LOAD.code, 2, 2, 0, 0, 4, 1, 0, 0));        // LOAD R4,R1
		prog.add(Simulator.encT0(Opcode.LOAD.code, 3, 2, 0, 0, 4, 2, 0, 0));        // LOAD F4,R2
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(1234L);
		prog.add(5678L);
		prog.add(Double.doubleToRawLongBits(1111.));
		prog.add(Double.doubleToRawLongBits(2222.));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(8, diff.size());
		assertDiff(diff, 3, 1234);
		assertDiff(diff, 3, 1111.);
		assertDiff(diff, 4, 5678);
		assertDiff(diff, 4, 2222.);
		assertDiff(diff, 0);

		// load YAC,YCA,YCC,YAR
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 10));                // MOVE R1,10
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 14));                // MOVE R2,14
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 1));                // MOVE R3,1
		prog.add(Simulator.encT3(Opcode.LOAD.code, 2, 2, 4, 1, 2));            // LOAD R4,R1,2
		prog.add(Simulator.encT3(Opcode.LOAD.code, 3, 2, 4, 2, 2));            // LOAD F4,R2,2
		prog.add(Simulator.encT3(Opcode.LOAD.code, 2, 1, 5, 10, 3));            // LOAD R5,10,3
		prog.add(Simulator.encT3(Opcode.LOAD.code, 3, 1, 5, 14, 3));            // LOAD F5,14,3
		prog.add(Simulator.encT0(Opcode.LOAD.code, 2, 2, 2, 0, 6, 1, 3, 0));        // LOAD R6,R1,R3
		prog.add(Simulator.encT0(Opcode.LOAD.code, 3, 2, 2, 0, 6, 2, 3, 0));        // LOAD F6,R2,R3
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(1234L);
		prog.add(5678L);
		prog.add(9012L);
		prog.add(3456L);
		prog.add(Double.doubleToRawLongBits(1111.));
		prog.add(Double.doubleToRawLongBits(2222.));
		prog.add(Double.doubleToRawLongBits(3333.));
		prog.add(Double.doubleToRawLongBits(4444.));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(11, diff.size());
		assertDiff(diff, 4, 9012L);
		assertDiff(diff, 4, 3333.);
		assertDiff(diff, 5, 3456L);
		assertDiff(diff, 5, 4444.);
		assertDiff(diff, 6, 5678L);
		assertDiff(diff, 6, 2222.);
		assertDiff(diff, 0);
	}

	@Test
	void testStore() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// store YC, YA
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 8));                	// MOVE R1,8
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 10));               	// MOVE R2,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 3, 7));                	// LOAD R3,7
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 3, 9));                	// LOAD F3,9
		prog.add(Simulator.encT0(Opcode.LOAD.code, 2, 2, 0, 0, 4, 1, 0, 0));    // LOAD R4,R1
		prog.add(Simulator.encT0(Opcode.LOAD.code, 3, 2, 0, 0, 4, 2, 0, 0));    // LOAD F4,R2
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(1234L);
		prog.add(5678L);
		prog.add(Double.doubleToRawLongBits(1111.));
		prog.add(Double.doubleToRawLongBits(2222.));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(8, diff.size());
		assertDiff(diff, 3, 1234);
		assertDiff(diff, 3, 1111.);
		assertDiff(diff, 4, 5678);
		assertDiff(diff, 4, 2222.);
		assertDiff(diff, 0);

		// store YAC,YCA,YCC,YAR
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 16));                // MOVE R1,16
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 20));                // MOVE R2,20
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 1));                // MOVE R3,1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 4, 1));                // MOVE R4,1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 5, 11));                // MOVE R5,11
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 6, 111));                // MOVE R6,111
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 4, 2));                // MOVE F4,2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 5, 22));                // MOVE F5,22
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 6, 222));                // MOVE F6,222
		prog.add(Simulator.encT3(Opcode.STORE.code, 2, 2, 4, 1, 2));            // STORE R4,R1,2
		prog.add(Simulator.encT3(Opcode.STORE.code, 3, 2, 4, 2, 2));            // STORE F4,R2,2
		prog.add(Simulator.encT3(Opcode.STORE.code, 2, 1, 5, 16, 3));            // STORE R5,16,3
		prog.add(Simulator.encT3(Opcode.STORE.code, 3, 1, 5, 20, 3));            // STORE F5,20,3
		prog.add(Simulator.encT0(Opcode.STORE.code, 2, 2, 2, 0, 6, 1, 3, 0));        // STORE R6,R1,R3
		prog.add(Simulator.encT0(Opcode.STORE.code, 3, 2, 2, 0, 6, 2, 3, 0));        // STORE F6,R2,R3
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(1234L);
		prog.add(5678L);
		prog.add(9012L);
		prog.add(3456L);
		prog.add(Double.doubleToRawLongBits(1111.));
		prog.add(Double.doubleToRawLongBits(2222.));
		prog.add(Double.doubleToRawLongBits(3333.));
		prog.add(Double.doubleToRawLongBits(4444.));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(11, diff.size());
		assertMem(sim, 16, 1234L);
		assertMem(sim, 17, 111L);
		assertMem(sim, 18, 1L);
		assertMem(sim, 19, 11L);
		assertMem(sim, 20, 1111.);
		assertMem(sim, 21, 222.);
		assertMem(sim, 22, 2.);
		assertMem(sim, 23, 22.);
		assertDiff(diff, 0);

		// STORE CAC,CCA,CCC,CAR
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 6));                // MOVE R1,6
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 1));                // MOVE R3,1
		prog.add(Simulator.encT3(Opcode.STORE.code, 1, 2, 4, 1, 2));            // STORE 4,R1,2
		prog.add(Simulator.encT3(Opcode.STORE.code, 1, 1, 5, 6, 3));            // STORE 6,6,3
		prog.add(Simulator.encT0(Opcode.STORE.code, 1, 2, 2, 0, 6, 1, 3, 0));        // STORE 8,R1,R3
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(1234L);
		prog.add(5678L);
		prog.add(9012L);
		prog.add(3456L);
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertMem(sim, 6, 1234L);
		assertMem(sim, 7, 6L);
		assertMem(sim, 8, 4L);
		assertMem(sim, 9, 5L);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testPushPop() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// push Y, C
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 2, 31415));            // MOVE F2,31415
		prog.add(Simulator.encT1(Opcode.PUSH.code, 123456));                // PUSH 123456
		prog.add(Simulator.encT0(Opcode.PUSH.code, 2, 0, 0, 0, 1, 0, 0, 0));        // PUSH R1
		prog.add(Simulator.encT0(Opcode.PUSH.code, 3, 0, 0, 0, 2, 0, 0, 0));        // PUSH F2
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.R_SP, 1096);
		assertMem(sim, 1099, 123456L);
		assertMem(sim, 1098, 326L);
		assertMem(sim, 1097, Double.doubleToRawLongBits(31415.));
		assertDiff(diff, 0);

		// pop N, Y
		sim.mem[0] = Simulator.encT0(Opcode.POP.code, 3, 0, 0, 0, 4, 0, 0, 0);    // POP F4
		sim.mem[1] = Simulator.encT0(Opcode.POP.code, 2, 0, 0, 0, 3, 0, 0, 0);    // POP R3
		sim.mem[2] = Simulator.encT0(Opcode.POP.code, 0, 0, 0, 0, 0, 0, 0, 0);    // POP
		sim.mem[3] = Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0);
		sim.R[sim.R_PC] = 0;
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.R_SP, 1099);
		assertDiff(diff, 3, 326);
		assertDiff(diff, 4, 31415.);
	}

	@Test
	void testJump() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// C
		prog.add(Simulator.encT1(Opcode.JUMP.code, 3));                        // JUMP 3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 0);

		// A
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 4));                // MOVE R2,4
		prog.add(Simulator.encT0(Opcode.JUMP.code, 2, 0, 0, 0, 2, 0, 0, 0));        // JUMP R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 0);

		// ZC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 4));                        // MOVE R2,4
		prog.add(Simulator.encT2(Opcode.JUMP.code, 1, Condition.NE.ordinal(), 4));    // JUMP NZ,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                    // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                        // MOVE R1,326
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 9));                        // MOVE R2,9
		prog.add(Simulator.encT2(Opcode.JUMP.code, 1, Condition.EQ.ordinal(), 9));    // JUMP Z,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                    // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                        // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.SR_P);

		// ZA
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 4));                                // MOVE R2,4
		prog.add(Simulator.encT0(Opcode.JUMP.code, 1, 2, 0, 0, Condition.NE.ordinal(), 2, 0, 0));    // JUMP NZ,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                // MOVE R1,326
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 9));                                // MOVE R2,9
		prog.add(Simulator.encT0(Opcode.JUMP.code, 1, 2, 0, 0, Condition.EQ.ordinal(), 2, 0, 0));    // JUMP Z,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                            // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.SR_P);

		// AC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                // MOVE R2,3
		prog.add(Simulator.encT2(Opcode.JUMP.code, 2, 2, 1));                // JUMP R2 + 1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 0);

		// AR
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                // MOVE R2,3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 2));                // MOVE R0,1
		prog.add(Simulator.encT0(Opcode.JUMP.code, 2, 2, 0, 0, 2, 0, 0, 0));        // JUMP R2 + R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 0);

		// ZAC,ZCA
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                                // MOVE R2,3
		prog.add(Simulator.encT3(Opcode.JUMP.code, 1, 2, Condition.NE.ordinal(), 2, 1));        // JUMP NZ,R2 + 1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                // MOVE R1,326
		prog.add(Simulator.encT3(Opcode.JUMP.code, 1, 2, Condition.EQ.ordinal(), 2, 5));        // JUMP Z,R2 + 5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                            // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.SR_P);

		// ZCC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                                // MOVE R2,3
		prog.add(Simulator.encT3(Opcode.JUMP.code, 1, 1, Condition.NE.ordinal(), 3, 1));        // JUMP NZ,3 + 1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                // MOVE R1,326
		prog.add(Simulator.encT3(Opcode.JUMP.code, 1, 1, Condition.EQ.ordinal(), 3, 5));        // JUMP Z,3 + 5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                            // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.SR_P);

		// ZAR
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                                    // MOVE R2,3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 4, 2));                                    // MOVE R4,2
		prog.add(Simulator.encT0(Opcode.JUMP.code, 1, 2, 2, 0, Condition.NE.ordinal(), 2, 4, 0));        // JUMP NZ,R2 + R4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                                // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                    // MOVE R1,326
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 4, 7));                                    // MOVE R4,7
		prog.add(Simulator.encT0(Opcode.JUMP.code, 1, 2, 2, 0, Condition.EQ.ordinal(), 2, 4, 0));        // JUMP Z,R2 + R4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                                // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                    // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testCall() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// C
		prog.add(Simulator.encT1(Opcode.CALL.code, 3));                        // CALL 3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.R_SP, 1097);
		assertDiff(diff, sim.R_SF, 1097);
		assertDiff(diff, 1, 326);
		assertDiff(diff, 0);

		// A
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 4));                // MOVE R2,4
		prog.add(Simulator.encT0(Opcode.CALL.code, 2, 0, 0, 0, 2, 0, 0, 0));        // CALL R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, sim.R_SP, 1097);
		assertDiff(diff, sim.R_SF, 1097);
		assertDiff(diff, 0);

		// ZC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 4));                        // MOVE R2,4
		prog.add(Simulator.encT2(Opcode.CALL.code, 1, Condition.NE.ordinal(), 4));    // CALL NZ,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                    // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                        // MOVE R1,326
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 9));                        // MOVE R2,9
		prog.add(Simulator.encT2(Opcode.CALL.code, 1, Condition.EQ.ordinal(), 9));    // CALL Z,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                    // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                        // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(7, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.R_SP, 1095);
		assertDiff(diff, sim.R_SF, 1095);
		assertDiff(diff, sim.SR_P);

		// ZA
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 4));                                // MOVE R2,4
		prog.add(Simulator.encT0(Opcode.CALL.code, 1, 2, 0, 0, Condition.NE.ordinal(), 2, 0, 0));    // CALL NZ,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                // MOVE R1,326
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 9));                                // MOVE R2,9
		prog.add(Simulator.encT0(Opcode.CALL.code, 1, 2, 0, 0, Condition.EQ.ordinal(), 2, 0, 0));    // CALL Z,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                            // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(7, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.R_SP, 1095);
		assertDiff(diff, sim.R_SF, 1095);
		assertDiff(diff, sim.SR_P);

		// AC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                // MOVE R2,3
		prog.add(Simulator.encT2(Opcode.CALL.code, 2, 2, 1));                // CALL R2 + 1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, sim.R_SP, 1097);
		assertDiff(diff, sim.R_SF, 1097);
		assertDiff(diff, 0);

		// AR
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                // MOVE R2,3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 2));                // MOVE R0,1
		prog.add(Simulator.encT0(Opcode.CALL.code, 2, 2, 0, 0, 2, 0, 0, 0));        // CALL R2 + R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                // MOVE R1,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(7, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, sim.R_SP, 1097);
		assertDiff(diff, sim.R_SF, 1097);
		assertDiff(diff, 0);

		// ZAC,ZCA
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                                // MOVE R2,3
		prog.add(Simulator.encT3(Opcode.CALL.code, 1, 2, Condition.NE.ordinal(), 2, 1));        // CALL NZ,R2 + 1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                // MOVE R1,326
		prog.add(Simulator.encT3(Opcode.CALL.code, 1, 2, Condition.EQ.ordinal(), 2, 5));        // CALL Z,R2 + 5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                            // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(7, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.R_SP, 1095);
		assertDiff(diff, sim.R_SF, 1095);
		assertDiff(diff, sim.SR_P);

		// ZCC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                                // MOVE R2,3
		prog.add(Simulator.encT3(Opcode.CALL.code, 1, 1, Condition.NE.ordinal(), 3, 1));        // CALL NZ,3 + 1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                            // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                // MOVE R1,326
		prog.add(Simulator.encT3(Opcode.CALL.code, 1, 1, Condition.EQ.ordinal(), 3, 5));        // CALL Z,3 + 5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                            // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(7, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.R_SP, 1095);
		assertDiff(diff, sim.R_SF, 1095);
		assertDiff(diff, sim.SR_P);

		// ZAR
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));                                    // MOVE R2,3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 4, 2));                                    // MOVE R4,2
		prog.add(Simulator.encT0(Opcode.CALL.code, 1, 2, 2, 0, Condition.NE.ordinal(), 2, 4, 0));        // CALL NZ,R2 + R4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));                                // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 326));                                    // MOVE R1,326
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 4, 7));                                    // MOVE R4,7
		prog.add(Simulator.encT0(Opcode.CALL.code, 1, 2, 2, 0, Condition.EQ.ordinal(), 2, 4, 0));        // CALL Z,R2 + R4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 31415));                                // MOVE R3,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));                                    // MOVE R3,326
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(8, diff.size());
		assertDiff(diff, 1, 326);
		assertDiff(diff, 3, 31415);
		assertDiff(diff, sim.R_SP, 1095);
		assertDiff(diff, sim.R_SF, 1095);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testReturn() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		prog.add(Simulator.encT1(Opcode.CALL.code, 3));             			// CALL 3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));				// MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 12345));				// MOVE R2,12345
		prog.add(Simulator.encT1(Opcode.CALL.code, 6));             			// CALL 6
		prog.add(Simulator.encT0(Opcode.RETURN.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 326));           		// MOVE R3,326
		prog.add(Simulator.encT0(Opcode.RETURN.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 2, 12345);
		assertDiff(diff, 3, 326);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testRInterrupt() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// C
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3,0, 21));           		// LOAD F0, 21
		prog.add(Simulator.encT1(Opcode.INTERRUPT.code, 106));             		// INT 106
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 1,0,0,0));			// MOVE F1,F0
		// A
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,0, 106));					// MOVE A0, 106
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3,0, 21));           		// LOAD F0, 21
		prog.add(Simulator.encT0(Opcode.INTERRUPT.code, 2,0,0,0, 0,0,0,0));		// INT A0
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 2,0,0,0));			// MOVE F2,F0
		// ZC
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3,0, 21));           		// LOAD F0, 21
		prog.add(Simulator.encT2(Opcode.INTERRUPT.code, 1,Condition.NE.ordinal(), 106)); // INT NZ, 106
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 3,0,0,0));			// MOVE F3,F0
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3,0, 21));           		// LOAD F0, 21
		prog.add(Simulator.encT2(Opcode.INTERRUPT.code, 1,Condition.EQ.ordinal(), 106)); // INT Z, 106
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 4,0,0,0));			// MOVE F4,F0
		// ZA
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,0, 106));					// MOVE A0, 106
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3,0, 21));           		// LOAD F0, 21
		prog.add(Simulator.encT0(Opcode.INTERRUPT.code, 1,2,0,0, Condition.NE.ordinal(),0,0,0));	// INT NZ A0
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 5,0,0,0));			// MOVE F5,F0
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3,0, 21));           		// LOAD F0, 21
		prog.add(Simulator.encT0(Opcode.INTERRUPT.code, 1,2,0,0, Condition.EQ.ordinal(),0,0,0));	// INT Z A0
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 6,0,0,0));			// MOVE F6,F0
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(10, diff.size());
		assertDiff(diff, 1, 4.);
		assertDiff(diff, 2, 4.);
		assertDiff(diff, 3, 4.);
		assertDiff(diff, 4, 3.5);
		assertDiff(diff, 5, 4.);
		assertDiff(diff, 6, 3.5);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testNegate() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// R, F
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 3, 9));           		// LOAD F3, 9
		prog.add(Simulator.encT0(Opcode.NEGATE.code, 2,0,0,0, 1,0,0,0));		// NEG R1
		prog.add(Simulator.encT0(Opcode.NEGATE.code, 3,0,0,0, 3,0,0,0));		// NEG F3
		prog.add(Simulator.encT0(Opcode.MOVE.code, 2,2,0,0, 2,1,0,0));          // MOVE R2,R1
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 2,3,0,0));          // MOVE F2,F3
		prog.add(Simulator.encT0(Opcode.NEGATE.code, 2,0,0,0, 2,0,0,0));		// NEG R2
		prog.add(Simulator.encT0(Opcode.NEGATE.code, 3,0,0,0, 2,0,0,0));		// NEG F2
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 1, -31415);
		assertDiff(diff, 3, -3.5);
		assertDiff(diff, 2, 31415);
		assertDiff(diff, 2, 3.5);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testAdd() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// AR, FX, YC
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 10));           		// LOAD F0,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 11));           		// LOAD F1,11
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 0,1,0,0));			// ADD R0,R1
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,0,0, 0,1,0,0));			// ADD F0,F1
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,2,0,0, 0,1,0,0));			// ADD F0,R1
		prog.add(Simulator.encT2(Opcode.ADD.code, 2, 0, -3));					// ADD R0,-3
		prog.add(Simulator.encT2(Opcode.ADD.code, 3, 0, -3));					// ADD F0,-3
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.1));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 0, 43757);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 0, 31414.4);
		assertDiff(diff, 1, -1.1);
		assertDiff(diff, 0);

		// AAR, FFX, AAC, FFC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 10));           		// LOAD F0,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 11));           		// LOAD F1,11
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,2,0, 2,0,1,0));			// ADD R2,R0,R1
		prog.add(Simulator.encT3(Opcode.ADD.code, 2,2, 3,0, 10));				// ADD R3,R0,10
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,3,0, 2,0,1,0));			// ADD F2,F0,F1
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,2,0, 3,0,1,0));			// ADD F3,F0,R1
		prog.add(Simulator.encT3(Opcode.ADD.code, 3,3, 4,1, 10));				// ADD F4,F1,10
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.1));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(11, diff.size());
		assertDiff(diff, 0, 12345);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 2, 43760);
		assertDiff(diff, 3, 12355);
		assertDiff(diff, 0, 3.5);
		assertDiff(diff, 1, -1.1);
		assertDiff(diff, 2, 2.4);
		assertDiff(diff, 3, 31418.5);
		assertDiff(diff, 4, 8.9);
		assertDiff(diff, 0);
	}

	@Test
	void testSubtract() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// AR, FX, YC
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 10));           		// LOAD F0,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 11));           		// LOAD F1,11
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 2,2,0,0, 0,1,0,0));		// SUB R0,R1
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 3,3,0,0, 0,1,0,0));		// SUB F0,F1
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 3,2,0,0, 0,1,0,0));		// SUB F0,R1
		prog.add(Simulator.encT2(Opcode.SUBTRACT.code, 2, 0, -3));				// SUB R0,-3
		prog.add(Simulator.encT2(Opcode.SUBTRACT.code, 3, 0, -3));				// SUB F0,-3
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.1));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 0, -19067);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 0, -31407.4);
		assertDiff(diff, 1, -1.1);
		assertDiff(diff, sim.SR_S);

		// AAR, FFX, AAC, FFC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 10));           		// LOAD F0,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 11));           		// LOAD F1,11
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 2,2,2,0, 2,0,1,0));		// SUB R2,R0,R1
		prog.add(Simulator.encT3(Opcode.SUBTRACT.code, 2,2, 3,0, 10));			// SUB R3,R0,10
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 3,3,3,0, 2,0,1,0));		// SUB F2,F0,F1
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 3,3,2,0, 3,0,1,0));		// SUB F3,F0,R1
		prog.add(Simulator.encT3(Opcode.SUBTRACT.code, 3,3, 4,1, 10));			// SUB F4,F1,10
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.1));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(11, diff.size());
		assertDiff(diff, 0, 12345);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 2, -19070);
		assertDiff(diff, 3, 12335);
		assertDiff(diff, 0, 3.5);
		assertDiff(diff, 1, -1.1);
		assertDiff(diff, 2, 4.6);
		assertDiff(diff, 3, -31411.5);
		assertDiff(diff, 4, -11.1);
		assertDiff(diff, sim.SR_S | sim.SR_P);
	}

	@Test
	void testMultiply() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// AR, FX, YC
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 10));           		// LOAD F0,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 11));           		// LOAD F1,11
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 2,2,0,0, 0,1,0,0));		// MUL R0,R1
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 3,3,0,0, 0,1,0,0));		// MUL F0,F1
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 3,2,0,0, 0,1,0,0));		// MUL F0,R1
		prog.add(Simulator.encT2(Opcode.MULTIPLY.code, 2, 0, -3));				// MUL R0,-3
		prog.add(Simulator.encT2(Opcode.MULTIPLY.code, 3, 0, -3));				// MUL F0,-3
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.25));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 0, -1163454525);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 0, 412321.875);
		assertDiff(diff, 1, -1.25);
		assertDiff(diff, sim.SR_P);

		// AAR, FFX, AAC, FFC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 10));           		// LOAD F0,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 11));           		// LOAD F1,11
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 2,2,2,0, 2,0,1,0));		// MUL R2,R0,R1
		prog.add(Simulator.encT3(Opcode.MULTIPLY.code, 2,2, 3,0, 10));			// MUL R3,R0,10
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 3,3,3,0, 2,0,1,0));		// MUL F2,F0,F1
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 3,3,2,0, 3,0,1,0));		// MUL F3,F0,R1
		prog.add(Simulator.encT3(Opcode.MULTIPLY.code, 3,3, 4,1, 10));			// MUL F4,F1,10
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.25));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(11, diff.size());
		assertDiff(diff, 0, 12345);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 2, 387818175);
		assertDiff(diff, 3, 123450);
		assertDiff(diff, 0, 3.5);
		assertDiff(diff, 1, -1.25);
		assertDiff(diff, 2, -4.375);
		assertDiff(diff, 3, 109952.5);
		assertDiff(diff, 4, -12.5);
		assertDiff(diff, sim.SR_S | sim.SR_P);
	}

	@Test
	void testDivide() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// AR, FX, YC
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 10));           		// LOAD F0,10
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 11));           		// LOAD F1,11
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 2,2,0,0, 1,0,0,0));		// DIV R1,R0
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 3,3,0,0, 0,1,0,0));		// DIV F0,F1
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 3,2,0,0, 0,1,0,0));		// DIV F0,R1
		prog.add(Simulator.encT2(Opcode.DIVIDE.code, 2, 0, -2));				// DIV R0,-2
		prog.add(Simulator.encT2(Opcode.DIVIDE.code, 3, 1, -2));				// DIV F1,-2
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.25));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 0, -6172);
		assertDiff(diff, 1, 2);
		assertDiff(diff, 0, -1.4);
		assertDiff(diff, 1, 0.625);
		assertDiff(diff, 0);

		// AAR, FFX, AAC, FFC, F
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 12));           		// LOAD F0,12
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 13));           		// LOAD F1,13
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 2,2,2,0, 2,1,0,0));		// DIV R2,R1,R0
		prog.add(Simulator.encT3(Opcode.DIVIDE.code, 2,2, 3,0, 10));			// DIV R3,R0,10
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 3,3,3,0, 2,0,1,0));		// DIV F2,F0,F1
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 3,3,2,0, 3,0,2,0));		// DIV F3,F0,R2
		prog.add(Simulator.encT3(Opcode.DIVIDE.code, 3,3, 4,1, 10));			// DIV F4,F1,10
		prog.add(Simulator.encT0(Opcode.MOVE.code, 3,3,0,0, 5,1,0,0));			// MOVE F5,F1
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 3,0,0,0, 5,0,0,0));		// RECIP F5
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(3.5));
		prog.add(Double.doubleToRawLongBits(-1.25));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(12, diff.size());
		assertDiff(diff, 0, 12345);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 2, 2);
		assertDiff(diff, 3, 1234);
		assertDiff(diff, 0, 3.5);
		assertDiff(diff, 1, -1.25);
		assertDiff(diff, 2, -2.8);
		assertDiff(diff, 3, 1.75);
		assertDiff(diff, 4, -0.125);
		assertDiff(diff, 5, -0.8);
		assertDiff(diff, sim.SR_S);

		// RRRR, RRRC
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));            	// MOVE R1,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 101));            		// MOVE R2,12345
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 2,2,2,2, 3,4,1,0));		// DIV R3,R4,R1,R0
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 2,2,2,1, 5,6,2,3));		// DIV R5,R6,R2,3
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(9, diff.size());
		assertDiff(diff, 0, 12345);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 2, 101);
		assertDiff(diff, 3, 2);
		assertDiff(diff, 4, 6725);
		assertDiff(diff, 5, 33);
		assertDiff(diff, 6, 2);
		assertDiff(diff, 0);
	}

	@Test
	void testComplement() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// R
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.COMPL.code, 2,0,0,0, 1,0,0,0));			// COMPL R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, 1, ~31415);
		assertDiff(diff, sim.SR_S | sim.SR_P);
	}

	@Test
	void testBitwiseLogical() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// RR, RC, RRR, RRC
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 12345));			// MOVE R0,12345
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));			// MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.AND.code, 2,2,2,0, 2,1,0,0));		// AND R2,R1,R0
		prog.add(Simulator.encT0(Opcode.OR.code, 2,2,2,0, 3,1,0,0));		// OR R3,R1,R0
		prog.add(Simulator.encT0(Opcode.XOR.code, 2,2,2,0, 4,1,0,0));		// XOR R4,R1,R0
		prog.add(Simulator.encT3(Opcode.AND.code, 2,2,5,1, 0xABCD));		// AND R5,R1,0xABCD
		prog.add(Simulator.encT3(Opcode.OR.code, 2,2,6,1, 0xABCD));			// OR R6,R1,0xABCD
		prog.add(Simulator.encT3(Opcode.XOR.code, 2,2,7,1, 0xABCD));		// XOR R7,R1,0xABCD
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 8, 31415));			// MOVE R8,31415
		prog.add(Simulator.encT0(Opcode.AND.code, 2,2,0,0, 8,0,0,0));		// AND R8,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 9, 31415));			// MOVE R9,31415
		prog.add(Simulator.encT0(Opcode.OR.code, 2,2,0,0, 9,0,0,0));		// OR R9,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 10, 31415));			// MOVE R10,31415
		prog.add(Simulator.encT0(Opcode.XOR.code, 2,2,0,0, 10,0,0,0));		// XOR R10,R0

		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,11, 31415));			// MOVE R11,31415
		prog.add(Simulator.encT2(Opcode.AND.code, 2,11, 0xABCD));			// AND R11,0xABCD
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,12, 31415));			// MOVE R12,31415
		prog.add(Simulator.encT2(Opcode.OR.code, 2,12, 0xABCD));			// AND R12,0xABCD
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,13, 31415));			// MOVE R13,31415
		prog.add(Simulator.encT2(Opcode.XOR.code, 2,13, 0xABCD));			// AND R13,0xABCD

		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(16, diff.size());
		assertDiff(diff, 0, 12345);
		assertDiff(diff, 1, 31415);
		assertDiff(diff, 2, 12345 & 31415);
		assertDiff(diff, 3, 12345 | 31415);
		assertDiff(diff, 4, 12345 ^ 31415);
		assertDiff(diff, 5, 31415 & 0xABCD);
		assertDiff(diff, 6, 31415 | 0xABCD);
		assertDiff(diff, 7, 31415 ^ 0xABCD);
		assertDiff(diff, 8, 12345 & 31415);
		assertDiff(diff, 9, 12345 | 31415);
		assertDiff(diff, 10, 12345 ^ 31415);
		assertDiff(diff, 11, 31415 & 0xABCD);
		assertDiff(diff, 12, 31415 | 0xABCD);
		assertDiff(diff, 13, 31415 ^ 0xABCD);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testTest() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

//		public static final int SR_P = 1 << 0; // Parity (1=odd, 0=even)
//		public static final int SR_Z = 1 << 1; // Zero
//		public static final int SR_S = 1 << 2; // Signed (negative)
//		public static final int SR_O = 1 << 3; // Overflow

		// R
		// positive int
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));            	// MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.TEST.code, 2,0,0,0, 1,0,0,0));			// TEST R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, sim.SR_P);

		// negative int
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, -31415));            	// MOVE R1,-31415
		prog.add(Simulator.encT0(Opcode.TEST.code, 2,0,0,0, 1,0,0,0));			// TEST R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, sim.SR_S);

		// zero int
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 0));            		// MOVE R1,0
		prog.add(Simulator.encT0(Opcode.TEST.code, 2,0,0,0, 1,0,0,0));			// TEST R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(2, diff.size());
		assertDiff(diff, sim.SR_Z);

		// positive float
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 1, 31415));            	// MOVE F1,31415
		prog.add(Simulator.encT0(Opcode.TEST.code, 3,0,0,0, 1,0,0,0));			// TEST F1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, 0);

		// negative float
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 1, -31415));            	// MOVE F1,-31415
		prog.add(Simulator.encT0(Opcode.TEST.code, 3,0,0,0, 1,0,0,0));			// TEST F1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, sim.SR_S | sim.SR_P);

		// zero float
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 1, 0));            		// MOVE F1,0
		prog.add(Simulator.encT0(Opcode.TEST.code, 3,0,0,0, 1,0,0,0));			// TEST F1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(2, diff.size());
		assertDiff(diff, sim.SR_Z);

		// int overflow addition
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 1, 3));     				// LOAD R1,3
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 1,1,0,0));			// ADD R1,R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Long.MAX_VALUE);
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_S | sim.SR_P);

		// int overflow subtraction
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 1, 4));     				// LOAD R1,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 2, 5));     				// LOAD R2,5
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 2,2,2,0, 3,2,1,0));		// SUB R3,R2,R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Long.MAX_VALUE);
		prog.add(Long.MIN_VALUE);
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_P);

		// int overflow multiplication
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 1, 4));     				// LOAD R1,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 2, 5));     				// LOAD R2,5
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 2,2,2,0, 3,2,1,0));		// MULT R3,R2,R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Long.MAX_VALUE);
		prog.add(Long.MIN_VALUE);
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_S | sim.SR_P);

		// int overflow division
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 1, 4));     				// LOAD R1,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 2, 5));     				// LOAD R2,5
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 2,2,2,0, 3,2,1,0));		// DIV R3,R2,R1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(-1L);
		prog.add(Long.MIN_VALUE);
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_S | sim.SR_P);

		// float overflow addition
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 3));     				// LOAD F1,3
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,0,0, 1,1,0,0));			// ADD F1,F1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(Double.MAX_VALUE));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_P);

		// float overflow subtraction
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 4));     				// LOAD F1,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 2, 5));     				// LOAD F2,5
		prog.add(Simulator.encT0(Opcode.SUBTRACT.code, 3,3,3,0, 3,2,1,0));		// SUB F3,F2,F1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(Double.MAX_VALUE));
		prog.add(Double.doubleToRawLongBits(-Double.MAX_VALUE));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_S);

		// float overflow multiplication
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 4));     				// LOAD F1,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 2, 5));     				// LOAD F2,5
		prog.add(Simulator.encT0(Opcode.MULTIPLY.code, 3,3,3,0, 3,2,1,0));		// MULT F3,F2,F1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(Double.MAX_VALUE));
		prog.add(Double.doubleToRawLongBits(Double.MAX_VALUE));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_P);

		// float overflow division
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 4));     				// LOAD F1,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 2, 5));     				// LOAD F2,5
		prog.add(Simulator.encT0(Opcode.DIVIDE.code, 3,3,3,0, 3,2,1,0));		// DIV F3,F2,F1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(Double.MIN_VALUE));
		prog.add(Double.doubleToRawLongBits(Double.MAX_VALUE));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertDiff(diff, sim.SR_O | sim.SR_P);
	}

	@Test
	void testCompare() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

//		public static final int SR_P = 1 << 0; // Parity (1=odd, 0=even)
//		public static final int SR_Z = 1 << 1; // Zero
//		public static final int SR_S = 1 << 2; // Signed (negative)
//		public static final int SR_O = 1 << 3; // Overflow

		// AA EQUAL
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 31415));               // MOVE R0,31415
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));               // MOVE R1,31415
		prog.add(Simulator.encT0(Opcode.CMP.code, 2, 2, 0, 0, 1, 0, 0, 0));		// CMP R1,R0
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, sim.SR_Z);

		// AA NOT EQUAL
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 31415));               // MOVE R0,31415
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, -31415));              // MOVE R1,-31415
		prog.add(Simulator.encT0(Opcode.CMP.code, 2, 2, 0, 0, 1, 0, 0, 0));		// CMP R1,R0
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, sim.SR_S | sim.SR_P);

		// AC EQUAL
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));               // MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.CMP.code, 2, 1, 31415));                // CMP R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, sim.SR_Z);

		// AC NOT EQUAL
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, -31415));              // MOVE R1,-31415
		prog.add(Simulator.encT2(Opcode.CMP.code, 2, 1, 31415));                // CMP R1,31415
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(3, diff.size());
		assertDiff(diff, sim.SR_S | sim.SR_P);

		// FF EQUAL
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 4));           		// LOAD F0,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 4));               	// LOAD F1,4
		prog.add(Simulator.encT0(Opcode.CMP.code, 3, 3, 0, 0, 1, 0, 0, 0));		// CMP F1,F0
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(31415.));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, sim.SR_Z);

		// FF NOT EQUAL
		sim.clearCPUState();
		prog.clear();
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 4));               	// LOAD F0,4
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 1, 5));              		// LOAD F1,5
		prog.add(Simulator.encT0(Opcode.CMP.code, 3, 3, 0, 0, 1, 0, 0, 0));		// CMP F1,F0
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Double.doubleToRawLongBits(31415));
		prog.add(Double.doubleToRawLongBits(-31415));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		startState = sim.getState();
		sim.run();
		diff = sim.diffState(startState);
		assertEquals(4, diff.size());
		assertDiff(diff, sim.SR_S);
	}

	@Test
	void testBitwiseShift() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// RR, RC, RRR, RRC
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 3));				// MOVE R0,3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 31415));			// MOVE R1,31415
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, -31415));			// MOVE R2,-31415
		prog.add(Simulator.encT0(Opcode.LSHIFT.code, 2,2,2,0, 3,1,0,0));	// LSH R3,R1,R0
		prog.add(Simulator.encT0(Opcode.RSHIFT.code, 2,2,2,0, 4,1,0,0));	// RSH R4,R1,R0
		prog.add(Simulator.encT0(Opcode.ARSHIFT.code, 2,2,2,0, 5,1,0,0));	// ARSH R5,R1,R0
		prog.add(Simulator.encT0(Opcode.LROTATE.code, 2,2,2,0, 6,1,0,0));	// LROT R6,R1,R0
		prog.add(Simulator.encT0(Opcode.RROTATE.code, 2,2,2,0, 7,1,0,0));	// RROT R7,R1,R0

		prog.add(Simulator.encT3(Opcode.LSHIFT.code, 2,2,8,1, 5));			// LSH R8,R1,5
		prog.add(Simulator.encT3(Opcode.RSHIFT.code, 2,2,9,1, 5));			// RSH R9,R1,5
		prog.add(Simulator.encT3(Opcode.ARSHIFT.code, 2,2,10,1, 5));		// ARSH R10,R1,5
		prog.add(Simulator.encT3(Opcode.LROTATE.code, 2,2,11,1, 5));		// LROT R11,R1,5
		prog.add(Simulator.encT3(Opcode.RROTATE.code, 2,2,12,1, 5));		// RROT R12,R1,5

		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 1));				// MOVE R0,1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 13, 31415));			// MOVE R13,31415
		prog.add(Simulator.encT0(Opcode.LSHIFT.code, 2,2,0,0, 13,0,0,0));	// LSH R13,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 14, 31415));			// MOVE R14,31415
		prog.add(Simulator.encT0(Opcode.RSHIFT.code, 2,2,0,0, 14,0,0,0));	// RSH R14,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 15, 31415));			// MOVE R15,31415
		prog.add(Simulator.encT0(Opcode.ARSHIFT.code, 2,2,0,0, 15,0,0,0));	// ARSH R15,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 16, 31415));			// MOVE R16,31415
		prog.add(Simulator.encT0(Opcode.LROTATE.code, 2,2,0,0, 16,0,0,0));	// LROT R16,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 17, 31415));			// MOVE R17,31415
		prog.add(Simulator.encT0(Opcode.RROTATE.code, 2,2,0,0, 17,0,0,0));	// RROT R17,R0

		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,18, 31415));			// MOVE R18,31415
		prog.add(Simulator.encT2(Opcode.LSHIFT.code, 2,18, 4));				// LSH R18,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,19, 31415));			// MOVE R19,31415
		prog.add(Simulator.encT2(Opcode.RSHIFT.code, 2,19, 4));				// RSH R19,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,20, 31415));			// MOVE R20,31415
		prog.add(Simulator.encT2(Opcode.ARSHIFT.code, 2,20, 4));			// ARSH R20,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,21, 31415));			// MOVE R21,31415
		prog.add(Simulator.encT2(Opcode.LROTATE.code, 2,21, 4));			// LROT R21,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2,22, 31415));			// MOVE R22,31415
		prog.add(Simulator.encT2(Opcode.RROTATE.code, 2,22, 4));			// RROT R22,4
		// negative number check
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, 1));				// MOVE R0,1
		prog.add(Simulator.encT0(Opcode.LSHIFT.code, 2,2,2,0, 23,2,0,0));	// LSH R23,R2,R0
		prog.add(Simulator.encT0(Opcode.RSHIFT.code, 2,2,2,0, 24,2,0,0));	// RSH R24,R2,R0
		prog.add(Simulator.encT0(Opcode.ARSHIFT.code, 2,2,2,0, 25,2,0,0));	// ARSH R25,R2,R0
		prog.add(Simulator.encT0(Opcode.LROTATE.code, 2,2,2,0, 26,2,0,0));	// LROT R26,R2,R0
		prog.add(Simulator.encT0(Opcode.RROTATE.code, 2,2,2,0, 27,2,0,0));	// RROT R27,R2,R0
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(30, diff.size());
		assertDiff(diff, 0, 1);
		assertDiff(diff, 1, 31415L);
		assertDiff(diff, 2, -31415L);
		assertDiff(diff, 3, 31415L << 3);
		assertDiff(diff, 4, 31415L >>> 3);
		assertDiff(diff, 5, 31415L >> 3);
		assertDiff(diff, 6, Long.rotateLeft(31415L, 3));
		assertDiff(diff, 7, Long.rotateRight(31415L, 3));
		assertDiff(diff, 8, 31415L << 5);
		assertDiff(diff, 9, 31415L >>> 5);
		assertDiff(diff, 10, 31415L >> 5);
		assertDiff(diff, 11, Long.rotateLeft(31415L, 5));
		assertDiff(diff, 12, Long.rotateRight(31415L, 5));
		assertDiff(diff, 13, 31415L << 1);
		assertDiff(diff, 14, 31415L >>> 1);
		assertDiff(diff, 15, 31415L >> 1);
		assertDiff(diff, 16, Long.rotateLeft(31415L, 1));
		assertDiff(diff, 17, Long.rotateRight(31415L, 1));
		assertDiff(diff, 18, 31415L << 4);
		assertDiff(diff, 19, 31415L >>> 4);
		assertDiff(diff, 20, 31415L >> 4);
		assertDiff(diff, 21, Long.rotateLeft(31415L, 4));
		assertDiff(diff, 22, Long.rotateRight(31415L, 4));
		assertDiff(diff, 23, -31415L << 1);
		assertDiff(diff, 24, -31415L >>> 1);
		assertDiff(diff, 25, -31415L >> 1);
		assertDiff(diff, 26, Long.rotateLeft(-31415L, 1));
		assertDiff(diff, 27, Long.rotateRight(-31415L, 1));
		assertDiff(diff, sim.SR_S);
	}

	@Test
	void testOut() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		var ph = sim.getPortHandler(5);
		if (ph == null) {
			sim.setPortHandler(5, new MemoryFilePortHandler(sim, 1));
		} else
			throw sim.new CPUException("Port 5 already mapped!");
		ph = sim.getPortHandler(5);
		// YZZ
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 0, 37));           	// LOAD R0,32
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 5));            	// MOVE R2,5
		prog.add(Simulator.encT2(Opcode.LOAD.code, 3, 0, 38));           	// LOAD F0,33
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,1,0, 0,1,5,0));		// OUT R0,1,5
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,1,0, 0,2,5,0));		// OUT R0,2,5
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,1,0, 0,4,5,0));		// OUT R0,4,5
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,1,0, 0,8,5,0));		// OUT R0,8,5
		prog.add(Simulator.encT0(Opcode.OUT.code, 3,1,1,0, 0,8,5,0));		// OUT F0,8,5
		// YZR
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,2,0, 0,1,2,0));		// OUT R0,1,R2
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,2,0, 0,2,2,0));		// OUT R0,2,R2
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,2,0, 0,4,2,0));		// OUT R0,4,R2
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,1,2,0, 0,8,2,0));		// OUT R0,8,R2
		prog.add(Simulator.encT0(Opcode.OUT.code, 3,1,2,0, 0,8,2,0));		// OUT F0,8,R2
		// YRR
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 1));            	// MOVE R1,1
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,2,0, 0,1,2,0));		// OUT R0,R1,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 2));            	// MOVE R1,2
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,2,0, 0,1,2,0));		// OUT R0,R1,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 4));            	// MOVE R1,4
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,2,0, 0,1,2,0));		// OUT R0,R1,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 8));            	// MOVE R1,8
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,2,0, 0,1,2,0));		// OUT R0,R1,R2
		prog.add(Simulator.encT0(Opcode.OUT.code, 3,2,2,0, 0,1,2,0));		// OUT F0,R1,R2
		// YRZ
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 1));            	// MOVE R1,1
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,1,0, 0,1,5,0));		// OUT R0,R1,5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 2));            	// MOVE R1,2
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,1,0, 0,1,5,0));		// OUT R0,R1,5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 4));            	// MOVE R1,4
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,1,0, 0,1,5,0));		// OUT R0,R1,5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 8));            	// MOVE R1,8
		prog.add(Simulator.encT0(Opcode.OUT.code, 2,2,1,0, 0,1,5,0));		// OUT R0,R1,5
		prog.add(Simulator.encT0(Opcode.OUT.code, 3,2,1,0, 0,1,5,0));		// OUT F0,R1,5

		// CZZ, CRR, CRZ, CZR
		prog.add(Simulator.encT0(Opcode.OUT.code, 1,1,1,0, 0x234,2,5,0));		// OUT 0x1234,2,5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 2));            		// MOVE R1,8
		prog.add(Simulator.encT0(Opcode.OUT.code, 1,2,2,0, 0x234,1,2,0));		// OUT 0x1234,R1,R2
		prog.add(Simulator.encT0(Opcode.OUT.code, 1,1,2,0, 0x234,2,2,0));		// OUT 0x1234,2,R2
		prog.add(Simulator.encT0(Opcode.OUT.code, 1,2,1,0, 0x234,1,5,0));		// OUT 0x1234,R1,5

		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(0x1234567890ABCDEFL);
		prog.add(Double.doubleToRawLongBits(3.1415926));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(6, diff.size());
		assertDiff(diff, 0, 0x1234567890ABCDEFL);
		assertDiff(diff, 0, 3.1415926);
		assertDiff(diff, sim.SR_P);
		byte[] output = ((MemoryFilePortHandler)ph).toBytes();
		byte[] expected = {(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74,
				(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74,
				(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74,
				(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74,
				0x2,0x34,0x2,0x34,0x2,0x34,0x2,0x34
		};
		assertEquals(expected.length, output.length);
		for (int i = 0; i < expected.length; ++i) {
			assertEquals(expected[i], output[i]);
		}
	}

	@Test
	void testIn() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		byte[] data = {(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74,
				(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74,
				(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74,
				(byte)0xEF, (byte)0xCD, (byte)0xEF, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xAB, (byte)0xCD, (byte)0xEF,
				64, 9, 33, -5, 77, 18, -40, 74
		};
		var ph = sim.getPortHandler(5);
		if (ph == null) {
			sim.setPortHandler(5, new MemoryFilePortHandler(sim, 0, data));
		} else
			throw sim.new CPUException("Port 5 already mapped!");
		ph = sim.getPortHandler(5);
		// YZZ
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 5));            	// MOVE R2,5
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,1,0, 0,1,5,0));		// IN R0,1,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,1,0, 0,2,5,0));		// IN R0,2,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,1,0, 0,4,5,0));		// IN R0,4,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,1,0, 0,8,5,0));		// IN R0,8,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 3,1,1,0, 0,8,5,0));		// IN F0,8,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,0,0, 3,0,0,0));		// ADD F3,F0
		// YZR
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,2,0, 0,1,2,0));		// IN R0,1,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,2,0, 0,2,2,0));		// IN R0,2,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,2,0, 0,4,2,0));		// IN R0,4,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 2,1,2,0, 0,8,2,0));		// IN R0,8,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 3,1,2,0, 0,8,2,0));		// IN F0,8,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,0,0, 3,0,0,0));		// ADD F3,F0
		// YRR
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 1));            	// MOVE R1,1
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,2,0, 0,1,2,0));		// IN R0,R1,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 2));            	// MOVE R1,2
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,2,0, 0,1,2,0));		// IN R0,R1,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 4));            	// MOVE R1,4
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,2,0, 0,1,2,0));		// IN R0,R1,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 8));            	// MOVE R1,8
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,2,0, 0,1,2,0));		// IN R0,R1,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 3,2,2,0, 0,1,2,0));		// IN F0,R1,R2
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,0,0, 3,0,0,0));		// ADD F3,F0
		// YRZ
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 1));            	// MOVE R1,1
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,1,0, 0,1,5,0));		// IN R0,R1,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 2));            	// MOVE R1,2
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,1,0, 0,1,5,0));		// IN R0,R1,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 4));            	// MOVE R1,4
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,1,0, 0,1,5,0));		// IN R0,R1,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 8));            	// MOVE R1,8
		prog.add(Simulator.encT0(Opcode.IN.code, 2,2,1,0, 0,1,5,0));		// IN R0,R1,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 2,2,0,0, 3,0,0,0));		// ADD R3,R0
		prog.add(Simulator.encT0(Opcode.IN.code, 3,2,1,0, 0,1,5,0));		// IN F0,R1,5
		prog.add(Simulator.encT0(Opcode.ADD.code, 3,3,0,0, 3,0,0,0));		// ADD F3,F0

		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(0x1234567890ABCDEFL);
		prog.add(Double.doubleToRawLongBits(3.1415926));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(8, diff.size());
		assertDiff(diff, 3, 0x48D159E48561AAF0L);
		assertDiff(diff, 3, 12.5663704);
		assertDiff(diff, 0);
	}

	@Test
	void testPack() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// RR, RRRR
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 0x1234));            	// MOVE R1,0x1234
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 0x5678));            	// MOVE R2,0x5678
		prog.add(Simulator.encT0(Opcode.PACK.code, 2,2,0,0, 1,2,0,0));			// PACK R1,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 0x12));            	// MOVE R2,0x12
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 0x34));            	// MOVE R3,0x34
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 4, 0x56));            	// MOVE R4,0x56
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 5, 0x78));            	// MOVE R5,0x78
		prog.add(Simulator.encT0(Opcode.PACK.code, 2,2,2,2, 2,3,4,5));			// PACK R2,R3,R4,R5
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(7, diff.size());
		assertDiff(diff, 1, 0x12345678);
		assertDiff(diff, 2, 0x12345678);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testPack64() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// RR, RRRR
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 0x12345678));         	// MOVE R1,0x12345678
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 0x90ABCDEF));          // MOVE R2,0x90ABCDEF
		prog.add(Simulator.encT0(Opcode.PACK64.code, 2,2,0,0, 1,2,0,0));		// PACK64 R1,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 0x1234));            	// MOVE R2,0x1234
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 0x5678));            	// MOVE R3,0x5678
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 4, 0x90AB));            	// MOVE R4,0x90AB
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 5, 0xCDEF));            	// MOVE R5,0xCDEF
		prog.add(Simulator.encT0(Opcode.PACK64.code, 2,2,2,2, 2,3,4,5));		// PACK64 R2,R3,R4,R5
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(7, diff.size());
		assertDiff(diff, 1, 0x1234567890ABCDEFL);
		assertDiff(diff, 2, 0x1234567890ABCDEFL);
		assertDiff(diff, 0);
	}

	@Test
	void testUnpack() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// RR, RRRR
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 0x12345678));          // MOVE R1,0x12345678
		prog.add(Simulator.encT0(Opcode.UNPACK.code, 2,2,0,0, 1,2,0,0));		// UNPACK R1,R2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 3, 0x12345678));          // MOVE R3,0x12345678
		prog.add(Simulator.encT0(Opcode.UNPACK.code, 2,2,2,2, 3,4,5,6));		// UNPACK R3,R4,R5,R6
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(8, diff.size());
		assertDiff(diff, 1, 0x1234);
		assertDiff(diff, 2, 0x5678);
		assertDiff(diff, 3, 0x12);
		assertDiff(diff, 4, 0x34);
		assertDiff(diff, 5, 0x56);
		assertDiff(diff, 6, 0x78);
		assertDiff(diff, 0);
	}

	@Test
	void testUnpack64() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// RR, RRRR
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 1, 5));          			// LOAD R1,5
		prog.add(Simulator.encT0(Opcode.UNPACK64.code, 2,2,0,0, 1,2,0,0));		// UNPACK64 R1,R2
		prog.add(Simulator.encT2(Opcode.LOAD.code, 2, 3, 5));          			// LOAD R3,5
		prog.add(Simulator.encT0(Opcode.UNPACK64.code, 2,2,2,2, 3,4,5,6));		// UNPACK64 R3,R4,R5,R6
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(0x1234567890ABCDEFL);
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(8, diff.size());
		assertDiff(diff, 1, 0x12345678L);
		assertDiff(diff, 2, 0x90ABCDEFL);
		assertDiff(diff, 3, 0x1234);
		assertDiff(diff, 4, 0x5678);
		assertDiff(diff, 5, 0x90AB);
		assertDiff(diff, 6, 0xCDEF);
		assertDiff(diff, sim.SR_P);
	}

	@Test
	void testCAS() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		// OOA
		int end = 13;
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, end + 1));			// MOVE R0,end + 1
		prog.add(Simulator.encT0(Opcode.CAS.code, 1,1,2,0, 1,2,0,0));		// CAS 1,2,R0
		prog.add(Simulator.encT2(Opcode.JUMP.code, 1, 8, end));          	// JUMP NO, END
		prog.add(Simulator.encT0(Opcode.CAS.code, 1,1,2,0, 1,2,0,0));		// CAS 1,2,R0
		prog.add(Simulator.encT2(Opcode.JUMP.code, 1, 7, end));          	// JUMP O, END
		// OOAO
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 0, end ));			// MOVE R0,end + 1
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 1, 2));				// MOVE R1,2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 2, 3));				// MOVE R2,3
		prog.add(Simulator.encT0(Opcode.CAS.code, 2,2,2,1, 1,2,0,1));		// CAS 1,2,R0
		prog.add(Simulator.encT2(Opcode.JUMP.code, 1, 8, end));          	// JUMP NO, END
		prog.add(Simulator.encT0(Opcode.CAS.code, 2,2,2,1, 1,2,0,1));		// CAS 1,2,R0
		prog.add(Simulator.encT2(Opcode.JUMP.code, 1, 7, end));          	// JUMP O, END

		prog.add(Simulator.encT2(Opcode.STORE.code, 1, 100, end + 1));      // STORE 100,end + 1
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(1L);
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(5, diff.size());
		assertEquals(100, (int)sim.memRead(end + 1));
		assertDiff(diff, sim.SR_Z);
	}

	@Test
	void testSave() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 26, 2));				// MOVE R26,2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 27, 3));				// MOVE R27,3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 28, 4));				// MOVE R28,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 31, 5));				// MOVE F31,5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 30, 6));				// MOVE F30,6
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 29, 7));				// MOVE F29,7
		prog.add(Simulator.encT0(Opcode.SAVE.code, 2,2,0,0, 26,28,0,0));	// SAVE R26,R28
		prog.add(Simulator.encT0(Opcode.SAVE.code, 3,3,0,0, 29,31,0,0));	// SAVE F29,F31
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(9, diff.size());
		int sp = (int)sim.R[sim.R_SP];
		assertEquals(5., Double.longBitsToDouble(sim.memRead(sp + 1)));
		assertEquals(6., Double.longBitsToDouble(sim.memRead(sp + 2)));
		assertEquals(7., Double.longBitsToDouble(sim.memRead(sp + 3)));
		assertEquals(4, (int)sim.memRead(sp + 4));
		assertEquals(3, (int)sim.memRead(sp + 5));
		assertEquals(2, (int)sim.memRead(sp + 6));
		assertDiff(diff, 0);
	}

	@Test
	void testRestore() {
		String[] args = {"test"};
		Simulator sim = new Simulator(1000, args);
		List<Long> prog = new ArrayList<Long>();
		sim.clearCPUState();

		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 26, 2));				// MOVE R26,2
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 27, 3));				// MOVE R27,3
		prog.add(Simulator.encT2(Opcode.MOVE.code, 2, 28, 4));				// MOVE R28,4
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 31, 5));				// MOVE F31,5
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 30, 6));				// MOVE F30,6
		prog.add(Simulator.encT2(Opcode.MOVE.code, 3, 29, 7));				// MOVE F29,7
		prog.add(Simulator.encT0(Opcode.SAVE.code, 2,2,0,0, 26,28,0,0));	// SAVE R26,R28
		prog.add(Simulator.encT0(Opcode.SAVE.code, 3,3,0,0, 29,31,0,0));	// SAVE F29,F31
		prog.add(Simulator.encT0(Opcode.STOP.code, 0, 0, 0, 0, 0, 0, 0, 0));
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 2, 2, 2, 0, 26, 27, 28, 0));
		prog.add(Simulator.encT0(Opcode.CLEAR.code, 3, 3, 3, 0, 29, 30, 31, 0));
		prog.add(Simulator.encT0(Opcode.RESTORE.code, 3,3,0,0, 29,31,0,0));	// RESTORE F29,F31
		prog.add(Simulator.encT0(Opcode.RESTORE.code, 2,2,0,0, 26,28,0,0));	// RESTORE R26,R28
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		sim.run();
		var diff = sim.diffState(startState);
		assertEquals(9, diff.size());
		int sp = (int)sim.R[sim.R_SP];
		assertDiff(diff, 26, 2);
		assertDiff(diff, 27, 3);
		assertDiff(diff, 28, 4);
		assertDiff(diff, 29, 7.);
		assertDiff(diff, 30, 6.);
		assertDiff(diff, 31, 5.);
		assertDiff(diff, 0);
	}
}