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

import org.antlr.v4.runtime.Token;

public final class ConditionMapper {
	private ConditionMapper() {}

	public static Condition fromToken(Token t, int type) {
		switch (type) {
			case CPUSim64Parser.U:  return Condition.U;
			case CPUSim64Parser.Z:
			case CPUSim64Parser.EQ: return Condition.EQ;
			case CPUSim64Parser.NZ:
			case CPUSim64Parser.NE: return Condition.NE;
			case CPUSim64Parser.N:
			case CPUSim64Parser.LT: return Condition.LT;
			case CPUSim64Parser.P:
			case CPUSim64Parser.GT: return Condition.GT;
			case CPUSim64Parser.NN:
			case CPUSim64Parser.GE: return Condition.GE;
			case CPUSim64Parser.NP:
			case CPUSim64Parser.LE: return Condition.LE;
		}
		throw new IllegalArgumentException("Not a condition: " + t.getText());
	}
}
