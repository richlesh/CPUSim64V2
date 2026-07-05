// SPDX-License-Identifier: Apache-2.0
import org.fife.ui.rsyntaxtextarea.*;
import javax.swing.text.Segment;
import java.util.Set;

/**
 * Custom TokenMaker for CPUSim64 assembly language syntax highlighting
 * in RSyntaxTextArea.
 */
public class CpuSim64TokenMaker extends AbstractTokenMaker {

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

    @Override
    public TokenMap getWordsToHighlight() {
        TokenMap map = new TokenMap();
        for (String s : INSTRUCTIONS) {
            map.put(s, Token.RESERVED_WORD);
        }
        for (String s : DIRECTIVES) {
            map.put(s, Token.FUNCTION);
        }
        for (String s : CONDITIONS) {
            map.put(s, Token.DATA_TYPE);
        }
        return map;
    }

    @Override
    public void addToken(Segment segment, int start, int end, int tokenType, int startOffset) {
        // Override to ensure case-insensitive keyword recognition
        if (tokenType == Token.IDENTIFIER) {
            int value = wordsToHighlight.get(segment, start, end);
            if (value != -1) {
                tokenType = value;
            }
        }
        super.addToken(segment, start, end, tokenType, startOffset);
    }

    @Override
    public Token getTokenList(Segment text, int initialTokenType, int startOffset) {
        resetTokenList();

        char[] array = text.array;
        int offset = text.offset;
        int count = text.count;
        int end = offset + count;

        int newStartOffset = startOffset - offset;

        int currentTokenStart = offset;
        int currentTokenType = Token.NULL;

        // Handle continuation of multi-line comment
        if (initialTokenType == Token.COMMENT_MULTILINE) {
            currentTokenType = Token.COMMENT_MULTILINE;
            currentTokenStart = offset;
        }

        for (int i = offset; i < end; i++) {
            char c = array[i];

            switch (currentTokenType) {
                case Token.NULL:
                    currentTokenStart = i;
                    if (c == '/' && i + 1 < end) {
                        if (array[i + 1] == '/') {
                            currentTokenType = Token.COMMENT_EOL;
                            break;
                        } else if (array[i + 1] == '*') {
                            currentTokenType = Token.COMMENT_MULTILINE;
                            i++; // skip *
                            break;
                        }
                    }
                    if (c == '"') {
                        currentTokenType = Token.LITERAL_STRING_DOUBLE_QUOTE;
                    } else if (c == '\'') {
                        currentTokenType = Token.LITERAL_CHAR;
                    } else if (c == '#') {
                        // Preprocessor directive
                        currentTokenType = Token.PREPROCESSOR;
                    } else if (c == '.' && i + 1 < end && Character.isLetter(array[i + 1])) {
                        currentTokenType = Token.IDENTIFIER;
                    } else if (Character.isDigit(c) || (c == '-' && i + 1 < end && Character.isDigit(array[i + 1]))) {
                        currentTokenType = Token.LITERAL_NUMBER_DECIMAL_INT;
                    } else if (c == '0' && i + 1 < end && (array[i + 1] == 'x' || array[i + 1] == 'X')) {
                        currentTokenType = Token.LITERAL_NUMBER_HEXADECIMAL;
                        i++; // skip x
                    } else if (Character.isLetter(c) || c == '_' || c == '$') {
                        currentTokenType = Token.IDENTIFIER;
                    } else if (Character.isWhitespace(c)) {
                        currentTokenType = Token.WHITESPACE;
                    } else {
                        // Operators, punctuation
                        currentTokenType = Token.OPERATOR;
                        addToken(text, currentTokenStart, i, Token.OPERATOR, newStartOffset + currentTokenStart);
                        currentTokenType = Token.NULL;
                    }
                    break;

                case Token.COMMENT_EOL:
                    // Continues to end of line
                    break;

                case Token.COMMENT_MULTILINE:
                    if (c == '*' && i + 1 < end && array[i + 1] == '/') {
                        i++; // skip /
                        addToken(text, currentTokenStart, i, Token.COMMENT_MULTILINE, newStartOffset + currentTokenStart);
                        currentTokenType = Token.NULL;
                    }
                    break;

                case Token.LITERAL_STRING_DOUBLE_QUOTE:
                    if (c == '\\' && i + 1 < end) {
                        i++; // skip escape sequence
                    } else if (c == '"') {
                        addToken(text, currentTokenStart, i, Token.LITERAL_STRING_DOUBLE_QUOTE, newStartOffset + currentTokenStart);
                        currentTokenType = Token.NULL;
                    }
                    break;

                case Token.LITERAL_CHAR:
                    if (c == '\\' && i + 1 < end) {
                        i++; // skip escape
                    } else if (c == '\'') {
                        addToken(text, currentTokenStart, i, Token.LITERAL_CHAR, newStartOffset + currentTokenStart);
                        currentTokenType = Token.NULL;
                    }
                    break;

                case Token.PREPROCESSOR:
                    if (!Character.isLetter(c) && c != '_') {
                        addToken(text, currentTokenStart, i - 1, Token.PREPROCESSOR, newStartOffset + currentTokenStart);
                        currentTokenType = Token.NULL;
                        i--; // re-process this char
                    }
                    break;

                case Token.IDENTIFIER:
                    if (!Character.isLetterOrDigit(c) && c != '_' && c != '$' && c != '.' && c != '{' && c != '}') {
                        if (c == ':') {
                            // Label definition
                            addToken(text, currentTokenStart, i, Token.VARIABLE, newStartOffset + currentTokenStart);
                            currentTokenType = Token.NULL;
                        } else {
                            // Check if it's a register
                            String word = new String(array, currentTokenStart, i - currentTokenStart);
                            int tokenType2 = classifyWord(word);
                            addToken(text, currentTokenStart, i - 1, tokenType2, newStartOffset + currentTokenStart);
                            currentTokenType = Token.NULL;
                            i--; // re-process
                        }
                    }
                    break;

                case Token.WHITESPACE:
                    if (!Character.isWhitespace(c)) {
                        addToken(text, currentTokenStart, i - 1, Token.WHITESPACE, newStartOffset + currentTokenStart);
                        currentTokenType = Token.NULL;
                        i--; // re-process
                    }
                    break;

                case Token.LITERAL_NUMBER_DECIMAL_INT:
                    if (!Character.isDigit(c) && c != '.' && c != 'e' && c != 'E' && c != '+' && c != '-') {
                        if (c == 'x' || c == 'X') {
                            // Hex number starting with 0x
                            currentTokenType = Token.LITERAL_NUMBER_HEXADECIMAL;
                        } else {
                            addToken(text, currentTokenStart, i - 1, Token.LITERAL_NUMBER_DECIMAL_INT, newStartOffset + currentTokenStart);
                            currentTokenType = Token.NULL;
                            i--; // re-process
                        }
                    }
                    break;

                case Token.LITERAL_NUMBER_HEXADECIMAL:
                    if (!isHexChar(c)) {
                        addToken(text, currentTokenStart, i - 1, Token.LITERAL_NUMBER_HEXADECIMAL, newStartOffset + currentTokenStart);
                        currentTokenType = Token.NULL;
                        i--; // re-process
                    }
                    break;

                case Token.OPERATOR:
                    addToken(text, currentTokenStart, i - 1, Token.OPERATOR, newStartOffset + currentTokenStart);
                    currentTokenType = Token.NULL;
                    i--; // re-process
                    break;
            }
        }

        // End of line: finalize current token
        switch (currentTokenType) {
            case Token.NULL:
                addNullToken();
                break;
            case Token.COMMENT_MULTILINE:
                addToken(text, currentTokenStart, end - 1, Token.COMMENT_MULTILINE, newStartOffset + currentTokenStart);
                break;
            case Token.IDENTIFIER:
                String word = new String(array, currentTokenStart, end - currentTokenStart);
                int tokenType2 = classifyWord(word);
                addToken(text, currentTokenStart, end - 1, tokenType2, newStartOffset + currentTokenStart);
                addNullToken();
                break;
            default:
                addToken(text, currentTokenStart, end - 1, currentTokenType, newStartOffset + currentTokenStart);
                addNullToken();
                break;
        }

        return firstToken;
    }

    private int classifyWord(String word) {
        String lower = word.toLowerCase();
        if (INSTRUCTIONS.contains(lower)) return Token.RESERVED_WORD;
        if (lower.startsWith(".") && DIRECTIVES.contains(lower)) return Token.FUNCTION;
        if (CONDITIONS.contains(lower)) return Token.DATA_TYPE;
        // Register pattern: r0-r31, f0-f31, sp, sf, pc, sr
        if (lower.matches("[rf]\\d+|s[fp]|pc|sr")) return Token.RESERVED_WORD_2;
        return Token.IDENTIFIER;
    }

    private boolean isHexChar(char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    @Override
    public String[] getLineCommentStartAndEnd(int languageIndex) {
        return new String[]{"//", null};
    }

    @Override
    public boolean getMarkOccurrencesOfTokenType(int type) {
        return type == Token.IDENTIFIER || type == Token.VARIABLE;
    }

    @Override
    public boolean getShouldIndentNextLineAfter(Token token) {
        return false;
    }
}
