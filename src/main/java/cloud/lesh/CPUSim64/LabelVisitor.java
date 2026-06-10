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
import org.antlr.v4.runtime.tree.ErrorNode;
import org.antlr.v4.runtime.tree.ParseTree;
import org.antlr.v4.runtime.tree.TerminalNode;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.StringReader;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class LabelVisitor extends CPUSim64BaseVisitor<Void> implements HasLocation {
	private final StringBuilder out = new StringBuilder();
	private final Map<String, Long> labelMap = new HashMap<>();
	private final Map<Long, String> reverseLabelMap = new HashMap<>();
	private final Set<String> definedLabels = new HashSet<>();
	private final Stack<String> blockNames = new Stack<>();
	private long currentAddress = 0;
	private long blockCount = 0;
	private boolean hasErrors = false;

	// TODO remove
	String filename = null;
	int lineNum = 1;
	boolean pauseLineIncrement = false;
	CommonTokenStream tokens;

	public void setTokens(CommonTokenStream tokens) {
		this.tokens = tokens;
	}
	public boolean hasErrors() { return hasErrors; }
	public String getLocation(int offendingLine) {
		String loc = filename + ", " + Integer.toString(lineNum);
		return loc;
	}

	public Map<String, Long> getLabelMap() {
		labelMap.putIfAbsent("__START__", 0L);
		labelMap.putIfAbsent("__CODE__", 0L);
		labelMap.putIfAbsent("__CODE_END__", currentAddress);
		labelMap.putIfAbsent("__DATA__", currentAddress);
		labelMap.putIfAbsent("__DATA_END__", currentAddress);
		labelMap.putIfAbsent("__HEAP_START__", currentAddress);
		return labelMap;
	}

	public Map<Long, String> getReverseLabelMap() {
		return reverseLabelMap;
	}

	private static Token startToken(ParseTree node) {
		if (node instanceof ParserRuleContext r) return r.getStart();
		if (node instanceof TerminalNode t)      return t.getSymbol();
		if (node instanceof ErrorNode e)         return e.getSymbol();
		return null;
	}

	private long parseIntLike(String text) {
		if (text.startsWith("0x") || text.startsWith("0X")) {
			return Long.parseUnsignedLong(text.substring(2), 16);
		} else if (text.startsWith("-0x") || text.startsWith("-0X")) {
			return -Long.parseUnsignedLong(text.substring(3), 16);
		} else if ((text.charAt(0) == '-') ||
				(text.charAt(0) >= '0' && text.charAt(0) <= '9')) {
			return Long.parseLong(text);
		} else {
			throw new IllegalArgumentException("Can't parse integer: " + text);
		}
	}

	@Override
	public Void visitProgram(CPUSim64Parser.ProgramContext ctx) {
		for (var child : ctx.children) {
			visit(child);
			Token t = startToken(child);
			if (t != null) {
				int line = t.getLine();
				int col  = t.getCharPositionInLine();
			}
		}
		return null;
	}

	private String getScopeName() {
		return String.join("$", blockNames).toUpperCase();
	}

	@Override
	public Void visitLabelDef(CPUSim64Parser.LabelDefContext ctx) {
		String labelName = ctx.IDENT().getText().toUpperCase();
		if (definedLabels.contains(labelName)) {
			System.err.println(getLocation(lineNum) + ":ASMERROR:Duplicate label '" + labelName + "'");
			hasErrors = true;
		} else {
			if (labelName.charAt(0) == '$')
				labelName = getScopeName() + labelName;
			definedLabels.add(labelName);
			labelMap.put(labelName, currentAddress);
			reverseLabelMap.put(currentAddress, labelName);
		}
		return null;
	}

	@Override
	public Void visitInstruction(CPUSim64Parser.InstructionContext ctx) {
		++currentAddress;
		String s = reflowTokens(ctx) + System.lineSeparator();
		out.append(s);
		return null;
	}

	@Override
	public Void visitData_Directive(CPUSim64Parser.Data_DirectiveContext ctx) {
		if (ctx.dataDirective() != null) {
			if (ctx.dataDirective().DCI() != null) {
				++currentAddress;
			} else if (ctx.dataDirective().DCF() != null) {
				++currentAddress;
			} else if (ctx.dataDirective().DCS() != null) {
				if (ctx.dataDirective().STRINGLIT() == null || ctx.dataDirective().STRINGLIT().getText().length() < 2)
				{
					System.err.println(getLocation(lineNum) + ":ASMERROR:Missing string literal for .DCS directive");
					hasErrors = true;
					return null;
				}
				String s = ctx.dataDirective().STRINGLIT().getText();
				s = s.substring(1, s.length() - 1);
				byte[] utf8 = Utils.parseStringLiteral(s);
				currentAddress += 1 + (utf8.length + 7) / 8;  // round up to nearest 8 bytes
			} else if (ctx.dataDirective().DCA() != null) {
				long b = 0;
				if (ctx.dataDirective().INTLIT() != null) {
					b = parseIntLike(ctx.dataDirective().INTLIT().getText());
				} else if (ctx.dataDirective().HEXLIT() != null) {
					b = parseIntLike(ctx.dataDirective().HEXLIT().getText());
				}
				currentAddress += 1 + b;
			} else if (ctx.dataDirective().DCB() != null) {
				currentAddress += 1 + (ctx.dataDirective().byteList().bLiteral().size() + 7) / 8;
			} else if (ctx.dataDirective().DCC() != null) {
				currentAddress += 1 + (ctx.dataDirective().byteList().bLiteral().size() + 3) / 4;
			} else if (ctx.dataDirective().DCW() != null) {
				int count = 0;
				if (ctx.dataDirective().intList() != null) {
					count = ctx.dataDirective().intList().kLiteral().size();
				} else if (ctx.dataDirective().floatList() != null) {
					count = ctx.dataDirective().floatList().FLOATLIT().size();
				} else if (ctx.dataDirective().charList() != null) {
					count = ctx.dataDirective().charList().CHARLIT().size();
				}
				currentAddress += 1 + count;
			}
		}
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitORG_Directive(CPUSim64Parser.ORG_DirectiveContext ctx) {
		if (ctx.INTLIT() != null) {
			currentAddress = Long.parseLong(ctx.INTLIT().getText());
		} else if (ctx.HEXLIT() != null) {
			currentAddress = Long.parseLong(ctx.HEXLIT().getText().substring(2), 16);
		} else {
			System.err.println(getLocation(lineNum) + ":ASMERROR:Missing integer literal for .ORG directive");
			hasErrors = true;
			return null;
		}
		currentAddress = Math.max(0, currentAddress); // prevent negative addresses
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_Directive(CPUSim64Parser.LINE_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		pauseLineIncrement = false;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_BEGIN_Directive(CPUSim64Parser.LINE_BEGIN_DirectiveContext ctx) {
		filename = ctx.FILENAMELIT().getText();
		lineNum = ctx.INTLIT() != null ? Integer.parseInt(ctx.INTLIT().getText()) : 1;
		pauseLineIncrement = true;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitLINE_END_Directive(CPUSim64Parser.LINE_END_DirectiveContext ctx) {
		pauseLineIncrement = false;
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitBLOCK_BEGIN_Directive(CPUSim64Parser.BLOCK_BEGIN_DirectiveContext ctx) {
		String blockname = null;
		if (ctx.IDENT() != null) {
			blockname = ctx.IDENT().getText();
		}
		if (blockname == null)
			throw new IllegalArgumentException(".block directive must have an argument!");
		if (blockname.contains("{}") || blockname.contains("%d") || blockname.contains("%x"))
			blockname = String.format(blockname.replace("{}", "%04x"), ++blockCount);
		blockNames.push(blockname);
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	public Void visitBLOCK_END_Directive(CPUSim64Parser.BLOCK_END_DirectiveContext ctx) {
		blockNames.pop();
		out.append(reflowTokens(ctx) + System.lineSeparator());
		return null;
	}

	@Override
	protected Void defaultResult() {
		return null;
	}

	/** Rebuild a directive line with spaces, rather than ctx.getText(). */
	private String reflowTokens(ParserRuleContext ctx) {
		String s = Utils.rebuildWithSingleSpaces(tokens, ctx);
		return s;
	}

	public String gatherLabels(String src) {
		CharStream input = CharStreams.fromString(src);
		var lex = new cloud.lesh.CPUSim64.CPUSim64Lexer(input);
		lex.removeErrorListeners();                				// remove ConsoleErrorListener
		lex.addErrorListener(this.new LabelErrorListener());     // collect lexer errors
		CommonTokenStream toks = new CommonTokenStream(lex);
		toks.fill();
		setTokens(toks);

		var parser = new cloud.lesh.CPUSim64.CPUSim64Parser(toks);
		parser.removeErrorListeners();             				// remove ConsoleErrorListener
		parser.addErrorListener(this.new LabelErrorListener());  // collect parser errors
		ParseTree tree = parser.program();
		visit(tree);
		return out.toString();
	}

	final class LabelErrorListener extends BaseErrorListener {
		public LabelErrorListener() {}

		@Override
		public void syntaxError(Recognizer<?, ?> recognizer,
								Object offendingSymbol,
								int line,
								int charPositionInLine,
								String msg,
								RecognitionException e) {

			String where = getLocation(line);

			System.err.println(where + ":ASMERROR:" + msg);
			hasErrors = true;
		}
	}
}
/*
Map<Integer, String> lineMap = getLineMap();
		for (int i = 0; i < errors.size(); ++i) {
String s = (String)errors.get(i);
			if (s.startsWith("Preprocessed line")) {
// Match and capture the line number
Matcher m = Pattern.compile("Preprocessed line (\\d+)").matcher(s);
				if (m.find()) {
int preLine = Integer.parseInt(m.group(1));
String mapped = lineMap.get(preLine);
					if (mapped != null) {
		// Replace with the mapped value
		errors.set(i, m.replaceAll(mapped));
		}
		}
		}
		}

 */