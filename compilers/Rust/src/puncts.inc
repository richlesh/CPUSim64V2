/*
 * puncts.inc - the Rust punctuation and delimiters.  P(ID, spelling) gives
 * the token kind TK_ID.  The lexer takes the longest match, so `>>`, `>>=`,
 * `&&`, `||`, `..=` and so on are single tokens; the parser splits them with
 * lex_split_punct() where a generic list or a closure needs the parts.
 * `_` is lexed with the identifiers.  Includers define P(id, s).
 */
P(LBRACE, "{") P(RBRACE, "}") P(LBRACK, "[") P(RBRACK, "]")
P(LPAREN, "(") P(RPAREN, ")") P(SEMI, ";") P(COMMA, ",")
P(DOT, ".") P(DOTDOT, "..") P(DOTDOTDOT, "...") P(DOTDOTEQ, "..=")
P(COLON, ":") P(PATHSEP, "::") P(RARROW, "->") P(FATARROW, "=>")
P(PLUS, "+") P(MINUS, "-") P(STAR, "*") P(SLASH, "/")
P(PERCENT, "%") P(CARET, "^") P(NOT, "!") P(AND, "&")
P(OR, "|") P(ANDAND, "&&") P(OROR, "||") P(SHL, "<<")
P(SHR, ">>") P(PLUSEQ, "+=") P(MINUSEQ, "-=") P(STAREQ, "*=")
P(SLASHEQ, "/=") P(PERCENTEQ, "%=") P(CARETEQ, "^=") P(ANDEQ, "&=")
P(OREQ, "|=") P(SHLEQ, "<<=") P(SHREQ, ">>=") P(EQ, "=")
P(EQEQ, "==") P(NE, "!=") P(GT, ">") P(LT, "<")
P(GE, ">=") P(LE, "<=") P(AT, "@") P(UNDERSCORE, "_")
P(POUND, "#") P(DOLLAR, "$") P(QUESTION, "?") P(TILDE, "~")
