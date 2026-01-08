// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2025-2026 Richard Lesh
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

import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.Interval;
import org.antlr.v4.runtime.tree.ParseTree;

import java.util.Map;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class ExpressionFolder implements HasLocation {
	private Vector<String> errors = new Vector<String>();
	public Vector<String> getErrors() { return errors; }

	private String filename;
	private int line;

	public ExpressionFolder(String filename, int line) {
		this.filename = filename;
		this.line = line;
	}

	public String getLocation(int offendingLine) { return "\u00abfilename\u00bb:" + line; }

	/**
	 * Result of visiting a subtree:
	 * - If isConst==true, value fields are valid and `text` is the folded literal string.
	 * - If isConst==false, `text` is the original source text slice for that subtree.
	 */
	public static final class ConstExprResult {
		public final boolean isConst;
		public final boolean isFloat;   // if const, whether value should be treated as float
		public final long longVal;      // valid if const && !isFloat
		public final double doubleVal;  // valid if const && isFloat
		public final String text;       // either folded literal or original text

		private ConstExprResult(boolean isConst, boolean isFloat, long longVal, double doubleVal, String text) {
			this.isConst = isConst;
			this.isFloat = isFloat;
			this.longVal = longVal;
			this.doubleVal = doubleVal;
			this.text = text;
		}

		public static ConstExprResult nonConst(String originalText) {
			return new ConstExprResult(false, false, 0L, 0.0, originalText);
		}

		public static ConstExprResult constLong(long v) {
			return new ConstExprResult(true, false, v, 0.0, Long.toString(v));
		}

		public static ConstExprResult constDouble(double v) {
			// Use Java’s canonical formatting; change if you want fixed decimals, etc.
			return new ConstExprResult(true, true, 0L, v, Double.toString(v));
		}
	}

	public static final class ConstExprVisitor extends ConstExprBaseVisitor<ConstExprResult> {
		private final CommonTokenStream tokens;

		public ConstExprVisitor(CommonTokenStream tokens) {
			this.tokens = tokens;
		}

		/** Exact original source slice for this context, including hidden-channel tokens (WS). */
		private String original(ParserRuleContext ctx) {
			// ctx.getSourceInterval() indexes token positions in the token stream
			Interval iv = ctx.getSourceInterval();
			return tokens.getText(iv);
		}

		private static boolean promoteToFloat(ConstExprResult a, ConstExprResult b, String op) {
			// Typical rule: if either operand is float -> float
			// Keep integer division when both are ints (like C/Java) unless you want / to promote.
			return a.isFloat || b.isFloat;
		}

		@Override
		public ConstExprResult visitLine(ConstExprParser.LineContext ctx) {
			StringBuilder out = new StringBuilder();
			for (var p : ctx.part()) {
				ConstExprResult r = visit(p);
				out.append(r.text);
			}
			return ConstExprResult.nonConst(out.toString());
		}

		@Override
		public ConstExprResult visitPartOther(ConstExprParser.PartOtherContext ctx) {
			// OTHER is a single token; preserve exactly (including any punctuation/letters)
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitPartChar(ConstExprParser.PartCharContext ctx) {
			// OTHER is a single token; preserve exactly (including any punctuation/letters)
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitPartString(ConstExprParser.PartStringContext ctx) {
			// OTHER is a single token; preserve exactly (including any punctuation/letters)
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitPartExpr(ConstExprParser.PartExprContext ctx) {
			// Fold the expr if possible; if not, return original expr text
			ConstExprResult r = visit(ctx.expr());
			if (r.isConst) return r;
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitPrimary(ConstExprParser.PrimaryContext ctx) {
			if (ctx.INT() != null) {
				String s = ctx.INT().getText();
				// Basic decimal ints only; extend if you later add hex etc.
				long v = Long.parseLong(s);
				return ConstExprResult.constLong(v);
			}
			if (ctx.HEXINT() != null) {
				String s = ctx.HEXINT().getText();
				// Basic decimal ints only; extend if you later add hex etc.
				long v = Long.decode(s);
				return ConstExprResult.constLong(v);
			}
			if (ctx.FLOAT() != null) {
				String s = ctx.FLOAT().getText();
				double v = Double.parseDouble(s);
				return ConstExprResult.constDouble(v);
			}
			// Fallback (shouldn't happen with this grammar)
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitUnaryExpr(ConstExprParser.UnaryExprContext ctx) {
			ConstExprResult inner = visit(ctx.expr());
			if (inner.isConst) {
				if (inner.isFloat) return ConstExprResult.constDouble(-inner.doubleVal);
				return ConstExprResult.constLong(-inner.longVal);
			}
			// Not foldable: preserve EXACT original text
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitMulExpr(ConstExprParser.MulExprContext ctx) {
			// mulExpr : expr op=('*'|'/') expr ;
			ConstExprResult left = visit(ctx.expr(0));
			ConstExprResult right = visit(ctx.expr(1));

			if (left.isConst && right.isConst) {
				String op = ctx.op.getText();
				boolean asFloat = promoteToFloat(left, right, op);

				if (asFloat) {
					double a = left.isFloat ? left.doubleVal : (double) left.longVal;
					double b = right.isFloat ? right.doubleVal : (double) right.longVal;
					return switch (op) {
						case "*" -> ConstExprResult.constDouble(a * b);
						case "/" -> ConstExprResult.constDouble(a / b);
						default  -> ConstExprResult.nonConst(original(ctx));
					};
				} else {
					long a = left.longVal;
					long b = right.longVal;
					return switch (op) {
						case "*" -> ConstExprResult.constLong(a * b);
						case "/" -> ConstExprResult.constLong(a / b); // integer division (trunc toward zero)
						default  -> ConstExprResult.nonConst(original(ctx));
					};
				}
			}

			// Not foldable: preserve EXACT original text (including whitespace around operator)
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitAddExpr(ConstExprParser.AddExprContext ctx) {
			// addExpr : expr op=('+'|'-') expr ;
			ConstExprResult left = visit(ctx.expr(0));
			ConstExprResult right = visit(ctx.expr(1));

			if (left.isConst && right.isConst) {
				String op = ctx.op.getText();
				boolean asFloat = promoteToFloat(left, right, op);

				if (asFloat) {
					double a = left.isFloat ? left.doubleVal : (double) left.longVal;
					double b = right.isFloat ? right.doubleVal : (double) right.longVal;
					return switch (op) {
						case "+" -> ConstExprResult.constDouble(a + b);
						case "-" -> ConstExprResult.constDouble(a - b);
						default  -> ConstExprResult.nonConst(original(ctx));
					};
				} else {
					long a = left.longVal;
					long b = right.longVal;
					return switch (op) {
						case "+" -> ConstExprResult.constLong(a + b);
						case "-" -> ConstExprResult.constLong(a - b);
						default  -> ConstExprResult.nonConst(original(ctx));
					};
				}
			}

			// Not foldable: preserve EXACT original text
			return ConstExprResult.nonConst(original(ctx));
		}

		@Override
		public ConstExprResult visitParensExpr(ConstExprParser.ParensExprContext ctx) {
			// parensExpr : '(' expr ')' ;
			ConstExprResult e = visit(ctx.expr());
			return e;
		}
	}

	/** Convenience: fold an expression string. */
	public String fold(String input) {
		errors = new Vector<String>();
		CharStream cs = CharStreams.fromString(input);
		ConstExprLexer lexer = new ConstExprLexer(cs);
		var lexerListener = new CollectingErrorListener(errors, this);
		lexer.removeErrorListeners();                // remove ConsoleErrorListener
		lexer.addErrorListener(lexerListener);       // collect lexer errors
		CommonTokenStream tokens = new CommonTokenStream(lexer);
		tokens.fill();

		ConstExprParser parser = new ConstExprParser(tokens);
		var parserListener = new CollectingErrorListener(errors, this);
		parser.removeErrorListeners();             // remove ConsoleErrorListener
		parser.addErrorListener(parserListener);   // collect parser errors
		ConstExprParser.LineContext tree = parser.line();
		var v = new ConstExprVisitor(tokens);

		// Rewriter preserves EVERYTHING by default; we only replace folded expr spans
		TokenStreamRewriter rewriter = new TokenStreamRewriter(tokens);

		for (var part : tree.part()) {
			if (part instanceof ConstExprParser.PartExprContext pe) {
				ConstExprResult r = v.visit(pe.expr());
				if (r.isConst) {
					int a = pe.getStart().getTokenIndex();
					int b = pe.getStop().getTokenIndex();
					rewriter.replace(a, b, r.text);
				}
			}
		}

		for (int i = 0; i < errors.size(); ++i) {
			String s = errors.get(i);
			if (s.startsWith("Preprocessed line")) {
				// Match and capture the line number
				Matcher m = Pattern.compile("Preprocessed line (\\d+)").matcher(s);
				if (m.find()) {
					int preLine = Integer.parseInt(m.group(1));
					errors.set(i, m.replaceAll(Integer.toString(line)));
				}
			}
		}

		if (errors.size() > 0) {
			for (int i = 0; i < errors.size(); ++i) {
				System.err.println(errors.get(i));
			}
			System.exit(1);
		}
		return rewriter.getText();
	}

	public static void main(String[] args) {
		String[] tests = {
				"Example: 1 + 2 * 3",
				"  1 +   2 *   ( 3 + 4 ) \"Hello+Goodbye\"",
				"-(1 + 2) * 3",
				"2 + 3.5 * 0x2",
				"'C' 10 / 4",          // integer division -> 2
				"10.0 / 4",        // float division -> 2.5
		};

		var folder = new ExpressionFolder("test", 1);
		for (String t : tests) {
			System.out.println("IN : [" + t + "]");
			System.out.println("OUT: [" + folder.fold(t) + "]");
			System.out.println();
		}
	}
}