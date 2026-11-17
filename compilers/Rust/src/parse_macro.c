/*
 * parse_macro.c - macro calls, macro_rules! definitions, and re-parsing of a
 * macro's tokens (see parser.h).
 *
 * A token tree is stored as a copy of the lexer's tokens between the matching
 * delimiters, without the delimiters and without doc comments (TK_DOC).  The
 * toks pointer of a stored tree is never null, even for an empty tree
 * (ntoks 0), so the dump prints `toks:0`.
 *
 * The reparse_* functions parse a COPY of the tokens (accept_prefix splits
 * joint punctuation in place) followed by a TK_EOF token, then restore the
 * cursor.  Anything left over is the error `unexpected token in macro`.
 */

static int mac_closer(int open) {
    if (open == TK_LPAREN) return TK_RPAREN;
    if (open == TK_LBRACK) return TK_RBRACK;
    return TK_RBRACE;
}

static long mac_delim_flag(int open) {
    if (open == TK_LPAREN) return F_DELIM_PAREN;
    if (open == TK_LBRACK) return F_DELIM_BRACKET;
    return F_DELIM_BRACE;
}

static int mac_at_open(int k) {
    return k == TK_LPAREN || k == TK_LBRACK || k == TK_LBRACE;
}

static int mac_at_close(int k) {
    return k == TK_RPAREN || k == TK_RBRACK || k == TK_RBRACE;
}

/* The cursor is at an opening delimiter.  Returns an N_MACRO_TT holding the
 * tokens up to the matching closer and leaves the cursor after the closer. */
static Node *mac_tt(const char *what) {
    Token *open = cur();
    int *stk;
    int sp = 0, cap = 16;
    long i, cnt = 0, j;
    Node *n;
    Token *t;
    long first;
    if (!mac_at_open(open->kind)) perr(open, "expected `(`, `[` or `{` after %s", what);
    n = mk(N_MACRO_TT, open->line, open->col);
    n->flags = mac_delim_flag(open->kind);
    stk = xalloc((size_t)cap * sizeof(int));
    stk[sp++] = mac_closer(open->kind);
    first = P.pos + 1;
    i = first;
    for (;;) {
        t = &P.toks[i];
        if (t->kind == TK_EOF) perr(open, "unclosed delimiter `%s`", tk_spelling(open->kind));
        if (t->kind != TK_DOC) {
            if (mac_at_open(t->kind)) {
                if (sp == cap) {
                    cap *= 2;
                    stk = xrealloc(stk, (size_t)cap * sizeof(int));
                }
                stk[sp++] = mac_closer(t->kind);
            } else if (mac_at_close(t->kind)) {
                if (t->kind != stk[sp - 1]) perr(t, "mismatched closing delimiter `%s`", tk_spelling(t->kind));
                sp--;
                if (sp == 0) break;
            }
            cnt++;
        }
        i++;
    }
    n->toks = ast_alloc(sizeof(Token) * (size_t)(cnt ? cnt : 1));
    n->ntoks = cnt;
    cnt = 0;
    for (j = first; j < i; j++) {
        if (P.toks[j].kind == TK_DOC) continue;
        n->toks[cnt++] = P.toks[j];
    }
    free(stk);
    P.pos = (int)i + 1;
    return n;
}

Node *parse_macro_call(Node *path, Token *start) {
    Node *tt = mac_tt("the macro name `!`");
    Node *n;
    if (start) n = mk(N_MACRO_CALL, start->line, start->col);
    else n = mk(N_MACRO_CALL, path->line, path->col);
    n->a = path;
    n->toks = tt->toks;
    n->ntoks = tt->ntoks;
    n->flags = tt->flags;
    return n;
}

Node *parse_macro_rules(void) {
    Token *t = cur();
    Node *n, *rule;
    int open, close;
    if (t->kind != TK_IDENT || strcmp(t->text->s, "macro_rules") != 0) perr(t, "expected `macro_rules`");
    n = mk(N_ITEM_MACRO_RULES, t->line, t->col);
    next();
    expect(TK_NOT, "`!` after `macro_rules`");
    n->name = expect_ident("macro name");
    open = cur()->kind;
    if (!mac_at_open(open)) perr(cur(), "expected `{`, `(` or `[` after the macro name");
    close = mac_closer(open);
    n->flags = mac_delim_flag(open);
    next();
    while (!at(close)) {
        t = cur();
        rule = mk(N_MACRO_RULE, t->line, t->col);
        rule->a = mac_tt("the start of a macro rule");
        expect(TK_FATARROW, "`=>` in a macro rule");
        rule->b = mac_tt("`=>` in a macro rule");
        list_push(&n->list, rule);
        if (!accept(TK_SEMI)) break;
    }
    if (!at(close)) perr(cur(), "expected `;` or `%s` after a macro rule", tk_spelling(close));
    next();
    if (open != TK_LBRACE) accept(TK_SEMI);
    return n;
}

/* ---------------------------------------------------------------- re-parsing */

/* switches the cursor to a copy of the tokens plus a TK_EOF; returns the old cursor */
static PState mac_begin(Token *toks, long ntoks) {
    PState s = p_save();
    Token *buf;
    Token eof;
    if (ntoks > 0) eof = toks[ntoks - 1];
    else eof = *cur();
    eof.kind = TK_EOF;
    eof.text = 0;
    eof.str = 0;
    eof.slen = 0;
    buf = xalloc((size_t)(ntoks + 1) * sizeof(Token));
    if (ntoks > 0) memcpy(buf, toks, (size_t)ntoks * sizeof(Token));
    buf[ntoks] = eof;
    p_begin(buf, (int)ntoks + 1, s.file);
    return s;
}

static void mac_end(PState s) {
    if (!at(TK_EOF)) perr(cur(), "unexpected token in macro");
    p_restore(s);
}

Node *reparse_expr(Token *toks, long ntoks) {
    PState s = mac_begin(toks, ntoks);
    Node *e = parse_expr();
    mac_end(s);
    return e;
}

void reparse_expr_list(Token *toks, long ntoks, NodeList *out) {
    PState s = mac_begin(toks, ntoks);
    while (!at(TK_EOF)) {
        list_push(out, parse_expr());
        if (!accept(TK_COMMA)) break;
    }
    mac_end(s);
}

Node *reparse_type(Token *toks, long ntoks) {
    PState s = mac_begin(toks, ntoks);
    Node *t = parse_type();
    mac_end(s);
    return t;
}

void reparse_items(Token *toks, long ntoks, NodeList *out) {
    PState s = mac_begin(toks, ntoks);
    Node *it;
    while (!at(TK_EOF)) {
        it = parse_item();
        if (!it) perr(cur(), "expected item in macro");
        list_push(out, it);
    }
    mac_end(s);
}

void reparse_stmts(Token *toks, long ntoks, NodeList *out) {
    PState s = mac_begin(toks, ntoks);
    Node *st;
    int before;
    while (!at(TK_EOF)) {
        before = P.pos;
        st = parse_stmt();
        if (P.pos == before) perr(cur(), "unexpected token in macro");
        list_push(out, st);
    }
    mac_end(s);
}
