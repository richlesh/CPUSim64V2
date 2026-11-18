/*
 * parse_pat.c - patterns (see parser.h; grammar rules `pattern`,
 * `patternNoTopAlt`, `patternWithoutRange`, `rangePattern`).
 *
 *   parse_pattern()          A | B (a leading `|` is allowed): N_P_OR for two or more
 *   parse_pattern_no_alt()   one alternative
 *
 * A lone identifier is a binding (N_P_IDENT) unless a `::`, `(`, `{` or `!`
 * follows, or a range operator (then it is a constant path ending or starting
 * a range).  Parentheses around a pattern leave no node; `(..)` and `(p,)` are
 * tuples.  `&&p` is two nested N_P_REF (the `&&` token is split).
 *
 * Syntax rejected: box patterns, `x @ p` inside a struct-pattern shorthand,
 * `..` followed by a path (only `..=b`, and `..lit` / `..-lit`, are half open
 * ranges at the start), and `...` as a prefix.
 */

static int pat_at_literal(void) {
    int k = cur()->kind;
    return k == TK_INT || k == TK_FLOAT || k == TK_CHAR || k == TK_BYTE ||
           k == TK_STR || k == TK_BSTR || k == TK_KW_true || k == TK_KW_false;
}

static int pat_at_range_op(void) {
    int k = cur()->kind;
    return k == TK_DOTDOTEQ || k == TK_DOTDOTDOT || k == TK_DOTDOT;
}

/* a literal, with an optional `-` (F_NEG) */
static Node *pat_lit(void) {
    Token *t = cur();
    Node *n = mk(N_P_LIT, t->line, t->col);
    if (accept(TK_MINUS)) {
        n->flags |= F_NEG;
        if (!at(TK_INT) && !at(TK_FLOAT)) perr(cur(), "expected a number after `-` in a pattern");
    }
    if (!pat_at_literal()) perr(cur(), "expected literal in pattern");
    n->a = parse_literal();
    return n;
}

static int pat_at_range_end(void) {
    return pat_at_literal() || at(TK_MINUS) || at_path_start();
}

static Node *pat_range_end(void) {
    Token *t;
    Node *n;
    if (pat_at_literal() || at(TK_MINUS)) return pat_lit();
    if (at_path_start()) {
        t = cur();
        n = mk(N_P_PATH, t->line, t->col);
        n->a = parse_path(PATH_EXPR);
        return n;
    }
    perr(cur(), "expected a literal or path at the end of a range pattern");
    return 0;
}

/* the cursor is at `..=`, `...` or `..` after the start lo */
static Node *pat_range_from(Node *lo) {
    Node *r = mk(N_P_RANGE, lo->line, lo->col);
    int k = cur()->kind;
    next();
    r->a = lo;
    if (k == TK_DOTDOT) {
        if (pat_at_range_end()) r->b = pat_range_end();
    } else {
        r->flags |= F_INCLUSIVE;
        r->b = pat_range_end();
    }
    return r;
}

/* [ref] [mut] name [@ pat] ; with shorthand != 0 no `@` is read */
static Node *pat_binding(int shorthand) {
    Token *t = cur();
    Node *n = mk(N_P_IDENT, t->line, t->col);
    if (accept(TK_KW_ref)) n->flags |= F_REF;
    if (accept(TK_KW_mut)) n->flags |= F_MUT;
    n->name = expect_ident("identifier in pattern");
    if (!shorthand && accept(TK_AT)) n->a = parse_pattern_no_alt();
    return n;
}

/* comma separated patterns up to the closing token (which is consumed) */
static void pat_list(Node *n, int close, const char *what, int *had_comma) {
    *had_comma = 0;
    while (!at(close)) {
        list_push(&n->list, parse_pattern());
        if (!accept(TK_COMMA)) break;
        *had_comma = 1;
    }
    expect(close, what);
}

static Node *pat_tuple(void) {
    Token *t = cur();
    Node *n = mk(N_P_TUPLE, t->line, t->col);
    int comma;
    next();
    pat_list(n, TK_RPAREN, "`)`", &comma);
    if (n->list.n == 1 && !comma && n->list.v[0]->kind != N_P_REST) return n->list.v[0];
    return n;
}

static Node *pat_slice(void) {
    Token *t = cur();
    Node *n = mk(N_P_SLICE, t->line, t->col);
    int comma;
    next();
    pat_list(n, TK_RBRACK, "`]`", &comma);
    return n;
}

/* after the path, the cursor is at `{` */
static Node *pat_struct(Node *path, int line, int col) {
    Node *n = mk(N_P_STRUCT, line, col);
    Node *f;
    Token *t;
    n->a = path;
    expect(TK_LBRACE, "`{`");
    while (!at(TK_RBRACE)) {
        if (at(TK_DOTDOT)) {
            next();
            n->flags |= F_REST;
            break;
        }
        t = cur();
        f = mk(N_P_FIELD, t->line, t->col);
        if (at(TK_POUND)) parse_outer_attrs(&f->attrs);
        t = cur();
        if (t->kind == TK_INT && peek(1)->kind == TK_COLON) {
            f->name = t->text;
            next();
            next();
            f->a = parse_pattern();
        } else if (t->kind == TK_IDENT && peek(1)->kind == TK_COLON) {
            f->name = t->text;
            next();
            next();
            f->a = parse_pattern();
        } else if (t->kind == TK_IDENT || t->kind == TK_KW_ref || t->kind == TK_KW_mut) {
            f->flags |= F_SHORTHAND;
            f->a = pat_binding(1);
            f->name = f->a->name;
        } else {
            perr(t, "expected field pattern");
        }
        list_push(&n->list, f);
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_RBRACE, "`}`");
    return n;
}

/* a pattern that starts with a path (not a lone identifier binding) */
static Node *pat_from_path(void) {
    Token *t = cur();
    int line = t->line, col = t->col;
    Token start = *t;
    Node *path = parse_path(PATH_EXPR);
    Node *n;
    if (at(TK_NOT)) {
        next();
        return parse_macro_call(path, &start);
    }
    if (at(TK_LPAREN)) {
        int comma;
        n = mk(N_P_TUPLE_STRUCT, line, col);
        n->a = path;
        next();
        pat_list(n, TK_RPAREN, "`)`", &comma);
        return n;
    }
    if (at(TK_LBRACE)) return pat_struct(path, line, col);
    n = mk(N_P_PATH, line, col);
    n->a = path;
    if (pat_at_range_op()) return pat_range_from(n);
    return n;
}

Node *parse_pattern_no_alt(void) {
    Token *t = cur();
    int k = t->kind;
    int line = t->line, col = t->col;
    Node *n;
    if (k == TK_UNDERSCORE) {
        next();
        return mk(N_P_WILD, line, col);
    }
    if (k == TK_DOTDOTEQ) {
        n = mk(N_P_RANGE, line, col);
        next();
        n->flags |= F_INCLUSIVE;
        n->b = pat_range_end();
        return n;
    }
    if (k == TK_DOTDOT) {
        next();
        if (pat_at_literal() || at(TK_MINUS)) {
            n = mk(N_P_RANGE, line, col);
            n->b = pat_range_end();
            return n;
        }
        return mk(N_P_REST, line, col);
    }
    if (k == TK_AND || k == TK_ANDAND) {
        n = mk(N_P_REF, line, col);
        accept_prefix(TK_AND);
        if (accept(TK_KW_mut)) n->flags |= F_MUT;
        n->a = parse_pattern_no_alt();
        return n;
    }
    if (k == TK_LPAREN) return pat_tuple();
    if (k == TK_LBRACK) return pat_slice();
    if (pat_at_literal() || k == TK_MINUS) {
        n = pat_lit();
        if (pat_at_range_op()) return pat_range_from(n);
        return n;
    }
    if (k == TK_KW_ref || k == TK_KW_mut) return pat_binding(0);
    if (k == TK_IDENT) {
        int k1 = peek(1)->kind;
        if (k1 == TK_PATHSEP || k1 == TK_LPAREN || k1 == TK_LBRACE || k1 == TK_NOT ||
            k1 == TK_DOTDOTEQ || k1 == TK_DOTDOTDOT || k1 == TK_DOTDOT)
            return pat_from_path();
        return pat_binding(0);
    }
    if (at_path_start()) return pat_from_path();
    perr(t, "expected pattern");
    return 0;
}

Node *parse_pattern(void) {
    Node *first, *alt = 0;
    accept(TK_OR);
    first = parse_pattern_no_alt();
    while (at(TK_OR)) {
        next();
        if (!alt) {
            alt = mk(N_P_OR, first->line, first->col);
            list_push(&alt->list, first);
        }
        list_push(&alt->list, parse_pattern_no_alt());
    }
    return alt ? alt : first;
}
