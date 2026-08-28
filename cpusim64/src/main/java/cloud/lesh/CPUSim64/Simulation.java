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

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class Simulation {

	public static void main(String[] args) throws Exception {
		System.exit(run(args));
	}

	public static int run(String[] args) throws Exception {
		if (args.length < 1) {
			System.err.println("Usage: simulation [--debug] [--trace] [--verbose] \n" +
					"        [--mem=memsize] [--stack=stacksize] <input.o64>");
			return 2;
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
						return 1;
					}
				} else if (arg.startsWith("--stack=")) {
					try {
						stackSize = Utils.decodeSI(arg.substring("--stack=".length()));
					} catch (NumberFormatException e) {
						System.err.println("Invalid stack size: " + arg);
						return 1;
					}
				} else {
					simulatorArgs.add(arg);
				}
			} else  {
				simulatorArgs.add(arg);
			}
		}

		if (verbose) {
			System.out.println("=".repeat(80));
			System.out.println("CPUSim64 " + BuildInfo.VERSION + " Emulator");
			System.out.println(BuildInfo.COPYRIGHT + " Richard Lesh");
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
		newPath = originalPath.resolveSibling(baseName + ".o64");
		if (!Files.isRegularFile(newPath)) {
			throw new RuntimeException("Can't locate object file for program: " + baseName);
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

		// Disable output buffering so ANSI escape sequences are sent immediately (CLI only)
		if (StdInterruptHandler.getGlobalTerminalSizeProvider() == null) {
			System.setOut(new java.io.PrintStream(new java.io.FileOutputStream(java.io.FileDescriptor.out), true));
		}

		var sim = new Simulator(memorySize, 0, stackSize, simulatorArgs.toArray(String[]::new));
		// Set up terminal size provider for real terminal (only if not already set by IDE)
		if (StdInterruptHandler.getGlobalTerminalSizeProvider() == null) {
		StdInterruptHandler.setGlobalTerminalSizeProvider(new StdInterruptHandler.TerminalSizeProvider() {
			private int cachedCols = -1;
			private int cachedRows = -1;
			private long lastQuery = 0;
			private org.jline.terminal.Terminal jlineTerminal = null;
			private boolean initFailed = false;
			private org.jline.terminal.Terminal getTerminal() {
				if (jlineTerminal == null && !initFailed) {
					try {
						jlineTerminal = org.jline.terminal.TerminalBuilder.terminal();
					} catch (Exception e) {
						initFailed = true;
					}
				}
				return jlineTerminal;
			}
			private void refresh() {
				long now = System.currentTimeMillis();
				if (now - lastQuery < 1000 && cachedCols > 0) return; // cache for 1 second
				lastQuery = now;
				try {
					org.jline.terminal.Terminal term = getTerminal();
					if (term != null) {
						int w = term.getWidth();
						int h = term.getHeight();
						if (w > 0 && h > 0) {
							cachedCols = w;
							cachedRows = h;
							return;
						}
					}
				} catch (Exception e) { /* fall through to defaults */ }
				// JLine unavailable or returned 0 — try OS-specific fallback
				try {
					String os = System.getProperty("os.name", "").toLowerCase();
					if (os.contains("win")) {
						Process p = new ProcessBuilder("cmd.exe", "/c", "mode", "con")
							.redirectErrorStream(true).start();
						String output = new String(p.getInputStream().readAllBytes());
						p.waitFor();
						for (String line : output.split("\\r?\\n")) {
							line = line.trim();
							if (line.toLowerCase().contains("columns") || line.toLowerCase().contains("col")) {
								String[] parts = line.split(":");
								if (parts.length >= 2) {
									int val = Integer.parseInt(parts[1].trim());
									if (val > 0) cachedCols = val;
								}
							} else if (line.toLowerCase().contains("lines") || line.toLowerCase().contains("lin")) {
								String[] parts = line.split(":");
								if (parts.length >= 2) {
									int val = Integer.parseInt(parts[1].trim());
									if (val > 0) cachedRows = val;
								}
							}
						}
					} else {
						Process p = new ProcessBuilder("stty", "size")
							.redirectErrorStream(true).start();
						String output = new String(p.getInputStream().readAllBytes()).trim();
						p.waitFor();
						String[] parts = output.split("\\s+");
						if (parts.length == 2) {
							int rows = Integer.parseInt(parts[0]);
							int cols = Integer.parseInt(parts[1]);
							if (rows > 0) cachedRows = rows;
							if (cols > 0) cachedCols = cols;
						}
					}
				} catch (Exception e) { /* ignore */ }
				if (cachedCols <= 0) { cachedCols = 80; cachedRows = 24; }
			}
			@Override public int getColumns() { refresh(); return cachedCols; }
			@Override public int getRows() { refresh(); return cachedRows; }
		});
		}
		if (debug) sim.setDebug(true);
		if (trace) sim.setTrace(true);
		sim.loadProgram(program, 0L, reverseSymbolMap);
		try {
			long result = sim.run(program.get(0));
			if (verbose) {
				System.out.println("Result: " + result);
				var totalTime = sim.getClock();
				System.out.printf("User CPU Cycles: %d\n", sim.getCycles());
				System.out.printf("User Time: %.3f sec\n", (totalTime - sim.getSystemClock()) / 1.e9);
				System.out.printf("System Time: %.3f sec\n", sim.getSystemClock() / 1.e9);
				System.out.printf("Total Time: %.3f sec\n", totalTime / 1.e9);
			}
		} finally {
			sim.unregister();
		}
		return 0;
	}
}
