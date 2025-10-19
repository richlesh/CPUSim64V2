package cloud.lesh.CPUSim64v2;

import org.apache.commons.lang3.tuple.Triple;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class MathInterruptTest {
	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src) {
		String[] args = {"test"};
		return runProgram(src, args);
	}

	public Triple<Integer, Simulator, SimStateDiff> runProgram(String src, String[] args) {
		var loader = new IncludeLoader(Path.of("."));
		String preprocessed = PreprocessorVisitor.preprocessText("Test.asm", src, loader);

		LiteralRewriter rewriter = new LiteralRewriter();
		String rewritten = rewriter.rewrite(preprocessed);

		LabelVisitor labelVisitor = new LabelVisitor();
		String noLabels = labelVisitor.gatherLabels(rewritten);
		var asm = new AssemblerVisitor(labelVisitor.getLabelMap());
		asm.assemble(noLabels);
		List<Long> prog = asm.result();
		Simulator sim = new Simulator(1000, args);
		sim.clearCPUState();
		sim.loadProgram(prog, 0L);
		sim.SR = 0xF;
		var startState = sim.getState();
		int result = sim.run();
		var diff = new SimStateDiff(sim, startState);
		return Triple.of(result, sim, diff);
	}

	@Test
	void testConstants() {
		String src = """
			#include <system/math.def>
			INT		iPI
			MOVE 	F1, F0
			INT		iE
			MOVE	F2, F0
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		diff.assertDiff(1, Math.PI);
		diff.assertDiff(2, Math.E);
	}

	@Test
	void testMisc() {
		String src = """
			#include <system/math.def>
			// ABS
			LOAD 	F0, -2.5
			INT		iABS_FP
			MOVE 	F31, F0
			LOAD 	F0, 2.5
			INT		iABS_FP
			MOVE 	F30, F0
			MOVE	R0, -2
			INT		iABS
			MOVE	R27, R0
			MOVE	R0, 2
			INT		iABS
			MOVE	R26, R0
			
			// CEIL
			LOAD 	F0, -2.5
			INT		iCEIL
			MOVE 	F29, F0
			LOAD 	F0, 2.5
			INT		iCEIL
			MOVE 	F28, F0
			
			// FLOOR
			LOAD 	F0, -2.5
			INT		iFLOOR
			MOVE 	F27, F0
			LOAD 	F0, 2.5
			INT		iFLOOR
			MOVE 	F26, F0
			
			// ROUND
			LOAD 	F0, -2.5
			INT		iROUND
			MOVE 	F25, F0
			LOAD 	F0, 2.5
			INT		iROUND
			MOVE 	F24, F0
			
			// MAX/MIN
			LOAD	F0, 2.5
			LOAD	F1, 2.0
			INT		iMAX_FP
			MOVE	F23, F0
			LOAD	F0, 2.0
			LOAD	F1, 2.5
			INT		iMIN_FP
			MOVE	F22, F0
			LOAD	F0, -2.5
			LOAD	F1, -2.0
			INT		iMAX_FP
			MOVE	F21, F0
			LOAD	F0, -2.0
			LOAD	F1, -2.5
			INT		iMIN_FP
			MOVE	F20, F0
			
			MOVE	R0, 4
			MOVE	R1, 2
			INT		iMAX
			MOVE	R25, R0
			MOVE	R0, 4
			MOVE	R1, 2
			INT		iMIN
			MOVE	R24, R0
			MOVE	R0, -4
			MOVE	R1, -2
			INT		iMAX
			MOVE	R23, R0
			MOVE	R0, -4
			MOVE	R1, -2
			INT		iMIN
			MOVE	R22, R0
			
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(24, diff.size());
		diff.assertDiff(31, 2.5);
		diff.assertDiff(30, 2.5);
		diff.assertDiff(27, 2);
		diff.assertDiff(26, 2);
		diff.assertDiff(29, -2.);
		diff.assertDiff(28, 3.);
		diff.assertDiff(27, -3.);
		diff.assertDiff(26, 2.);
		diff.assertDiff(25, -3.);
		diff.assertDiff(24, 3.);
		diff.assertDiff(23, 2.5);
		diff.assertDiff(22, 2.);
		diff.assertDiff(21, -2.);
		diff.assertDiff(20, -2.5);
		diff.assertDiff(25, 4);
		diff.assertDiff(24, 2);
		diff.assertDiff(23, -2);
		diff.assertDiff(22, -4);
	}

	@Test
	void testLogExpPowerRoot() {
		String src = """
			#include <system/math.def>
			LOAD 	F0, 2.
			INT		iSQRT
			MOVE 	F31, F0
			LOAD 	F0, 2.
			INT		iEXP
			MOVE 	F30, F0
			LOAD 	F0, 2.
			INT		iLOG
			MOVE 	F29, F0
			LOAD 	F0, 2.
			LOAD	F1, 3.5
			INT		iPOW
			MOVE 	F28, F0
			LOAD 	F0, 3.5
			LOAD	F1, 2.
			INT		iREMAINDER
			MOVE 	F27, F0
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(9, diff.size());
		diff.assertDiff(31, Math.sqrt(2.));
		diff.assertDiff(30, Math.exp(2.));
		diff.assertDiff(29, Math.log(2.));
		diff.assertDiff(28, Math.pow(2., 3.5));
		diff.assertDiff(27, Math.IEEEremainder(3.5, 2.));
	}

	@Test
	void testRandom() {
		String src = """
			#include <system/math.def>
			#include <system/system.def>
			#include <system/io.def>
			#var i
			#fvar r,zero,one
			load zero, 0.0
			load one, 1.0
			#for 0, i lt 100, 1
				INT		iRANDOM
				MOVE	r, F0
				MOVE	r0, STDOUT
				MOVE	r1, 16
				INT		iPUT_FP
				INT		iPUT_NL
				#macro COMPARE_RANGE(zero, le, r, lt, one)
				jump	z, FAILURE
			#endfor
			move r0, 0
			STOP
			FAILURE:
			move r0, -1
			STOP
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, 0);
	}

	@Test
	void testRand() {
		String src = """
			#include <system/math.def>
			#include <system/system.def>
			#include <system/io.def>
			#var i,r,low,high
			MOVE	low, 1
			MOVE	high, 10
			#for 0, i lt 100, 1
				MOVE	R0, low
				MOVE	R1, high
				INT		iRAND
				MOVE	r, r0
				MOVE	r0, STDOUT
				MOVE	r1, r
				INT		iPUT_DEC
				INT		iPUT_NL
				#macro COMPARE_RANGE(low, le, r, le, high)
				jump	z, FAILURE
			#endfor
			move r0, 0
			STOP
			FAILURE:
			move r0, -1
			STOP
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		diff.assertDiff(0, 0);
	}

	@Test
	void testTrig() {
		String src = """
			#include <system/math.def>
			LOAD 	F0, 0.5
			INT		iTO_DEGREES
			MOVE 	F31, F0
			INT		iTO_RADIANS
			MOVE 	F30, F0
			
			LOAD 	F0, 0.5
			INT		iSIN
			MOVE 	F29, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iSIN
			MOVE 	F28, F0
			LOAD 	F0, 2.0
			INT		iSIN
			MOVE 	F27, F0
			
			LOAD 	F0, 0.5
			INT		iCOS
			MOVE 	F26, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iCOS
			MOVE 	F25, F0
			LOAD 	F0, 2.0
			INT		iCOS
			MOVE 	F24, F0
			
			LOAD 	F0, 0.5
			INT		iTAN
			MOVE 	F23, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iTAN
			MOVE 	F22, F0
			LOAD 	F0, 2.0
			INT		iTAN
			MOVE 	F21, F0
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(14, diff.size());
		diff.assertDiff(31, Math.toDegrees(0.5));
		diff.assertDiff(30, 0.5);
		diff.assertDiff(29, Math.sin(0.5));
		diff.assertDiff(28, Math.sin(-0.5));
		diff.assertDiff(27, Math.sin(2.0));
		diff.assertDiff(26, Math.cos(0.5));
		diff.assertDiff(25, Math.cos(-0.5));
		diff.assertDiff(24, Math.cos(2.0));
		diff.assertDiff(23, Math.tan(0.5));
		diff.assertDiff(22, Math.tan(-0.5));
		diff.assertDiff(21, Math.tan(2.0));
	}

	@Test
	void testInvTrig() {
		String src = """
			#include <system/math.def>
			LOAD 	F0, 0.5
			INT		iTO_RADIANS
			MOVE 	F31, F0
			INT		iTO_DEGREES
			MOVE 	F30, F0
			
			LOAD 	F0, 0.5
			INT		iASIN
			MOVE 	F29, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iASIN
			MOVE 	F28, F0
			LOAD 	F0, 2.0
			INT		iASIN
			MOVE 	F27, F0
			
			LOAD 	F0, 0.5
			INT		iACOS
			MOVE 	F26, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iACOS
			MOVE 	F25, F0
			LOAD 	F0, 2.0
			INT		iACOS
			MOVE 	F24, F0
			
			LOAD 	F0, 0.5
			INT		iATAN
			MOVE 	F23, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iATAN
			MOVE 	F22, F0
			LOAD 	F0, 2.0
			INT		iATAN
			MOVE 	F21, F0
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(14, diff.size());
		diff.assertDiff(31, Math.toRadians(0.5));
		diff.assertDiff(30, 0.5);
		diff.assertDiff(29, Math.asin(0.5));
		diff.assertDiff(28, Math.asin(-0.5));
		diff.assertDiff(27, Math.asin(2.0));
		diff.assertDiff(26, Math.acos(0.5));
		diff.assertDiff(25, Math.acos(-0.5));
		diff.assertDiff(24, Math.acos(2.0));
		diff.assertDiff(23, Math.atan(0.5));
		diff.assertDiff(22, Math.atan(-0.5));
		diff.assertDiff(21, Math.atan(2.0));
	}

	@Test
	void testHyperbolicTrig() {
		String src = """
			#include <system/math.def>
			LOAD 	F0, 0.5
			INT		iSINH
			MOVE 	F29, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iSINH
			MOVE 	F28, F0
			LOAD 	F0, 2.0
			INT		iSINH
			MOVE 	F27, F0
			
			LOAD 	F0, 0.5
			INT		iCOSH
			MOVE 	F26, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iCOSH
			MOVE 	F25, F0
			LOAD 	F0, 2.0
			INT		iCOSH
			MOVE 	F24, F0
			
			LOAD 	F0, 0.5
			INT		iTANH
			MOVE 	F23, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iTANH
			MOVE 	F22, F0
			LOAD 	F0, 2.0
			INT		iTANH
			MOVE 	F21, F0
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(12, diff.size());
		diff.assertDiff(29, Math.sinh(0.5));
		diff.assertDiff(28, Math.sinh(-0.5));
		diff.assertDiff(27, Math.sinh(2.0));
		diff.assertDiff(26, Math.cosh(0.5));
		diff.assertDiff(25, Math.cosh(-0.5));
		diff.assertDiff(24, Math.cosh(2.0));
		diff.assertDiff(23, Math.tanh(0.5));
		diff.assertDiff(22, Math.tanh(-0.5));
		diff.assertDiff(21, Math.tanh(2.0));
	}

	@Test
	void testInvHyperbolicTrig() {
		String src = """
			#include <system/math.def>			
			LOAD 	F0, 0.5
			INT		iASINH
			MOVE 	F29, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iASINH
			MOVE 	F28, F0
			LOAD 	F0, 2.0
			INT		iASINH
			MOVE 	F27, F0
			
			LOAD 	F0, 0.5
			INT		iACOSH
			MOVE 	F26, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iACOSH
			MOVE 	F25, F0
			LOAD 	F0, 2.0
			INT		iACOSH
			MOVE 	F24, F0
			
			LOAD 	F0, 0.5
			INT		iATANH
			MOVE 	F23, F0
			LOAD 	F0, 0.5
			NEG		F0
			INT		iATANH
			MOVE 	F22, F0
			LOAD 	F0, 2.0
			INT		iATANH
			MOVE 	F21, F0
			STOP
			""";
		var tuple = runProgram(src);
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(12, diff.size());
		diff.assertDiff(29, Utils.asinh(0.5));
		diff.assertDiff(28, Utils.asinh(-0.5));
		diff.assertDiff(27, Utils.asinh(2.0));
		diff.assertDiff(26, Utils.acosh(0.5));
		diff.assertDiff(25, Utils.acosh(-0.5));
		diff.assertDiff(24, Utils.acosh(2.0));
		diff.assertDiff(23, Utils.atanh(0.5));
		diff.assertDiff(22, Utils.atanh(-0.5));
		diff.assertDiff(21, Utils.atanh(2.0));
	}
}