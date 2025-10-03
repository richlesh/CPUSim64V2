import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.ArrayList;

import cloud.lesh.CPUSim64v2.Simulator;

public class Simulation {
	public static void main(String[] args) throws Exception {
		System.out.println("CPUSim64v2 Simulator");
		System.out.println("By Richard Lesh ©2025");
		System.out.println("CPUSim64 Virtual Machine");
		if (args.length < 1) {
			System.err.println("Usage: simulation <input.obj.gz>");
			System.exit(2);
		}

		boolean debug = false;
		int memorySize = 1024; // default value
		String filespec = "";

		List<String> simulatorArgs = new ArrayList<String>();
		for (String arg : args) {
			if (arg.charAt(0) == '-') {
				if (arg.equals("--debug") || arg.equals("-D")) {
					debug = true;
				} else if (arg.startsWith("--mem=")) {
					try {
						memorySize = Integer.parseInt(arg.substring("--mem=".length()));
					} catch (NumberFormatException e) {
						System.err.println("Invalid memory size: " + arg);
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

		System.out.println("Debug: " + debug);
		System.out.println("Memory size: " + memorySize);

		Path originalPath = Path.of(simulatorArgs.get(0)).toAbsolutePath();
		Path newPath = originalPath;
		if (!Files.isRegularFile(newPath)) {
			// Get filename without extension
			String fileName = newPath.getFileName().toString();
			int dot = fileName.lastIndexOf('.');
			String baseName = (dot == -1) ? fileName : fileName.substring(0, dot);
			// Compose new path
			newPath = originalPath.resolveSibling(baseName + ".obj.gz");
			if (!Files.isRegularFile(newPath)) {
				newPath = originalPath.resolveSibling(baseName + ".obj");
				if (!Files.isRegularFile(newPath)) {
					throw new RuntimeException("Can't locate object file for program: " + baseName);
				}
			}
		}

		// 1) Read object file
		var program = cloud.lesh.CPUSim64v2.AsmIO.readU64BE(newPath.toFile());
		System.out.println("Read " + program.size() + " words from " + newPath.getFileName().toString());

		var sim = new Simulator(memorySize, simulatorArgs.toArray(String[]::new));
		if (debug) sim.setDebug(true);
		sim.loadProgram(program, 0L);
		sim.run();
	}
}
