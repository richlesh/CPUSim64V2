/*
 * constexpr.c - constant evaluation of calls of constexpr functions.
 *
 * A call whose arguments are integral constants is evaluated by walking the
 * AST of the function body: integral and bool values, local variables,
 * the usual statements, and calls of other constexpr functions.  Anything
 * else makes the call an ordinary run-time call.
 */

#define CE_MAXVARS 64
#define CE_MAXDEPTH 400

typedef struct CeFrame CeFrame;
struct CeFrame {
    Name *name[CE_MAXVARS];
    i64 val[CE_MAXVARS];
    Type *type[CE_MAXVARS];
    int n;
};

typedef struct CeCtx CeCtx;
struct CeCtx {
    CeFrame *fr;
    int returned, broke, continued;
    i64 retval;
    long steps;
    int depth;
    int failed;
};

static i64 ce_expr(CeCtx *c, Node *n);
static void ce_stmt(CeCtx *c, Node *n);

static void ce_fail(CeCtx *c) {
    c->failed = 1;
}

static int ce_find(CeFrame *f, Name *nm) {
    int i;
    for (i = f->n - 1; i >= 0; i--)
        if (f->name[i] == nm) return i;
    return -1;
}

static i64 ce_norm(Type *t, i64 v) {
    if (t && ty_unqual(t)->kind == TY_BOOL) return v != 0;
    return v;
}

/* a variable of the enclosing function frame or a constant of the program */
static i64 ce_id(CeCtx *c, Node *n) {
    if (n->a && !n->a->next && (n->a->flags & 7) == PART_NAME && !(n->a->flags & PART_ARGS) && !(n->flags & ID_GLOBAL)) {
        int i = ce_find(c->fr, n->a->name);
        if (i >= 0) return c->fr->val[i];
    }
    {
        Val *v = expr(n);
        if (!v->is_const) ce_fail(c);
        return v->cv;
    }
}

static int ce_declare(CeCtx *c, Name *nm, Type *t, i64 v) {
    if (c->fr->n >= CE_MAXVARS) {
        ce_fail(c);
        return 0;
    }
    c->fr->name[c->fr->n] = nm;
    c->fr->val[c->fr->n] = ce_norm(t, v);
    c->fr->type[c->fr->n] = t;
    c->fr->n++;
    return 1;
}

static i64 ce_binop(CeCtx *c, int op, i64 a, i64 b, Type *t) {
    i64 r = 0;
    int uns = t && ty_is_unsigned(t);
    switch (op) {
    case P_PLUS: return (i64)((u64)a + (u64)b);
    case P_MINUS: return (i64)((u64)a - (u64)b);
    case P_STAR: return (i64)((u64)a * (u64)b);
    case P_SLASH:
    case P_PERCENT:
        if (b == 0) {
            ce_fail(c);
            return 0;
        }
        if (uns) return op == P_SLASH ? (i64)((u64)a / (u64)b) : (i64)((u64)a % (u64)b);
        return op == P_SLASH ? a / b : a % b;
    case P_AMP: return a & b;
    case P_PIPE: return a | b;
    case P_CARET: return a ^ b;
    case P_SHL: return (b < 0 || b > 63) ? 0 : (i64)((u64)a << b);
    case P_SHR: return (b < 0 || b > 63) ? 0 : (uns ? (i64)((u64)a >> b) : a >> b);
    case P_EQ: return a == b;
    case P_NE: return a != b;
    case P_LT: return uns ? (u64)a < (u64)b : a < b;
    case P_GT: return uns ? (u64)a > (u64)b : a > b;
    case P_LE: return uns ? (u64)a <= (u64)b : a <= b;
    case P_GE: return uns ? (u64)a >= (u64)b : a >= b;
    }
    ce_fail(c);
    return r;
}

static int ce_compound_op(int op) {
    switch (op) {
    case P_ADDEQ: return P_PLUS;
    case P_SUBEQ: return P_MINUS;
    case P_MULEQ: return P_STAR;
    case P_DIVEQ: return P_SLASH;
    case P_MODEQ: return P_PERCENT;
    case P_ANDEQ: return P_AMP;
    case P_OREQ: return P_PIPE;
    case P_XOREQ: return P_CARET;
    case P_SHLEQ: return P_SHL;
    case P_SHREQ: return P_SHR;
    }
    return 0;
}

static i64 ce_call(CeCtx *c, Node *n);

static i64 ce_expr(CeCtx *c, Node *n) {
    if (c->failed) return 0;
    if (++c->steps > 2000000) {
        ce_fail(c);
        return 0;
    }
    switch (n->kind) {
    case N_NUM: case N_CHARLIT: case N_BOOLLIT: case N_SIZEOF: case N_NULLPTR:
        {
            Val *v = expr(n);
            if (!v->is_const) ce_fail(c);
            return v->cv;
        }
    case N_PAREN: return ce_expr(c, n->a);
    case N_ID: return ce_id(c, n);
    case N_UNARY:
        {
            i64 a;
            if (n->op == P_INC || n->op == P_DEC) {
                int i = n->a->kind == N_ID && n->a->a && !n->a->a->next ? ce_find(c->fr, n->a->a->name) : -1;
                if (i < 0) {
                    ce_fail(c);
                    return 0;
                }
                c->fr->val[i] += n->op == P_INC ? 1 : -1;
                return c->fr->val[i];
            }
            a = ce_expr(c, n->a);
            switch (n->op) {
            case P_MINUS: return -a;
            case P_PLUS: return a;
            case P_NOT: return !a;
            case P_TILDE: return ~a;
            }
            ce_fail(c);
            return 0;
        }
    case N_POSTFIX:
        {
            int i = n->a->kind == N_ID && n->a->a && !n->a->a->next ? ce_find(c->fr, n->a->a->name) : -1;
            i64 old;
            if (i < 0) {
                ce_fail(c);
                return 0;
            }
            old = c->fr->val[i];
            c->fr->val[i] += n->op == P_INC ? 1 : -1;
            return old;
        }
    case N_BINARY:
        {
            i64 a, b;
            if (n->op == P_ANDAND) return ce_expr(c, n->a) ? (ce_expr(c, n->b) != 0) : 0;
            if (n->op == P_OROR) return ce_expr(c, n->a) ? 1 : (ce_expr(c, n->b) != 0);
            a = ce_expr(c, n->a);
            b = ce_expr(c, n->b);
            {
                /* unsigned operands: use the type of the left operand when it is a variable */
                Type *t = 0;
                if (n->a->kind == N_ID && n->a->a && !n->a->a->next) {
                    int i = ce_find(c->fr, n->a->a->name);
                    if (i >= 0) t = c->fr->type[i];
                }
                return ce_binop(c, n->op, a, b, t);
            }
        }
    case N_COND: return ce_expr(c, n->a) ? ce_expr(c, n->b) : ce_expr(c, n->c);
    case N_COMMA: ce_expr(c, n->a); return ce_expr(c, n->b);
    case N_ASSIGN:
        {
            int i = n->a->kind == N_ID && n->a->a && !n->a->a->next ? ce_find(c->fr, n->a->a->name) : -1;
            i64 r;
            if (i < 0) {
                ce_fail(c);
                return 0;
            }
            r = ce_expr(c, n->b);
            if (n->op != P_ASSIGN) {
                int bop = ce_compound_op(n->op);
                if (!bop) {
                    ce_fail(c);
                    return 0;
                }
                r = ce_binop(c, bop, c->fr->val[i], r, c->fr->type[i]);
            }
            c->fr->val[i] = ce_norm(c->fr->type[i], r);
            return c->fr->val[i];
        }
    case N_CAST: case N_NAMEDCAST: case N_FUNCCAST:
        {
            Type *t;
            i64 v;
            Node *operand = n->kind == N_FUNCCAST ? (n->a && !n->a->next ? n->a : 0) : n->a;
            if (!operand) {
                ce_fail(c);
                return 0;
            }
            t = typeid_type(n->b);
            if (!ty_is_integral(ty_unqual(t)) && ty_unqual(t)->kind != TY_ENUM) {
                ce_fail(c);
                return 0;
            }
            v = ce_expr(c, operand);
            return ce_norm(t, v);
        }
    case N_CALL: return ce_call(c, n);
    }
    ce_fail(c);
    return 0;
}

static void ce_stmts(CeCtx *c, Node *list) {
    for (; list && !c->failed && !c->returned && !c->broke && !c->continued; list = list->next) ce_stmt(c, list);
}

static void ce_decl(CeCtx *c, Node *d) {
    Node *id;
    Type *base;
    int dsflags = 0;
    if (d->kind != N_DECL) {
        ce_fail(c);
        return;
    }
    base = decl_specs_type(d->a, &dsflags);
    for (id = d->b; id; id = id->next) {
        Node *nid = 0, *fn = 0;
        Type *t = apply_declarator(base, id->a, &nid, &fn);
        i64 v = 0;
        if (!nid || !nid->a || !(ty_is_integral(ty_unqual(t)) || ty_unqual(t)->kind == TY_ENUM) || ty_is_ref(t)) {
            ce_fail(c);
            return;
        }
        if (id->b) {
            Node *ie = id->b->kind == N_INITLIST ? id->b->a : id->b;
            if (!ie || ie->next) {
                ce_fail(c);
                return;
            }
            v = ce_expr(c, ie);
        }
        ce_declare(c, nid->a->name, t, v);
    }
}

static void ce_stmt(CeCtx *c, Node *n) {
    int mark;
    if (c->failed) return;
    if (++c->steps > 2000000) {
        ce_fail(c);
        return;
    }
    switch (n->kind) {
    case N_NULL_STMT: return;
    case N_EXPR_STMT: ce_expr(c, n->a); return;
    case N_DECL_STMT: ce_decl(c, n->a); return;
    case N_BLOCK:
        mark = c->fr->n;
        ce_stmts(c, n->a);
        c->fr->n = mark;
        return;
    case N_RETURN:
        c->retval = n->a ? ce_expr(c, n->a) : 0;
        c->returned = 1;
        return;
    case N_IF:
        mark = c->fr->n;
        if (n->d) ce_stmt(c, n->d);
        if (ce_expr(c, n->a)) ce_stmt(c, n->b);
        else if (n->c) ce_stmt(c, n->c);
        c->fr->n = mark;
        return;
    case N_WHILE:
        while (!c->failed && ce_expr(c, n->a)) {
            ce_stmt(c, n->b);
            c->continued = 0;
            if (c->broke || c->returned) break;
        }
        c->broke = 0;
        return;
    case N_DO:
        do {
            ce_stmt(c, n->a);
            c->continued = 0;
            if (c->broke || c->returned) break;
        } while (!c->failed && ce_expr(c, n->b));
        c->broke = 0;
        return;
    case N_FOR:
        mark = c->fr->n;
        if (n->a) {
            if (n->a->kind == N_DECL) ce_decl(c, n->a);
            else if (n->a->kind == N_DECL_STMT) ce_decl(c, n->a->a);
            else if (n->a->kind == N_EXPR_STMT) ce_expr(c, n->a->a);
            else ce_expr(c, n->a);
        }
        while (!c->failed && (!n->b || ce_expr(c, n->b))) {
            ce_stmt(c, n->d);
            c->continued = 0;
            if (c->broke || c->returned) break;
            if (n->c) ce_expr(c, n->c);
        }
        c->broke = 0;
        c->fr->n = mark;
        return;
    case N_BREAK: c->broke = 1; return;
    case N_CONTINUE: c->continued = 1; return;
    }
    ce_fail(c);
}

/* the value of a call of a constexpr function with constant arguments; 0 if it is not a constant expression */
static int ce_run(Ent *f, Val **args, int nargs, i64 *result, int depth, long *steps) {
    CeFrame fr;
    CeCtx ctx;
    Node *p;
    int i = 0;
    if (!f->cdef || depth > CE_MAXDEPTH) return 0;
    if (nargs > f->type->nparams) return 0;
    memset(&fr, 0, sizeof fr);
    memset(&ctx, 0, sizeof ctx);
    ctx.fr = &fr;
    ctx.depth = depth;
    ctx.steps = *steps;
    for (p = f->cfn ? f->cfn->b : 0; p && i < f->type->nparams; p = p->next, i++) {
        Node *pid = 0;
        Type *pt = f->type->params[i];
        i64 v;
        if (!(ty_is_integral(ty_unqual(pt)) || ty_unqual(pt)->kind == TY_ENUM)) return 0;
        if (i < nargs) {
            if (!args[i]->is_const) return 0;
            v = args[i]->cv;
        } else if (f->defaults && f->defaults[i]) {
            Val *dv = expr(f->defaults[i]);
            if (!dv->is_const) return 0;
            v = dv->cv;
        } else return 0;
        if (p->b) apply_declarator(decl_specs_type(p->a, 0), p->b, &pid, 0);
        if (pid && pid->a) ce_declare(&ctx, pid->a->name, pt, v);
        else ce_declare(&ctx, intern_str("__unnamed"), pt, v);
    }
    if (i < f->type->nparams) return 0;
    {
        Node *body = f->cdef->d;
        Scope2 *saved = scope2_cur;
        if (!body || body->kind != N_BLOCK) return 0;
        scope2_cur = f->scope;
        ce_stmts(&ctx, body->a);
        scope2_cur = saved;
    }
    *steps = ctx.steps;
    if (ctx.failed || !ctx.returned) return 0;
    *result = ce_norm(f->type->base, ctx.retval);
    return 1;
}

static long ce_total_steps = 0;
static int ce_depth = 0;

static i64 ce_call(CeCtx *c, Node *n) {
    Val *args[16];
    int na = 0, amb;
    Node *a;
    Val *callee;
    Ent *f;
    i64 r;
    if (n->a->kind != N_ID) {
        ce_fail(c);
        return 0;
    }
    for (a = n->b; a; a = a->next) {
        i64 v;
        if (na >= 16 || a->kind == N_PACKEXP || a->kind == N_INITLIST) {
            ce_fail(c);
            return 0;
        }
        v = ce_expr(c, a);
        if (c->failed) return 0;
        args[na++] = const_val(T_LONG, v);
    }
    callee = expr(n->a);
    if (!callee->fn) {
        ce_fail(c);
        return 0;
    }
    f = resolve_overload(callee->fn, args, na, &amb);
    if (!f || !(f->flags & EF_CONSTEXPR) || ((f->flags & EF_MEMBER) && !(f->flags & EF_STATIC))) {
        ce_fail(c);
        return 0;
    }
    use_function(f);
    {
        long steps = c->steps;
        if (!ce_run(f, args, na, &r, c->depth + 1, &steps)) {
            ce_fail(c);
            return 0;
        }
        c->steps = steps;
    }
    return r;
}

/* is the call of f with these arguments a constant expression?  Then *result is its value. */
static int const_call(Ent *f, Val **args, int nargs, i64 *result) {
    Type *rt;
    long steps = 0;
    int i, ok;
    if (!(f->flags & EF_CONSTEXPR) || ((f->flags & EF_MEMBER) && !(f->flags & EF_STATIC))) return 0;
    rt = ty_unqual(f->type->base);
    if (!(ty_is_integral(rt) || rt->kind == TY_ENUM)) return 0;
    for (i = 0; i < nargs; i++)
        if (!args[i]->is_const) return 0;
    use_function(f);
    if (!f->cdef) return 0;
    if (ce_depth > CE_MAXDEPTH) return 0;
    ce_depth++;
    ok = ce_run(f, args, nargs, result, ce_depth, &steps);
    ce_depth--;
    return ok;
}
