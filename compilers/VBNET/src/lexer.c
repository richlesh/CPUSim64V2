/*
 * lexer.c - VB.NET lexical analysis (vb.g4 section 2) and conditional
 * compilation (vb.g4 section 3).
 *
 * lex_file() turns a source file into tokens.  Each logical line ends with a
 * T_NEWLINE token; ':' is a separate T_COLON token.  Explicit line
 * continuation (" _" at the end of a line, optionally followed by a comment)
 * produces no token.  Implicit line continuation is applied afterwards by
 * join_lines(), for the cases that vb.g4 attaches to tokens or to binary
 * operators (a LineTerminator? after ',' '(' '{' '.' '=' ':=' and the
 * binary and compound assignment operators, 'Is', 'IsNot', 'Like', 'In',
 * before ')' and '}').  The context-dependent cases (around query
 * operators) are left to the parser.
 *
 * Keywords are case-insensitive; contextual keywords (From, Where, Strict,
 * ...) are identifiers that the parser recognizes by name.  Directive lines
 * (#Const, #If ... #End If, #Region, #ExternalSource, #ExternalChecksum,
 * #Disable/#Enable) are handled here and produce no tokens.
 */

const char *tok_kind_names[] = {
    "eof", "newline", "colon", "identifier", "integer", "float", "string",
    "char", "interp"
};

static const char *punct_spell[] = {
    "(", ")", "{", "}", "!", "#", ",", ".", "?", "?.", ":=",
    "&", "*", "+", "-", "/", "\\", "^",
    "<", "=", ">", "<=", ">=", "<>", "<<", ">>",
    "+=", "-=", "*=", "/=", "\\=", "^=", "&=", "<<=", ">>="
};

static const char *kw_spell[] = {
#define KW(x) #x,
#include "keywords.inc"
#undef KW
    ""
};

#define K_FIRST (P_LAST + 1)

const char *tok_spelling_of(int kind) {
    if (kind == T_COLON) return ":";
    if (kind == T_NEWLINE) return "end of line";
    if (kind == T_EOF) return "end of file";
    if (kind >= P_LPAREN && kind < P_LAST) return punct_spell[kind - P_LPAREN];
    if (kind >= K_FIRST && kind < K_LAST) return kw_spell[kind - K_FIRST];
    return tok_kind_names[kind];
}

static Name *name_rem;

/* ------------------------------------------------------------ characters */

static int is_newline(int c) {
    return c == 10 || c == 13 || c == 0x2028 || c == 0x2029;
}

static int is_ws(int c) {
    return c == ' ' || c == '\t' || c == 0xA0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200A) || c == 0x202F || c == 0x205F ||
           c == 0x3000 || c == 0xFEFF;
}

static int is_letter(int c) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return 1;
    if (c < 0xAA) return 0;
    if (c == 0xAA || c == 0xB5 || c == 0xBA) return 1;
    if (c >= 0xC0 && c <= 0x24F) return c != 0xD7 && c != 0xF7;
    if (c >= 0x250 && c <= 0x2C1) return 1;
    if (c >= 0x386 && c <= 0x3FF) return c != 0x387 && c != 0x3F6;
    if (c >= 0x400 && c <= 0x52F) return c < 0x482 || c > 0x489;
    if (c >= 0x531 && c <= 0x587) return c < 0x557 || c > 0x560;
    if (c >= 0x5D0 && c <= 0x5EA) return 1;
    if (c >= 0x620 && c <= 0x64A) return 1;
    if (c >= 0x904 && c <= 0x939) return 1;
    if (c >= 0xE01 && c <= 0xE30) return 1;
    if (c >= 0x1E00 && c <= 0x1FBC) return 1;
    if (c >= 0x3041 && c <= 0x30FF) return c != 0x30FB && (c < 0x3097 || c > 0x30A0);
    if (c >= 0x3400 && c <= 0x4DBF) return 1;
    if (c >= 0x4E00 && c <= 0x9FFF) return 1;
    if (c >= 0xAC00 && c <= 0xD7A3) return 1;
    if (c >= 0xF900 && c <= 0xFAFF) return 1;
    if ((c >= 0xFF21 && c <= 0xFF3A) || (c >= 0xFF41 && c <= 0xFF5A)) return 1;
    if (c >= 0x20000 && c <= 0x2FFFF) return 1;
    return 0;
}

static int is_digit(int c) { return c >= '0' && c <= '9'; }

static int is_combining(int c) {
    return (c >= 0x300 && c <= 0x36F) || (c >= 0x1AB0 && c <= 0x1AFF) ||
           (c >= 0x20D0 && c <= 0x20FF) || (c >= 0xFE20 && c <= 0xFE2F);
}

static int is_formatting(int c) {
    return c == 0xAD || (c >= 0x200B && c <= 0x200F) || (c >= 0x202A && c <= 0x202E) ||
           (c >= 0x2060 && c <= 0x2064);
}

static int is_connector(int c) {
    return c == '_' || c == 0x203F || c == 0x2040 || c == 0x2054 || c == 0xFE33 ||
           c == 0xFE34 || (c >= 0xFE4D && c <= 0xFE4F) || c == 0xFF3F;
}

static int is_idchar(int c) {
    return is_letter(c) || is_digit(c) || is_combining(c) || is_formatting(c) || is_connector(c);
}

static int is_dquote(int c) { return c == '"' || c == 0x201C || c == 0x201D; }
static int is_squote(int c) { return c == '\'' || c == 0x2018 || c == 0x2019; }
static int lower_ascii(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

/* ------------------------------------------------------------ lexer state */

/* conditional compilation values */
enum { CV_NOTHING, CV_BOOL, CV_INT, CV_DBL, CV_STR };
typedef struct {
    int k;
    i64 i;          /* CV_BOOL (0 or 1) and CV_INT */
    double d;
    const char *s;
} CCVal;

typedef struct CCConst {
    Name *name;
    CCVal val;
    struct CCConst *next;
} CCConst;

static CCConst *project_consts = 0;     /* --define and the predefined ones */

typedef struct {
    int parent_active;
    int active;
    int taken;          /* a branch was chosen */
    int seen_else;
    int line;
} CondFrame;

typedef struct {
    int file;
    const int *s;
    int pos, end;
    int line;
    int line_start;     /* position of the first character of the line */
    int col0;           /* column of position line_start (for sub-lexers) */
    TokVec *out;
    int sub;            /* an interpolation hole: no directives */
    CondFrame *conds;
    int nconds, cap_conds;
    int region_depth;
    int region_line;    /* line of the outermost open #Region */
    int extsrc_depth;
    int extsrc_line;
    CCConst *consts;    /* #Const of this file */
} Lexer;

static int cur(Lexer *L) { return L->pos < L->end ? L->s[L->pos] : 0; }
static int peek(Lexer *L, int k) { return L->pos + k < L->end ? L->s[L->pos + k] : 0; }
static int at_end(Lexer *L) { return L->pos >= L->end; }
static int col_of(Lexer *L) { return L->pos - L->line_start + L->col0; }

static void advance_newline(Lexer *L) {
    int c = cur(L);
    L->pos++;
    if (c == 13 && cur(L) == 10) L->pos++;
    L->line++;
    L->line_start = L->pos;
    L->col0 = 1;
}

static void tv_push(TokVec *tv, Token *t) {
    if (tv->n == tv->cap) {
        tv->cap = tv->cap ? tv->cap * 2 : 256;
        tv->v = xrealloc(tv->v, sizeof(Token) * tv->cap);
    }
    tv->v[tv->n++] = *t;
}

static TokVec *tv_new(void) {
    return xalloc(sizeof(TokVec));
}

static void lex_error(Lexer *L, const char *msg) {
    error_at(L->file, L->line, "%s", msg);
}

static void tok_begin(Lexer *L, Token *t, int space) {
    memset(t, 0, sizeof(Token));
    t->file = L->file;
    t->line = L->line;
    t->col = col_of(L);
    t->start = L->pos;
    t->flags = space ? TF_SPACE : 0;
}

static void skip_to_eol(Lexer *L) {
    while (!at_end(L) && !is_newline(cur(L))) L->pos++;
}

/* ------------------------------------------------------------ tokens */

static void scan_ident(Lexer *L, Token *t) {
    StrBuf sp, key;
    int escaped = 0, c;
    Name *name;
    sb_init(&sp);
    sb_init(&key);
    if (cur(L) == '[') {
        escaped = 1;
        L->pos++;
        c = cur(L);
        if (!(is_letter(c) || (is_connector(c) && is_idchar(peek(L, 1)))))
            lex_error(L, "Identifier expected.");
    }
    while (is_idchar(c = cur(L))) {
        sb_put_cp(&sp, c);
        sb_put_cp(&key, fold_cp(c));
        L->pos++;
    }
    if (escaped) {
        if (cur(L) != ']') lex_error(L, "Bracketed identifier is missing closing ']'.");
        L->pos++;
        t->flags |= TF_ESCAPED;
    }
    name = intern(key.s);
    free(key.s);
    t->kind = T_IDENT;
    t->name = name;
    t->text = sp.s;
    if (!escaped && name == name_rem) {     /* REM comment */
        skip_to_eol(L);
        t->kind = -1;
        return;
    }
    /* a type character directly after the name */
    c = cur(L);
    if (c == '%' || c == '&' || c == '@' || c == '!' || c == '#' || c == '$') {
        int nx = peek(L, 1), ok = 1;
        if (c == '!' && (is_letter(nx) || is_connector(nx) || nx == '[')) ok = 0;  /* a!b */
        if (c == '&' && (nx == '=' || is_idchar(nx) || is_dquote(nx))) ok = 0;    /* a&b, a&="x" */
        if (c == '$' && is_dquote(nx)) ok = 0;
        if (is_idchar(nx) && c != '&') ok = 0;
        if (ok) {
            t->tchar = c;
            L->pos++;
        }
    }
    if (!escaped && !t->tchar && name->kw) t->kind = name->kw;
}

/* digits of a base (with '_' separators between digits, a VB 15 addition) */
static int digit_val(int c, int base) {
    int v;
    if (c >= '0' && c <= '9') v = c - '0';
    else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
    else return -1;
    return v < base ? v : -1;
}

static void scan_number(Lexer *L, Token *t) {
    StrBuf txt;
    int base = 10, is_float = 0, ndig = 0, overflow = 0, c, d, suf = LS_NONE;
    u64 v = 0;
    sb_init(&txt);
    if (cur(L) == '&') {
        c = lower_ascii(peek(L, 1));
        base = c == 'h' ? 16 : c == 'o' ? 8 : 2;
        L->pos += 2;
        while ((d = digit_val(cur(L), base)) >= 0 ||
               (cur(L) == '_' && digit_val(peek(L, 1), base) >= 0)) {
            if (cur(L) == '_') { L->pos++; continue; }
            if (base == 16 && (v >> 60) != 0) overflow = 1;
            if (base == 8 && (v >> 61) != 0) overflow = 1;
            if (base == 2 && (v >> 63) != 0) overflow = 1;
            v = v * (u64)base + (u64)d;
            ndig++;
            L->pos++;
        }
        if (ndig == 0) lex_error(L, "Syntax error.");
    } else {
        while (is_digit(c = cur(L)) || (c == '_' && is_digit(peek(L, 1)) && ndig > 0)) {
            L->pos++;
            if (c == '_') continue;
            sb_putc(&txt, c);
            d = c - '0';
            if (v > (0xFFFFFFFFFFFFFFFFULL - (u64)d) / 10ULL) overflow = 1;
            v = v * 10ULL + (u64)d;
            ndig++;
        }
        if (cur(L) == '.' && is_digit(peek(L, 1))) {
            is_float = 1;
            sb_putc(&txt, '.');
            L->pos++;
            while (is_digit(c = cur(L)) || (c == '_' && is_digit(peek(L, 1)))) {
                L->pos++;
                if (c != '_') sb_putc(&txt, c);
            }
        }
        if (lower_ascii(cur(L)) == 'e' &&
            (is_digit(peek(L, 1)) || ((peek(L, 1) == '+' || peek(L, 1) == '-') && is_digit(peek(L, 2))))) {
            is_float = 1;
            sb_putc(&txt, 'e');
            L->pos++;
            if (cur(L) == '+' || cur(L) == '-') sb_putc(&txt, cur(L)), L->pos++;
            while (is_digit(c = cur(L))) {
                sb_putc(&txt, c);
                L->pos++;
            }
        }
    }
    /* suffix */
    c = lower_ascii(cur(L));
    d = lower_ascii(peek(L, 1));
    if (c == 's') suf = LS_S, L->pos++;
    else if (c == 'u' && d == 's') suf = LS_US, L->pos += 2;
    else if (c == 'u' && d == 'i') suf = LS_UI, L->pos += 2;
    else if (c == 'u' && d == 'l') suf = LS_UL, L->pos += 2;
    else if (c == 'i') suf = LS_I, L->pos++;
    else if (c == 'l') suf = LS_L, L->pos++;
    else if (c == 'f' && base == 10) suf = LS_F, L->pos++;
    else if (c == 'r' && base == 10) suf = LS_R, L->pos++;
    else if (c == 'd' && base == 10) suf = LS_D, L->pos++;
    else if (c == '%') suf = LS_I, L->pos++;
    else if (c == '&' && !(is_idchar(d) || d == '=' || is_dquote(d))) suf = LS_L, L->pos++;
    else if (c == '!' && base == 10) suf = LS_F, L->pos++;
    else if (c == '#' && base == 10) suf = LS_R, L->pos++;
    else if (c == '@' && base == 10) suf = LS_D, L->pos++;
    if (is_idchar(cur(L))) lex_error(L, "Syntax error.");
    if (suf == LS_F || suf == LS_R || suf == LS_D) is_float = 1;
    else if (is_float && suf != LS_NONE) lex_error(L, "Syntax error.");
    t->tchar = suf;
    t->base = base;
    if (is_float) {
        t->kind = T_FLOAT;
        t->dval = strtod(txt.s, 0);
    } else {
        if (overflow) lex_error(L, "Overflow.");
        t->kind = T_INT;
        t->ival = v;
    }
    free(txt.s);
}

static void scan_string(Lexer *L, Token *t) {
    StrBuf val;
    int n = 0, c, cp = 0, line = L->line;
    sb_init(&val);
    L->pos++;
    for (;;) {
        if (at_end(L)) {
            L->line = line;
            lex_error(L, "String constants must end with a double quote.");
        }
        c = cur(L);
        if (is_dquote(c)) {
            if (is_dquote(peek(L, 1))) {
                sb_put_cp(&val, '"');
                cp = '"';
                n++;
                L->pos += 2;
                continue;
            }
            L->pos++;
            break;
        }
        if (is_newline(c)) {            /* multi-line string (VB 14) */
            int st = L->pos, k;
            advance_newline(L);
            for (k = st; k < L->pos; k++) sb_put_cp(&val, L->s[k]), n++;
            continue;
        }
        sb_put_cp(&val, c);
        cp = c;
        n++;
        L->pos++;
    }
    t->kind = T_STRING;
    t->text = val.s;
    if (lower_ascii(cur(L)) == 'c' && !is_idchar(peek(L, 1))) {
        L->pos++;
        if (n != 1) lex_error(L, "Character constant must contain exactly one character.");
        t->kind = T_CHAR;
        t->ival = (u64)cp;
    }
}

static void skip_interp(Lexer *L);

/* an interpolation hole: up to the '}' that closes it */
static void skip_hole(Lexer *L) {
    int depth = 0, c, line = L->line;
    for (;;) {
        if (at_end(L)) {
            L->line = line;
            lex_error(L, "'}' expected.");
        }
        c = cur(L);
        if (is_newline(c)) { advance_newline(L); continue; }
        if (c == '$' && is_dquote(peek(L, 1))) { skip_interp(L); continue; }
        if (is_dquote(c)) {
            L->pos++;
            for (;;) {
                if (at_end(L) || is_newline(cur(L))) lex_error(L, "String constants must end with a double quote.");
                if (is_dquote(cur(L))) {
                    if (is_dquote(peek(L, 1))) { L->pos += 2; continue; }
                    L->pos++;
                    break;
                }
                L->pos++;
            }
            continue;
        }
        if (c == '(' || c == '[' || c == '{') depth++;
        else if (c == ')' || c == ']') depth--;
        else if (c == '}') {
            if (depth == 0) { L->pos++; return; }
            depth--;
        }
        L->pos++;
    }
}

/* $"..." from the '$' */
static void skip_interp(Lexer *L) {
    int c, line = L->line;
    L->pos += 2;
    for (;;) {
        if (at_end(L)) {
            L->line = line;
            lex_error(L, "String constants must end with a double quote.");
        }
        c = cur(L);
        if (is_dquote(c)) {
            if (is_dquote(peek(L, 1))) { L->pos += 2; continue; }
            L->pos++;
            return;
        }
        if (c == '{') {
            if (peek(L, 1) == '{') { L->pos += 2; continue; }
            L->pos++;
            skip_hole(L);
            continue;
        }
        if (c == '}') {
            if (peek(L, 1) == '}') { L->pos += 2; continue; }
            lex_error(L, "Unescaped '}' in an interpolated string; use '}}'.");
        }
        if (is_newline(c)) { advance_newline(L); continue; }
        L->pos++;
    }
}

/* punctuation: the longest match */
static int scan_punct(Lexer *L) {
    int c = cur(L), n = peek(L, 1), n2 = peek(L, 2);
    switch (c) {
    case '(': L->pos++; return P_LPAREN;
    case ')': L->pos++; return P_RPAREN;
    case '{': L->pos++; return P_LBRACE;
    case '}': L->pos++; return P_RBRACE;
    case '!': L->pos++; return P_BANG;
    case '#': L->pos++; return P_HASH;
    case ',': L->pos++; return P_COMMA;
    case '.': L->pos++; return P_DOT;
    case '?':
        if (n == '.' && !is_digit(n2)) { L->pos += 2; return P_QDOT; }
        L->pos++;
        return P_QUESTION;
    case ':':
        if (n == '=') { L->pos += 2; return P_COLONEQ; }
        L->pos++;
        return T_COLON;
    case '&': if (n == '=') { L->pos += 2; return P_AMPEQ; } L->pos++; return P_AMP;
    case '*': if (n == '=') { L->pos += 2; return P_STAREQ; } L->pos++; return P_STAR;
    case '+': if (n == '=') { L->pos += 2; return P_PLUSEQ; } L->pos++; return P_PLUS;
    case '-': if (n == '=') { L->pos += 2; return P_MINUSEQ; } L->pos++; return P_MINUS;
    case '/': if (n == '=') { L->pos += 2; return P_SLASHEQ; } L->pos++; return P_SLASH;
    case '\\': if (n == '=') { L->pos += 2; return P_BACKSLASHEQ; } L->pos++; return P_BACKSLASH;
    case '^': if (n == '=') { L->pos += 2; return P_CARETEQ; } L->pos++; return P_CARET;
    case '=': L->pos++; return P_EQ;
    case '<':
        if (n == '=') { L->pos += 2; return P_LE; }
        if (n == '>') { L->pos += 2; return P_NE; }
        if (n == '<') {
            if (n2 == '=') { L->pos += 3; return P_SHLEQ; }
            L->pos += 2;
            return P_SHL;
        }
        L->pos++;
        return P_LT;
    case '>':
        if (n == '=') { L->pos += 2; return P_GE; }
        if (n == '>') {
            if (n2 == '=') { L->pos += 3; return P_SHREQ; }
            L->pos += 2;
            return P_SHR;
        }
        L->pos++;
        return P_GT;
    }
    return -1;
}

/* scans one token at the current position (not white space, a line end or
   a comment); t->kind is -1 if nothing was produced (REM comment) */
static void scan_token(Lexer *L, Token *t, int space) {
    int c = cur(L), n = peek(L, 1), k;
    tok_begin(L, t, space);
    if (is_letter(c) || c == '[' || (is_connector(c) && is_idchar(n))) {
        scan_ident(L, t);
    } else if (is_digit(c) || (c == '.' && is_digit(n)) ||
               (c == '&' && (lower_ascii(n) == 'h' || lower_ascii(n) == 'o' || lower_ascii(n) == 'b') &&
                digit_val(peek(L, 2), lower_ascii(n) == 'h' ? 16 : lower_ascii(n) == 'o' ? 8 : 2) >= 0)) {
        scan_number(L, t);
    } else if (is_dquote(c)) {
        scan_string(L, t);
    } else if (c == '$' && is_dquote(n)) {
        skip_interp(L);
        t->kind = T_INTERP;
    } else if (c == '#' && (is_digit(n) || (is_ws(n) && is_digit(peek(L, 2))))) {
        lex_error(L, "Date literals are not supported.");
    } else {
        k = scan_punct(L);
        if (k < 0) {
            char msg[80];
            if (c < 0x80 && c >= 32) snprintf(msg, sizeof msg, "Character '%c' is not valid.", c);
            else snprintf(msg, sizeof msg, "Character U+%04X is not valid.", c);
            lex_error(L, msg);
        }
        t->kind = k;
    }
    t->end = L->pos;
}

/* ------------------------------------------------------------ #If expressions */

static TokVec *cc_tv;
static int cc_i;
static Lexer *cc_L;

static int cc_kind(void) { return cc_tv->v[cc_i].kind; }

static void cc_fail(void) {
    lex_error(cc_L, "Syntax error in conditional compilation expression.");
}

static int cc_bool(CCVal v) {
    switch (v.k) {
    case CV_NOTHING: return 0;
    case CV_BOOL: case CV_INT: return v.i != 0;
    case CV_DBL: return v.d != 0.0;
    }
    {
        Name *n = intern_word(v.s);
        if (n == intern_word("true")) return 1;
        if (n == intern_word("false")) return 0;
    }
    lex_error(cc_L, "Conversion from String to Boolean is not valid in a conditional compilation expression.");
    return 0;
}

static CCVal mk_int(i64 i) { CCVal v; v.k = CV_INT; v.i = i; v.d = 0; v.s = 0; return v; }
static CCVal mk_bool(int b) { CCVal v = mk_int(b ? 1 : 0); v.k = CV_BOOL; return v; }
static CCVal mk_dbl(double d) { CCVal v = mk_int(0); v.k = CV_DBL; v.d = d; return v; }
static CCVal mk_str(const char *s) { CCVal v = mk_int(0); v.k = CV_STR; v.s = s; return v; }

static i64 cc_int(CCVal v) {
    if (v.k == CV_BOOL) return v.i ? -1 : 0;        /* True is -1 */
    if (v.k == CV_DBL) return (i64)v.d;
    if (v.k == CV_STR) lex_error(cc_L, "String operand in an arithmetic conditional compilation expression.");
    return v.i;
}
static double cc_dbl(CCVal v) { return v.k == CV_DBL ? v.d : (double)cc_int(v); }

static CCVal cc_expr(void);
static CCVal cc_value_from_text(const char *text);

static CCVal cc_lookup(Name *n) {
    CCConst *c;
    for (c = cc_L->consts; c; c = c->next) if (c->name == n) return c->val;
    for (c = project_consts; c; c = c->next) if (c->name == n) return c->val;
    { CCVal v = mk_int(0); v.k = CV_NOTHING; return v; }
}

static void cc_expect(int k) {
    if (cc_kind() != k) cc_fail();
    cc_i++;
}

static CCVal cc_primary(void) {
    Token *t = &cc_tv->v[cc_i];
    CCVal v;
    switch (t->kind) {
    case T_INT: cc_i++; return mk_int((i64)t->ival);
    case T_FLOAT: cc_i++; return mk_dbl(t->dval);
    case T_STRING: cc_i++; return mk_str(t->text);
    case T_CHAR: cc_i++; return mk_str(t->text);
    case K_True: cc_i++; return mk_bool(1);
    case K_False: cc_i++; return mk_bool(0);
    case K_Nothing: cc_i++; v = mk_int(0); v.k = CV_NOTHING; return v;
    case T_IDENT: cc_i++; return cc_lookup(t->name);
    case P_LPAREN:
        cc_i++;
        v = cc_expr();
        cc_expect(P_RPAREN);
        return v;
    case K_If: {
        CCVal a, b, c;
        cc_i++;
        cc_expect(P_LPAREN);
        a = cc_expr();
        cc_expect(P_COMMA);
        b = cc_expr();
        if (cc_kind() == P_COMMA) {
            cc_i++;
            c = cc_expr();
            cc_expect(P_RPAREN);
            return cc_bool(a) ? b : c;
        }
        cc_expect(P_RPAREN);
        return a.k != CV_NOTHING ? a : b;
    }
    case K_CBool: case K_CInt: case K_CLng: case K_CShort: case K_CByte: case K_CSByte:
    case K_CUInt: case K_CULng: case K_CUShort: case K_CDbl: case K_CSng: case K_CStr: {
        int k = t->kind;
        cc_i++;
        cc_expect(P_LPAREN);
        v = cc_expr();
        cc_expect(P_RPAREN);
        if (k != K_CStr && v.k == CV_STR) {
            v = cc_value_from_text(v.s);
            if (v.k == CV_STR) lex_error(cc_L, "Conversion from String to a number is not valid in a conditional compilation expression.");
        }
        if (k == K_CBool) return mk_bool(cc_bool(v));
        if (k == K_CDbl || k == K_CSng) return mk_dbl(cc_dbl(v));
        if (k == K_CStr) {
            char buf[64];
            if (v.k == CV_STR) return v;
            if (v.k == CV_BOOL) return mk_str(v.i ? "True" : "False");
            if (v.k == CV_DBL) snprintf(buf, sizeof buf, "%.15g", v.d);
            else snprintf(buf, sizeof buf, "%lld", cc_int(v));
            return mk_str(xstrdup(buf));
        }
        return mk_int(cc_int(v));
    }
    }
    cc_fail();
    return mk_int(0);
}

static CCVal cc_unary(void);

static CCVal cc_pow(void) {
    CCVal a = cc_primary(), b;
    while (cc_kind() == P_CARET) {
        int neg = 0;
        cc_i++;
        while (cc_kind() == P_MINUS || cc_kind() == P_PLUS) {
            if (cc_kind() == P_MINUS) neg = !neg;
            cc_i++;
        }
        b = cc_primary();
        {
            double x = cc_dbl(a), y = cc_dbl(b), r = 1.0;
            i64 e = (i64)y, k;
            if (neg) y = -y, e = -e;
            if ((double)e != y) lex_error(cc_L, "Fractional powers are not supported in conditional compilation expressions.");
            for (k = 0; k < (e < 0 ? -e : e); k++) r *= x;
            if (e < 0) r = 1.0 / r;
            a = mk_dbl(r);
        }
    }
    return a;
}

static CCVal cc_unary(void) {
    if (cc_kind() == P_MINUS) {
        CCVal v;
        cc_i++;
        v = cc_unary();
        return v.k == CV_DBL ? mk_dbl(-v.d) : mk_int(-cc_int(v));
    }
    if (cc_kind() == P_PLUS) {
        cc_i++;
        return cc_unary();
    }
    return cc_pow();
}

static CCVal cc_arith(CCVal a, int op, CCVal b) {
    if (op == P_SLASH) {
        if (cc_dbl(b) == 0.0 && a.k != CV_DBL && b.k != CV_DBL)
            lex_error(cc_L, "Division by zero occurred while evaluating this expression.");
        return mk_dbl(cc_dbl(a) / cc_dbl(b));
    }
    if (a.k == CV_DBL || b.k == CV_DBL) {
        double x = cc_dbl(a), y = cc_dbl(b);
        switch (op) {
        case P_STAR: return mk_dbl(x * y);
        case P_PLUS: return mk_dbl(x + y);
        case P_MINUS: return mk_dbl(x - y);
        }
    }
    {
        i64 x = cc_int(a), y = cc_int(b);
        switch (op) {
        case P_STAR: return mk_int(x * y);
        case P_PLUS: return mk_int(x + y);
        case P_MINUS: return mk_int(x - y);
        case P_BACKSLASH: case K_Mod:
            if (y == 0) lex_error(cc_L, "Division by zero occurred while evaluating this expression.");
            return mk_int(op == K_Mod ? x % y : x / y);
        case P_SHL: return mk_int(x << (y & 63));
        case P_SHR: return mk_int(x >> (y & 63));
        }
    }
    return mk_int(0);
}

static CCVal cc_mul(void) {
    CCVal a = cc_unary();
    while (cc_kind() == P_STAR || cc_kind() == P_SLASH) {
        int op = cc_kind();
        cc_i++;
        a = cc_arith(a, op, cc_unary());
    }
    return a;
}

static CCVal cc_idiv(void) {
    CCVal a = cc_mul();
    while (cc_kind() == P_BACKSLASH) {
        cc_i++;
        a = cc_arith(a, P_BACKSLASH, cc_mul());
    }
    return a;
}

static CCVal cc_mod(void) {
    CCVal a = cc_idiv();
    while (cc_kind() == K_Mod) {
        cc_i++;
        a = cc_arith(a, K_Mod, cc_idiv());
    }
    return a;
}

static CCVal cc_add(void) {
    CCVal a = cc_mod();
    while (cc_kind() == P_PLUS || cc_kind() == P_MINUS) {
        int op = cc_kind();
        CCVal b;
        cc_i++;
        b = cc_mod();
        if (op == P_PLUS && a.k == CV_STR && b.k == CV_STR) {
            StrBuf s;
            sb_init(&s);
            sb_puts(&s, a.s);
            sb_puts(&s, b.s);
            a = mk_str(s.s);
        } else a = cc_arith(a, op, b);
    }
    return a;
}

static const char *cc_str(CCVal v) {
    char buf[64];
    if (v.k == CV_STR) return v.s;
    if (v.k == CV_NOTHING) return "";
    if (v.k == CV_BOOL) return v.i ? "True" : "False";
    if (v.k == CV_DBL) snprintf(buf, sizeof buf, "%.15g", v.d);
    else snprintf(buf, sizeof buf, "%lld", v.i);
    return xstrdup(buf);
}

static CCVal cc_concat(void) {
    CCVal a = cc_add();
    while (cc_kind() == P_AMP) {
        StrBuf s;
        CCVal b;
        cc_i++;
        b = cc_add();
        sb_init(&s);
        sb_puts(&s, cc_str(a));
        sb_puts(&s, cc_str(b));
        a = mk_str(s.s);
    }
    return a;
}

static CCVal cc_shift(void) {
    CCVal a = cc_concat();
    while (cc_kind() == P_SHL || cc_kind() == P_SHR) {
        int op = cc_kind();
        cc_i++;
        a = cc_arith(a, op, cc_concat());
    }
    return a;
}

static CCVal cc_cmp(void) {
    CCVal a = cc_shift();
    for (;;) {
        int op = cc_kind(), r;
        CCVal b;
        if (op != P_EQ && op != P_NE && op != P_LT && op != P_GT && op != P_LE && op != P_GE) return a;
        cc_i++;
        b = cc_shift();
        if (a.k == CV_STR || b.k == CV_STR) {
            r = strcmp(cc_str(a), cc_str(b));
        } else if (a.k == CV_DBL || b.k == CV_DBL) {
            double x = cc_dbl(a), y = cc_dbl(b);
            r = x < y ? -1 : x > y ? 1 : 0;
        } else {
            i64 x = cc_int(a), y = cc_int(b);
            r = x < y ? -1 : x > y ? 1 : 0;
        }
        switch (op) {
        case P_EQ: a = mk_bool(r == 0); break;
        case P_NE: a = mk_bool(r != 0); break;
        case P_LT: a = mk_bool(r < 0); break;
        case P_GT: a = mk_bool(r > 0); break;
        case P_LE: a = mk_bool(r <= 0); break;
        default: a = mk_bool(r >= 0); break;
        }
    }
}

static CCVal cc_not(void) {
    if (cc_kind() == K_Not) {
        CCVal v;
        cc_i++;
        v = cc_not();
        if (v.k == CV_BOOL || v.k == CV_NOTHING) return mk_bool(!cc_bool(v));
        return mk_int(~cc_int(v));
    }
    return cc_cmp();
}

static CCVal cc_logic(CCVal a, int op, CCVal b) {
    if (op == K_AndAlso) return mk_bool(cc_bool(a) && cc_bool(b));
    if (op == K_OrElse) return mk_bool(cc_bool(a) || cc_bool(b));
    if ((a.k == CV_BOOL || a.k == CV_NOTHING) && (b.k == CV_BOOL || b.k == CV_NOTHING)) {
        int x = cc_bool(a), y = cc_bool(b);
        return mk_bool(op == K_And ? (x && y) : op == K_Or ? (x || y) : (x != y));
    }
    {
        i64 x = cc_int(a), y = cc_int(b);
        return mk_int(op == K_And ? (x & y) : op == K_Or ? (x | y) : (x ^ y));
    }
}

static CCVal cc_and(void) {
    CCVal a = cc_not();
    while (cc_kind() == K_And || cc_kind() == K_AndAlso) {
        int op = cc_kind();
        cc_i++;
        a = cc_logic(a, op, cc_not());
    }
    return a;
}

static CCVal cc_or(void) {
    CCVal a = cc_and();
    while (cc_kind() == K_Or || cc_kind() == K_OrElse) {
        int op = cc_kind();
        cc_i++;
        a = cc_logic(a, op, cc_and());
    }
    return a;
}

static CCVal cc_expr(void) {
    CCVal a = cc_or();
    while (cc_kind() == K_Xor) {
        cc_i++;
        a = cc_logic(a, K_Xor, cc_or());
    }
    return a;
}

/* evaluates tokens [from, n) of tv, optionally followed by Then */
static CCVal cc_eval(Lexer *L, TokVec *tv, int from, int then_ok) {
    CCVal v;
    cc_tv = tv;
    cc_i = from;
    cc_L = L;
    if (cc_kind() == T_EOF) cc_fail();
    v = cc_expr();
    if (then_ok && cc_kind() == K_Then) cc_i++;
    if (cc_kind() != T_EOF) cc_fail();
    return v;
}

static CCVal cc_value_from_text(const char *text) {
    /* --define values: True/False, an integer, a number, else a string */
    char *end;
    i64 i;
    Name *n = intern_word(text);
    if (n == intern_word("true")) return mk_bool(1);
    if (n == intern_word("false")) return mk_bool(0);
    if (*text) {
        i = strtoll(text, &end, 10);
        if (*end == 0) return mk_int(i);
        {
            double d = strtod(text, &end);
            if (*end == 0) return mk_dbl(d);
        }
    }
    return mk_str(xstrdup(text));
}

/* ------------------------------------------------------------ directives */

static int cond_active(Lexer *L) {
    return L->nconds == 0 || L->conds[L->nconds - 1].active;
}

static int tok_is_word(Token *t, const char *w) {
    return t->kind == T_IDENT && !(t->flags & TF_ESCAPED) && t->name == intern_word(w);
}

/* tokens of the rest of the current physical line, ending with T_EOF */
static TokVec *line_tokens(Lexer *L) {
    TokVec *tv = tv_new();
    Token t;
    for (;;) {
        int space = 0;
        while (!at_end(L) && is_ws(cur(L))) L->pos++, space = 1;
        if (at_end(L) || is_newline(cur(L))) break;
        if (is_squote(cur(L))) {
            skip_to_eol(L);
            break;
        }
        scan_token(L, &t, space);
        if (t.kind >= 0) tv_push(tv, &t);
    }
    tok_begin(L, &t, 0);
    t.kind = T_EOF;
    tv_push(tv, &t);
    return tv;
}

/* a directive line, from the '#' */
static void directive(Lexer *L) {
    TokVec *tv;
    Token *d;
    int active = cond_active(L), line = L->line;
    L->pos++;
    tv = line_tokens(L);
    d = &tv->v[0];
    if (d->kind == K_If || d->kind == K_ElseIf || (d->kind == K_Else && tv->v[1].kind == K_If)) {
        int is_if = d->kind == K_If, from = d->kind == K_Else ? 2 : 1;
        if (is_if) {
            CondFrame f;
            f.parent_active = active;
            f.taken = active ? cc_bool(cc_eval(L, tv, from, 1)) : 0;
            f.active = f.taken;
            f.seen_else = 0;
            f.line = line;
            if (L->nconds == L->cap_conds) {
                L->cap_conds = L->cap_conds ? 2 * L->cap_conds : 8;
                L->conds = xrealloc(L->conds, sizeof(CondFrame) * L->cap_conds);
            }
            L->conds[L->nconds++] = f;
        } else {
            CondFrame *f;
            if (L->nconds == 0 || L->conds[L->nconds - 1].seen_else)
                lex_error(L, "'#ElseIf' must be preceded by a matching '#If' or '#ElseIf'.");
            f = &L->conds[L->nconds - 1];
            if (f->parent_active && !f->taken && cc_bool(cc_eval(L, tv, from, 1))) {
                f->active = 1;
                f->taken = 1;
            } else f->active = 0;
        }
    } else if (d->kind == K_Else) {
        CondFrame *f;
        if (L->nconds == 0 || L->conds[L->nconds - 1].seen_else)
            lex_error(L, "'#Else' must be preceded by a matching '#If' or '#ElseIf'.");
        if (tv->v[1].kind != T_EOF) lex_error(L, "End of statement expected.");
        f = &L->conds[L->nconds - 1];
        f->seen_else = 1;
        f->active = f->parent_active && !f->taken;
        f->taken = 1;
    } else if (d->kind == K_End && tv->v[1].kind == K_If) {
        if (L->nconds == 0) lex_error(L, "'#End If' must be preceded by a matching '#If'.");
        if (tv->v[2].kind != T_EOF) lex_error(L, "End of statement expected.");
        L->nconds--;
    } else if (!active) {
        /* other directives in a skipped section are ignored */
    } else if (d->kind == K_Const) {
        CCConst *c;
        if (tv->v[1].kind != T_IDENT) lex_error(L, "Identifier expected.");
        if (tv->v[2].kind != P_EQ) lex_error(L, "'=' expected.");
        c = xalloc(sizeof(CCConst));
        c->name = tv->v[1].name;
        c->val = cc_eval(L, tv, 3, 0);
        c->next = L->consts;
        L->consts = c;
    } else if (tok_is_word(d, "Region")) {
        if (tv->v[1].kind != T_STRING || tv->v[2].kind != T_EOF)
            lex_error(L, "'#Region' must be followed by a string constant.");
        if (L->region_depth++ == 0) L->region_line = line;
    } else if (d->kind == K_End && tok_is_word(&tv->v[1], "Region")) {
        if (L->region_depth == 0) lex_error(L, "'#End Region' must be preceded by a matching '#Region'.");
        L->region_depth--;
    } else if (tok_is_word(d, "ExternalSource")) {
        if (L->extsrc_depth++ == 0) L->extsrc_line = line;
    } else if (d->kind == K_End && tok_is_word(&tv->v[1], "ExternalSource")) {
        if (L->extsrc_depth == 0)
            lex_error(L, "'#End ExternalSource' must be preceded by a matching '#ExternalSource'.");
        L->extsrc_depth--;
    } else if (tok_is_word(d, "ExternalChecksum") || tok_is_word(d, "Disable") || tok_is_word(d, "Enable")) {
        /* accepted and ignored */
    } else {
        lex_error(L, "Unknown preprocessor directive.");
    }
    if (!at_end(L)) advance_newline(L);
}

/* ------------------------------------------------------------ main loop */

static void emit_simple(Lexer *L, int kind) {
    Token t;
    tok_begin(L, &t, 0);
    t.kind = kind;
    t.end = t.start;
    tv_push(L->out, &t);
}

static void lex_loop(Lexer *L) {
    int bol = 1;            /* at the beginning of a physical line */
    Token t;
    for (;;) {
        int space = bol;
        while (!at_end(L) && is_ws(cur(L))) L->pos++, space = 1;
        if (at_end(L)) break;
        if (is_newline(cur(L))) {
            if (cond_active(L)) emit_simple(L, T_NEWLINE);
            advance_newline(L);
            bol = 1;
            continue;
        }
        if (bol && !L->sub && cur(L) == '#') {
            directive(L);
            continue;
        }
        if (bol && !cond_active(L)) {
            skip_to_eol(L);
            continue;
        }
        bol = 0;
        if (is_squote(cur(L))) {
            skip_to_eol(L);
            continue;
        }
        if (is_connector(cur(L)) && !is_idchar(peek(L, 1))) {
            /* explicit line continuation: " _" [comment] line end */
            int line = L->line;
            L->pos++;
            while (!at_end(L) && is_ws(cur(L))) L->pos++;
            if (is_squote(cur(L))) skip_to_eol(L);
            if (at_end(L) || !is_newline(cur(L))) {
                L->line = line;
                lex_error(L, "Character '_' is not valid; a line continuation must be the last thing on the line.");
            }
            advance_newline(L);
            continue;
        }
        scan_token(L, &t, space);
        if (t.kind >= 0) tv_push(L->out, &t);
    }
}

/* implicit line continuation and removal of empty lines */
static int cont_after(int k) {
    switch (k) {
    case P_COMMA: case P_LPAREN: case P_LBRACE: case P_DOT: case P_QDOT:
    case P_EQ: case P_COLONEQ:
    case P_AMP: case P_STAR: case P_PLUS: case P_MINUS: case P_SLASH:
    case P_BACKSLASH: case P_CARET:
    case P_LT: case P_GT: case P_LE: case P_GE: case P_NE: case P_SHL: case P_SHR:
    case P_PLUSEQ: case P_MINUSEQ: case P_STAREQ: case P_SLASHEQ: case P_BACKSLASHEQ:
    case P_CARETEQ: case P_AMPEQ: case P_SHLEQ: case P_SHREQ:
    case K_Mod: case K_And: case K_Or: case K_Xor: case K_AndAlso: case K_OrElse:
    case K_Like: case K_Is: case K_IsNot: case K_In:
        return 1;
    }
    return 0;
}

static TokVec *join_lines(TokVec *in, int add_newline) {
    TokVec *out = tv_new();
    int i, j;
    for (i = 0; i < in->n; i++) {
        Token *t = &in->v[i];
        if (t->kind == T_NEWLINE) {
            int prev = out->n ? out->v[out->n - 1].kind : -1;
            if (prev < 0 || prev == T_NEWLINE || cont_after(prev)) continue;
            for (j = i + 1; j < in->n && in->v[j].kind == T_NEWLINE; j++) ;
            if (j < in->n && (in->v[j].kind == P_RPAREN || in->v[j].kind == P_RBRACE)) continue;
        }
        tv_push(out, t);
    }
    if (add_newline && out->n && out->v[out->n - 1].kind != T_NEWLINE) {
        Token t = out->v[out->n - 1];
        t.kind = T_NEWLINE;
        t.start = t.end;
        t.flags = 0;
        t.text = 0;
        t.name = 0;
        tv_push(out, &t);
    }
    return out;
}

void lex_define(const char *name, const char *value) {
    CCConst *c = xalloc(sizeof(CCConst));
    c->name = intern_word(name);
    c->val = cc_value_from_text(value);
    c->next = project_consts;
    project_consts = c;
}

void lex_init(void) {
    int i;
    for (i = 0; i < K_LAST - K_FIRST; i++) intern_word(kw_spell[i])->kw = K_FIRST + i;
    name_rem = intern_word("rem");
    /* predefined conditional compilation constants */
    lex_define("VBC_VER", "11.0");
    lex_define("TARGET", "exe");
    lex_define("CPUSIM64", "True");
}

static void lexer_init(Lexer *L, int file, int start, int end, int line, int col) {
    memset(L, 0, sizeof(Lexer));
    L->file = file;
    L->s = files[file].text;
    L->pos = start;
    L->end = end;
    L->line = line;
    L->line_start = start;
    L->col0 = col;
    L->out = tv_new();
}

TokVec *lex_file(int file) {
    Lexer L;
    TokVec *tv;
    Token t;
    lexer_init(&L, file, 0, files[file].len, 1, 1);
    lex_loop(&L);
    if (L.nconds) {
        L.line = L.conds[L.nconds - 1].line;
        lex_error(&L, "'#If' block must end with a matching '#End If'.");
    }
    if (L.region_depth) {
        L.line = L.region_line;
        lex_error(&L, "'#Region' statement must end with a matching '#End Region'.");
    }
    if (L.extsrc_depth) {
        L.line = L.extsrc_line;
        lex_error(&L, "'#ExternalSource' statement must end with a matching '#End ExternalSource'.");
    }
    tv = join_lines(L.out, 1);
    tok_begin(&L, &t, 0);
    t.kind = T_EOF;
    tv_push(tv, &t);
    return tv;
}

TokVec *lex_range(int file, int start, int end, int line, int col) {
    Lexer L;
    TokVec *tv;
    Token t;
    lexer_init(&L, file, start, end, line, col);
    L.sub = 1;
    lex_loop(&L);
    tv = join_lines(L.out, 0);
    tok_begin(&L, &t, 0);
    t.kind = T_EOF;
    tv_push(tv, &t);
    return tv;
}

/* ------------------------------------------------------------ --tokens */

static const char *suffix_name[] = { "", "S", "US", "I", "UI", "L", "UL", "F", "R", "D" };

static void dump_text(FILE *f, const char *s) {
    for (; *s; s++) {
        if (*s == '\n') fputs("\\n", f);
        else if (*s == '\r') fputs("\\r", f);
        else fputc(*s, f);
    }
}

void dump_tokens(FILE *f, TokVec *tv) {
    int i, k;
    for (i = 0; i < tv->n; i++) {
        Token *t = &tv->v[i];
        const int *s = files[t->file].text;
        fprintf(f, "%d:%d ", t->line, t->col);
        if (t->kind == T_EOF || t->kind == T_NEWLINE) {
            fprintf(f, "%s\n", tok_kind_names[t->kind]);
            continue;
        }
        if (t->kind >= K_FIRST) fputs("keyword", f);
        else if (t->kind >= P_LPAREN || t->kind == T_COLON) fputs("punct", f);
        else fputs(tok_kind_names[t->kind], f);
        fputc(' ', f);
        for (k = t->start; k < t->end; k++) {
            if (s[k] == 10) fputs("\\n", f);
            else if (s[k] == 13) fputs("\\r", f);
            else out_cp(f, s[k]);
        }
        switch (t->kind) {
        case T_IDENT:
            if (t->tchar) fprintf(f, " = %s type %c", t->name->key, (int)t->tchar);
            else if (t->flags & TF_ESCAPED) fprintf(f, " = %s escaped", t->name->key);
            break;
        case T_INT:
            fprintf(f, " = %llu", t->ival);
            if (t->tchar) fprintf(f, " %s", suffix_name[t->tchar]);
            break;
        case T_FLOAT:
            fprintf(f, " = %.17g", t->dval);
            if (t->tchar) fprintf(f, " %s", suffix_name[t->tchar]);
            break;
        case T_STRING:
            fputs(" = [", f);
            dump_text(f, t->text);
            fputc(']', f);
            break;
        case T_CHAR:
            fprintf(f, " = U+%04llX", t->ival);
            break;
        }
        fputc('\n', f);
    }
}
