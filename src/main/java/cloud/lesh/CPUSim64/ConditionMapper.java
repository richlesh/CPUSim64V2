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
