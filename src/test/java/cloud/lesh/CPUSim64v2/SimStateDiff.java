package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class SimStateDiff {
	Simulator.CPUState initalState;
	Simulator sim;
	List<String> diffs;
	public SimStateDiff(Simulator sim, Simulator.CPUState initalState) {
		this.initalState = initalState;
		this.sim = sim;
		diffs = sim.diffState(initalState);
	}

	public int size() { return diffs.size(); }

	public long getReg(int i) { return sim.getR(i); }
	public double getFP(int i) { return sim.getFP(i); }

	public void assertSRDiff(int val) {
		assertTrue(diffs.contains("SR:" + val));
	}

	public void assertDiff(int reg, long val) {
		if (reg == Simulator.R_SF)
			assertTrue(diffs.contains("SF:" + val));
		else if (reg == Simulator.R_SP)
			assertTrue(diffs.contains("SP:" + val));
		else if (reg == Simulator.R_PC)
			assertTrue(diffs.contains("PC:" + val));
		else if (val == 0)
			assertTrue(diffs.stream().noneMatch(s -> s.startsWith("R" + reg + ":")));
		else
			assertTrue(diffs.contains("R" + reg + ":" + val));
	}

	public void assertDiff(int reg, double val) {
		assertTrue(diffs.contains("F" + reg + ":" + val));
	}

	public void assertMem(long addr, long val) {
		assertEquals(val, sim.memRead(addr));
	}

	public void assertMem(long addr, double val) {
		assertEquals(Double.doubleToRawLongBits(val), sim.memRead(addr));
	}

	@Override
	public String toString() {
		diffs.sort(null); 		// sorts in natural order
		return String.join(", ", diffs);
	}
}

