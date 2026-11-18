/*
 * parse_item.c - crate, items, attributes, visibility, generics, where
 * clauses and function signatures (see parser.h and nodes.inc).
 *
 * Syntax that is deliberately rejected with an error:
 *   - unnamed fn parameters (`fn f(i32);` in a trait, 2015 style)
 *   - trait aliases (`trait A = B;`), `default fn`, `safe fn`, `impl const`
 *   - generic const items (`const X<T>: ..`), `extern "C" { ... }` without a body
 *   - a stray `;` between items
 */

/* from parse_core.c */
Token *p_doc_here(void);
void p_doc_skip_one(void);

/* ---------------------------------------------------------------- small helpers */

static void it_nl_init(NodeList *l) {
    l->v = 0;
    l->n = 0;
    l->cap = 0;
}

static int it_is_word(Token *t, const char *w) {
    return t->kind == TK_IDENT && !t->raw && t->text == name_intern_str(w);
}

static Node *it_mk_at(int kind, Token *t) {
    return mk(kind, t->line, t->col);
}

/* a path segment node for an identifier-like token */
static Node *it_seg(Token *t) {
    Node *s = it_mk_at(N_PATH_SEG, t);
    s->name = t->text;
    return s;
}

/* Is the next non-doc token of this kind?  Unlike at() this does not move the
 * cursor past doc comments, which parse_outer_attrs() must still see. */
static int it_at_close(int kind) {
    int j = P.pos;
    while (j < P.n - 1 && P.toks[j].kind == TK_DOC) j++;
    return P.toks[j].kind == kind;
}

/* ---------------------------------------------------------------- token trees */

/* The cursor is at an opening delimiter: stores the tokens up to the matching
 * closer in n->toks and sets the DELIM flag, then moves past the closer. */
static void it_copy_tt(Node *n) {
    Token *open = cur();
    long depth = 1;
    long cnt = 0;
    int j, k, start;
    if (open->kind == TK_LPAREN) n->flags |= F_DELIM_PAREN;
    else if (open->kind == TK_LBRACK) n->flags |= F_DELIM_BRACKET;
    else n->flags |= F_DELIM_BRACE;
    next();
    start = P.pos;
    j = start;
    for (;;) {
        k = P.toks[j].kind;
        if (k == TK_EOF) perr(open, "unclosed delimiter");
        if (k != TK_DOC) {
            if (k == TK_LPAREN || k == TK_LBRACK || k == TK_LBRACE) depth++;
            else if (k == TK_RPAREN || k == TK_RBRACK || k == TK_RBRACE) {
                depth--;
                if (depth == 0) break;
            }
        }
        j++;
    }
    for (k = start; k < j; k++)
        if (P.toks[k].kind != TK_DOC) cnt++;
    if (cnt > 0) {
        long m = 0;
        n->toks = (Token *)ast_alloc(sizeof(Token) * cnt);
        for (k = start; k < j; k++)
            if (P.toks[k].kind != TK_DOC) n->toks[m++] = P.toks[k];
        n->ntoks = cnt;
    }
    P.pos = j + 1;
}

/* ---------------------------------------------------------------- attributes */

static Node *it_doc_attr(Token *d) {
    Node *a = it_mk_at(N_ATTR, d);
    a->name = name_intern_str("doc");
    node_set_str(a, d->str, d->slen);
    a->flags = F_DOC | F_EQ;
    if (d->inner) a->flags |= F_INNER;
    return a;
}

/* the cursor is at `#` */
static Node *it_attr(int inner) {
    Token *st = cur();
    Node *a = it_mk_at(N_ATTR, st);
    StrBuf sb;
    char *s;
    next();
    if (inner) {
        expect(TK_NOT, "`!`");
        a->flags |= F_INNER;
    }
    expect(TK_LBRACK, "`[`");
    sb_init(&sb);
    for (;;) {
        Token *t = cur();
        if (t->kind != TK_IDENT && !tk_is_kw(t->kind)) perr(t, "expected attribute name");
        sb_puts(&sb, t->text->s);
        next();
        if (at(TK_PATHSEP)) {
            sb_puts(&sb, "::");
            next();
        } else
            break;
    }
    s = sb_take(&sb);
    a->name = name_intern_str(s);
    if (at(TK_LPAREN) || at(TK_LBRACK) || at(TK_LBRACE)) {
        it_copy_tt(a);
    } else if (at(TK_EQ)) {
        next();
        a->flags |= F_EQ;
        a->a = parse_expr();
    }
    expect(TK_RBRACK, "`]`");
    return a;
}

void parse_outer_attrs(NodeList *out) {
    for (;;) {
        Token *d = p_doc_here();
        if (d) {
            if (!d->inner) list_push(out, it_doc_attr(d));
            p_doc_skip_one();
            continue;
        }
        if (at(TK_POUND) && peek(1)->kind == TK_LBRACK) list_push(out, it_attr(0));
        else break;
    }
}

void parse_inner_attrs(NodeList *out) {
    for (;;) {
        Token *d = p_doc_here();
        if (d) {
            if (!d->inner) break;
            list_push(out, it_doc_attr(d));
            p_doc_skip_one();
            continue;
        }
        if (at(TK_POUND) && peek(1)->kind == TK_NOT && peek(2)->kind == TK_LBRACK)
            list_push(out, it_attr(1));
        else
            break;
    }
}

/* ---------------------------------------------------------------- visibility */

/* number of tokens of the visibility at the cursor (0 if none) */
static int it_vis_len(void) {
    int k;
    if (peek(0)->kind != TK_KW_pub) return 0;
    if (peek(1)->kind == TK_LPAREN) {
        k = peek(2)->kind;
        if ((k == TK_KW_crate || k == TK_KW_super || k == TK_KW_self) && peek(3)->kind == TK_RPAREN)
            return 4;
        if (k == TK_KW_in) {
            int j = 3;
            while (peek(j)->kind != TK_RPAREN && peek(j)->kind != TK_EOF) j++;
            return j + 1;
        }
    }
    return 1;
}

Node *parse_vis(void) {
    Token *st;
    Node *v;
    int k;
    if (!at(TK_KW_pub)) return 0;
    st = next();
    v = it_mk_at(N_VIS, st);
    if (at(TK_LPAREN)) {
        k = peek(1)->kind;
        if ((k == TK_KW_crate || k == TK_KW_super || k == TK_KW_self) && peek(2)->kind == TK_RPAREN) {
            next();
            next();
            next();
            if (k == TK_KW_crate) v->flags |= F_VIS_CRATE;
            else if (k == TK_KW_super) v->flags |= F_VIS_SUPER;
            else v->flags |= F_VIS_SELF;
        } else if (k == TK_KW_in) {
            next();
            next();
            v->flags |= F_VIS_IN;
            v->a = parse_path(PATH_SIMPLE);
            expect(TK_RPAREN, "`)`");
        }
    }
    return v;
}

/* ---------------------------------------------------------------- generics */

static int it_at_bound_start(void) {
    int k = cur()->kind;
    if (k == TK_LIFETIME || k == TK_QUESTION || k == TK_KW_for || k == TK_LPAREN) return 1;
    return at_path_start();
}

static Node *it_bounds_opt(void) {
    if (it_at_bound_start()) return parse_bounds();
    return it_mk_at(N_BOUNDS, cur());
}

/* 'a + 'b : N_BOUNDS of N_BOUND_LIFETIME (possibly empty) */
static Node *it_lifetime_bounds(void) {
    Node *b = it_mk_at(N_BOUNDS, cur());
    while (at(TK_LIFETIME)) {
        Token *t = next();
        Node *l = it_mk_at(N_BOUND_LIFETIME, t);
        l->name = t->text;
        list_push(&b->list, l);
        if (!accept(TK_PLUS)) break;
    }
    return b;
}

/* a const generic argument default: literal, -literal, block or a path */
static Node *it_const_default(void) {
    Token *st = cur();
    int k = st->kind;
    if (k == TK_LBRACE) return parse_block();
    if (k == TK_MINUS) {
        Node *u = it_mk_at(N_E_UNARY, st);
        next();
        u->op = OP_NEG;
        u->a = parse_literal();
        return u;
    }
    if (k == TK_INT || k == TK_FLOAT || k == TK_CHAR || k == TK_BYTE || k == TK_STR ||
        k == TK_BSTR || k == TK_KW_true || k == TK_KW_false)
        return parse_literal();
    {
        Node *e = it_mk_at(N_E_PATH, st);
        e->a = parse_path(PATH_EXPR);
        return e;
    }
}

Node *parse_generics(void) {
    Node *g;
    if (!at(TK_LT)) return 0;
    g = it_mk_at(N_GENERICS, cur());
    next();
    while (!at_prefix(TK_GT)) {
        NodeList pa;
        Token *st;
        Node *p;
        it_nl_init(&pa);
        parse_outer_attrs(&pa);
        st = cur();
        if (st->kind == TK_LIFETIME) {
            next();
            p = it_mk_at(N_GEN_LIFETIME, st);
            p->name = st->text;
            if (accept(TK_COLON)) p->b = it_lifetime_bounds();
        } else if (st->kind == TK_KW_const) {
            next();
            p = it_mk_at(N_GEN_CONST, st);
            p->name = expect_ident("const parameter name");
            expect(TK_COLON, "`:`");
            p->a = parse_type();
            if (accept(TK_EQ)) p->b = it_const_default();
        } else {
            p = it_mk_at(N_GEN_TYPE, st);
            p->name = expect_ident("generic parameter");
            if (accept(TK_COLON)) p->b = it_bounds_opt();
            if (accept(TK_EQ)) p->a = parse_type();
        }
        p->attrs = pa;
        list_push(&g->list, p);
        if (!accept(TK_COMMA)) break;
    }
    expect_prefix(TK_GT, "`>`");
    return g;
}

/* `for<'a, 'b>` at the cursor on `for` */
static Node *it_for_binder(void) {
    expect(TK_KW_for, "`for`");
    if (!at(TK_LT)) perr(cur(), "expected `<`, found %s", tk_spelling(cur()->kind));
    return parse_generics();
}

Node *parse_where(void) {
    Node *w;
    if (!at(TK_KW_where)) return 0;
    w = it_mk_at(N_WHERE, cur());
    next();
    for (;;) {
        int k = cur()->kind;
        Token *st = cur();
        Node *p;
        if (k == TK_LBRACE || k == TK_SEMI || k == TK_EQ || k == TK_EOF) break;
        if (k == TK_LIFETIME) {
            next();
            p = it_mk_at(N_WHERE_LIFETIME, st);
            p->name = st->text;
            expect(TK_COLON, "`:`");
            p->b = it_lifetime_bounds();
        } else {
            p = it_mk_at(N_WHERE_TYPE, st);
            if (k == TK_KW_for) p->c = it_for_binder();
            p->a = parse_type();
            expect(TK_COLON, "`:`");
            p->b = it_bounds_opt();
        }
        list_push(&w->list, p);
        if (!accept(TK_COMMA)) break;
    }
    return w;
}

/* ---------------------------------------------------------------- functions */

/* Is there a self parameter at the cursor?  (lookahead only) */
static int it_self_ahead(void) {
    int i = 0;
    if (peek(0)->kind == TK_AND) {
        i = 1;
        if (peek(i)->kind == TK_LIFETIME) i++;
        if (peek(i)->kind == TK_KW_mut) i++;
        return peek(i)->kind == TK_KW_self && peek(i + 1)->kind != TK_PATHSEP;
    }
    if (peek(0)->kind == TK_KW_mut) i = 1;
    return peek(i)->kind == TK_KW_self && peek(i + 1)->kind != TK_PATHSEP;
}

static Node *it_self_param(Token *st) {
    Node *p = it_mk_at(N_SELF_PARAM, st);
    if (at(TK_AND)) {
        next();
        p->flags |= F_REF;
        if (at(TK_LIFETIME)) p->a = parse_lifetime();
        if (accept(TK_KW_mut)) p->flags |= F_MUT;
        expect(TK_KW_self, "`self`");
    } else {
        if (accept(TK_KW_mut)) p->flags |= F_MUT;
        expect(TK_KW_self, "`self`");
        if (accept(TK_COLON)) {
            p->flags |= F_TYPED;
            p->b = parse_type();
        }
    }
    return p;
}

Node *parse_fn_sig_and_body(Node *fn) {
    int first = 1;
    fn->a = parse_generics();
    expect(TK_LPAREN, "`(`");
    while (!it_at_close(TK_RPAREN)) {
        NodeList pa;
        Token *st;
        Node *p;
        it_nl_init(&pa);
        parse_outer_attrs(&pa);
        st = cur();
        if (first && it_self_ahead()) {
            p = it_self_param(st);
            p->attrs = pa;
            list_push(&fn->list, p);
        } else if (at(TK_DOTDOTDOT)) {
            next();
            fn->flags |= F_VARIADIC;
        } else {
            p = it_mk_at(N_PARAM, st);
            p->attrs = pa;
            p->a = parse_pattern_no_alt();
            expect(TK_COLON, "`:` (unnamed parameters are not supported)");
            if (at(TK_DOTDOTDOT)) {
                next();
                fn->flags |= F_VARIADIC;
            } else {
                p->b = parse_type();
                list_push(&fn->list, p);
            }
        }
        first = 0;
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_RPAREN, "`)`");
    if (accept(TK_RARROW)) fn->b = parse_type();
    fn->d = parse_where();
    if (accept(TK_SEMI)) {
        fn->c = 0;
    } else if (at(TK_LBRACE)) {
        fn->c = parse_block();
    } else {
        perr(cur(), "expected `{` or `;` after function signature");
    }
    return fn;
}

/* qualifiers, `fn`, name, signature.  The cursor is at the first qualifier. */
static Node *it_fn(Token *st) {
    Node *n = it_mk_at(N_ITEM_FN, st);
    for (;;) {
        int k = cur()->kind;
        if (k == TK_KW_const) {
            n->flags |= F_CONST;
            next();
        } else if (k == TK_KW_async) {
            n->flags |= F_ASYNC;
            next();
        } else if (k == TK_KW_unsafe) {
            n->flags |= F_UNSAFE;
            next();
        } else if (k == TK_KW_extern) {
            Token *t;
            n->flags |= F_EXTERN;
            next();
            t = cur();
            if (t->kind == TK_STR) {
                node_set_str(n, t->str, t->slen);
                next();
            }
        } else
            break;
    }
    expect(TK_KW_fn, "`fn`");
    n->name = expect_ident("function name");
    parse_fn_sig_and_body(n);
    return n;
}

/* ---------------------------------------------------------------- structs, enums, unions */

/* `{ field, ... }` into n->list, flags NAMED */
static void it_named_fields(Node *n) {
    expect(TK_LBRACE, "`{`");
    n->flags |= F_NAMED;
    while (!it_at_close(TK_RBRACE)) {
        NodeList pa;
        Token *st;
        Node *f;
        it_nl_init(&pa);
        parse_outer_attrs(&pa);
        st = cur();
        f = it_mk_at(N_FIELD, st);
        f->attrs = pa;
        f->vis = parse_vis();
        f->name = expect_ident("field name");
        expect(TK_COLON, "`:`");
        f->a = parse_type();
        list_push(&n->list, f);
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_RBRACE, "`}`");
}

/* `( T, pub U )` into n->list, flags TUPLE */
static void it_tuple_fields(Node *n) {
    expect(TK_LPAREN, "`(`");
    n->flags |= F_TUPLE;
    while (!it_at_close(TK_RPAREN)) {
        NodeList pa;
        Token *st;
        Node *f;
        it_nl_init(&pa);
        parse_outer_attrs(&pa);
        st = cur();
        f = it_mk_at(N_FIELD, st);
        f->attrs = pa;
        f->vis = parse_vis();
        f->a = parse_type();
        list_push(&n->list, f);
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_RPAREN, "`)`");
}

static Node *it_struct(Token *st) {
    Node *n = it_mk_at(N_ITEM_STRUCT, st);
    Node *w;
    next();
    n->name = expect_ident("struct name");
    n->a = parse_generics();
    w = parse_where();
    if (at(TK_LBRACE)) {
        n->d = w;
        it_named_fields(n);
    } else if (at(TK_LPAREN)) {
        if (w) perr(cur(), "a tuple struct's where clause comes after the fields");
        it_tuple_fields(n);
        n->d = parse_where();
        expect(TK_SEMI, "`;`");
    } else {
        n->d = w;
        expect(TK_SEMI, "`;`, `{` or `(` after struct name");
    }
    return n;
}

static Node *it_union(Token *st) {
    Node *n = it_mk_at(N_ITEM_UNION, st);
    next();
    n->name = expect_ident("union name");
    n->a = parse_generics();
    n->d = parse_where();
    it_named_fields(n);
    return n;
}

static Node *it_enum(Token *st) {
    Node *n = it_mk_at(N_ITEM_ENUM, st);
    next();
    n->name = expect_ident("enum name");
    n->a = parse_generics();
    n->d = parse_where();
    expect(TK_LBRACE, "`{`");
    while (!it_at_close(TK_RBRACE)) {
        NodeList pa;
        Token *vt;
        Node *v;
        it_nl_init(&pa);
        parse_outer_attrs(&pa);
        vt = cur();
        v = it_mk_at(N_VARIANT, vt);
        v->attrs = pa;
        v->vis = parse_vis();
        v->name = expect_ident("variant name");
        if (at(TK_LBRACE)) it_named_fields(v);
        else if (at(TK_LPAREN)) it_tuple_fields(v);
        if (accept(TK_EQ)) v->a = parse_expr();
        list_push(&n->list, v);
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_RBRACE, "`}`");
    return n;
}

/* ---------------------------------------------------------------- items */

static Node *it_parse_item(int allow_macro);

/* which kinds may appear in a braced item list */
enum { IT_CTX_ASSOC = 0, IT_CTX_EXTERN = 1, IT_CTX_MOD = 2 };

static int it_kind_ok(int kind, int ctx) {
    if (ctx == IT_CTX_MOD) return 1;
    if (kind == N_ITEM_FN || kind == N_ITEM_TYPE || kind == N_MACRO_CALL) return 1;
    if (ctx == IT_CTX_ASSOC) return kind == N_ITEM_CONST;
    return kind == N_ITEM_STATIC;
}

/* `{ [#![inner]] items }` into n->list; inner attributes go to n->attrs */
static void it_item_list(Node *n, int ctx) {
    expect(TK_LBRACE, "`{`");
    parse_inner_attrs(&n->attrs);
    while (!it_at_close(TK_RBRACE)) {
        Node *it;
        if (it_at_close(TK_EOF)) perr(cur(), "expected `}`, found end of file");
        it = it_parse_item(1);
        if (!it) perr(cur(), ctx == IT_CTX_MOD ? "expected item" : "expected associated item");
        if (!it_kind_ok(it->kind, ctx)) perr(cur(), "this item is not allowed here");
        list_push(&n->list, it);
    }
    expect(TK_RBRACE, "`}`");
}

static Node *it_trait(Token *st) {
    Node *n = it_mk_at(N_ITEM_TRAIT, st);
    if (accept(TK_KW_unsafe)) n->flags |= F_UNSAFE;
    if (it_is_word(cur(), "auto")) {
        next();
        n->flags |= F_AUTO;
    }
    expect(TK_KW_trait, "`trait`");
    n->name = expect_ident("trait name");
    n->a = parse_generics();
    if (accept(TK_COLON)) n->c = it_bounds_opt();
    n->d = parse_where();
    it_item_list(n, IT_CTX_ASSOC);
    return n;
}

static Node *it_impl(Token *st) {
    Node *n = it_mk_at(N_ITEM_IMPL, st);
    Node *ty;
    if (accept(TK_KW_unsafe)) n->flags |= F_UNSAFE;
    expect(TK_KW_impl, "`impl`");
    if (at(TK_LT) && peek(2)->kind != TK_KW_as) n->a = parse_generics();
    if (at(TK_NOT)) {
        next();
        n->flags |= F_NEG;
    }
    ty = parse_type();
    if (accept(TK_KW_for)) {
        if (ty->kind != N_T_PATH) perr(cur(), "expected a trait path before `for`");
        n->c = ty->a;
        n->b = parse_type();
    } else {
        if (n->flags & F_NEG) perr(cur(), "expected `for` after a negative impl's trait");
        n->b = ty;
    }
    n->d = parse_where();
    it_item_list(n, IT_CTX_ASSOC);
    return n;
}

static Node *it_mod(Token *st) {
    Node *n = it_mk_at(N_ITEM_MOD, st);
    if (accept(TK_KW_unsafe)) n->flags |= F_UNSAFE;
    expect(TK_KW_mod, "`mod`");
    n->name = expect_ident("module name");
    if (accept(TK_SEMI)) return n;
    n->flags |= F_INLINE;
    it_item_list(n, IT_CTX_MOD);
    return n;
}

/* one path segment of a use tree; returns 0 if the cursor is not at one */
static Node *it_use_seg(void) {
    Token *t = cur();
    int k = t->kind;
    if (k == TK_IDENT || k == TK_KW_self || k == TK_KW_super || k == TK_KW_crate || k == TK_KW_Self) {
        next();
        return it_seg(t);
    }
    if (k == TK_DOLLAR && peek(1)->kind == TK_KW_crate) {
        Node *s = it_mk_at(N_PATH_SEG, t);
        next();
        next();
        s->name = name_intern_str("$crate");
        return s;
    }
    return 0;
}

static Node *it_use_tree(void) {
    Token *st = cur();
    Node *t = it_mk_at(N_USE_TREE, st);
    Node *path = 0;
    int tail = 0;
    if (at(TK_STAR)) {
        next();
        t->flags |= F_GLOB;
        return t;
    }
    if (at(TK_LBRACE)) {
        tail = 1;
    } else {
        path = it_mk_at(N_PATH, st);
        if (accept(TK_PATHSEP)) path->flags |= F_GLOBAL;
        if (at(TK_LBRACE) || at(TK_STAR)) {
            tail = 1;
        } else {
            for (;;) {
                Node *s = it_use_seg();
                if (!s) perr(cur(), "expected path segment in `use`");
                list_push(&path->list, s);
                if (at(TK_PATHSEP) && (peek(1)->kind == TK_STAR || peek(1)->kind == TK_LBRACE)) {
                    next();
                    tail = 1;
                    break;
                }
                if (!accept(TK_PATHSEP)) break;
            }
        }
        t->a = path;
    }
    if (tail) {
        if (accept(TK_STAR)) {
            t->flags |= F_GLOB;
            return t;
        }
        expect(TK_LBRACE, "`{`");
        t->flags |= F_GROUP;
        while (!at(TK_RBRACE)) {
            list_push(&t->list, it_use_tree());
            if (!accept(TK_COMMA)) break;
        }
        expect(TK_RBRACE, "`}`");
        return t;
    }
    if (accept(TK_KW_as)) {
        if (accept(TK_UNDERSCORE)) t->name2 = name_intern_str("_");
        else t->name2 = expect_ident("alias name");
    }
    return t;
}

static Node *it_use(Token *st) {
    Node *n = it_mk_at(N_ITEM_USE, st);
    next();
    n->a = it_use_tree();
    expect(TK_SEMI, "`;`");
    return n;
}

static Node *it_const(Token *st) {
    Node *n = it_mk_at(N_ITEM_CONST, st);
    next();
    if (accept(TK_UNDERSCORE)) n->name = name_intern_str("_");
    else n->name = expect_ident("constant name");
    expect(TK_COLON, "`:`");
    n->a = parse_type();
    if (accept(TK_EQ)) n->b = parse_expr();
    expect(TK_SEMI, "`;`");
    return n;
}

static Node *it_static(Token *st) {
    Node *n = it_mk_at(N_ITEM_STATIC, st);
    next();
    if (accept(TK_KW_mut)) n->flags |= F_MUT;
    n->name = expect_ident("static name");
    expect(TK_COLON, "`:`");
    n->a = parse_type();
    if (accept(TK_EQ)) n->b = parse_expr();
    expect(TK_SEMI, "`;`");
    return n;
}

static Node *it_type_item(Token *st) {
    Node *n = it_mk_at(N_ITEM_TYPE, st);
    next();
    n->name = expect_ident("type name");
    n->a = parse_generics();
    if (accept(TK_COLON)) n->c = it_bounds_opt();
    n->d = parse_where();
    if (accept(TK_EQ)) {
        n->b = parse_type();
        if (!n->d) n->d = parse_where();
    }
    expect(TK_SEMI, "`;`");
    return n;
}

static Node *it_extern_crate(Token *st) {
    Node *n = it_mk_at(N_ITEM_EXTERN_CRATE, st);
    next();
    next();
    if (at(TK_KW_self)) {
        n->name = next()->text;
    } else {
        n->name = expect_ident("crate name");
    }
    if (accept(TK_KW_as)) {
        if (accept(TK_UNDERSCORE)) n->name2 = name_intern_str("_");
        else n->name2 = expect_ident("alias name");
    }
    expect(TK_SEMI, "`;`");
    return n;
}

static Node *it_extern_block(Token *st) {
    Node *n = it_mk_at(N_ITEM_EXTERN_BLOCK, st);
    Token *t;
    if (accept(TK_KW_unsafe)) n->flags |= F_UNSAFE;
    expect(TK_KW_extern, "`extern`");
    t = cur();
    if (t->kind == TK_STR) {
        node_set_str(n, t->str, t->slen);
        next();
    }
    it_item_list(n, IT_CTX_EXTERN);
    return n;
}

static Node *it_macro_item(Token *st) {
    Node *path = parse_path(PATH_SIMPLE);
    Node *m;
    expect(TK_NOT, "`!`");
    m = parse_macro_call(path, st);
    if (accept(TK_SEMI)) m->flags |= F_SEMI;
    else if (m->flags & (F_DELIM_PAREN | F_DELIM_BRACKET)) perr(cur(), "expected `;` after macro invocation");
    return m;
}

static Node *it_macro_rules(void) {
    Node *n = parse_macro_rules();
    if (n && P.pos > 0 && P.toks[P.pos - 1].kind != TK_RBRACE) accept(TK_SEMI);
    return n;
}

/* ---- lookahead: does an item start at peek(i)? */

static int it_ahead(int i) {
    Token *t = peek(i);
    int k = t->kind;
    int k1 = peek(i + 1)->kind;
    switch (k) {
    case TK_KW_fn:
    case TK_KW_struct:
    case TK_KW_enum:
    case TK_KW_trait:
    case TK_KW_impl:
    case TK_KW_mod:
    case TK_KW_use:
    case TK_KW_type:
    case TK_KW_static:
    case TK_KW_extern:
        return 1;
    case TK_KW_const:
        return k1 == TK_IDENT || k1 == TK_UNDERSCORE || k1 == TK_KW_fn || k1 == TK_KW_async ||
               k1 == TK_KW_unsafe || k1 == TK_KW_extern;
    case TK_KW_unsafe:
        if (k1 == TK_KW_fn || k1 == TK_KW_impl || k1 == TK_KW_trait || k1 == TK_KW_extern || k1 == TK_KW_mod)
            return 1;
        return it_is_word(peek(i + 1), "auto") && peek(i + 2)->kind == TK_KW_trait;
    case TK_KW_async:
        return k1 == TK_KW_fn || (k1 == TK_KW_unsafe && peek(i + 2)->kind == TK_KW_fn);
    case TK_IDENT:
        if (it_is_word(t, "union")) return k1 == TK_IDENT;
        if (it_is_word(t, "auto")) return k1 == TK_KW_trait;
        if (it_is_word(t, "macro_rules")) return k1 == TK_NOT && peek(i + 2)->kind == TK_IDENT;
        return 0;
    }
    return 0;
}

/* `path ! (` / `[` / `{` at peek(i) */
static int it_macro_ahead(int i) {
    int k;
    if (peek(i)->kind == TK_PATHSEP) i++;
    for (;;) {
        k = peek(i)->kind;
        if (k == TK_IDENT || k == TK_KW_self || k == TK_KW_super || k == TK_KW_crate || k == TK_KW_Self)
            i++;
        else if (k == TK_DOLLAR && peek(i + 1)->kind == TK_KW_crate)
            i += 2;
        else
            return 0;
        if (peek(i)->kind == TK_PATHSEP) {
            i++;
            continue;
        }
        break;
    }
    if (peek(i)->kind != TK_NOT) return 0;
    k = peek(i + 1)->kind;
    return k == TK_LPAREN || k == TK_LBRACK || k == TK_LBRACE;
}

int at_item_start(void) {
    PState save = p_save();
    NodeList tmp;
    int i, r;
    it_nl_init(&tmp);
    parse_outer_attrs(&tmp);
    i = it_vis_len();
    r = it_ahead(i);
    p_restore(save);
    return r;
}

/* `extern` / `unsafe extern` at the cursor: is it a block (not an fn)? */
static int it_extern_is_block(int i) {
    /* peek(i) is `extern` */
    int j = i + 1;
    if (peek(j)->kind == TK_STR) j++;
    return peek(j)->kind == TK_LBRACE;
}

static Node *it_parse_item(int allow_macro) {
    PState save = p_save();
    NodeList attrs;
    Token *st;
    Node *vis;
    Node *n;
    int i, k, k1;
    it_nl_init(&attrs);
    parse_outer_attrs(&attrs);
    st = cur();
    i = it_vis_len();
    if (!(it_ahead(i) || (allow_macro && it_macro_ahead(i)))) {
        if (i > 0) perr(st, "expected item after visibility");
        p_restore(save);
        return 0;
    }
    vis = parse_vis();
    st = cur();
    k = st->kind;
    k1 = peek(1)->kind;
    if (k == TK_KW_fn || k == TK_KW_async) {
        n = it_fn(st);
    } else if (k == TK_KW_const) {
        if (k1 == TK_KW_fn || k1 == TK_KW_async || k1 == TK_KW_unsafe || k1 == TK_KW_extern) n = it_fn(st);
        else n = it_const(st);
    } else if (k == TK_KW_unsafe) {
        if (k1 == TK_KW_fn) n = it_fn(st);
        else if (k1 == TK_KW_impl) n = it_impl(st);
        else if (k1 == TK_KW_mod) n = it_mod(st);
        else if (k1 == TK_KW_extern) n = it_extern_is_block(1) ? it_extern_block(st) : it_fn(st);
        else n = it_trait(st);
    } else if (k == TK_KW_extern) {
        if (k1 == TK_KW_crate) n = it_extern_crate(st);
        else if (it_extern_is_block(0)) n = it_extern_block(st);
        else n = it_fn(st);
    } else if (k == TK_KW_struct) {
        n = it_struct(st);
    } else if (k == TK_KW_enum) {
        n = it_enum(st);
    } else if (k == TK_KW_trait) {
        n = it_trait(st);
    } else if (k == TK_KW_impl) {
        n = it_impl(st);
    } else if (k == TK_KW_mod) {
        n = it_mod(st);
    } else if (k == TK_KW_use) {
        n = it_use(st);
    } else if (k == TK_KW_type) {
        n = it_type_item(st);
    } else if (k == TK_KW_static) {
        n = it_static(st);
    } else if (it_is_word(st, "union")) {
        n = it_union(st);
    } else if (it_is_word(st, "auto")) {
        n = it_trait(st);
    } else if (it_is_word(st, "macro_rules")) {
        n = it_macro_rules();
    } else {
        n = it_macro_item(st);
    }
    {
        /* inner attributes found inside the item's braces come after the outer ones */
        NodeList inner = n->attrs;
        long q;
        n->attrs = attrs;
        for (q = 0; q < inner.n; q++) list_push(&n->attrs, inner.v[q]);
    }
    n->vis = vis;
    return n;
}

Node *parse_item(void) {
    return it_parse_item(1);
}

Node *parse_crate(void) {
    Node *c = it_mk_at(N_CRATE, &P.toks[P.pos]);
    parse_inner_attrs(&c->attrs);
    while (!it_at_close(TK_EOF)) {
        Node *it = parse_item();
        if (!it) perr(cur(), "expected item, found %s", tk_spelling(cur()->kind));
        list_push(&c->list, it);
    }
    return c;
}
