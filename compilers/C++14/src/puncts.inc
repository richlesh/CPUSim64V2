/*
 * puncts.inc - the preprocessing operators and punctuators of C++14
 * ([lex.operators]).  P(ID, spelling).  The digraphs and the alternative
 * tokens (and, or, ...) are turned into these by the lexer.
 */
P(LBRACE, "{") P(RBRACE, "}") P(LBRACK, "[") P(RBRACK, "]")
P(LPAREN, "(") P(RPAREN, ")") P(SEMI, ";") P(COLON, ":")
P(ELLIPSIS, "...") P(QUESTION, "?") P(SCOPE, "::") P(DOT, ".")
P(DOTSTAR, ".*") P(PLUS, "+") P(MINUS, "-") P(STAR, "*")
P(SLASH, "/") P(PERCENT, "%") P(CARET, "^") P(AMP, "&")
P(PIPE, "|") P(TILDE, "~") P(NOT, "!") P(ASSIGN, "=")
P(LT, "<") P(GT, ">") P(ADDEQ, "+=") P(SUBEQ, "-=")
P(MULEQ, "*=") P(DIVEQ, "/=") P(MODEQ, "%=") P(XOREQ, "^=")
P(ANDEQ, "&=") P(OREQ, "|=") P(SHL, "<<") P(SHR, ">>")
P(SHLEQ, "<<=") P(SHREQ, ">>=") P(EQ, "==") P(NE, "!=")
P(LE, "<=") P(GE, ">=") P(ANDAND, "&&") P(OROR, "||")
P(INC, "++") P(DEC, "--") P(COMMA, ",") P(ARROWSTAR, "->*")
P(ARROW, "->") P(HASH, "#") P(HASHHASH, "##")
