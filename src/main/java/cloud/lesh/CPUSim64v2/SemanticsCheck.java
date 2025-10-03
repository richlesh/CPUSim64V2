package cloud.lesh.CPUSim64v2;

import java.util.*;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.tree.*;

public class SemanticsCheck extends CPUSim64v2BaseListener {
	private final List<SemanticError> errors = new ArrayList<>();

	public List<SemanticError> getErrors() { return errors; }

	private void err(Token t, String msg) {
		errors.add(new SemanticError(t.getLine(), t.getCharPositionInLine(), msg));
	}

	/* ----- Register range checks ----- */

	@Override
	public void visitTerminal(TerminalNode node) {
		Token t = node.getSymbol();
		switch (t.getType()) {
			case CPUSim64v2Parser.REG_R:
				checkIntReg(t);
				break;
			case CPUSim64v2Parser.REG_F:
				checkFloatReg(t);
				break;
		}
	}

	private void checkIntReg(Token t) {
		String s = t.getText().substring(1); // strip leading 'r'
		try {
			int n = Integer.parseInt(s);
			if (n < 0 || n > 28) {
				err(t, "Integer register out of range: r" + n + " (valid r0..r28)");
			}
		} catch (NumberFormatException e) {
			err(t, "Malformed integer register: " + t.getText());
		}
	}

	private void checkFloatReg(Token t) {
		String s = t.getText().substring(1); // strip leading 'f'
		try {
			int n = Integer.parseInt(s);
			if (n < 0 || n > 31) {
				err(t, "Float register out of range: f" + n + " (valid f0..f31)");
			}
		} catch (NumberFormatException e) {
			err(t, "Malformed float register: " + t.getText());
		}
	}

	/* ----- Z-port checks for IN/OUT ----- */

	@Override
	public void enterInstrIN(CPUSim64v2Parser.InstrINContext ctx) {
		for (CPUSim64v2Parser.ZPortContext zctx : ctx.zPort()) {
			checkZPort(zctx.start);
		}
	}

	@Override
	public void enterInstrOUT(CPUSim64v2Parser.InstrOUTContext ctx) {
		for (CPUSim64v2Parser.ZPortContext zctx : ctx.zPort()) {
			checkZPort(zctx.start);
		}
	}

	private void checkZPort(Token t) {
		Integer val = parseIntLike(t);
		if (val == null) {
			err(t, "I/O port/size must be an integer literal (dec or hex).");
		} else if (val < 0 || val > 15) {
			err(t, "I/O port/size out of range: " + val + " (valid 0..15).");
		}
	}

	/* ----- Canonical condition capture (optional) ----- */

	@Override
	public void enterInstrJUMP(CPUSim64v2Parser.InstrJUMPContext ctx) {
		if (ctx.branchModes() != null && ctx.branchModes().zCond() != null) {
			Token t = ctx.branchModes().zCond().start;
			Condition c = ConditionMapper.fromToken(t, t.getType());
			// you can store/use `c` here
		}
	}

	@Override
	public void enterInstrCALL(CPUSim64v2Parser.InstrCALLContext ctx) {
		if (ctx.branchModes() != null && ctx.branchModes().zCond() != null) {
			Token t = ctx.branchModes().zCond().start;
			Condition c = ConditionMapper.fromToken(t, t.getType());
			// you can store/use `c` here
		}
	}

	/* ----- helpers ----- */

	private Integer parseIntLike(Token t) {
		String s = t.getText();
		try {
			if (s.startsWith("0x") || s.startsWith("0X")) {
				return Integer.parseInt(s.substring(2), 16);
			}
			return Integer.parseInt(s);
		} catch (Exception e) {
			return null;
		}
	}
}
