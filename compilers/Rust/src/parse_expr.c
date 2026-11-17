/*
 * parse_expr.c - expressions, statements and blocks (see parser.h, nodes.inc).
 *
 * Precedence climbing, lowest to highest:
 *   1 = += -= ...  (right assoc)      2 .. ..=  (non-assoc)
 *   3 ||   4 &&   5 == != < <= > >= (non-assoc)   6 |   7 ^   8 &
 *   9 << >>   10 + -   11 * / %   12 as   then prefix, then postfix.
 *
 * Restrictions kept in g_nostruct: 1 while parsing the head of an if / while
 * / match / for, where `Name {` is not a struct literal.  Parentheses,
 * brackets, call arguments, blocks and struct bodies clear it.
 */

static int g_nostruct;

#define PREC_ASSIGN 1
#define PREC_RANGE  2
#define PREC_LOR    3
#define PREC_LAND   4
#define PREC_CMP    5
#define PREC_BOR    6
#define PREC_BXOR   7
#define PREC_BAND   8
#define PREC_SHIFT  9
#define PREC_ADD    10
#define PREC_MUL    11
#define PREC_AS     12

static Node *x_prec(int minp);
static Node *x_unary(void);
static Node *x_primary(void);
static Node *x_postfix(Node *e);
static Node *x_rest(Node *lhs, int minp);
static Node *x_path_primary(Node *path, Token *start);
static Node *x_stmt1(void);
static Node *x_stmt_mode(int *complete);

static Node *mkt(int kind, Token *t) {
    return mk(kind, t->line, t->col);
}

static void x_nl_init(NodeList *l) {
    l->v = 0;
    l->n = 0;
    l->cap = 0;
}

static const char *x_desc(Token *t) {
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

static int x_is_delim(int k) {
    return k == TK_LPAREN || k == TK_LBRACK || k == TK_LBRACE;
}

/* ---------------------------------------------------------------- literals */

Node *parse_literal(void) {
    Token *t = cur();
    Node *n;
    switch (t->kind) {
    case TK_INT:
        n = mkt(N_E_LIT_INT, t);
        n->ival = (long long)t->value;
        n->name = t->suffix;
        if (t->text) node_set_str(n, t->text->s, t->text->len);
        break;
    case TK_FLOAT:
        n = mkt(N_E_LIT_FLOAT, t);
        n->fval = t->fval;
        n->name = t->suffix;
        if (t->text) node_set_str(n, t->text->s, t->text->len);
        break;
    case TK_STR:
        n = mkt(N_E_LIT_STR, t);
        node_set_str(n, t->str, t->slen);
        if (t->raw) n->flags |= F_RAW;
        break;
    case TK_BSTR:
        n = mkt(N_E_LIT_BYTESTR, t);
        node_set_str(n, t->str, t->slen);
        if (t->raw) n->flags |= F_RAW;
        break;
    case TK_CHAR:
        n = mkt(N_E_LIT_CHAR, t);
        n->ival = (long long)t->value;
        break;
    case TK_BYTE:
        n = mkt(N_E_LIT_BYTE, t);
        n->ival = (long long)t->value;
        break;
    case TK_KW_true:
        n = mkt(N_E_LIT_BOOL, t);
        n->ival = 1;
        break;
    case TK_KW_false:
        n = mkt(N_E_LIT_BOOL, t);
        n->ival = 0;
        break;
    default:
        perr(t, "expected literal, found %s", x_desc(t));
        return 0;
    }
    next();
    return n;
}

/* ---------------------------------------------------------------- helpers */

/* the current token can begin an expression (used for optional operands) */
static int x_can_begin(void) {
    Token *t = cur();
    switch (t->kind) {
    case TK_IDENT: case TK_LIFETIME: case TK_INT: case TK_FLOAT: case TK_CHAR:
    case TK_BYTE: case TK_STR: case TK_BSTR:
    case TK_KW_true: case TK_KW_false: case TK_KW_self: case TK_KW_Self:
    case TK_KW_super: case TK_KW_crate: case TK_KW_if: case TK_KW_match:
    case TK_KW_loop: case TK_KW_while: case TK_KW_for: case TK_KW_unsafe:
    case TK_KW_move: case TK_KW_async: case TK_KW_return: case TK_KW_break:
    case TK_KW_continue: case TK_KW_let: case TK_KW_const:
    case TK_LPAREN: case TK_LBRACK: case TK_MINUS: case TK_NOT: case TK_STAR:
    case TK_AND: case TK_ANDAND: case TK_OR: case TK_OROR: case TK_LT:
    case TK_PATHSEP: case TK_DOTDOT: case TK_DOTDOTEQ: case TK_UNDERSCORE:
    case TK_DOLLAR:
        return 1;
    case TK_LBRACE:
        return !g_nostruct;
    default:
        return 0;
    }
}

static int x_binop(Token *t, int *prec, int *op) {
    switch (t->kind) {
    case TK_OROR:    *prec = PREC_LOR;   *op = OP_LOR;    return 1;
    case TK_ANDAND:  *prec = PREC_LAND;  *op = OP_LAND;   return 1;
    case TK_EQEQ:    *prec = PREC_CMP;   *op = OP_EQ;     return 1;
    case TK_NE:      *prec = PREC_CMP;   *op = OP_NE;     return 1;
    case TK_LT:      *prec = PREC_CMP;   *op = OP_LT;     return 1;
    case TK_LE:      *prec = PREC_CMP;   *op = OP_LE;     return 1;
    case TK_GT:      *prec = PREC_CMP;   *op = OP_GT;     return 1;
    case TK_GE:      *prec = PREC_CMP;   *op = OP_GE;     return 1;
    case TK_OR:      *prec = PREC_BOR;   *op = OP_BITOR;  return 1;
    case TK_CARET:   *prec = PREC_BXOR;  *op = OP_BITXOR; return 1;
    case TK_AND:     *prec = PREC_BAND;  *op = OP_BITAND; return 1;
    case TK_SHL:     *prec = PREC_SHIFT; *op = OP_SHL;    return 1;
    case TK_SHR:     *prec = PREC_SHIFT; *op = OP_SHR;    return 1;
    case TK_PLUS:    *prec = PREC_ADD;   *op = OP_ADD;    return 1;
    case TK_MINUS:   *prec = PREC_ADD;   *op = OP_SUB;    return 1;
    case TK_STAR:    *prec = PREC_MUL;   *op = OP_MUL;    return 1;
    case TK_SLASH:   *prec = PREC_MUL;   *op = OP_DIV;    return 1;
    case TK_PERCENT: *prec = PREC_MUL;   *op = OP_REM;    return 1;
    default: return 0;
    }
}

static int x_assign_op(int kind) {
    switch (kind) {
    case TK_PLUSEQ:    return OP_ADD;
    case TK_MINUSEQ:   return OP_SUB;
    case TK_STAREQ:    return OP_MUL;
    case TK_SLASHEQ:   return OP_DIV;
    case TK_PERCENTEQ: return OP_REM;
    case TK_CARETEQ:   return OP_BITXOR;
    case TK_ANDEQ:     return OP_BITAND;
    case TK_OREQ:      return OP_BITOR;
    case TK_SHLEQ:     return OP_SHL;
    case TK_SHREQ:     return OP_SHR;
    default: return 0;
    }
}

/* ---------------------------------------------------------------- entry points */

Node *parse_expr(void) {
    int old = g_nostruct;
    Node *e;
    g_nostruct = 0;
    e = x_prec(PREC_ASSIGN);
    g_nostruct = old;
    return e;
}

Node *parse_expr_no_struct(void) {
    int old = g_nostruct;
    Node *e;
    g_nostruct = 1;
    e = x_prec(PREC_ASSIGN);
    g_nostruct = old;
    return e;
}

/* ---------------------------------------------------------------- binary operators */

static Node *x_prec(int minp) {
    Token *t = cur();
    if (minp <= PREC_RANGE && (t->kind == TK_DOTDOT || t->kind == TK_DOTDOTEQ)) {
        Node *r = mkt(N_E_RANGE, t);
        next();
        if (t->kind == TK_DOTDOTEQ) r->flags |= F_INCLUSIVE;
        if (x_can_begin()) r->b = x_prec(PREC_LOR);
        else if (r->flags & F_INCLUSIVE) perr(cur(), "inclusive range with no end");
        if (at(TK_DOTDOT) || at(TK_DOTDOTEQ)) perr(cur(), "ranges cannot be chained");
        return r;
    }
    return x_rest(x_unary(), minp);
}

static Node *x_rest(Node *lhs, int minp) {
    for (;;) {
        Token *t = cur();
        int prec, op;
        Node *n;
        if (t->kind == TK_EQ) {
            if (PREC_ASSIGN < minp) break;
            next();
            n = mk(N_E_ASSIGN, lhs->line, lhs->col);
            n->a = lhs;
            n->b = x_prec(PREC_ASSIGN);
            lhs = n;
            continue;
        }
        op = x_assign_op(t->kind);
        if (op) {
            if (PREC_ASSIGN < minp) break;
            next();
            n = mk(N_E_ASSIGN_OP, lhs->line, lhs->col);
            n->op = op;
            n->a = lhs;
            n->b = x_prec(PREC_ASSIGN);
            lhs = n;
            continue;
        }
        if (t->kind == TK_DOTDOT || t->kind == TK_DOTDOTEQ) {
            if (PREC_RANGE < minp) break;
            next();
            n = mk(N_E_RANGE, lhs->line, lhs->col);
            n->a = lhs;
            if (t->kind == TK_DOTDOTEQ) n->flags |= F_INCLUSIVE;
            if (x_can_begin()) n->b = x_prec(PREC_LOR);
            else if (n->flags & F_INCLUSIVE) perr(cur(), "inclusive range with no end");
            if (at(TK_DOTDOT) || at(TK_DOTDOTEQ)) perr(cur(), "ranges cannot be chained");
            lhs = n;
            continue;
        }
        if (t->kind == TK_KW_as) {
            if (PREC_AS < minp) break;
            next();
            n = mk(N_E_CAST, lhs->line, lhs->col);
            n->a = lhs;
            n->b = parse_type_no_bounds();
            lhs = n;
            continue;
        }
        if (!x_binop(t, &prec, &op)) break;
        if (prec < minp) break;
        next();
        n = mk(N_E_BINARY, lhs->line, lhs->col);
        n->op = op;
        n->a = lhs;
        n->b = x_prec(prec + 1);
        lhs = n;
        if (prec == PREC_CMP) {
            int p2, o2;
            if (x_binop(cur(), &p2, &o2) && p2 == PREC_CMP)
                perr(cur(), "comparison operators cannot be chained");
        }
    }
    return lhs;
}

/* ---------------------------------------------------------------- unary */

static Node *x_unary(void) {
    Token *t = cur();
    Node *n;
    if (t->kind == TK_MINUS || t->kind == TK_NOT) {
        n = mkt(N_E_UNARY, t);
        n->op = t->kind == TK_MINUS ? OP_NEG : OP_NOT;
        next();
        n->a = x_unary();
        return n;
    }
    if (t->kind == TK_STAR) {
        n = mkt(N_E_DEREF, t);
        next();
        n->a = x_unary();
        return n;
    }
    if (t->kind == TK_AND || t->kind == TK_ANDAND) {
        n = mkt(N_E_REF, t);
        accept_prefix(TK_AND);
        if (accept(TK_KW_mut)) n->flags |= F_MUT;
        n->a = x_unary();
        return n;
    }
    return x_postfix(x_primary());
}

/* ---------------------------------------------------------------- postfix */

static void x_args(NodeList *out, int close) {
    int old = g_nostruct;
    g_nostruct = 0;
    while (!at(close)) {
        list_push(out, x_prec(PREC_ASSIGN));
        if (!accept(TK_COMMA)) break;
    }
    g_nostruct = old;
}

static Node *x_tuple_field(Node *e, Token *idx) {
    Node *n = mk(N_E_TUPLE_FIELD, e->line, e->col);
    n->a = e;
    n->ival = (long long)idx->value;
    return n;
}

static Node *x_postfix(Node *e) {
    int pending_dot = 0;
    for (;;) {
        Token *t = cur();
        Node *n;
        if (pending_dot) {
            pending_dot = 0;
            goto after_dot;
        }
        if (t->kind == TK_QUESTION) {
            next();
            n = mk(N_E_TRY, e->line, e->col);
            n->a = e;
            e = n;
        } else if (t->kind == TK_LPAREN) {
            next();
            n = mk(N_E_CALL, e->line, e->col);
            n->a = e;
            x_args(&n->list, TK_RPAREN);
            expect(TK_RPAREN, "`)`");
            e = n;
        } else if (t->kind == TK_LBRACK) {
            int old = g_nostruct;
            next();
            n = mk(N_E_INDEX, e->line, e->col);
            n->a = e;
            g_nostruct = 0;
            n->b = x_prec(PREC_ASSIGN);
            g_nostruct = old;
            expect(TK_RBRACK, "`]`");
            e = n;
        } else if (t->kind == TK_DOT) {
            next();
        after_dot:
            t = cur();
            if (t->kind == TK_KW_await) {
                next();
                n = mk(N_E_AWAIT, e->line, e->col);
                n->a = e;
                e = n;
            } else if (t->kind == TK_INT) {
                next();
                e = x_tuple_field(e, t);
            } else if (t->kind == TK_FLOAT) {
                Token parts[3];
                int k = lex_split_tuple_index(t, parts);
                if (!k) perr(t, "unexpected number after `.`");
                next();
                e = x_tuple_field(e, &parts[0]);
                if (k == 3) e = x_tuple_field(e, &parts[2]);
                else pending_dot = 1;
            } else if (t->kind == TK_IDENT) {
                Name *name = t->text;
                Node *seg = 0;
                next();
                if (at(TK_PATHSEP) && (peek(1)->kind == TK_LT || peek(1)->kind == TK_SHL)) {
                    next();
                    seg = mkt(N_PATH_SEG, t);
                    seg->name = name;
                    seg->flags = F_ANGLE | F_TURBOFISH;
                    parse_generic_args(&seg->list);
                    if (!at(TK_LPAREN)) perr(cur(), "expected `(` after method turbofish, found %s", x_desc(cur()));
                }
                if (at(TK_LPAREN)) {
                    next();
                    n = mk(N_E_METHOD_CALL, e->line, e->col);
                    n->a = e;
                    n->name = name;
                    n->b = seg;
                    x_args(&n->list, TK_RPAREN);
                    expect(TK_RPAREN, "`)`");
                    e = n;
                } else {
                    n = mk(N_E_FIELD, e->line, e->col);
                    n->a = e;
                    n->name = name;
                    e = n;
                }
            } else {
                perr(t, "expected field or method name after `.`, found %s", x_desc(t));
            }
        } else {
            break;
        }
    }
    return e;
}

/* ---------------------------------------------------------------- primaries */

static Node *x_struct_lit(Node *path, Token *start) {
    Node *n = mkt(N_E_STRUCT, start);
    int old = g_nostruct;
    n->a = path;
    expect(TK_LBRACE, "`{`");
    g_nostruct = 0;
    while (!at(TK_RBRACE)) {
        NodeList attrs;
        Token *t;
        Node *f;
        x_nl_init(&attrs);
        if (at(TK_POUND)) parse_outer_attrs(&attrs);
        t = cur();
        if (t->kind == TK_DOTDOT) {
            next();
            if (!at(TK_RBRACE)) n->b = x_prec(PREC_ASSIGN);
            break;
        }
        if (t->kind != TK_IDENT && t->kind != TK_INT)
            perr(t, "expected field name, found %s", x_desc(t));
        next();
        f = mkt(N_FIELD_INIT, t);
        f->name = t->text;
        f->attrs = attrs;
        if (accept(TK_COLON)) {
            f->a = x_prec(PREC_ASSIGN);
        } else {
            Node *p, *seg;
            if (t->kind != TK_IDENT) perr(t, "expected `:` after tuple field index");
            f->flags |= F_SHORTHAND;
            seg = mkt(N_PATH_SEG, t);
            seg->name = t->text;
            p = mkt(N_PATH, t);
            list_push(&p->list, seg);
            f->a = mkt(N_E_PATH, t);
            f->a->a = p;
        }
        list_push(&n->list, f);
        if (!accept(TK_COMMA)) break;
    }
    g_nostruct = old;
    expect(TK_RBRACE, "`}`");
    return n;
}

/* a path that was already parsed: macro call, struct literal or plain path */
static Node *x_path_primary(Node *path, Token *start) {
    Node *n;
    if (at(TK_NOT) && x_is_delim(peek(1)->kind)) {
        next();
        return parse_macro_call(path, start);
    }
    if (at(TK_LBRACE) && !g_nostruct) return x_struct_lit(path, start);
    n = mkt(N_E_PATH, start);
    n->a = path;
    return n;
}

static Node *x_cond(void) {
    return parse_expr_no_struct();
}

static Node *x_if(void) {
    Token *t = next();
    Node *n = mkt(N_E_IF, t);
    n->a = x_cond();
    n->b = parse_block();
    if (accept(TK_KW_else)) {
        if (at(TK_KW_if)) n->c = x_if();
        else n->c = parse_block();
    }
    return n;
}

static Node *x_while(Token *label) {
    Token *t = next();
    Node *n = mkt(N_E_WHILE, label ? label : t);
    if (label) n->name = label->text;
    n->a = x_cond();
    n->b = parse_block();
    return n;
}

static Node *x_loop(Token *label) {
    Token *t = next();
    Node *n = mkt(N_E_LOOP, label ? label : t);
    if (label) n->name = label->text;
    n->a = parse_block();
    return n;
}

static Node *x_for(Token *label) {
    Token *t = next();
    Node *n = mkt(N_E_FOR, label ? label : t);
    if (label) n->name = label->text;
    n->a = parse_pattern();
    expect(TK_KW_in, "`in`");
    n->b = parse_expr_no_struct();
    n->c = parse_block();
    return n;
}

/* is the current expression start one that ends at its closing brace in statement position */
static int x_starts_blocklike(void) {
    Token *t = cur();
    switch (t->kind) {
    case TK_LBRACE: case TK_KW_if: case TK_KW_match: case TK_KW_loop:
    case TK_KW_while: case TK_KW_for:
        return 1;
    case TK_KW_unsafe: case TK_KW_const:
        return peek(1)->kind == TK_LBRACE;
    case TK_LIFETIME:
        return peek(1)->kind == TK_COLON;
    default:
        return 0;
    }
}

static Node *x_match(void) {
    Token *t = next();
    Node *n = mkt(N_E_MATCH, t);
    int old = g_nostruct;
    n->a = parse_expr_no_struct();
    expect(TK_LBRACE, "`{`");
    g_nostruct = 0;
    parse_inner_attrs(&n->attrs);
    while (!at(TK_RBRACE)) {
        NodeList attrs;
        Node *arm;
        int complete = 0;
        Node *body;
        if (at(TK_EOF)) perr(cur(), "expected `}`, found end of file");
        x_nl_init(&attrs);
        parse_outer_attrs(&attrs);
        arm = mkt(N_ARM, cur());
        arm->attrs = attrs;
        arm->a = parse_pattern();
        if (accept(TK_KW_if)) arm->b = x_prec(PREC_ASSIGN);
        expect(TK_FATARROW, "`=>`");
        /* the body is parsed like an expression statement */
        body = x_stmt_mode(&complete);
        arm->c = body;
        list_push(&n->list, arm);
        if (accept(TK_COMMA)) continue;
        if (!complete && !at(TK_RBRACE))
            perr(cur(), "expected `,` or `}` after match arm, found %s", x_desc(cur()));
    }
    g_nostruct = old;
    expect(TK_RBRACE, "`}`");
    return n;
}

/* |a, b: T| expr, move |x| expr, || -> T { } */
static Node *x_closure(void) {
    Token *t = cur();
    Node *n = mkt(N_E_CLOSURE, t);
    if (accept(TK_KW_async)) n->flags |= F_ASYNC;
    if (accept(TK_KW_move)) n->flags |= F_MOVE;
    expect_prefix(TK_OR, "`|`");
    while (!at(TK_OR)) {
        Node *p = mkt(N_PARAM, cur());
        if (at(TK_POUND)) parse_outer_attrs(&p->attrs);
        p->a = parse_pattern_no_alt();
        if (accept(TK_COLON)) p->b = parse_type();
        list_push(&n->list, p);
        if (!accept(TK_COMMA)) break;
    }
    expect(TK_OR, "`|`");
    if (accept(TK_RARROW)) {
        n->a = parse_type_no_bounds();
        if (!at(TK_LBRACE)) perr(cur(), "expected `{` after closure return type, found %s", x_desc(cur()));
        n->b = parse_block();
    } else {
        n->b = x_prec(PREC_ASSIGN);
    }
    return n;
}

static Node *x_labeled(void) {
    Token *label = next();
    Node *n;
    expect(TK_COLON, "`:`");
    switch (cur()->kind) {
    case TK_KW_loop:  return x_loop(label);
    case TK_KW_while: return x_while(label);
    case TK_KW_for:   return x_for(label);
    case TK_LBRACE:
        n = parse_block();
        n->name = label->text;
        n->line = label->line;
        n->col = label->col;
        return n;
    default:
        perr(cur(), "expected `loop`, `while`, `for` or `{` after label, found %s", x_desc(cur()));
        return 0;
    }
}

/* `unsafe { }`, `const { }`, `async { }`, `async move { }` */
static Node *x_flagged_block(long flag) {
    Token *t = next();
    Node *n;
    if (flag == F_ASYNC && accept(TK_KW_move)) flag |= F_MOVE;
    n = parse_block();
    n->flags |= flag;
    n->line = t->line;
    n->col = t->col;
    return n;
}

static Node *x_primary(void) {
    Token *t = cur();
    Node *n, *e;
    int old;
    switch (t->kind) {
    case TK_INT: case TK_FLOAT: case TK_STR: case TK_BSTR: case TK_CHAR:
    case TK_BYTE: case TK_KW_true: case TK_KW_false:
        return parse_literal();
    case TK_UNDERSCORE:
        next();
        return mkt(N_E_UNDERSCORE, t);
    case TK_LPAREN:
        next();
        old = g_nostruct;
        g_nostruct = 0;
        if (accept(TK_RPAREN)) {
            g_nostruct = old;
            return mkt(N_E_TUPLE, t);
        }
        e = x_prec(PREC_ASSIGN);
        if (at(TK_COMMA)) {
            n = mkt(N_E_TUPLE, t);
            list_push(&n->list, e);
            while (accept(TK_COMMA)) {
                if (at(TK_RPAREN)) break;
                list_push(&n->list, x_prec(PREC_ASSIGN));
            }
        } else {
            n = mkt(N_E_PAREN, t);
            n->a = e;
        }
        g_nostruct = old;
        expect(TK_RPAREN, "`)`");
        return n;
    case TK_LBRACK:
        next();
        old = g_nostruct;
        g_nostruct = 0;
        if (at(TK_RBRACK)) {
            n = mkt(N_E_ARRAY, t);
        } else {
            e = x_prec(PREC_ASSIGN);
            if (accept(TK_SEMI)) {
                n = mkt(N_E_REPEAT, t);
                n->a = e;
                n->b = x_prec(PREC_ASSIGN);
            } else {
                n = mkt(N_E_ARRAY, t);
                list_push(&n->list, e);
                while (accept(TK_COMMA)) {
                    if (at(TK_RBRACK)) break;
                    list_push(&n->list, x_prec(PREC_ASSIGN));
                }
            }
        }
        g_nostruct = old;
        expect(TK_RBRACK, "`]`");
        return n;
    case TK_LBRACE:
        return parse_block();
    case TK_KW_unsafe:
        if (peek(1)->kind != TK_LBRACE) perr(t, "expected `{` after `unsafe`");
        return x_flagged_block(F_UNSAFE);
    case TK_KW_const:
        if (peek(1)->kind != TK_LBRACE) perr(peek(1), "expected `{` after `const`, found %s", x_desc(peek(1)));
        return x_flagged_block(F_CONST);
    case TK_KW_async:
        if (peek(1)->kind == TK_LBRACE ||
            (peek(1)->kind == TK_KW_move && peek(2)->kind == TK_LBRACE))
            return x_flagged_block(F_ASYNC);
        return x_closure();
    case TK_KW_move: case TK_OR: case TK_OROR:
        return x_closure();
    case TK_KW_if:
        return x_if();
    case TK_KW_match:
        return x_match();
    case TK_KW_loop:
        return x_loop(0);
    case TK_KW_while:
        return x_while(0);
    case TK_KW_for:
        return x_for(0);
    case TK_LIFETIME:
        return x_labeled();
    case TK_KW_break:
        next();
        n = mkt(N_E_BREAK, t);
        if (at(TK_LIFETIME)) n->name = next()->text;
        if (x_can_begin()) n->a = x_prec(PREC_ASSIGN);
        return n;
    case TK_KW_continue:
        next();
        n = mkt(N_E_CONTINUE, t);
        if (at(TK_LIFETIME)) n->name = next()->text;
        return n;
    case TK_KW_return:
        next();
        n = mkt(N_E_RETURN, t);
        if (x_can_begin()) n->a = x_prec(PREC_ASSIGN);
        return n;
    case TK_KW_let:
        next();
        n = mkt(N_E_LET, t);
        n->a = parse_pattern();
        expect(TK_EQ, "`=`");
        n->b = x_prec(PREC_CMP);
        return n;
    default:
        break;
    }
    if (at_path_start()) {
        Node *path = parse_path(PATH_EXPR);
        return x_path_primary(path, t);
    }
    perr(t, "expected expression, found %s", x_desc(t));
    return 0;
}

/* ---------------------------------------------------------------- statements */

/* An expression in statement position (also a match arm body): a block-like
 * expression ends where its closing brace is, unless `.` or `?` follows.
 * *complete is set when the expression ended that way (no `;` or `,` needed). */
static Node *x_stmt_mode(int *complete) {
    Token *t = cur();
    Node *e;
    *complete = 0;
    if (x_starts_blocklike()) {
        e = x_primary();
        if (at(TK_DOT) || at(TK_QUESTION)) {
            e = x_postfix(e);
            return x_rest(e, PREC_ASSIGN);
        }
        *complete = 1;
        return e;
    }
    if (t->kind == TK_IDENT || t->kind == TK_PATHSEP || t->kind == TK_KW_self ||
        t->kind == TK_KW_Self || t->kind == TK_KW_super || t->kind == TK_KW_crate) {
        Node *path = parse_path(PATH_EXPR);
        if (at(TK_NOT) && peek(1)->kind == TK_LBRACE) {
            next();
            e = parse_macro_call(path, t);
            if (!at(TK_DOT) && !at(TK_QUESTION)) {
                *complete = 1;
                return e;
            }
        } else {
            e = x_path_primary(path, t);
        }
        e = x_postfix(e);
        return x_rest(e, PREC_ASSIGN);
    }
    return x_prec(PREC_ASSIGN);
}

static void x_attrs_prepend(Node *n, NodeList *a) {
    NodeList merged;
    long i;
    if (a->n == 0) return;
    x_nl_init(&merged);
    for (i = 0; i < a->n; i++) list_push(&merged, a->v[i]);
    for (i = 0; i < n->attrs.n; i++) list_push(&merged, n->attrs.v[i]);
    n->attrs = merged;
}

static Node *x_stmt_let(NodeList *attrs) {
    Token *t = next();
    Node *n = mkt(N_STMT_LET, t);
    n->attrs = *attrs;
    n->a = parse_pattern();
    if (accept(TK_COLON)) n->b = parse_type();
    if (accept(TK_EQ)) {
        n->c = parse_expr();
        if (accept(TK_KW_else)) {
            if (!at(TK_LBRACE)) perr(cur(), "expected `{` after `else`, found %s", x_desc(cur()));
            n->d = parse_block();
        }
    }
    expect(TK_SEMI, "`;`");
    return n;
}

static Node *x_stmt1(void) {
    NodeList attrs;
    Token *t;
    Node *n, *e;
    int complete;
    x_nl_init(&attrs);
    if (at(TK_POUND)) parse_outer_attrs(&attrs);
    t = cur();
    if (t->kind == TK_SEMI) {
        next();
        n = mkt(N_STMT_EMPTY, t);
        n->attrs = attrs;
        return n;
    }
    if (t->kind == TK_KW_let) return x_stmt_let(&attrs);
    if (at_item_start()) {
        n = parse_item();
        if (n) {
            x_attrs_prepend(n, &attrs);
            return n;
        }
    }
    e = x_stmt_mode(&complete);
    if (e->kind == N_MACRO_CALL) {
        if (accept(TK_SEMI)) e->flags |= F_SEMI;
        else if (!complete && !at(TK_RBRACE) && !at(TK_EOF))
            perr(cur(), "expected `;` or `}`, found %s", x_desc(cur()));
        x_attrs_prepend(e, &attrs);
        return e;
    }
    n = mkt(N_STMT_EXPR, t);
    n->a = e;
    n->attrs = attrs;
    if (accept(TK_SEMI)) n->flags |= F_SEMI;
    else if (!complete && !at(TK_RBRACE) && !at(TK_EOF))
        perr(cur(), "expected `;` or `}`, found %s", x_desc(cur()));
    return n;
}

Node *parse_stmt(void) {
    int old = g_nostruct;
    Node *s;
    g_nostruct = 0;
    s = x_stmt1();
    g_nostruct = old;
    return s;
}

Node *parse_block(void) {
    Token *t = expect(TK_LBRACE, "`{`");
    Node *n = mkt(N_BLOCK, t);
    int old = g_nostruct;
    g_nostruct = 0;
    parse_inner_attrs(&n->attrs);
    while (!at(TK_RBRACE)) {
        Node *s;
        if (at(TK_EOF)) perr(cur(), "expected `}`, found end of file");
        s = x_stmt1();
        if (at(TK_RBRACE)) {
            if (s->kind == N_STMT_EXPR && !(s->flags & F_SEMI)) {
                x_attrs_prepend(s->a, &s->attrs);
                n->a = s->a;
                break;
            }
            if (s->kind == N_MACRO_CALL && !(s->flags & F_SEMI)) {
                n->a = s;
                break;
            }
        }
        list_push(&n->list, s);
    }
    g_nostruct = old;
    expect(TK_RBRACE, "`}`");
    return n;
}
