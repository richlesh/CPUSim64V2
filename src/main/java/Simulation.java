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

import cloud.lesh.CPUSim64.Simulator;
import cloud.lesh.CPUSim64.Utils;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class Simulation {

	public static void main(String[] args) throws Exception {
		if (args.length < 1) {
			System.err.println("Usage: simulation [--debug] [--trace] [--verbose] \n" +
					"        [--mem=memsize] [--stack=stacksize] <input.obj.gz>");
			System.exit(2);
		}

		boolean debug = false;
		boolean trace = false;
		boolean verbose = false;
		int memorySize = 1048576;		// default 1M
		int stackSize = 8192; 			// default 8k
		String filespec = "";

		List<String> simulatorArgs = new ArrayList<String>();
		for (String arg : args) {
			if (arg.charAt(0) == '-') {
				if (arg.equals("--debug")) {
					debug = true;
				} else if (arg.equals("--trace") || arg.equals("-t")) {
					trace = true;
				} else if (arg.equals("--verbose") || arg.equals("-v")) {
					verbose = true;
				} else if (arg.startsWith("--mem=")) {
					try {
						memorySize = Utils.decodeSI(arg.substring("--mem=".length()));
					} catch (NumberFormatException e) {
						System.err.println("Invalid memory size: " + arg);
						System.exit(1);
					}
				} else if (arg.startsWith("--stack=")) {
					try {
						stackSize = Utils.decodeSI(arg.substring("--stack=".length()));
					} catch (NumberFormatException e) {
						System.err.println("Invalid stack size: " + arg);
						System.exit(1);
					}
				} else {
					simulatorArgs.add(arg);
				}
			} else  {
				simulatorArgs.add(arg);
			}
		}

		if (verbose || debug) {
			System.out.println("=".repeat(80));
			System.out.println("CPUSim64 2.0 Emulator");
			System.out.println("By Richard Lesh ©2025");
			System.out.println("Implements a CPUSim64 Virtual Machine.");
			System.out.println("=".repeat(80));
			System.out.println("Debug: " + debug);
			System.out.println("Memory size: " + memorySize);
			System.out.println("Stack size: " + stackSize);
		}
		Path originalPath = Path.of(simulatorArgs.get(0)).toAbsolutePath();
		Path newPath = originalPath;
		Path symbolPath = originalPath;
		Map<String, Long> symbolMap = null;
		Map<Long, String> reverseSymbolMap = null;
		// Get filename without extension
		String fileName = newPath.getFileName().toString();
		int dot = fileName.indexOf('.');
		String baseName = (dot == -1) ? fileName : fileName.substring(0, dot);
		// Compose new path
		newPath = originalPath.resolveSibling(baseName + ".obj.gz");
		if (!Files.isRegularFile(newPath)) {
			newPath = originalPath.resolveSibling(baseName + ".obj");
			if (!Files.isRegularFile(newPath)) {
				throw new RuntimeException("Can't locate object file for program: " + baseName);
			}
		}
		symbolPath = originalPath.resolveSibling(baseName + ".sym1");
		if (!Files.isRegularFile(symbolPath)) {
			System.out.println("Can't locate reverse label file for program: " + baseName);
		} else {
			reverseSymbolMap = Simulator.readReverseLabelMapFromFile(symbolPath.toFile());
			if (reverseSymbolMap.get(0L) == null) {
				reverseSymbolMap.put(0L, "__START__");
			}
		}

		// 1) Read object file
		var program = cloud.lesh.CPUSim64.AsmIO.readU64BE(newPath.toFile());
		if (verbose || debug) {
			System.out.println("Read " + program.size() + " words from " + newPath.getFileName().toString());
		}

		var sim = new Simulator(memorySize, 0, stackSize, simulatorArgs.toArray(String[]::new));
		if (debug) sim.setDebug(true);
		if (trace) sim.setTrace(true);
		sim.loadProgram(program, 0L, reverseSymbolMap);
		long result = sim.run(program.get(0));
		if (verbose) {
			System.out.println("Result: " + result);
			var totalTime = sim.getClock();
			System.out.printf("User CPU Cycles: %d\n", sim.getCycles());
			System.out.printf("User Time: %.3f sec\n", (totalTime - sim.getSystemClock()) / 1.e9);
			System.out.printf("System Time: %.3f sec\n", sim.getSystemClock() / 1.e9);
			System.out.printf("Total Time: %.3f sec\n", totalTime / 1.e9);
		}
	}
}
