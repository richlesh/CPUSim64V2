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

import static org.junit.jupiter.api.Assertions.*;

/** Tests for {@link Utils#decodeSI}, the parser of --mem= and --stack= (mirrors util::decode_si). */
public class DecodeSITest {

	@Test
	void testDecimalSuffixes() {
		assertEquals(42, Utils.decodeSI("42"));
		assertEquals(1000, Utils.decodeSI("1K"));
		assertEquals(2_500_000, Utils.decodeSI("2.5M"));
		assertEquals(16_000_000, Utils.decodeSI("16m"));
		assertEquals(2_000_000_000, Utils.decodeSI("2G"));
	}

	@Test
	void testBinarySuffixes() {
		assertEquals(1024, Utils.decodeSI("1Ki"));
		assertEquals(8192, Utils.decodeSI("8ki"));
		assertEquals(16 * 1024 * 1024, Utils.decodeSI("16Mi"));
		assertEquals(512 * 1024, Utils.decodeSI("0.5Mi"));
		assertEquals(1 << 30, Utils.decodeSI("1GI"));
	}

	@Test
	void testErrors() {
		assertThrows(NumberFormatException.class, () -> Utils.decodeSI("2Gi"));   // 2^31 > Integer.MAX_VALUE
		assertThrows(NumberFormatException.class, () -> Utils.decodeSI("5i"));    // 'i' alone is not a suffix
		assertThrows(NumberFormatException.class, () -> Utils.decodeSI("Ki"));
		assertThrows(NumberFormatException.class, () -> Utils.decodeSI("-1K"));
		assertThrows(IllegalArgumentException.class, () -> Utils.decodeSI(" "));
	}
}
