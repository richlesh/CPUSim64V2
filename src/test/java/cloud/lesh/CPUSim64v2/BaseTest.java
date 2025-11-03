package cloud.lesh.CPUSim64v2;

import org.apache.commons.lang3.tuple.Triple;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class BaseTest {
	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src) {
		String[] args = {"test"};
		return runProgram(src, args);
	}

	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src, String[] args) {
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader, args);

		LiteralRewriter rewriter = new LiteralRewriter();
		String rewritten = rewriter.rewrite(preprocessed);

		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(rewritten);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> prog = asm.result();
		Simulator sim = new Simulator(0x2000, 0x1000, 0x500, args);
		sim.clearCPUState();
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		int result = sim.run();
		var diff = new SimStateDiff(sim, startState);
		return Triple.of(result, sim, diff);
	}
}