package cloud.lesh.CPUSim64v2;

import java.util.HashMap;
import java.util.Map;

public enum Opcode {
	// Numbers from your “Instructions” table (iiiiii, 0..63)
	NOP(0),
	DEBUG(0),
	CLEAR(1),
	MOVE(2),
	LOAD(3),
	STORE(4),
	POP(5),
	PUSH(6),
	JUMP(7),
	CALL(8),
	RETURN(9),
	INTERRUPT(10),
	STOP(11),
	NEGATE(12),
	ADD(13),
	SUBTRACT(14),
	MULTIPLY(15),
	DIVIDE(16),
	RECIP(16),           // RECIP is grouped under 16 in your doc
	COMPL(17),
	AND(18),
	OR(19),
	XOR(20),
	TEST(21),
	CMP(22),
	LSHIFT(23),
	RSHIFT(24),
	ARSHIFT(25),
	LROTATE(26),
	RROTATE(27),
	IN(28),
	OUT(29),
	PACK(30),
	PACK64(31),
	UNPACK(32),
	UNPACK64(33),
	CAS(34),
	ENDIAN(35),
	SAVE(36),
	RESTORE(37),
	UNDEFINED(-1);

	public final int code;
	static Map<Integer, Opcode> codeMap = new HashMap<Integer, Opcode>();
	static Map<String, Opcode> nameMap = new HashMap<String, Opcode>();
	Opcode(int code) {
		this.code = code;
	}

	static {
		for (var e : Opcode.values()) {
			nameMap.put(e.name(), e);
			codeMap.put(e.code, e);
		}
	}

	public int getCode() { return code; }
	public String getName() { return name(); }

	public static Opcode fromCode(int op) {
		var e = codeMap.get(op);
		if (e == null) e = UNDEFINED;
		return e;
	}

	public static Opcode fromName(String n) {
		var e = nameMap.get(n);
		if (e == null) e = UNDEFINED;
		return e;
	}
}
