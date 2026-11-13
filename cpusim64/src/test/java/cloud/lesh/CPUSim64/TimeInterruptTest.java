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

import static org.junit.jupiter.api.Assertions.assertTrue;

import java.time.Instant;

import org.junit.jupiter.api.Test;

/**
 * End-to-end test for iTIME (wall clock time in ns since the Unix epoch).
 */
public class TimeInterruptTest extends BaseTest {

	@Test
	void testWallClockTime() throws Exception {
		String src = """
			#include <system/system.def>

			MAIN:
				int    iTIME
				move   r5, r0
				int    iTIME
				move   r6, r0
				stop
			""";
		long before = secondsToNs(Instant.now());
		var tuple = runProgram(src, new String[] {"test"});
		long after = secondsToNs(Instant.now());
		var diff = tuple.getRight();
		long t1 = diff.getReg(5);
		long t2 = diff.getReg(6);
		assertTrue(t1 >= before && t1 <= after, "iTIME is the current time in ns since 1970");
		assertTrue(t2 >= t1 && t2 <= after, "iTIME does not go backwards");
	}

	private static long secondsToNs(Instant i) {
		return i.getEpochSecond() * 1_000_000_000L + i.getNano();
	}
}
