/*
 * parse_type.c - types, paths, generic arguments and bounds (see parser.h).
 *
 * Helpers private to this file start with ty_.  Nothing here backtracks: the
 * joint-punctuation splitting of accept_prefix() changes the token array, so
 * every decision is made with one or two tokens of lookahead.
 */

/* ---------------------------------------------------------------- lifetimes */

Node *parse_lifetime(void) {
    Token *t = cur();
    Node *n;
    if (t->kind != TK_LIFETIME) perr(t, "expected lifetime, found `%s`", t->text ? t->text->s : tk_spelling(t->kind));
    next();
    n = mk(N_LIFETIME, t->line, t->col);
    n->name = t->text;
    return n;
}

/* ---------------------------------------------------------------- paths */

/* a token that can be a path segment name */
static int ty_seg_tok(Token *t) {
    return t->kind == TK_IDENT || t->kind == TK_KW_self || t->kind == TK_KW_Self ||
           t->kind == TK_KW_super || t->kind == TK_KW_crate;
}

/* the token k ahead starts a path segment (`$crate` is DOLLAR + crate) */
static int ty_seg_start(int k) {
    Token *t = peek(k);
    if (ty_seg_tok(t)) return 1;
    return t->kind == TK_DOLLAR && peek(k + 1)->kind == TK_KW_crate;
}

int at_path_start(void) {
    int k = cur()->kind;
    if (k == TK_PATHSEP || k == TK_LT || k == TK_SHL) return 1;
    return ty_seg_start(0);
}

/* `(A, B) -> C` after a segment name (cursor at `(`) */
static void ty_parens_sugar(Node *seg) {
    seg->flags |= F_PARENS;
    expect(TK_LPAREN, "`(`");
    while (!at(TK_RPAREN)) {
        list_push(&seg->list, parse_type());
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_RPAREN, "`)`");
    if (accept(TK_RARROW)) seg->a = parse_type_no_bounds();
}

static Node *ty_parse_seg(int mode) {
    Token *t = cur();
    Node *s = mk(N_PATH_SEG, t->line, t->col);
    if (t->kind == TK_DOLLAR) {
        next();
        if (!at(TK_KW_crate)) perr(cur(), "expected `crate` after `$`");
        next();
        s->name = name_intern_str("$crate");
    } else if (ty_seg_tok(t)) {
        next();
        s->name = t->text;
    } else {
        perr(t, "expected identifier in path, found `%s`", tk_spelling(t->kind));
    }
    if (mode == PATH_SIMPLE) return s;
    if (at(TK_PATHSEP) && (peek(1)->kind == TK_LT || peek(1)->kind == TK_SHL)) {
        next();
        s->flags |= F_TURBOFISH | F_ANGLE;
        parse_generic_args(&s->list);
    } else if (mode == PATH_TYPE) {
        if (at(TK_LT) || at(TK_SHL)) {
            s->flags |= F_ANGLE;
            parse_generic_args(&s->list);
        } else if (at(TK_LPAREN)) {
            ty_parens_sugar(s);
        }
    }
    return s;
}

Node *parse_path(int mode) {
    Token *t = cur();
    Node *p = mk(N_PATH, t->line, t->col);
    if (at(TK_LT) || at(TK_SHL)) {
        /* <T as Trait>::name  or  <T>::name */
        expect_prefix(TK_LT, "`<`");
        p->flags |= F_QUALIFIED;
        p->a = parse_type();
        if (accept(TK_KW_as)) p->b = parse_path(PATH_TYPE);
        expect_prefix(TK_GT, "`>`");
        expect(TK_PATHSEP, "`::` after qualified path");
    } else if (accept(TK_PATHSEP)) {
        p->flags |= F_GLOBAL;
    }
    for (;;) {
        list_push(&p->list, ty_parse_seg(mode));
        if (at(TK_PATHSEP) && ty_seg_start(1)) {
            next();
            continue;
        }
        break;
    }
    return p;
}

/* ---------------------------------------------------------------- generic arguments */

static int ty_is_lit_tok(int k) {
    return k == TK_INT || k == TK_FLOAT || k == TK_CHAR || k == TK_BYTE || k == TK_STR ||
           k == TK_BSTR || k == TK_KW_true || k == TK_KW_false;
}

/* a generic argument without the `Name =` / `Name:` forms */
static Node *ty_arg_value(void) {
    Token *t = cur();
    if (t->kind == TK_LIFETIME) return parse_lifetime();
    if (ty_is_lit_tok(t->kind)) return parse_literal();
    if (t->kind == TK_MINUS) {
        Node *n = mk(N_E_UNARY, t->line, t->col);
        next();
        if (!ty_is_lit_tok(cur()->kind)) perr(cur(), "expected literal after `-` in generic argument");
        n->op = OP_NEG;
        n->a = parse_literal();
        return n;
    }
    if (t->kind == TK_LBRACE) return parse_block();
    return parse_type();
}

static Node *ty_generic_arg(void) {
    Token *t = cur();
    if (t->kind == TK_IDENT) {
        int k = peek(1)->kind;
        if (k == TK_EQ) {
            Node *n = mk(N_ASSOC_BIND, t->line, t->col);
            next();
            next();
            n->name = t->text;
            n->a = ty_arg_value();
            return n;
        }
        if (k == TK_COLON) {
            Node *n = mk(N_ASSOC_BIND, t->line, t->col);
            next();
            next();
            n->name = t->text;
            n->b = parse_bounds();
            return n;
        }
    }
    return ty_arg_value();
}

void parse_generic_args(NodeList *out) {
    if (!(at(TK_LT) || at(TK_SHL))) perr(cur(), "expected `<`");
    expect_prefix(TK_LT, "`<`");
    while (!at_prefix(TK_GT)) {
        list_push(out, ty_generic_arg());
        if (!accept(TK_COMMA)) break;
    }
    expect_prefix(TK_GT, "`>`");
}

/* ---------------------------------------------------------------- bounds */

/* for<'a, 'b: 'a> : N_GENERICS of N_GEN_LIFETIME (cursor at `for`) */
static Node *ty_for_generics(void) {
    Token *t = cur();
    Node *g = mk(N_GENERICS, t->line, t->col);
    expect(TK_KW_for, "`for`");
    if (!at(TK_LT)) perr(cur(), "expected `<` after `for`");
    next();
    while (!at_prefix(TK_GT)) {
        Token *lt = cur();
        Node *l = mk(N_GEN_LIFETIME, lt->line, lt->col);
        Node *lf = parse_lifetime();
        l->name = lf->name;
        if (accept(TK_COLON)) {
            Node *bs = mk(N_BOUNDS, cur()->line, cur()->col);
            while (at(TK_LIFETIME)) {
                Node *bl = mk(N_BOUND_LIFETIME, cur()->line, cur()->col);
                bl->name = cur()->text;
                next();
                list_push(&bs->list, bl);
                if (!accept(TK_PLUS)) break;
            }
            l->b = bs;
        }
        list_push(&g->list, l);
        if (!accept(TK_COMMA)) break;
    }
    expect_prefix(TK_GT, "`>`");
    return g;
}

static int ty_bound_start(void) {
    int k = cur()->kind;
    if (k == TK_LIFETIME || k == TK_QUESTION || k == TK_KW_for || k == TK_LPAREN || k == TK_PATHSEP)
        return 1;
    return ty_seg_start(0);
}

/* one bound: 'a, ?Sized, for<'a> Trait, (Trait), path */
static Node *ty_one_bound(void) {
    Token *t = cur();
    Node *n;
    if (t->kind == TK_LIFETIME) {
        n = mk(N_BOUND_LIFETIME, t->line, t->col);
        n->name = t->text;
        next();
        return n;
    }
    if (t->kind == TK_LPAREN) {
        next();
        n = ty_one_bound();
        expect(TK_RPAREN, "`)`");
        n->flags |= F_PAREN;
        return n;
    }
    n = mk(N_BOUND_TRAIT, t->line, t->col);
    if (accept(TK_QUESTION)) n->flags |= F_MAYBE;
    if (at(TK_KW_for)) n->b = ty_for_generics();
    if (!at_path_start() || at(TK_LT) || at(TK_SHL)) perr(cur(), "expected trait bound");
    n->a = parse_path(PATH_TYPE);
    return n;
}

/* bounds separated by `+` into b.  have_first: the first bound is already in
 * b.  single: stop after one bound (no `+`). */
static void ty_bounds_loop(Node *b, int single, int have_first) {
    for (;;) {
        if (have_first) {
            have_first = 0;
        } else {
            if (!ty_bound_start()) break;
            list_push(&b->list, ty_one_bound());
        }
        if (single || !accept(TK_PLUS)) break;
    }
}

Node *parse_bounds(void) {
    Node *b = mk(N_BOUNDS, cur()->line, cur()->col);
    ty_bounds_loop(b, 0, 0);
    return b;
}

/* ---------------------------------------------------------------- types */

static Node *ty_fn_type(Token *start, Node *gen) {
    Node *n = mk(N_T_FN, start->line, start->col);
    n->c = gen;
    if (accept(TK_KW_unsafe)) n->flags |= F_UNSAFE;
    if (accept(TK_KW_extern)) {
        n->flags |= F_EXTERN;
        if (at(TK_STR)) {
            Token *s = next();
            node_set_str(n, s->str, s->slen);
        }
    }
    expect(TK_KW_fn, "`fn`");
    expect(TK_LPAREN, "`(`");
    while (!at(TK_RPAREN)) {
        Token *pt = cur();
        Node *p;
        if (at(TK_DOTDOTDOT)) {
            next();
            n->flags |= F_VARIADIC;
            break;
        }
        p = mk(N_PARAM, pt->line, pt->col);
        parse_outer_attrs(&p->attrs);
        pt = cur();
        if ((pt->kind == TK_IDENT || pt->kind == TK_UNDERSCORE) && peek(1)->kind == TK_COLON) {
            if (pt->kind == TK_IDENT) {
                p->a = mk(N_P_IDENT, pt->line, pt->col);
                p->a->name = pt->text;
            } else {
                p->a = mk(N_P_WILD, pt->line, pt->col);
            }
            next();
            next();
            if (at(TK_DOTDOTDOT)) {
                next();
                n->flags |= F_VARIADIC;
                break;
            }
        }
        p->b = parse_type();
        list_push(&n->list, p);
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_RPAREN, "`)`");
    if (accept(TK_RARROW)) n->b = parse_type_no_bounds();
    return n;
}

static Node *ty_ref_type(void) {
    Token *t = cur();
    Node *n = mk(N_T_REF, t->line, t->col);
    accept_prefix(TK_AND);          /* one `&` of `&&` */
    if (at(TK_LIFETIME)) n->b = parse_lifetime();
    if (accept(TK_KW_mut)) n->flags |= F_MUT;
    n->a = parse_type_no_bounds();
    if (at(TK_PLUS) && (n->a->kind == N_T_DYN || n->a->kind == N_T_IMPL))
        perr(cur(), "ambiguous `+` in a type; add parentheses around the bounds");
    return n;
}

/* a trait object / impl type with its bounds; the keyword is at the cursor */
static Node *ty_bounds_type(int kind, int allow_bounds) {
    Token *t = cur();
    Node *n = mk(kind, t->line, t->col);
    Node *b;
    next();
    if (kind == N_T_DYN) n->flags |= F_DYN;
    b = mk(N_BOUNDS, cur()->line, cur()->col);
    ty_bounds_loop(b, !allow_bounds, 0);
    if (b->list.n == 0) perr(cur(), "expected a bound after `%s`", tk_spelling(t->kind));
    n->a = b;
    return n;
}

/* a bare trait object `Trait + Other` starting with an already built first bound */
static Node *ty_bare_trait(Token *start, Node *first, int allow_bounds) {
    Node *n = mk(N_T_DYN, start->line, start->col);
    Node *b = mk(N_BOUNDS, start->line, start->col);
    list_push(&b->list, first);
    if (allow_bounds && accept(TK_PLUS)) ty_bounds_loop(b, 0, 0);
    n->a = b;
    return n;
}

static Node *ty_parse(int allow_bounds) {
    Token *t = cur();
    Node *n;
    switch (t->kind) {
    case TK_NOT:
        next();
        return mk(N_T_NEVER, t->line, t->col);
    case TK_UNDERSCORE:
        next();
        return mk(N_T_INFER, t->line, t->col);
    case TK_AND:
    case TK_ANDAND:
        return ty_ref_type();
    case TK_STAR:
        n = mk(N_T_PTR, t->line, t->col);
        next();
        if (accept(TK_KW_mut)) n->flags |= F_MUT;
        else if (!accept(TK_KW_const)) perr(cur(), "expected `const` or `mut` after `*` in a pointer type");
        n->a = parse_type_no_bounds();
        return n;
    case TK_LBRACK:
        next();
        n = mk(N_T_SLICE, t->line, t->col);
        n->a = parse_type();
        if (accept(TK_SEMI)) {
            n->kind = N_T_ARRAY;
            n->b = parse_expr();
        }
        expect(TK_RBRACK, "`]`");
        return n;
    case TK_LPAREN:
        next();
        if (accept(TK_RPAREN)) return mk(N_T_TUPLE, t->line, t->col);
        {
            Node *first = parse_type();
            if (accept(TK_RPAREN)) return first;      /* (T): no node */
            n = mk(N_T_TUPLE, t->line, t->col);
            list_push(&n->list, first);
            while (accept(TK_COMMA)) {
                if (at(TK_RPAREN)) break;
                list_push(&n->list, parse_type());
            }
            expect(TK_RPAREN, "`)`");
            return n;
        }
    case TK_KW_fn:
    case TK_KW_unsafe:
    case TK_KW_extern:
        return ty_fn_type(t, 0);
    case TK_KW_dyn:
        return ty_bounds_type(N_T_DYN, allow_bounds);
    case TK_KW_impl:
        return ty_bounds_type(N_T_IMPL, allow_bounds);
    case TK_KW_for: {
        Node *gen = ty_for_generics();
        Node *bt;
        if (at(TK_KW_fn) || at(TK_KW_unsafe) || at(TK_KW_extern)) return ty_fn_type(t, gen);
        bt = mk(N_BOUND_TRAIT, t->line, t->col);
        bt->b = gen;
        if (!at_path_start() || at(TK_LT) || at(TK_SHL)) perr(cur(), "expected a trait after `for<...>`");
        bt->a = parse_path(PATH_TYPE);
        return ty_bare_trait(t, bt, allow_bounds);
    }
    default:
        break;
    }
    if (at_path_start()) {
        Node *path = parse_path(PATH_TYPE);
        if (at(TK_NOT) && !(path->flags & F_QUALIFIED)) {
            next();
            return parse_macro_call(path, t);
        }
        n = mk(N_T_PATH, t->line, t->col);
        n->a = path;
        if (allow_bounds && at(TK_PLUS) && !(path->flags & F_QUALIFIED)) {
            Node *bt = mk(N_BOUND_TRAIT, t->line, t->col);
            bt->a = path;
            return ty_bare_trait(t, bt, 1);
        }
        return n;
    }
    perr(t, "expected type, found %s", tok_desc(t));
    return 0;
}

Node *parse_type(void) {
    return ty_parse(1);
}

Node *parse_type_no_bounds(void) {
    return ty_parse(0);
}
