// SPDX-License-Identifier: Apache-2.0
//
// VBCheck - parses a VB.NET file with the ANTLR grammar grammar/VBLexer.g4
// and grammar/VBParser.g4; exit status 0 if the file is accepted, 1 if not.
// Used by tests/antlr_check.sh to compare the grammar with the compiler's
// hand-written parser (vbc --parse-only).
//
// Between the lexer and the parser a token filter does what the compiler's
// lexer (src/lexer.c) does and a grammar without actions cannot:
//   - the type characters '!' and '&' directly after an identifier belong to
//     the identifier (x! x&), unless a name or '=' follows (a!b, a&=b);
//   - implicit line continuation (join_lines in src/lexer.c): a line end is
//     dropped after a token that continues the line (',' '(' '{' '.' '='
//     ':=' the binary and compound assignment operators, Mod And Or Xor
//     AndAlso OrElse Like Is IsNot In), before ')' and '}', and repeated or
//     leading line ends are dropped; a line end is added at the end;
//   - the holes of an interpolated string ($"a{x + 1,5:N2}") are parsed as
//     expressions (rule holeExpression), as src/parser.c parse_interp does.

import java.util.ArrayList;
import java.util.List;

import org.antlr.v4.runtime.BaseErrorListener;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonToken;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.ListTokenSource;
import org.antlr.v4.runtime.RecognitionException;
import org.antlr.v4.runtime.Recognizer;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.misc.Interval;

public class VBCheck {

    static int errors = 0;
    static String fileName;

    static class Listener extends BaseErrorListener {
        final int lineOffset;
        Listener(int lineOffset) { this.lineOffset = lineOffset; }
        @Override
        public void syntaxError(Recognizer<?, ?> r, Object sym, int line, int col, String msg, RecognitionException e) {
            if (errors++ == 0) System.out.println(fileName + ":" + (line + lineOffset) + ":" + (col + 1) + ": " + msg);
        }
    }

    public static void main(String[] args) throws Exception {
        fileName = args[0];
        CharStream cs = CharStreams.fromFileName(fileName);
        List<Token> toks = filter(lex(cs, 0), cs, true);
        if (errors == 0) parseFile(toks);
        if (errors == 0) checkHoles(toks);
        System.exit(errors == 0 ? 0 : 1);
    }

    static List<Token> lex(CharStream cs, int lineOffset) {
        VBLexer lexer = new VBLexer(cs);
        lexer.removeErrorListeners();
        lexer.addErrorListener(new Listener(lineOffset));
        List<Token> out = new ArrayList<>();
        for (Token t : lexer.getAllTokens()) out.add(t);
        return out;
    }

    static boolean isLetter(int c) {
        return Character.isLetter(c) || Character.getType(c) == Character.LETTER_NUMBER;
    }

    static boolean isIdChar(int c) {
        int t = Character.getType(c);
        return isLetter(c) || t == Character.DECIMAL_DIGIT_NUMBER || t == Character.NON_SPACING_MARK
            || t == Character.COMBINING_SPACING_MARK || t == Character.FORMAT || t == Character.CONNECTOR_PUNCTUATION;
    }

    static boolean isDQ(int c) { return c == '"' || c == 0x201C || c == 0x201D; }

    static int charAt(CharStream cs, int i) {
        return i < cs.size() ? cs.getText(Interval.of(i, i)).codePointAt(0) : -1;
    }

    static boolean contAfter(int k) {
        switch (k) {
            case VBLexer.COMMA: case VBLexer.LPAREN: case VBLexer.LBRACE: case VBLexer.DOT: case VBLexer.QDOT:
            case VBLexer.EQ: case VBLexer.COLONEQ:
            case VBLexer.AMP: case VBLexer.STAR: case VBLexer.PLUS: case VBLexer.MINUS: case VBLexer.SLASH:
            case VBLexer.BACKSLASH: case VBLexer.CARET:
            case VBLexer.LT: case VBLexer.GT: case VBLexer.LE: case VBLexer.GE: case VBLexer.NE:
            case VBLexer.SHL: case VBLexer.SHR:
            case VBLexer.PLUSEQ: case VBLexer.MINUSEQ: case VBLexer.STAREQ: case VBLexer.SLASHEQ:
            case VBLexer.BACKSLASHEQ: case VBLexer.CARETEQ: case VBLexer.AMPEQ: case VBLexer.SHLEQ: case VBLexer.SHREQ:
            case VBLexer.MOD: case VBLexer.AND: case VBLexer.OR: case VBLexer.XOR: case VBLexer.ANDALSO:
            case VBLexer.ORELSE: case VBLexer.LIKE: case VBLexer.IS: case VBLexer.ISNOT: case VBLexer.IN:
                return true;
        }
        return false;
    }

    static List<Token> filter(List<Token> in, CharStream cs, boolean addNewline) {
        // type characters ! and &
        List<Token> merged = new ArrayList<>();
        for (int i = 0; i < in.size(); i++) {
            Token t = in.get(i);
            if (t.getType() == VBLexer.IDENTIFIER && i + 1 < in.size()) {
                Token n = in.get(i + 1);
                String txt = t.getText();
                char last = txt.charAt(txt.length() - 1);
                boolean hasType = last == '%' || last == '@' || last == '#' || last == '$';
                if (!hasType && n.getStartIndex() == t.getStopIndex() + 1
                        && (n.getType() == VBLexer.BANG || n.getType() == VBLexer.AMP)) {
                    int c = charAt(cs, n.getStopIndex() + 1);
                    boolean ok = n.getType() == VBLexer.BANG
                        ? !(isIdChar(c) || c == '[')
                        : !(c == '=' || isIdChar(c) || isDQ(c));
                    if (ok) {
                        CommonToken m = new CommonToken(t);
                        m.setStopIndex(n.getStopIndex());
                        m.setText(txt + n.getText());
                        merged.add(m);
                        i++;
                        continue;
                    }
                }
            }
            merged.add(t);
        }
        // implicit line continuation
        List<Token> out = new ArrayList<>();
        for (int i = 0; i < merged.size(); i++) {
            Token t = merged.get(i);
            if (t.getType() == VBLexer.NEWLINE) {
                int prev = out.isEmpty() ? -1 : out.get(out.size() - 1).getType();
                if (prev < 0 || prev == VBLexer.NEWLINE || contAfter(prev)) continue;
                int j = i + 1;
                while (j < merged.size() && merged.get(j).getType() == VBLexer.NEWLINE) j++;
                if (j < merged.size() && (merged.get(j).getType() == VBLexer.RPAREN
                        || merged.get(j).getType() == VBLexer.RBRACE)) continue;
            }
            out.add(t);
        }
        if (addNewline && !out.isEmpty() && out.get(out.size() - 1).getType() != VBLexer.NEWLINE) {
            CommonToken nl = new CommonToken(out.get(out.size() - 1));
            nl.setType(VBLexer.NEWLINE);
            nl.setText("\n");
            out.add(nl);
        }
        return out;
    }

    static VBParser parser(List<Token> toks, int lineOffset) {
        VBParser p = new VBParser(new CommonTokenStream(new ListTokenSource(toks)));
        p.removeErrorListeners();
        p.addErrorListener(new Listener(lineOffset));
        return p;
    }

    static void parseFile(List<Token> toks) {
        parser(toks, 0).start();
    }

    // parses s[from, to) of an interpolated string token as an expression
    static void parseHole(String s, int from, int to, int line) {
        CharStream cs = CharStreams.fromString(s.substring(from, to));
        List<Token> toks = filter(lex(cs, line - 1), cs, false);
        if (errors == 0) parser(toks, line - 1).holeExpression();
    }

    static void checkHoles(List<Token> toks) {
        for (Token t : toks) {
            if (errors != 0) return;
            if (t.getType() != VBLexer.INTERPOLATED_STRING) continue;
            int[] s = t.getText().codePoints().toArray();
            String str = new String(s, 0, s.length);
            int i = 2, end = s.length - 1, line = t.getLine();
            while (i < end) {
                int c = s[i];
                if (c == '{' && s[i + 1] == '{') { i += 2; continue; }
                if (c == '}' && s[i + 1] == '}') { i += 2; continue; }
                if (isDQ(c)) { i += 2; continue; }
                if (c == '\n') line++;
                if (c != '{') { i++; continue; }
                int hs = i + 1, j = hs, depth = 0, comma = -1, colon = -1;
                for (;;) {
                    if (j >= end) { errors++; System.out.println(fileName + ":" + line + ": '}' expected"); return; }
                    int d = s[j];
                    if (colon >= 0) {
                        if (d == '}') break;
                        j++;
                        continue;
                    }
                    if (isDQ(d)) {
                        j++;
                        while (j < end && !(isDQ(s[j]) && !isDQ(s[j + 1]))) j += isDQ(s[j]) ? 2 : 1;
                        j++;
                        continue;
                    }
                    if (d == '(' || d == '[' || d == '{') depth++;
                    else if (d == ')' || d == ']') depth--;
                    else if (d == '}') {
                        if (depth == 0) break;
                        depth--;
                    } else if (depth == 0 && d == ',' && comma < 0) comma = j;
                    else if (depth == 0 && d == ':') colon = j;
                    j++;
                }
                int ee = comma >= 0 ? comma : colon >= 0 ? colon : j;
                parseHole(str, offset(s, hs), offset(s, ee), line);
                if (comma >= 0 && errors == 0) parseHole(str, offset(s, comma + 1), offset(s, colon >= 0 ? colon : j), line);
                i = j + 1;
            }
        }
    }

    // UTF-16 offset of codepoint index k
    static int offset(int[] s, int k) {
        int o = 0;
        for (int i = 0; i < k; i++) o += Character.charCount(s[i]);
        return o;
    }
}
