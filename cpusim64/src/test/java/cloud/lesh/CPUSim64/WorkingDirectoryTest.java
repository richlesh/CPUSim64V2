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

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

/**
 * The working directory a host (the IDE) sets for the in-process simulator:
 * relative file names, iSYSTEM commands and $PWD resolve against it.
 */
public class WorkingDirectoryTest extends BaseTest {

	private static final String SRC = """
		#include <system/system.def>
		#include <system/io.def>

		MAIN:
			move   r1, "data.txt"
			int    iFILE_EXISTS
			move   r5, r0              // -1 if data.txt is in the working directory
			move   r1, "./sum.sh 4 5"
			int    iSYSTEM
			move   r6, r0              // 9 if sum.sh ran in the working directory
			move   r1, "PWD"
			int    iENV
			move   r7, r0
			move   r1, "./pwd.sh"
			int    iSYSTEM
			stop
		""";

	@Test
	void testRelativeNamesResolveAgainstWorkingDirectory(@TempDir Path dir) throws Exception {
		Files.writeString(dir.resolve("data.txt"), "x");
		Path script = dir.resolve("sum.sh");
		Files.copy(Path.of("src/test/resources/sum.sh"), script, StandardCopyOption.REPLACE_EXISTING);
		script.toFile().setExecutable(true);
		// writes the $PWD that a command started by iSYSTEM sees
		Path pwdScript = dir.resolve("pwd.sh");
		Files.writeString(pwdScript, "#!/bin/sh\nprintf '%s' \"$PWD\" > pwd.txt\n");
		pwdScript.toFile().setExecutable(true);
		StdInterruptHandler.setGlobalWorkingDirectory(dir.toFile());
		try {
			var tuple = runProgram(SRC, new String[] {"test"});
			var diff = tuple.getRight();
			diff.assertDiff(5, -1);
			diff.assertDiff(6, 9);
			assertEquals(dir.toFile().getAbsolutePath(), tuple.getMiddle().convertString(diff.getReg(7)));
			assertEquals(dir.toFile().getAbsolutePath(), Files.readString(dir.resolve("pwd.txt")));
		} finally {
			StdInterruptHandler.setGlobalWorkingDirectory(null);
		}
	}

	@Test
	void testResolveFileWithoutWorkingDirectory() {
		StdInterruptHandler.setGlobalWorkingDirectory(null);
		assertEquals("data.txt", StdInterruptHandler.resolveFile("data.txt").getPath());
		StdInterruptHandler.setGlobalWorkingDirectory(new java.io.File("/tmp/wd"));
		try {
			assertEquals(new java.io.File("/tmp/wd", "a/b.txt"), StdInterruptHandler.resolveFile("a/b.txt"));
			assertEquals("/etc/hosts", StdInterruptHandler.resolveFile("/etc/hosts").getPath());
			assertEquals("", StdInterruptHandler.resolveFile("").getPath());
		} finally {
			StdInterruptHandler.setGlobalWorkingDirectory(null);
		}
	}
}
