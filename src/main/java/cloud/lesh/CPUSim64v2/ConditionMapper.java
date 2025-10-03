package cloud.lesh.CPUSim64v2;

import org.antlr.v4.runtime.Token;

public final class ConditionMapper {
	private ConditionMapper() {}

	public static Condition fromToken(Token t, int type) {
		switch (type) {
			case CPUSim64v2Parser.U:  return Condition.U;
			case CPUSim64v2Parser.Z:
			case CPUSim64v2Parser.EQ: return Condition.EQ;
			case CPUSim64v2Parser.NZ:
			case CPUSim64v2Parser.NE: return Condition.NE;
			case CPUSim64v2Parser.N:
			case CPUSim64v2Parser.LT: return Condition.LT;
			case CPUSim64v2Parser.P:
			case CPUSim64v2Parser.GT: return Condition.GT;
			case CPUSim64v2Parser.NN:
			case CPUSim64v2Parser.GE: return Condition.GE;
			case CPUSim64v2Parser.NP:
			case CPUSim64v2Parser.LE: return Condition.LE;
		}
		throw new IllegalArgumentException("Not a condition: " + t.getText());
	}
}
