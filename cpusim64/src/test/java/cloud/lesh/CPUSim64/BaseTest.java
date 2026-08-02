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

import org.apache.commons.lang3.tuple.Triple;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;
import java.util.stream.Collectors;

import cloud.lesh.CPUSim64.SimStateDiff;
import cloud.lesh.CPUSim64.ConsoleOutputCapturer;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class BaseTest {
	@Test
	void testTemplate() {
		String src = """
			START:
			
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

	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src) {
		String[] args = {"test"};
		return runProgram(src, args);
	}

	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src, String[] args) {
		return runProgram(src, args, null);
	}

	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src, String[] args, byte[] inputData) {
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor.resetGlobals();
		PreprocessorVisitor pp = new PreprocessorVisitor("Test.asm", loader);
		String preprocessed = pp.preprocessText(src, args);

		LiteralRewriter rewriter = new LiteralRewriter();
		preprocessed = rewriter.rewrite(preprocessed, pp.getSourceLocations());

		preprocessed = pp.addGlobals(preprocessed, false);

		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(preprocessed);

		if (noLabels == null) {
			return Triple.of(-1, null, null);
		}

		var asm = new AssemblerVisitor(labelVisitor.getLabelMap(), labelVisitor.getReverseLabelMap());
		asm.assemble(noLabels);
		List<Long> prog = asm.result();

		if (prog == null) {
			return Triple.of(-1, null, null);
		}

		Simulator sim = new Simulator(0x2000, 0x1000, 0x500, args);
		if (inputData != null)
			sim.setPortHandler(0, new MemoryFilePortHandler(sim, 0, inputData));
		sim.clearCPUState();
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		int result = sim.run(1L);
		var diff = new SimStateDiff(sim, startState);
		return Triple.of(result, sim, diff);
	}
}