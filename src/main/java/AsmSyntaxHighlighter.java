// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import javax.swing.text.*;
import java.awt.*;
import java.util.Set;
import java.util.regex.*;

public class AsmSyntaxHighlighter {

    private static final Set<String> INSTRUCTIONS = Set.of(
        "nop", "debug", "clear", "move", "mov", "load", "loa", "store", "sto",
        "pop", "push", "jump", "call", "return", "ret", "interrupt", "int", "stop",
        "neg", "add", "sub", "mult", "mul", "div", "recip", "compl", "not",
        "and", "or", "xor", "test", "tst", "cmp",
        "lshift", "lsh", "rshift", "rsh", "arshift", "arsh",
        "lrotate", "lrot", "rrotate", "rrot",
        "in", "out", "pack", "pack64", "unpack", "unpack64",
        "cas", "endian", "save", "restore", "rest", "readonly"
    );

    private static final Set<String> DIRECTIVES = Set.of(
        ".dci", ".dcf", ".dca", ".dcb", ".dcc", ".dcw", ".dcs", ".org",
        ".line", ".line_begin", ".line_end", ".block", ".block_end"
    );

    private static final Set<String> CONDITIONS = Set.of(
        "u", "z", "nz", "eq", "ne", "n", "lt", "p", "gt",
        "nn", "ge", "np", "le", "o", "no", "pe", "po"
    );

    private static final Pattern TOKEN_PATTERN = Pattern.compile(
        "(?<COMMENT>//[^\\n]*|/\\*[\\s\\S]*?\\*/)" +
        "|(?<STRING>\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\")" +
        "|(?<CHAR>'[^'\\\\]'|'\\\\[^']')" +
        "|(?<HEX>-?0[xX][0-9A-Fa-f]+)" +
        "|(?<FLOAT>-?\\d+\\.\\d*(?:[eE][+-]?\\d+)?|-?\\.\\d+(?:[eE][+-]?\\d+)?|-?\\d+[eE][+-]?\\d+)" +
        "|(?<INT>-?\\d+)" +
        "|(?<REG>[rRfF]\\d+|[sS][fFpP]|[pP][cC]|[sS][rR])" +
        "|(?<LABEL>[\\w$]+:)" +
        "|(?<DIRECTIVE>\\.[a-zA-Z_]+)" +
        "|(?<WORD>[a-zA-Z_$][a-zA-Z0-9_${}]*)"
    );

    private final Style normal;
    private final Style keyword;
    private final Style directive;
    private final Style comment;
    private final Style string;
    private final Style number;
    private final Style register;
    private final Style label;
    private final Style condition;

    private final JTextPane textPane;

    public static final String[] CATEGORY_NAMES = {
        "Normal", "Keywords", "Directives", "Comments",
        "Strings", "Numbers", "Registers", "Labels", "Conditions"
    };

    public AsmSyntaxHighlighter(JTextPane pane) {
        this.textPane = pane;
        StyledDocument doc = pane.getStyledDocument();

        normal = doc.addStyle("normal", null);
        StyleConstants.setForeground(normal, Color.BLACK);

        keyword = doc.addStyle("keyword", null);
        StyleConstants.setForeground(keyword, new Color(0, 0, 180));
        StyleConstants.setBold(keyword, true);

        directive = doc.addStyle("directive", null);
        StyleConstants.setForeground(directive, new Color(128, 0, 128));
        StyleConstants.setBold(directive, true);

        comment = doc.addStyle("comment", null);
        StyleConstants.setForeground(comment, new Color(0, 128, 0));
        StyleConstants.setItalic(comment, true);

        string = doc.addStyle("string", null);
        StyleConstants.setForeground(string, new Color(163, 21, 21));

        number = doc.addStyle("number", null);
        StyleConstants.setForeground(number, new Color(180, 100, 0));

        register = doc.addStyle("register", null);
        StyleConstants.setForeground(register, new Color(200, 0, 100));

        label = doc.addStyle("label", null);
        StyleConstants.setForeground(label, new Color(0, 100, 100));
        StyleConstants.setBold(label, true);

        condition = doc.addStyle("condition", null);
        StyleConstants.setForeground(condition, new Color(100, 0, 150));
    }

    public Color getColor(int index) {
        return StyleConstants.getForeground(getStyleByIndex(index));
    }

    public void setColor(int index, Color c) {
        StyleConstants.setForeground(getStyleByIndex(index), c);
    }

    private Style getStyleByIndex(int index) {
        return switch (index) {
            case 0 -> normal;
            case 1 -> keyword;
            case 2 -> directive;
            case 3 -> comment;
            case 4 -> string;
            case 5 -> number;
            case 6 -> register;
            case 7 -> label;
            case 8 -> condition;
            default -> normal;
        };
    }

    public void highlight() {
        StyledDocument doc = textPane.getStyledDocument();
        String text;
        try {
            text = doc.getText(0, doc.getLength());
        } catch (BadLocationException e) {
            return;
        }

        doc.setCharacterAttributes(0, text.length(), normal, true);

        Matcher m = TOKEN_PATTERN.matcher(text);
        while (m.find()) {
            Style style = null;
            if (m.group("COMMENT") != null) {
                style = comment;
            } else if (m.group("STRING") != null || m.group("CHAR") != null) {
                style = string;
            } else if (m.group("HEX") != null || m.group("FLOAT") != null || m.group("INT") != null) {
                style = number;
            } else if (m.group("REG") != null) {
                style = register;
            } else if (m.group("LABEL") != null) {
                style = label;
            } else if (m.group("DIRECTIVE") != null) {
                if (DIRECTIVES.contains(m.group().toLowerCase())) {
                    style = directive;
                }
            } else if (m.group("WORD") != null) {
                String word = m.group().toLowerCase();
                if (INSTRUCTIONS.contains(word)) {
                    style = keyword;
                } else if (CONDITIONS.contains(word)) {
                    style = condition;
                }
            }
            if (style != null) {
                doc.setCharacterAttributes(m.start(), m.end() - m.start(), style, true);
            }
        }
    }
}
