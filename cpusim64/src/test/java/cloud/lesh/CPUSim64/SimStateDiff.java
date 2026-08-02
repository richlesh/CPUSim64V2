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
			assertTrue(diffs.contains("R" + reg + ":" + val) ||
					diffs.stream().noneMatch(s -> s.startsWith("R" + reg + ":")));
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

	public void assertMemListEquals(long addr, List<Long> vals) {
		assertEquals(sim.memRead(addr), vals.size());
		for (int i = 0; i < vals.size(); i++) {
			assertEquals(vals.get(i), sim.memRead(addr + i + 1));
		}
	}
	@Override
	public String toString() {
		diffs.sort(null); 		// sorts in natural order
		return String.join(", ", diffs);
	}
}

