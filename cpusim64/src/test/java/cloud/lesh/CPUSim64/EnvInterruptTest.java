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
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.Test;

/**
 * End-to-end test for iENV (cached environment variable lookup).
 */
public class EnvInterruptTest extends BaseTest {

	@Test
	void testEnvIsCached() throws Exception {
		String expected = System.getenv("PATH");
		assertTrue(expected != null && !expected.isEmpty(), "PATH must be set for this test");
		String src = """
			#include <system/system.def>

			MAIN:
				move   r1, "PATH"
				int    iENV
				move   r5, r0              // first fetch
				move   r1, "PATH"
				int    iENV
				move   r6, r0              // second fetch: same cached string
				move   r1, "CPUSIM64_SURELY_NOT_SET_12345"
				int    iENV
				move   r7, r0              // not set -> 0
				move   r1, "CPUSIM64_SURELY_NOT_SET_12345"
				int    iENV
				move   r8, r0
				move   r1, 0
				int    iENV
				move   r9, r0              // null name -> 0
				stop
			""";
		var tuple = runProgram(src, new String[] {"test"});
		var diff = tuple.getRight();
		long first = diff.getReg(5);
		assertNotEquals(0, first);
		assertEquals(first, diff.getReg(6));
		assertEquals(expected, tuple.getMiddle().convertString(first));
		diff.assertDiff(7, 0);
		diff.assertDiff(8, 0);
		diff.assertDiff(9, 0);
	}
}
