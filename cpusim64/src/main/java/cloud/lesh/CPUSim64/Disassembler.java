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
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class Disassembler {
	private final static String fmtLabel = "%08x: %s";

	public static void main(String[] args) throws Exception {
		System.out.println("=".repeat(80));
		System.out.println("CPUSim64 " + BuildInfo.VERSION + " Disassembler");
		System.out.println(BuildInfo.COPYRIGHT + " Richard Lesh");
		System.out.println("Disassembles .o64 object files into source code.");
		System.out.println("=".repeat(80));
		if (args.length < 1) {
			System.err.println("Usage: disassemble [-mainOnly] <input.o64>");
			System.exit(2);
		}

		boolean mainOnly = false;
		String filespec = "";

		List<String> simulatorArgs = new ArrayList<String>();
		for (String arg : args) {
			if (arg.charAt(0) == '-') {
				if (arg.equals("--mainOnly")) {
					mainOnly = true;
				} else {
					System.err.println("Unknown option: " + arg);
					System.exit(1);
				}
			} else  {
				simulatorArgs.add(arg);
			}
		}

		Path originalPath = Path.of(simulatorArgs.get(0)).toAbsolutePath();
		Path newPath = originalPath;
		Path symbolPath = originalPath;
		Map<String, Long> symbolMap = null;
		Map<Long, String> reverseSymbolMap = null;
		Map<String, Simulator.LabelType> symbolTypes = null;
		// Get filename without extension
		String fileName = newPath.getFileName().toString();
		int dot = fileName.indexOf('.');
		String baseName = (dot == -1) ? fileName : fileName.substring(0, dot);
		// Compose new path
		newPath = originalPath.resolveSibling(baseName + ".o64");
		System.out.println("Looking for object file: " + newPath.toString());
		if (!Files.isRegularFile(newPath)) {
			throw new RuntimeException("Can't locate object file for program: " + baseName);
		}
		symbolPath = originalPath.resolveSibling(baseName + ".sym");
		if (!Files.isRegularFile(symbolPath)) {
			System.out.println("Can't locate label file for program: " + baseName);
		} else {
			symbolMap = Simulator.readLabelMapFromFile(symbolPath.toFile());
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
		symbolPath = originalPath.resolveSibling(baseName + ".sym2");
		if (!Files.isRegularFile(symbolPath)) {
			System.out.println("Can't locate label type file for program: " + baseName);
		} else {
			symbolTypes = Simulator.readLabelTypesFromFile(symbolPath.toFile());
		}

		// 1) Read object file
		var program = cloud.lesh.CPUSim64.AsmIO.readU64BE(newPath.toFile());
		System.out.println("Read " + program.size() + " words from " + newPath.getFileName().toString());

		var sim = new Simulator(program.size(), 0, 1024, simulatorArgs.toArray(String[]::new));
		sim.loadProgram(program, 0L, reverseSymbolMap);
		System.out.println(sim.disassemble(mainOnly ? program.get(0) : 1L, symbolTypes));

		System.out.println("=".repeat(80));
		System.out.println("Labels");
		System.out.println("=".repeat(80));
		List<String> keys = new ArrayList<>(symbolMap.keySet());
		Collections.sort(keys);
		for (var key : keys) {
			System.out.println(String.format(fmtLabel, symbolMap.get(key) , key));
		}
	}
}
