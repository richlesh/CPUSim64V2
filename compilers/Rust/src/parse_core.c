/*
 * parse_core.c - the parser's token cursor (see parser.h).
 */

PState P;

/* Doc comments are attributes only where an outer or inner attribute may
 * stand.  To keep the cursor simple they stay in the token stream and the
 * cursor skips them; parse_outer_attrs()/parse_inner_attrs() read them with
 * p_doc_here() before skipping. */

static void p_skip_docs(void) {
    while (P.pos < P.n - 1 && P.toks[P.pos].kind == TK_DOC) P.pos++;
}

void p_begin(Token *toks, int n, int file) {
    P.toks = toks;
    P.n = n;
    P.pos = 0;
    P.file = file;
    ast_cur_file = file;
}

PState p_save(void) {
    return P;
}

void p_restore(PState s) {
    P = s;
    ast_cur_file = s.file;
}

/* the doc comment at the cursor, if any, without skipping it */
Token *p_doc_here(void) {
    if (P.pos < P.n && P.toks[P.pos].kind == TK_DOC) return &P.toks[P.pos];
    return 0;
}

/* advances over one doc comment (used by the attribute parsers) */
void p_doc_skip_one(void) {
    if (P.pos < P.n - 1 && P.toks[P.pos].kind == TK_DOC) P.pos++;
}

Token *cur(void) {
    p_skip_docs();
    return &P.toks[P.pos];
}

Token *peek(int k) {
    int i;
    p_skip_docs();
    i = P.pos;
    while (k > 0 && i < P.n - 1) {
        i++;
        while (i < P.n - 1 && P.toks[i].kind == TK_DOC) i++;
        k--;
    }
    return &P.toks[i];
}

int at(int kind) {
    return cur()->kind == kind;
}

int at2(int k0, int k1) {
    return cur()->kind == k0 && peek(1)->kind == k1;
}

int at_ident(void) {
    return cur()->kind == TK_IDENT;
}

Token *next(void) {
    Token *t = cur();
    if (P.pos < P.n - 1) P.pos++;
    return t;
}

int accept(int kind) {
    if (!at(kind)) return 0;
    next();
    return 1;
}

const char *tok_desc(Token *t) {
    static char buf[200];
    if (t->kind == TK_EOF) return "end of file";
    if (t->kind == TK_IDENT) {
        snprintf(buf, sizeof buf, "identifier `%s`", t->text ? t->text->s : "?");
        return buf;
    }
    if (t->kind == TK_INT || t->kind == TK_FLOAT) return "number";
    if (t->kind == TK_STR || t->kind == TK_BSTR) return "string literal";
    if (t->kind == TK_CHAR || t->kind == TK_BYTE) return "character literal";
    if (t->kind == TK_LIFETIME) return "lifetime";
    snprintf(buf, sizeof buf, "`%s`", tk_spelling(t->kind));
    return buf;
}

void perr(Token *t, const char *fmt, ...) {
    char buf[500];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    error_at(t->file, t->line, "%s", buf);
}

Token *expect(int kind, const char *what) {
    Token *t = cur();
    if (t->kind != kind) perr(t, "expected %s, found %s", what, tok_desc(t));
    return next();
}

Name *expect_ident(const char *what) {
    Token *t = cur();
    if (t->kind != TK_IDENT) perr(t, "expected %s, found %s", what, tok_desc(t));
    next();
    return t->text;
}

/* ---------------------------------------------------------------- joint punctuation */

int at_prefix(int kind) {
    Token *t = cur();
    const char *a, *b;
    if (t->kind == kind) return 1;
    if (!tk_is_punct(t->kind) || !tk_is_punct(kind)) return 0;
    a = tk_spelling(kind);
    b = tk_spelling(t->kind);
    return strlen(b) > strlen(a) && strncmp(a, b, strlen(a)) == 0;
}

int accept_prefix(int kind) {
    Token *t;
    Token rest;
    if (!at_prefix(kind)) return 0;
    t = cur();
    if (t->kind == kind) {
        next();
        return 1;
    }
    if (!lex_split_punct(t, (int)strlen(tk_spelling(kind)), &rest)) return 0;
    /* t now holds the prefix: consume it by overwriting it with the rest */
    *t = rest;
    return 1;
}

void expect_prefix(int kind, const char *what) {
    if (!accept_prefix(kind)) perr(cur(), "expected %s, found %s", what, tok_desc(cur()));
}
