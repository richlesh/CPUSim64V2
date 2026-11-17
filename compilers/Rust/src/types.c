/*
 * types.c - semantic types: constructors, resolution, equality, substitution,
 * unification, printing, defaulting, and the lowering of AST types.
 *
 * Representation notes (beyond sema.h):
 *   - Every structural operation (equal, unify, subst, has_infer, occurs)
 *     compares the scalar fields (kind, ik, mut, len, len_param, def, name,
 *     node) and then the children elem, args[], params[], ret in that order.
 *   - A const generic ARGUMENT (the `3` in Foo<3>) is a Type of kind TY_ARRAY
 *     with elem == 0 and len = the value (ty_const()); a const generic
 *     parameter used as an argument is a TY_PARAM whose def is a
 *     SYM_CONST_GENERIC.
 *   - TY_DYN: args = the trait's generic args.  Associated type bindings
 *     (`dyn Iterator<Item = u32>`) are in params[]: each is a TY_PROJ with
 *     args[0] = TY_SELF, def = trait, name = the associated name and
 *     ret = the bound type.
 *   - TY_PROJ: args[0] = the self type, args[1..] = the trait's generic args.
 */

/* ------------------------------------------------------------ extra API (not in sema.h) */

Type *ty_fn_def(Sym *def, Type **targs, int nt, Type **params, int np, Type *ret);
Type *ty_closure(Node *node, Type **params, int n, Type *ret);
Type *ty_dyn(Sym *trait, Type **args, int n);
Type *ty_proj(Type *self, Sym *trait, Name *name, Type **targs, int nt);
Type *ty_self(void);
Type *ty_array_param(Type *elem, Sym *len_param);
Type *ty_const(long v);
Type *ty_from_path_prefix(Node *path, int nseg);

/* when nonzero, missing generic arguments of a path become inference
 * variables instead of an error (check.c sets it around expression paths) */
int ty_infer_missing_args = 0;

/* `-> impl Trait` bookkeeping, see sema.h */
Sym *ty_impl_ret_fn = 0;
ImplRet *impl_rets = 0;
int nimpl_rets = 0;

/* ------------------------------------------------------------ constructors */

static Type *ty_new(int kind) {
    Type *t = ast_alloc(sizeof(Type));
    t->kind = kind;
    return t;
}

static Type **ty_copy_list(Type **v, int n) {
    Type **r;
    int i;
    if (n <= 0) return 0;
    r = ast_alloc(sizeof(Type *) * (size_t)n);
    for (i = 0; i < n; i++) r[i] = v[i];
    return r;
}

static Type *prim_tab[TY_SELF + 1];
static Type *int_tab[13];
static Type *float_tab[2];
static Type *unit_ty = 0;
static Type *self_ty = 0;
static int infer_counter = 0;

Type *ty_prim(int kind) {
    if (kind != TY_BOOL && kind != TY_CHAR && kind != TY_STR && kind != TY_NEVER)
        kind = TY_ERR;
    if (!prim_tab[kind]) prim_tab[kind] = ty_new(kind);
    return prim_tab[kind];
}

Type *ty_int(int ik) {
    if (ik < IK_I8 || ik > IK_USIZE) ik = IK_I32;
    if (!int_tab[ik]) {
        int_tab[ik] = ty_new(TY_INT);
        int_tab[ik]->ik = ik;
    }
    return int_tab[ik];
}

Type *ty_float(int bits) {
    int i = bits == 32 ? 0 : 1;
    if (!float_tab[i]) {
        float_tab[i] = ty_new(TY_FLOAT);
        float_tab[i]->ik = i == 0 ? 32 : 64;
    }
    return float_tab[i];
}

Type *ty_unit(void) {
    if (!unit_ty) unit_ty = ty_new(TY_TUPLE);
    return unit_ty;
}

Type *ty_self(void) {
    if (!self_ty) self_ty = ty_new(TY_SELF);
    return self_ty;
}

Type *ty_tuple(Type **args, int n) {
    Type *t;
    if (n <= 0) return ty_unit();
    t = ty_new(TY_TUPLE);
    t->args = ty_copy_list(args, n);
    t->nargs = n;
    return t;
}

Type *ty_ref(Type *elem, int mut) {
    Type *t = ty_new(TY_REF);
    t->elem = elem;
    t->mut = mut ? 1 : 0;
    return t;
}

Type *ty_ptr(Type *elem, int mut) {
    Type *t = ty_new(TY_PTR);
    t->elem = elem;
    t->mut = mut ? 1 : 0;
    return t;
}

Type *ty_array(Type *elem, long len) {
    Type *t = ty_new(TY_ARRAY);
    t->elem = elem;
    t->len = len;
    return t;
}

Type *ty_array_param(Type *elem, Sym *len_param) {
    Type *t = ty_new(TY_ARRAY);
    t->elem = elem;
    t->len_param = len_param;
    return t;
}

Type *ty_const(long v) {
    return ty_array(0, v);
}

Type *ty_slice(Type *elem) {
    Type *t = ty_new(TY_SLICE);
    t->elem = elem;
    return t;
}

Type *ty_adt(Sym *def, Type **args, int n) {
    Type *t = ty_new(TY_ADT);
    t->def = def;
    t->args = ty_copy_list(args, n);
    t->nargs = n > 0 ? n : 0;
    return t;
}

Type *ty_param(Sym *gen) {
    Type *t = ty_new(TY_PARAM);
    t->def = gen;
    return t;
}

Type *ty_infer(int ik) {
    Type *t = ty_new(TY_INFER);
    t->ik = ik;
    t->id = ++infer_counter;
    return t;
}

Type *ty_fn_ptr(Type **params, int n, Type *ret) {
    Type *t = ty_new(TY_FN_PTR);
    t->params = ty_copy_list(params, n);
    t->nparams = n > 0 ? n : 0;
    t->ret = ret ? ret : ty_unit();
    return t;
}

Type *ty_fn_def(Sym *def, Type **targs, int nt, Type **params, int np, Type *ret) {
    Type *t = ty_new(TY_FN_DEF);
    t->def = def;
    t->args = ty_copy_list(targs, nt);
    t->nargs = nt > 0 ? nt : 0;
    t->params = ty_copy_list(params, np);
    t->nparams = np > 0 ? np : 0;
    t->ret = ret ? ret : ty_unit();
    return t;
}

Type *ty_closure(Node *node, Type **params, int n, Type *ret) {
    Type *t = ty_new(TY_CLOSURE);
    t->node = node;
    t->params = ty_copy_list(params, n);
    t->nparams = n > 0 ? n : 0;
    t->ret = ret ? ret : ty_unit();
    return t;
}

Type *ty_dyn(Sym *trait, Type **args, int n) {
    Type *t = ty_new(TY_DYN);
    t->def = trait;
    t->args = ty_copy_list(args, n);
    t->nargs = n > 0 ? n : 0;
    return t;
}

Type *ty_proj(Type *self, Sym *trait, Name *name, Type **targs, int nt) {
    Type *t = ty_new(TY_PROJ);
    int i;
    t->def = trait;
    t->name = name;
    t->nargs = 1 + (nt > 0 ? nt : 0);
    t->args = ast_alloc(sizeof(Type *) * (size_t)t->nargs);
    t->args[0] = self;
    for (i = 0; i < nt; i++) t->args[1 + i] = targs[i];
    return t;
}

/* ------------------------------------------------------------ queries */

Type *ty_resolve(Type *t) {
    while (t && t->kind == TY_INFER && t->link) t = t->link;
    return t;
}

/* the non-child fields of two resolved, non-null types agree */
static int ty_scalar_eq(Type *a, Type *b) {
    return a->kind == b->kind && a->ik == b->ik && a->mut == b->mut && a->len == b->len &&
           a->len_param == b->len_param && a->def == b->def && a->name == b->name &&
           a->node == b->node && a->nargs == b->nargs && a->nparams == b->nparams &&
           (a->elem != 0) == (b->elem != 0) && (a->ret != 0) == (b->ret != 0);
}

int ty_equal(Type *a, Type *b) {
    int i;
    a = ty_resolve(a);
    b = ty_resolve(b);
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->kind == TY_INFER) return 0;
    if (!ty_scalar_eq(a, b)) return 0;
    if (a->elem && !ty_equal(a->elem, b->elem)) return 0;
    for (i = 0; i < a->nargs; i++)
        if (!ty_equal(a->args[i], b->args[i])) return 0;
    for (i = 0; i < a->nparams; i++)
        if (!ty_equal(a->params[i], b->params[i])) return 0;
    if (a->ret && !ty_equal(a->ret, b->ret)) return 0;
    return 1;
}

int ty_is_unit(Type *t) {
    t = ty_resolve(t);
    return t && t->kind == TY_TUPLE && t->nargs == 0;
}

int ty_is_integral(Type *t) {
    t = ty_resolve(t);
    return t && (t->kind == TY_INT || (t->kind == TY_INFER && t->ik == INFER_INT));
}

int ty_is_numeric(Type *t) {
    t = ty_resolve(t);
    if (!t) return 0;
    return t->kind == TY_INT || t->kind == TY_FLOAT ||
           (t->kind == TY_INFER && (t->ik == INFER_INT || t->ik == INFER_FLOAT));
}

int ty_has_infer(Type *t) {
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t->kind == TY_INFER) return 1;
    if (t->elem && ty_has_infer(t->elem)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (ty_has_infer(t->args[i])) return 1;
    for (i = 0; i < t->nparams; i++)
        if (ty_has_infer(t->params[i])) return 1;
    if (t->ret && ty_has_infer(t->ret)) return 1;
    return 0;
}

/* ------------------------------------------------------------ substitution */

static Type *subst_param(Type *t, Subst *s) {
    int i;
    for (i = 0; i < s->n; i++) {
        if (!s->args[i]) continue;
        if (s->params) {
            if (s->params[i] == t->def) return s->args[i];
        } else if (t->def && t->def->index == i) {
            return s->args[i];
        }
    }
    return t;
}

Type *ty_subst(Type *t, Subst *s) {
    Type *r, *elem, *ret, *c;
    Type **args = 0, **params = 0;
    Sym *lp;
    long len;
    int i, changed = 0;

    t = ty_resolve(t);
    if (!t || !s) return t;
    if (t->kind == TY_PARAM) return subst_param(t, s);
    if (t->kind == TY_SELF) return s->self ? s->self : t;
    if (t->kind == TY_INFER) return t;

    elem = t->elem ? ty_subst(t->elem, s) : 0;
    if (elem != t->elem) changed = 1;
    ret = t->ret ? ty_subst(t->ret, s) : 0;
    if (ret != t->ret) changed = 1;
    lp = t->len_param;
    len = t->len;
    if (t->kind == TY_ARRAY && lp) {
        Type probe;
        memset(&probe, 0, sizeof probe);
        probe.kind = TY_PARAM;
        probe.def = lp;
        c = ty_resolve(subst_param(&probe, s));
        if (c->kind == TY_PARAM) {
            if (c->def != lp) { lp = c->def; changed = 1; }
        } else if (c->kind == TY_ARRAY && !c->elem) {
            lp = 0;
            len = c->len;
            changed = 1;
        }
    }
    if (t->nargs > 0) {
        for (i = 0; i < t->nargs; i++) {
            c = ty_subst(t->args[i], s);
            if (c != t->args[i] && !args) {
                args = ast_alloc(sizeof(Type *) * (size_t)t->nargs);
                memcpy(args, t->args, sizeof(Type *) * (size_t)i);
            }
            if (args) args[i] = c;
        }
        if (args) changed = 1;
    }
    if (t->nparams > 0) {
        /* a dyn's associated bindings are projections on the object itself
           (`<Self as FnOnce>::Output = i32`): that Self is not the enclosing
           impl's Self, so it is not substituted */
        Subst dyn_s, *ps = s;
        if (t->kind == TY_DYN && s->self) {
            dyn_s = *s;
            dyn_s.self = 0;
            ps = &dyn_s;
        }
        for (i = 0; i < t->nparams; i++) {
            c = ty_subst(t->params[i], ps);
            if (c != t->params[i] && !params) {
                params = ast_alloc(sizeof(Type *) * (size_t)t->nparams);
                memcpy(params, t->params, sizeof(Type *) * (size_t)i);
            }
            if (params) params[i] = c;
        }
        if (params) changed = 1;
    }
    if (!changed) return t;
    r = ty_new(t->kind);
    *r = *t;
    r->elem = elem;
    r->ret = ret;
    r->len = len;
    r->len_param = lp;
    if (args) r->args = args;
    if (params) r->params = params;
    return r;
}

/* ------------------------------------------------------------ unification */

static Type **trail = 0;
static int ntrail = 0;
static int captrail = 0;

static void trail_bind(Type *v, Type *to) {
    if (ntrail >= captrail) {
        captrail = captrail ? captrail * 2 : 64;
        trail = xrealloc(trail, sizeof(Type *) * (size_t)captrail);
    }
    trail[ntrail++] = v;
    v->link = to;
}

static int occurs(Type *v, Type *t) {
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t == v) return 1;
    if (t->elem && occurs(v, t->elem)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (occurs(v, t->args[i])) return 1;
    for (i = 0; i < t->nparams; i++)
        if (occurs(v, t->params[i])) return 1;
    if (t->ret && occurs(v, t->ret)) return 1;
    return 0;
}

/* a and b are resolved, distinct, and at least a is an unbound variable */
static int bind_var(Type *a, Type *b) {
    if (b->kind == TY_INFER) {
        if (a->ik == 0) { trail_bind(a, b); return 1; }
        if (b->ik == 0) { trail_bind(b, a); return 1; }
        if (a->ik == b->ik) { trail_bind(a, b); return 1; }
        return 0;
    }
    if (a->ik == INFER_INT) {
        if (b->kind != TY_INT) return 0;
    } else if (a->ik == INFER_FLOAT) {
        if (b->kind != TY_FLOAT) return 0;
    } else if (occurs(a, b)) {
        return 0;
    }
    trail_bind(a, b);
    return 1;
}

static int unify_rec(Type *a, Type *b) {
    int i;
    a = ty_resolve(a);
    b = ty_resolve(b);
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->kind == TY_ERR || b->kind == TY_ERR) return 1;
    if (a->kind == TY_NEVER || b->kind == TY_NEVER) return 1;
    if (a->kind == TY_INFER) return bind_var(a, b);
    if (b->kind == TY_INFER) return bind_var(b, a);
    if (!ty_scalar_eq(a, b)) return 0;
    if (a->elem && !unify_rec(a->elem, b->elem)) return 0;
    for (i = 0; i < a->nargs; i++)
        if (!unify_rec(a->args[i], b->args[i])) return 0;
    for (i = 0; i < a->nparams; i++)
        if (!unify_rec(a->params[i], b->params[i])) return 0;
    if (a->ret && !unify_rec(a->ret, b->ret)) return 0;
    return 1;
}

/* On failure every binding made by this call is undone. */
int unify(Type *a, Type *b) {
    int mark = ntrail, r;
    r = unify_rec(a, b);
    if (!r) {
        while (ntrail > mark) trail[--ntrail]->link = 0;
    }
    ntrail = mark;
    return r;
}

/* ------------------------------------------------------------ printing */

static const char *ik_names[] = {
    "?", "i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64", "u128", "usize"
};

static void print_list(Type **v, int n, StrBuf *out) {
    int i;
    for (i = 0; i < n; i++) {
        if (i) sb_puts(out, ", ");
        ty_print(v[i], out);
    }
}

static void put_long(StrBuf *out, long v) {
    char tmp[32];
    int n = 0;
    unsigned long u = v < 0 ? 0UL - (unsigned long)v : (unsigned long)v;
    if (v < 0) sb_putc(out, '-');
    do {
        tmp[n++] = (char)('0' + (int)(u % 10));
        u /= 10;
    } while (u);
    while (n > 0) sb_putc(out, tmp[--n]);
}

/* "<A, B, Item = C>" for a dyn / trait argument list; nothing when empty */
static void print_dyn_args(Type *t, StrBuf *out) {
    int i;
    if (t->nargs == 0 && t->nparams == 0) return;
    sb_putc(out, '<');
    print_list(t->args, t->nargs, out);
    for (i = 0; i < t->nparams; i++) {
        Type *p = t->params[i];
        if (i > 0 || t->nargs > 0) sb_puts(out, ", ");
        if (p && p->name) sb_puts(out, p->name->s);
        sb_puts(out, " = ");
        if (p) ty_print(p->ret, out);
    }
    sb_putc(out, '>');
}

void ty_print(Type *t, StrBuf *out) {
    t = ty_resolve(t);
    if (!t) {
        sb_puts(out, "{null}");
        return;
    }
    switch (t->kind) {
    case TY_ERR: sb_puts(out, "{type error}"); break;
    case TY_INFER:
        if (t->ik == INFER_INT) sb_puts(out, "{integer}");
        else if (t->ik == INFER_FLOAT) sb_puts(out, "{float}");
        else sb_puts(out, "_");
        break;
    case TY_BOOL: sb_puts(out, "bool"); break;
    case TY_CHAR: sb_puts(out, "char"); break;
    case TY_INT: sb_puts(out, ik_names[t->ik >= IK_I8 && t->ik <= IK_USIZE ? t->ik : 0]); break;
    case TY_FLOAT: sb_puts(out, t->ik == 32 ? "f32" : "f64"); break;
    case TY_STR: sb_puts(out, "str"); break;
    case TY_NEVER: sb_putc(out, '!'); break;
    case TY_TUPLE:
        sb_putc(out, '(');
        print_list(t->args, t->nargs, out);
        if (t->nargs == 1) sb_putc(out, ',');
        sb_putc(out, ')');
        break;
    case TY_ARRAY:
        if (!t->elem) {                       /* a const generic argument */
            if (t->len_param) sb_puts(out, t->len_param->name->s);
            else put_long(out, t->len);
            break;
        }
        sb_putc(out, '[');
        ty_print(t->elem, out);
        sb_puts(out, "; ");
        if (t->len_param && t->len_param->name) sb_puts(out, t->len_param->name->s);
        else put_long(out, t->len);
        sb_putc(out, ']');
        break;
    case TY_SLICE:
        sb_putc(out, '[');
        ty_print(t->elem, out);
        sb_putc(out, ']');
        break;
    case TY_REF:
        sb_puts(out, t->mut ? "&mut " : "&");
        ty_print(t->elem, out);
        break;
    case TY_PTR:
        sb_puts(out, t->mut ? "*mut " : "*const ");
        ty_print(t->elem, out);
        break;
    case TY_ADT:
        sb_puts(out, t->def && t->def->name ? t->def->name->s : "{adt}");
        if (t->nargs > 0) {
            sb_putc(out, '<');
            print_list(t->args, t->nargs, out);
            sb_putc(out, '>');
        }
        break;
    case TY_FN_DEF:
    case TY_FN_PTR:
        sb_puts(out, "fn(");
        print_list(t->params, t->nparams, out);
        sb_putc(out, ')');
        if (t->ret && !ty_is_unit(t->ret)) {
            sb_puts(out, " -> ");
            ty_print(t->ret, out);
        }
        if (t->kind == TY_FN_DEF) {
            sb_puts(out, " {");
            sb_puts(out, t->def && t->def->name ? t->def->name->s : "?");
            if (t->nargs > 0) {
                sb_puts(out, "::<");
                print_list(t->args, t->nargs, out);
                sb_putc(out, '>');
            }
            sb_putc(out, '}');
        }
        break;
    case TY_CLOSURE: sb_puts(out, "{closure}"); break;
    case TY_PARAM: sb_puts(out, t->def && t->def->name ? t->def->name->s : "{param}"); break;
    case TY_SELF: sb_puts(out, "Self"); break;
    case TY_DYN:
        sb_puts(out, "dyn ");
        sb_puts(out, t->def && t->def->name ? t->def->name->s : "?");
        print_dyn_args(t, out);
        break;
    case TY_PROJ:
        sb_putc(out, '<');
        if (t->nargs > 0) ty_print(t->args[0], out);
        sb_puts(out, " as ");
        sb_puts(out, t->def && t->def->name ? t->def->name->s : "?");
        if (t->nargs > 1) {
            sb_putc(out, '<');
            print_list(t->args + 1, t->nargs - 1, out);
            sb_putc(out, '>');
        }
        sb_puts(out, ">::");
        sb_puts(out, t->name ? t->name->s : "?");
        break;
    default: sb_puts(out, "{?}"); break;
    }
}

const char *ty_str(Type *t) {
    StrBuf b;
    sb_init(&b);
    ty_print(t, &b);
    return sb_take(&b);
}

/* ------------------------------------------------------------ defaulting */

void ty_default(Type *t) {
    int i;
    t = ty_resolve(t);
    if (!t) return;
    if (t->kind == TY_INFER) {
        if (t->ik == INFER_INT) t->link = ty_int(IK_I32);
        else if (t->ik == INFER_FLOAT) t->link = ty_float(64);
        return;
    }
    if (t->elem) ty_default(t->elem);
    for (i = 0; i < t->nargs; i++) ty_default(t->args[i]);
    for (i = 0; i < t->nparams; i++) ty_default(t->params[i]);
    if (t->ret) ty_default(t->ret);
}

/* ------------------------------------------------------------ lowering of AST types */

static int name_is(Name *n, const char *s) {
    return n && strcmp(n->s, s) == 0;
}

/* the symbol of a path node: on the node, on its PATH, or on the last segment */
static Sym *path_sym(Node *holder, Node *path) {
    Node *last;
    if (holder && holder->sym) return (Sym *)holder->sym;
    if (!path) return 0;
    if (path->sym) return (Sym *)path->sym;
    if (path->list.n > 0) {
        last = path->list.v[path->list.n - 1];
        if (last->sym) return (Sym *)last->sym;
    }
    return 0;
}

static int const_eval(Node *e, long *out, int depth) {
    long x, y;
    Sym *s;
    if (!e || depth > 32) return 0;
    switch (e->kind) {
    case N_E_LIT_INT:
        *out = (long)e->ival;
        return 1;
    case N_E_PAREN:
        return const_eval(e->a, out, depth + 1);
    case N_E_PATH:
        s = path_sym(e, e->a);
        if (s && s->kind == SYM_CONST && s->node && s->node->b)
            return const_eval(s->node->b, out, depth + 1);
        return 0;
    case N_E_BINARY:
        if (!const_eval(e->a, &x, depth + 1) || !const_eval(e->b, &y, depth + 1)) return 0;
        switch (e->op) {
        case OP_ADD: *out = x + y; return 1;
        case OP_SUB: *out = x - y; return 1;
        case OP_MUL: *out = x * y; return 1;
        case OP_DIV: if (y == 0) return 0; *out = x / y; return 1;
        case OP_REM: if (y == 0) return 0; *out = x % y; return 1;
        default: return 0;
        }
    default:
        return 0;
    }
}

static Type *lower_array_len(Node *n, Type *elem) {
    Node *e = n->b;
    Sym *s;
    long v;
    while (e && e->kind == N_E_PAREN) e = e->a;
    if (e && e->kind == N_E_PATH) {
        s = path_sym(e, e->a);
        if (s && s->kind == SYM_CONST_GENERIC) return ty_array_param(elem, s);
    }
    if (e && const_eval(e, &v, 0)) {
        if (v < 0) error_at(n->file, n->line, "array length must be a constant");
        return ty_array(elem, v);
    }
    error_at(n->file, n->line, "array length must be a constant");
    return 0;
}

static Type *lower_path_type(Node *t, Node *path, int upto, Sym *sym);

/* a generic argument: a type, or a const */
static Type *lower_garg(Node *a) {
    long v;
    Sym *s;
    if (a->kind >= N_T_PATH && a->kind <= N_T_INFER) return ty_from_ast(a);
    if (a->kind == N_E_LIT_INT) return ty_const((long)a->ival);
    if (a->kind == N_E_PATH) {
        s = path_sym(a, a->a);
        if (s && s->kind == SYM_CONST_GENERIC) return ty_param(s);
    }
    if (const_eval(a, &v, 0)) return ty_const(v);
    error_at(a->file, a->line, "unsupported generic argument");
    return 0;
}

/* the declaration node (GEN_TYPE / GEN_CONST) of the i-th type/const parameter */
static Node *gen_decl(Sym *sym, int i) {
    Node *g;
    long k, j = 0;
    if (sym->gparams && i < sym->ngparams && sym->gparams[i] && sym->gparams[i]->node)
        return sym->gparams[i]->node;
    if (!sym->node || !sym->node->a || sym->node->a->kind != N_GENERICS) return 0;
    for (k = 0; k < sym->node->a->list.n; k++) {
        g = sym->node->a->list.v[k];
        if (g->kind == N_GEN_LIFETIME) continue;
        if (j++ == i) return g;
    }
    return 0;
}

static int count_params(Sym *sym) {
    long k;
    int n = 0;
    if (sym->ngparams > 0) return sym->ngparams;
    if (!sym->node || !sym->node->a || sym->node->a->kind != N_GENERICS) return 0;
    for (k = 0; k < sym->node->a->list.n; k++)
        if (sym->node->a->list.v[k]->kind != N_GEN_LIFETIME) n++;
    return n;
}

/* lowers the generic arguments of segment `seg` for the item `sym`, filling
 * defaults / inference variables; returns the count, args in *out */
static int lower_args(Sym *sym, Node *seg, Node *errn, Type *self, Type ***out) {
    Type **args, *d;
    Node *g;
    Subst s;
    long k;
    int np = count_params(sym), given = 0, i;

    args = np > 0 ? ast_alloc(sizeof(Type *) * (size_t)np) : 0;
    if (seg) {
        for (k = 0; k < seg->list.n; k++) {
            Node *a = seg->list.v[k];
            if (a->kind == N_LIFETIME || a->kind == N_ASSOC_BIND) continue;
            if (given >= np) {
                error_at(errn->file, errn->line,
                         "%s `%s` takes %d generic argument%s but more were supplied",
                         sym_kind_name(sym->kind), sym->name ? sym->name->s : "?", np,
                         np == 1 ? "" : "s");
            }
            args[given++] = lower_garg(a);
        }
    }
    for (i = given; i < np; i++) {
        g = gen_decl(sym, i);
        d = 0;
        if (g && g->kind == N_GEN_TYPE && g->a) d = ty_from_ast(g->a);
        else if (g && g->kind == N_GEN_CONST && g->b) d = lower_garg(g->b);
        if (d) {
            s.params = sym->gparams;
            s.args = args;
            s.n = i;
            s.self = self;
            args[i] = sym->gparams ? ty_subst(d, &s) : d;
            if (self) {
                Subst s2;
                s2.params = 0; s2.args = 0; s2.n = 0; s2.self = self;
                args[i] = ty_subst(args[i], &s2);
            }
        } else if (ty_infer_missing_args) {
            if (sym->gparams && i < sym->ngparams && sym->gparams[i] &&
                sym->gparams[i]->kind == SYM_CONST_GENERIC) {
                /* an inferred const argument is an array-length variable (see check.c) */
                Sym *lv = sym_new(SYM_CONST_GENERIC, name_intern_str("{len}"), 0, 0);
                args[i] = ty_array(0, 0);
                args[i]->len_param = lv;
            } else {
                args[i] = ty_infer(0);
            }
        } else {
            error_at(errn->file, errn->line, "missing generics for %s `%s`",
                     sym_kind_name(sym->kind), sym->name ? sym->name->s : "?");
        }
    }
    *out = args;
    return np;
}

static int prim_ik_by_name(Name *n) {
    int i;
    if (!n) return 0;
    for (i = IK_I8; i <= IK_USIZE; i++)
        if (strcmp(n->s, ik_names[i]) == 0) return i;
    return 0;
}

static Type *prim_type(Sym *sym, Node *errn) {
    int ik;
    switch (sym->prim) {
    case TY_BOOL: case TY_CHAR: case TY_STR: case TY_NEVER:
        return ty_prim(sym->prim);
    case TY_INT:
        ik = sym->ik >= IK_I8 && sym->ik <= IK_USIZE ? sym->ik : prim_ik_by_name(sym->name);
        if (!ik) break;
        return ty_int(ik);
    case TY_FLOAT:
        if (sym->ik == 32 || name_is(sym->name, "f32")) return ty_float(32);
        return ty_float(64);
    default:
        break;
    }
    error_at(errn->file, errn->line, "unknown primitive type `%s`", sym->name ? sym->name->s : "?");
    return 0;
}

static Sym *alias_stack[64];
static int nalias = 0;

/* the aliased type of a type alias / impl associated type, lowered on demand */
static Type *alias_target(Sym *sym, Node *errn) {
    int i;
    Type *t;
    if (sym->ty) return sym->ty;
    if (!sym->node || !sym->node->b) {
        error_at(errn->file, errn->line, "`%s` has no definition here", sym->name ? sym->name->s : "?");
    }
    for (i = 0; i < nalias; i++)
        if (alias_stack[i] == sym)
            error_at(errn->file, errn->line, "cycle detected when expanding type alias `%s`",
                     sym->name ? sym->name->s : "?");
    if (nalias >= 64) error_at(errn->file, errn->line, "type alias nesting too deep");
    alias_stack[nalias++] = sym;
    t = ty_from_ast(sym->node->b);
    nalias--;
    sym->ty = t;
    return t;
}

static Type *self_type_of(Sym *sym, Node *errn) {
    Sym *p = sym->parent;
    if (p) {
        if (p->kind == SYM_TRAIT) return ty_self();
        if (p->kind == SYM_IMPL && p->impl_self) return p->impl_self;
        if ((p->kind == SYM_STRUCT || p->kind == SYM_ENUM || p->kind == SYM_UNION) && p->ty)
            return p->ty;
    }
    error_at(errn->file, errn->line, "the type `Self` is not available here");
    return 0;
}

/* the trait that owns the associated type `sym` */
static Sym *assoc_trait(Sym *sym) {
    Sym *p = sym->parent;
    if (!p) return 0;
    if (p->kind == SYM_TRAIT) return p;
    if (p->kind == SYM_IMPL) return p->trait;
    return 0;
}

/* the type named by segments 0..upto of path (a prefix such as `T` in `T::Item`) */
static Type *lower_prefix(Node *t, Node *path, int upto) {
    Node *seg = path->list.v[upto];
    Sym *s = upto == path->list.n - 1 ? path_sym(t, path) : (Sym *)seg->sym;
    if (!s) error_at(t->file, t->line, "failed to resolve type path `%s`", seg->name ? seg->name->s : "?");
    return lower_path_type(t, path, upto, s);
}

static Type *lower_trait_dyn(Node *t, Node *seg, Sym *trait) {
    Type **args, *dyn;
    Type **bind = 0;
    int n, nb = 0, cap = 0;
    long k;
    if (seg && (seg->flags & F_PARENS)) {
        /* Fn(A, B) -> C sugar */
        Type **in = seg->list.n ? ast_alloc(sizeof(Type *) * (size_t)seg->list.n) : 0;
        Type *tup, *out, *p;
        for (k = 0; k < seg->list.n; k++) in[k] = ty_from_ast(seg->list.v[k]);
        tup = ty_tuple(in, (int)seg->list.n);
        out = seg->a ? ty_from_ast(seg->a) : ty_unit();
        dyn = ty_dyn(trait, 0, 0);
        dyn->args = ast_alloc(sizeof(Type *));
        dyn->args[0] = tup;
        dyn->nargs = 1;
        p = ty_proj(ty_self(), trait, name_intern_str("Output"), 0, 0);
        p->ret = out;
        dyn->params = ast_alloc(sizeof(Type *));
        dyn->params[0] = p;
        dyn->nparams = 1;
        return dyn;
    }
    n = lower_args(trait, seg, t, ty_self(), &args);
    dyn = ty_dyn(trait, args, n);
    if (seg) {
        for (k = 0; k < seg->list.n; k++) {
            Node *a = seg->list.v[k];
            Type *p;
            if (a->kind != N_ASSOC_BIND || !a->a) continue;
            if (nb >= cap) {
                Type **nbv = ast_alloc(sizeof(Type *) * (size_t)(cap ? cap * 2 : 4));
                if (nb) memcpy(nbv, bind, sizeof(Type *) * (size_t)nb);
                bind = nbv;
                cap = cap ? cap * 2 : 4;
            }
            p = ty_proj(ty_self(), trait, a->name, 0, 0);
            p->ret = ty_from_ast(a->a);
            bind[nb++] = p;
        }
    }
    dyn->params = bind;
    dyn->nparams = nb;
    return dyn;
}

static Type *lower_path_type(Node *t, Node *path, int upto, Sym *sym) {
    Node *seg = path->list.v[upto];
    Type **args, *self;
    Sym *tr;
    int n;

    switch (sym->kind) {
    case SYM_PRIM:
        return prim_type(sym, t);
    case SYM_STRUCT: case SYM_ENUM: case SYM_UNION:
        n = lower_args(sym, seg, t, 0, &args);
        return ty_adt(sym, args, n);
    case SYM_GENERIC: case SYM_CONST_GENERIC:
        return ty_param(sym);
    case SYM_SELF_TY:
        return self_type_of(sym, t);
    case SYM_TYPEALIAS: {
        Type *target = alias_target(sym, t);
        Subst s;
        n = lower_args(sym, seg, t, 0, &args);
        s.params = sym->gparams;
        s.args = args;
        s.n = n;
        s.self = 0;
        if (n > 0 && !sym->gparams)
            error_at(t->file, t->line, "type alias `%s` is generic but its parameters are not collected",
                     sym->name ? sym->name->s : "?");
        return n > 0 ? ty_subst(target, &s) : target;
    }
    case SYM_ASSOC_TYPE:
        if (upto == 0) {
            error_at(t->file, t->line, "ambiguous associated type `%s`",
                     sym->name ? sym->name->s : "?");
        }
        self = lower_prefix(t, path, upto - 1);
        if (sym->parent && sym->parent->kind == SYM_IMPL) {
            /* `Self::X` inside the impl that defines X */
            Type *sp = ty_resolve(sym->parent->impl_self);
            if (sp && ty_equal(sp, self) && (sym->ty || (sym->node && sym->node->b)))
                return alias_target(sym, t);
        }
        tr = assoc_trait(sym);
        if (!tr) error_at(t->file, t->line, "ambiguous associated type `%s`", sym->name ? sym->name->s : "?");
        return ty_proj(self, tr, sym->name, 0, 0);
    case SYM_TRAIT:
        return lower_trait_dyn(t, seg, sym);
    case SYM_CONST: {
        long v;
        if (sym->node && sym->node->b && const_eval(sym->node->b, &v, 0)) return ty_const(v);
        error_at(t->file, t->line, "expected type, found constant `%s`", sym->name ? sym->name->s : "?");
        return 0;
    }
    default:
        error_at(t->file, t->line, "expected type, found %s `%s`", sym_kind_name(sym->kind),
                 sym->name ? sym->name->s : "?");
        return 0;
    }
}

static Type *lower_qualified(Node *t, Node *path) {
    Type *self, *cur, **targs = 0;
    Sym *tr = 0;
    int i, nt = 0;
    Node *seg;
    Sym *ss;

    self = ty_from_ast(path->a);
    if (path->list.n == 0) error_at(t->file, t->line, "expected an associated type name");
    if (path->b) {
        Node *tseg = path->b->list.n ? path->b->list.v[path->b->list.n - 1] : 0;
        tr = path_sym(0, path->b);
        if (!tr || tr->kind != SYM_TRAIT)
            error_at(t->file, t->line, "expected a trait in qualified path");
        nt = lower_args(tr, tseg, t, self, &targs);
    }
    cur = self;
    for (i = 0; i < (int)path->list.n; i++) {
        seg = path->list.v[i];
        ss = (Sym *)seg->sym;
        if (i == path->list.n - 1 && !ss) ss = path_sym(t, 0);
        if (i > 0 || !tr) {
            tr = ss ? assoc_trait(ss) : 0;
            if (!tr)
                error_at(t->file, t->line, "ambiguous associated type `%s`", seg->name ? seg->name->s : "?");
            nt = 0;
            targs = 0;
        }
        cur = ty_proj(cur, tr, seg->name, targs, nt);
        tr = 0;
    }
    return cur;
}

/* the type named by the first nseg segments of a (resolved) path */
Type *ty_from_path_prefix(Node *path, int nseg) {
    Node *seg, *t = path;
    Sym *s;
    if (nseg < 1 || nseg > path->list.n) error_at(path->file, path->line, "bad path prefix");
    seg = path->list.v[nseg - 1];
    s = nseg == path->list.n ? path_sym(0, path) : (Sym *)seg->sym;
    if (!s) error_at(t->file, t->line, "failed to resolve type path `%s`", seg->name ? seg->name->s : "?");
    return lower_path_type(t, path, nseg - 1, s);
}

static int is_marker(Sym *s) {
    return s && s->name &&
           (name_is(s->name, "Send") || name_is(s->name, "Sync") || name_is(s->name, "Unpin") ||
            name_is(s->name, "UnwindSafe") || name_is(s->name, "RefUnwindSafe"));
}

static Type *lower_dyn(Node *t) {
    Node *bounds = t->a, *b, *path, *first_marker = 0, *pick = 0;
    long k;
    Sym *s;
    if (bounds) {
        for (k = 0; k < bounds->list.n; k++) {
            b = bounds->list.v[k];
            if (b->kind != N_BOUND_TRAIT || (b->flags & F_MAYBE)) continue;
            s = path_sym(b, b->a);
            if (!s || s->kind != SYM_TRAIT)
                error_at(b->file, b->line, "expected a trait in `dyn` type");
            if (is_marker(s)) {
                if (!first_marker) first_marker = b;
            } else if (!pick) {
                pick = b;
            }
        }
    }
    if (!pick) pick = first_marker;
    if (!pick) error_at(t->file, t->line, "at least one trait is required for an object type");
    path = pick->a;
    s = path_sym(pick, path);
    return lower_trait_dyn(t, path && path->list.n ? path->list.v[path->list.n - 1] : 0, s);
}

Type *ty_from_ast(Node *t) {
    Type **v, *e;
    Node *path;
    long k;

    if (!t) return ty_unit();
    switch (t->kind) {
    case N_T_PATH:
        path = t->a;
        if (!path || path->kind != N_PATH || path->list.n == 0)
            error_at(t->file, t->line, "malformed type path");
        if (path->flags & F_QUALIFIED) return lower_qualified(t, path);
        {
            Sym *s = path_sym(t, path);
            if (!s)
                error_at(t->file, t->line, "failed to resolve type `%s`",
                         path->list.v[path->list.n - 1]->name ? path->list.v[path->list.n - 1]->name->s : "?");
            return lower_path_type(t, path, (int)path->list.n - 1, s);
        }
    case N_T_REF:
        return ty_ref(ty_from_ast(t->a), (t->flags & F_MUT) != 0);
    case N_T_PTR:
        return ty_ptr(ty_from_ast(t->a), (t->flags & F_MUT) != 0);
    case N_T_ARRAY:
        e = ty_from_ast(t->a);
        return lower_array_len(t, e);
    case N_T_SLICE:
        return ty_slice(ty_from_ast(t->a));
    case N_T_TUPLE:
        if (t->list.n == 0) return ty_unit();
        v = ast_alloc(sizeof(Type *) * (size_t)t->list.n);
        for (k = 0; k < t->list.n; k++) v[k] = ty_from_ast(t->list.v[k]);
        return ty_tuple(v, (int)t->list.n);
    case N_T_NEVER:
        return ty_prim(TY_NEVER);
    case N_T_FN:
        v = t->list.n ? ast_alloc(sizeof(Type *) * (size_t)t->list.n) : 0;
        for (k = 0; k < t->list.n; k++) {
            Node *p = t->list.v[k];
            v[k] = ty_from_ast(p->kind == N_PARAM ? p->b : p);
        }
        return ty_fn_ptr(v, (int)t->list.n, t->b ? ty_from_ast(t->b) : ty_unit());
    case N_T_DYN:
        return lower_dyn(t);
    case N_T_IMPL:
        if (ty_impl_ret_fn) {
            int ri;
            Type *h;
            for (ri = 0; ri < nimpl_rets; ri++)
                if (impl_rets[ri].fn == ty_impl_ret_fn && impl_rets[ri].node == t) return impl_rets[ri].hidden;
            h = ty_infer(0);
            impl_rets = (ImplRet *)xrealloc(impl_rets, sizeof(ImplRet) * (size_t)(nimpl_rets + 1));
            impl_rets[nimpl_rets].fn = ty_impl_ret_fn;
            impl_rets[nimpl_rets].node = t;
            impl_rets[nimpl_rets].hidden = h;
            impl_rets[nimpl_rets].bound = lower_dyn(t);
            nimpl_rets++;
            return h;
        }
        error_at(t->file, t->line, "`impl Trait` is only allowed in function argument and return types");
        return 0;
    case N_T_INFER:
        return ty_infer(0);
    default:
        error_at(t->file, t->line, "expected a type");
        return 0;
    }
}

/* ------------------------------------------------------------ dump hook */

void sema_dump_hook(FILE *f, Node *n) {
    StrBuf b;
    const char *s;
    if (n->sym) {
        sb_init(&b);
        sym_path((Sym *)n->sym, &b);
        fputs(" ->", f);
#ifdef __CPUSIM64__
        for (s = b.s; *s; s++) fputc(*s, f);
#else
        fputs(b.s, f);
#endif
        free(b.s);
    }
    if (n->ty) {
        s = ty_str((Type *)n->ty);
        fputs(" :", f);
#ifdef __CPUSIM64__
        while (*s) fputc(*s++, f);
#else
        fputs(s, f);
#endif
    }
}
