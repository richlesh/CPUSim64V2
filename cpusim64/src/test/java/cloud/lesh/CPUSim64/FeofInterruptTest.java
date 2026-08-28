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

import org.junit.jupiter.api.Test;

/**
 * End-to-end tests for the iFEOF / iFERROR interrupts and the FEOF / FERROR
 * macros. Verifies that after a short 8-byte word read returns -1, FEOF reports
 * end-of-file, the pushed-back tail bytes are recoverable via byte reads, and
 * FERROR reports no error.
 */
public class FeofInterruptTest extends BaseTest {

	@Test
	void testFeofAfterShortWordRead() {
		// 11 bytes big-endian: full word 0x0001020304050607 then tail 8, 9, 10.
		byte[] input = new byte[] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		String src = """
			#include <system/system.def>
			#include <system/io.def>

			MAIN:
			#var w
				// Port 0 (STDIN) is big-endian by default.
				// First 8-byte word read succeeds.
				#macro IN8(w, 0)          // w = 0x0001020304050607
				move   r5, w              // r5 = full word

				// FEOF before EOF should be FALSE (0).
				#macro FEOF(0)
				move   r6, r0             // r6 = eof-before (expect 0)

				// Second 8-byte word read hits EOF after 3 bytes -> -1.
				#macro IN8(w, 0)
				move   r7, w              // r7 = short read result (expect -1)

				// FEOF now reports end-of-file (TRUE = -1).
				#macro FEOF(0)
				move   r8, r0             // r8 = eof-after (expect -1)

				// FERROR reports no error (FALSE = 0).
				#macro FERROR(0)
				move   r9, r0             // r9 = error (expect 0)

				// The 3 pushed-back tail bytes are recoverable via byte reads.
				#macro IN1(w, 0)
				move   r2, w              // r2 = 8
				#macro IN1(w, 0)
				move   r3, w              // r3 = 9
				#macro IN1(w, 0)
				move   r4, w              // r4 = 10

				// Stream now truly exhausted: byte read returns -1.
				#macro IN1(w, 0)
				move   r11, w             // r11 = -1

				stop
				stop
			""";

		var tuple = runProgram(src, new String[] {"test"}, input);
		var diff = tuple.getRight();

		diff.assertDiff(5, 0x0001020304050607L); // full word
		diff.assertDiff(6, 0);                    // FEOF before EOF -> FALSE
		diff.assertDiff(7, -1);                   // short 8-byte read -> -1
		diff.assertDiff(8, -1);                   // FEOF after short read -> TRUE
		diff.assertDiff(9, 0);                    // FERROR -> FALSE (no error)
		diff.assertDiff(2, 8);                    // tail byte 1
		diff.assertDiff(3, 9);                    // tail byte 2
		diff.assertDiff(4, 10);                   // tail byte 3
		diff.assertDiff(11, -1);                  // final byte read -> EOF
	}
}
