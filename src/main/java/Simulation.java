import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;

import cloud.lesh.CPUSim64v2.Simulator;

public class Simulation {
	public static void main(String[] args) throws Exception {
		if (args.length < 1) {
			System.err.println("Usage: simulation <input.obj.gz>");
			System.exit(2);
		}

		boolean debug = false;
		boolean verbose = false;
		int memorySize = 1024 * 1024;	// default value
		int stackSize = 4096; 			// default value
		String filespec = "";

		List<String> simulatorArgs = new ArrayList<String>();
		for (String arg : args) {
			if (arg.charAt(0) == '-') {
				if (arg.equals("--debug")) {
					debug = true;
				} else if (arg.equals("--verbose") || arg.equals("-v")) {
					verbose = true;
				} else if (arg.startsWith("--mem=")) {
					try {
						memorySize = Integer.parseInt(arg.substring("--mem=".length()));
					} catch (NumberFormatException e) {
						System.err.println("Invalid memory size: " + arg);
						System.exit(1);
					}
				} else if (arg.startsWith("--stack=")) {
					try {
						stackSize = Integer.parseInt(arg.substring("--stack=".length()));
					} catch (NumberFormatException e) {
						System.err.println("Invalid stack size: " + arg);
						System.exit(1);
					}
				} else {
					System.err.println("Unknown option: " + arg);
					System.exit(1);
				}
			} else  {
				simulatorArgs.add(arg);
			}
		}

		if (verbose) {
			System.out.println("CPUSim64v2 Simulator");
			System.out.println("By Richard Lesh ©2025");
			System.out.println("CPUSim64 Virtual Machine");
			System.out.println("Debug: " + debug);
			System.out.println("Memory size: " + memorySize);
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
		symbolPath = originalPath.resolveSibling(baseName + ".sym");
		if (!Files.isRegularFile(symbolPath)) {
			System.out.println("Can't locate symbol file for program: " + baseName);
		} else {
			symbolMap = Simulator.readLabelMapFromFile(symbolPath.toFile());
			// Create reverse map
			reverseSymbolMap = new java.util.HashMap<>();
			for (var entry : symbolMap.entrySet()) {
				reverseSymbolMap.put(entry.getValue(), entry.getKey());
			}
			if (reverseSymbolMap.get(0L) == null) {
				reverseSymbolMap.put(0L, "__START");
			}
		}

		// 1) Read object file
		var program = cloud.lesh.CPUSim64v2.AsmIO.readU64BE(newPath.toFile());
		if (verbose) {
			System.out.println("Read " + program.size() + " words from " + newPath.getFileName().toString());
		}

		var sim = new Simulator(memorySize, 0, stackSize, simulatorArgs.toArray(String[]::new));
		if (debug) sim.setDebug(true);
		sim.loadProgram(program, 0L);
		long result = sim.run(reverseSymbolMap);
		if (verbose) {
			System.out.println("Result: " + result);
		}
	}
}
