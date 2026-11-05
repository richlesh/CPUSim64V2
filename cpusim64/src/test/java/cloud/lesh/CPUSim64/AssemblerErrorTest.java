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

import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.Test;

import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Verifies that assembler/preprocessor errors surface as recoverable exceptions
 * (or non-zero return codes) rather than terminating the JVM via System.exit().
 *
 * Regression test for the IDE crashing when a program has assembler errors:
 * previously library code called System.exit(1) on error, which killed the
 * host JVM (the IDE) instead of allowing Assembler.run() to return a status.
 */
public class AssemblerErrorTest {

	@Test
	void preprocessorErrorDirectiveThrowsInsteadOfExiting() {
		// A #error directive previously called System.exit(1).
		String src = "#error This build is intentionally broken\n";
		var loader = new IncludeLoader(Path.of("."));
		PreprocessorVisitor.resetGlobals();
		PreprocessorVisitor pp = new PreprocessorVisitor("test.asm", loader);
		assertThrows(RuntimeException.class,
				() -> pp.preprocessText(src, new String[] { "test.asm" }),
				"#error directive should throw, not call System.exit()");
	}

	@Test
	void assemblerRunReturnsNonZeroOnPreprocessorSyntaxError() throws Exception {
		// A malformed preprocessor directive triggers preprocessor errors.
		// Assembler.run() must return non-zero rather than terminate the JVM.
		Path tmp = Files.createTempFile("cpusim64_pp", ".asm");
		try {
			Files.writeString(tmp, "#if\n#endif\n");
			int result = Assembler.run(new String[] { tmp.toString() });
			assertNotEquals(0, result,
					"Assembler.run() should return non-zero on preprocessor error");
		} finally {
			Files.deleteIfExists(tmp);
		}
	}

	@Test
	void assemblerRunReturnsNonZeroOnErrorDirective() throws Exception {
		// End-to-end: Assembler.run() on a file with #error must return a
		// non-zero status (not terminate the JVM).
		Path tmp = Files.createTempFile("cpusim64_err", ".asm");
		try {
			Files.writeString(tmp, "#error intentional failure\n");
			int result = Assembler.run(new String[] { tmp.toString() });
			assertNotEquals(0, result,
					"Assembler.run() should return non-zero on assembly error");
		} finally {
			Files.deleteIfExists(tmp);
		}
	}
}
