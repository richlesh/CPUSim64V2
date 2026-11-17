/*
 * check.c - type checking of every body (see sema.h, stage 4).
 *
 * check_crate() checks every fn body, const and static of both crates.  It
 * sets Node.ty on every expression and pattern, Sym.ty on every binding, and
 * records what code generation needs:
 *
 *   node->sym   E_PATH: the item an associated path names (fn / const / variant);
 *               E_METHOD_CALL: the method (the inherent fn, or the TRAIT's fn);
 *               E_FIELD / E_TUPLE_FIELD: the field Sym (0 for a tuple);
 *               overloaded E_BINARY / E_UNARY / E_ASSIGN_OP / E_INDEX / E_DEREF:
 *               the trait method (add, neg, index, deref, eq, ...).
 *   node->adj   the implicit adjustments (ADJ_xxx in sema.h) applied to the VALUE
 *               OF THAT NODE: coercions of an argument / initializer / branch,
 *               and for E_METHOD_CALL / E_FIELD / E_TUPLE_FIELD / E_INDEX the
 *               auto-deref / auto-ref of the receiver / base, stored on the
 *               receiver (node->a->adj).  Low bits = number of derefs (builtin
 *               for & / &mut / Box via Deref: look at the types); ADJ_AUTOREF[_MUT]
 *               afterwards; ADJ_UNSIZE last ([T; N] -> [T] before the autoref).
 *               Patterns: P_xxx->adj low bits = references peeled by default
 *               binding modes; a P_IDENT that binds by reference has
 *               ADJ_AUTOREF / ADJ_AUTOREF_MUT.
 *   callees     check_callee(node, slot) gives the callee of an overloaded or
 *               desugared operation: the fn Sym (a trait method when the impl is
 *               not known yet, else the impl's fn), the instantiated TY_FN_DEF
 *               (args = the fn's generic args, impl's / trait's first) and the Self
 *               type for trait methods.  Slots: method calls, calls and operators 0;
 *               E_FOR: 0 = IntoIterator::into_iter, 1 = Iterator::next;
 *               E_TRY: 0 = From::from; E_CALL through Fn bounds / closures: 0 = call.
 *   closures    closure_captures(node, &caps): the captured locals and modes.
 *
 * Errors stop at the first one (error_at), rustc wording.
 */
#include "sema.h"

typedef struct Callee {
    Node *node;
    int slot;
    Sym *fn;
    Type *fnty;
    Type *self;
    struct Callee *next;
} Callee;

typedef struct Capture {
    Sym *local;
    int mode;                /* CAP_REF / CAP_MUT / CAP_MOVE */
} Capture;
enum { CAP_REF = 1, CAP_MUT = 2, CAP_MOVE = 3 };

Callee *check_callee(Node *n, int slot);
int closure_captures(Node *closure, Capture **out);

/* hook set by collect.c: does the closure / fn item type implement the trait (Fn*, Clone, Copy)? */
int (*collect_closure_impl)(Type *, Sym *);
void collect_set_env(Sym *fn_or_impl);

/* ================================================================== state */

static Name *n_lenvar;
static Name *nm_output, *nm_self_l;
static int core_file = -1;
static int in_core = 0;

#define TBOOL  (ty_prim(TY_BOOL))
#define TCHAR  (ty_prim(TY_CHAR))
#define TSTR   (ty_prim(TY_STR))
#define TNEVER (ty_prim(TY_NEVER))
#define TUNIT  (ty_unit())
#define TUSIZE (ty_int(IK_USIZE))

static Type **tvec(int n) {
    return n > 0 ? (Type **)ast_alloc(sizeof(Type *) * (size_t)n) : 0;
}

/* ---------------------------------------------------------------- messages */

static const char *tn(Type *t) {
    return ty_str(t);
}

/* the "found" side of a mismatch: rustc says "integer" for {integer} */
static const char *tn_found(Type *t) {
    Type *r = ty_resolve(t);
    if (r && r->kind == TY_INFER && r->ik == INFER_INT) return "integer";
    if (r && r->kind == TY_INFER && r->ik == INFER_FLOAT) return "floating-point number";
    return ty_str(t);
}

static void mismatch(Node *n, Type *expected, Type *found) {
    error_at(n->file, n->line, "mismatched types: expected `%s`, found `%s`",
             tn(expected), tn_found(found));
}

static const char *kind_word(Type *t) {
    t = ty_resolve(t);
    if (!t) return "type";
    switch (t->kind) {
    case TY_ADT:
        if (t->def && t->def->kind == SYM_ENUM) return "enum";
        if (t->def && t->def->kind == SYM_UNION) return "union";
        return "struct";
    case TY_REF: return "reference";
    case TY_PARAM: return "type parameter";
    case TY_TUPLE: return t->nargs == 0 ? "unit type" : "tuple";
    case TY_ARRAY: return "array";
    case TY_SLICE: return "slice";
    case TY_PTR: return "raw pointer";
    }
    return "type";
}

/* ================================================================== unification */

typedef struct { Type *t; Sym *s; } TrailEnt;
static TrailEnt *ctrail = 0;
static int nct = 0, cct = 0;

static void ct_push(Type *t, Sym *s) {
    if (nct >= cct) {
        cct = cct ? cct * 2 : 256;
        ctrail = (TrailEnt *)xrealloc(ctrail, sizeof(TrailEnt) * (size_t)cct);
    }
    ctrail[nct].t = t;
    ctrail[nct].s = s;
    nct++;
}

static int snap(void) {
    return nct;
}

static void rollback(int m) {
    while (nct > m) {
        nct--;
        if (ctrail[nct].t) ctrail[nct].t->link = 0;
        else ctrail[nct].s->ty = 0;
    }
}

static void bind_to(Type *v, Type *to) {
    ct_push(v, 0);
    v->link = to;
}

static int occurs_in(Type *v, Type *t) {
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t == v) return 1;
    if (t->elem && occurs_in(v, t->elem)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (occurs_in(v, t->args[i])) return 1;
    for (i = 0; i < t->nparams; i++)
        if (occurs_in(v, t->params[i])) return 1;
    if (t->ret && occurs_in(v, t->ret)) return 1;
    return 0;
}

static int bind_var2(Type *a, Type *b) {
    if (b->kind == TY_INFER) {
        if (a->ik == 0) { bind_to(a, b); return 1; }
        if (b->ik == 0) { bind_to(b, a); return 1; }
        if (a->ik == b->ik) { bind_to(a, b); return 1; }
        return 0;
    }
    if (a->ik == INFER_INT) {
        if (b->kind != TY_INT) return 0;
    } else if (a->ik == INFER_FLOAT) {
        if (b->kind != TY_FLOAT) return 0;
    } else if (occurs_in(a, b)) {
        return 0;
    }
    bind_to(a, b);
    return 1;
}

/* array-length variables: a SYM_CONST_GENERIC named n_lenvar whose ty (when bound) is a
   const type (TY_ARRAY with elem == 0: len, or len_param of a real const generic) */
static int is_lenvar(Sym *s) {
    return s && s->name == n_lenvar;
}

static Type *new_lenvar(void) {
    Sym *s = sym_new(SYM_CONST_GENERIC, n_lenvar, 0, 0);
    Type *t = ty_array(0, 0);
    t->len_param = s;
    return t;
}

/* the length of an array / const type, following bound length variables */
static void lenres(Type *arr, long *len, Sym **lp) {
    long l = arr->len;
    Sym *p = arr->len_param;
    while (p && is_lenvar(p) && p->ty) {
        Type *c = ty_resolve(p->ty);
        l = c->len;
        p = c->len_param;
    }
    *len = l;
    *lp = p;
}

static int uni_len(Type *a, Type *b) {
    long la, lb;
    Sym *pa, *pb;
    Type *c;
    lenres(a, &la, &pa);
    lenres(b, &lb, &pb);
    if (pa && pa == pb) return 1;
    if (pa && is_lenvar(pa)) {
        c = ty_array(0, lb);
        c->len_param = pb;
        ct_push(0, pa);
        pa->ty = c;
        return 1;
    }
    if (pb && is_lenvar(pb)) {
        c = ty_array(0, la);
        c->len_param = pa;
        ct_push(0, pb);
        pb->ty = c;
        return 1;
    }
    if (pa || pb) return 0;
    return la == lb;
}

static int cu(Type *a, Type *b) {
    int i;
    a = ty_resolve(a);
    b = ty_resolve(b);
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->kind == TY_ERR || b->kind == TY_ERR) return 1;
    if (a->kind == TY_NEVER || b->kind == TY_NEVER) return 1;
    if (a->kind == TY_INFER) return bind_var2(a, b);
    if (b->kind == TY_INFER) return bind_var2(b, a);
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
    case TY_BOOL: case TY_CHAR: case TY_STR: case TY_SELF:
        return 1;
    case TY_INT: case TY_FLOAT:
        return a->ik == b->ik;
    case TY_TUPLE:
        if (a->nargs != b->nargs) return 0;
        for (i = 0; i < a->nargs; i++)
            if (!cu(a->args[i], b->args[i])) return 0;
        return 1;
    case TY_ARRAY:
        if ((a->elem != 0) != (b->elem != 0)) return 0;
        if (a->elem && !cu(a->elem, b->elem)) return 0;
        return uni_len(a, b);
    case TY_SLICE:
        return cu(a->elem, b->elem);
    case TY_REF: case TY_PTR:
        return a->mut == b->mut && cu(a->elem, b->elem);
    case TY_ADT:
        if (a->def != b->def || a->nargs != b->nargs) return 0;
        for (i = 0; i < a->nargs; i++)
            if (!cu(a->args[i], b->args[i])) return 0;
        return 1;
    case TY_FN_DEF:
        if (a->def != b->def || a->nargs != b->nargs) return 0;
        for (i = 0; i < a->nargs; i++)
            if (!cu(a->args[i], b->args[i])) return 0;
        return 1;
    case TY_FN_PTR:
        if (a->nparams != b->nparams) return 0;
        for (i = 0; i < a->nparams; i++)
            if (!cu(a->params[i], b->params[i])) return 0;
        return cu(a->ret, b->ret);
    case TY_CLOSURE:
        return a->node == b->node;
    case TY_PARAM:
        return a->def == b->def;
    case TY_DYN: {
        int j;
        if (a->def != b->def || a->nargs != b->nargs) return 0;
        for (i = 0; i < a->nargs; i++)
            if (!cu(a->args[i], b->args[i])) return 0;
        for (i = 0; i < a->nparams; i++)
            for (j = 0; j < b->nparams; j++)
                if (a->params[i]->name == b->params[j]->name &&
                    !cu(a->params[i]->ret, b->params[j]->ret)) return 0;
        return 1;
    }
    case TY_PROJ:
        if (a->def != b->def || a->name != b->name || a->nargs != b->nargs) return 0;
        for (i = 0; i < a->nargs; i++)
            if (!cu(a->args[i], b->args[i])) return 0;
        return 1;
    }
    return 0;
}

/* unify; on failure nothing stays bound */
static int uni(Type *a, Type *b) {
    int m = snap();
    if (cu(a, b)) return 1;
    rollback(m);
    return 0;
}

/* ================================================================== zonk and substitution */

static Type *copy_type(Type *t) {
    Type *r = (Type *)ast_alloc(sizeof(Type));
    *r = *t;
    return r;
}

/* a deep copy with every bound inference variable and array length replaced by its value */
static Type *zonk(Type *t) {
    Type *r, *c, *elem, *ret;
    Type **args = 0, **params = 0;
    Sym *lp;
    long len;
    int i, changed = 0;

    t = ty_resolve(t);
    if (!t) return t;
    switch (t->kind) {
    case TY_ERR: case TY_INFER: case TY_BOOL: case TY_CHAR: case TY_INT: case TY_FLOAT:
    case TY_STR: case TY_NEVER: case TY_PARAM: case TY_SELF: case TY_CLOSURE:
        return t;
    }
    elem = t->elem ? zonk(t->elem) : 0;
    if (elem != t->elem) changed = 1;
    ret = t->ret ? zonk(t->ret) : 0;
    if (ret != t->ret) changed = 1;
    lp = t->len_param;
    len = t->len;
    if (t->kind == TY_ARRAY) {
        lenres(t, &len, &lp);
        if (len != t->len || lp != t->len_param) changed = 1;
    }
    if (t->nargs > 0) {
        for (i = 0; i < t->nargs; i++) {
            c = zonk(t->args[i]);
            if (c != t->args[i] && !args) {
                args = tvec(t->nargs);
                memcpy(args, t->args, sizeof(Type *) * (size_t)i);
            }
            if (args) args[i] = c;
        }
        if (args) changed = 1;
    }
    if (t->nparams > 0) {
        for (i = 0; i < t->nparams; i++) {
            c = zonk(t->params[i]);
            if (c != t->params[i] && !params) {
                params = tvec(t->nparams);
                memcpy(params, t->params, sizeof(Type *) * (size_t)i);
            }
            if (params) params[i] = c;
        }
        if (params) changed = 1;
    }
    if (!changed) return t;
    r = copy_type(t);
    r->elem = elem;
    r->ret = ret;
    r->len = len;
    r->len_param = lp;
    if (args) r->args = args;
    if (params) r->params = params;
    return r;
}

/* substitute generic parameters (by Sym) and Self.  Unlike ty_subst this knows array
   length variables and leaves the Self of a dyn's associated-type bindings alone. */
static Type *isub(Type *t, Subst *s) {
    Type *r, *c, *elem, *ret;
    Type **args = 0, **params = 0;
    Sym *lp;
    long len;
    int i, changed = 0;

    t = ty_resolve(t);
    if (!t || !s) return t;
    switch (t->kind) {
    case TY_PARAM:
        for (i = 0; i < s->n; i++)
            if (s->params[i] == t->def && s->args[i]) return s->args[i];
        return t;
    case TY_SELF:
        return s->self ? s->self : t;
    case TY_ERR: case TY_INFER: case TY_BOOL: case TY_CHAR: case TY_INT: case TY_FLOAT:
    case TY_STR: case TY_NEVER: case TY_CLOSURE:
        return t;
    }
    elem = t->elem ? isub(t->elem, s) : 0;
    if (elem != t->elem) changed = 1;
    ret = t->ret ? isub(t->ret, s) : 0;
    if (ret != t->ret) changed = 1;
    lp = t->len_param;
    len = t->len;
    if (t->kind == TY_ARRAY) {
        lenres(t, &len, &lp);
        if (lp && !is_lenvar(lp)) {
            for (i = 0; i < s->n; i++) {
                if (s->params[i] != lp || !s->args[i]) continue;
                c = ty_resolve(s->args[i]);
                if (c->kind == TY_PARAM) { lp = c->def; }
                else if (c->kind == TY_ARRAY && !c->elem) { lenres(c, &len, &lp); }
                break;
            }
        }
        if (len != t->len || lp != t->len_param) changed = 1;
    }
    if (t->nargs > 0) {
        for (i = 0; i < t->nargs; i++) {
            c = isub(t->args[i], s);
            if (c != t->args[i] && !args) {
                args = tvec(t->nargs);
                memcpy(args, t->args, sizeof(Type *) * (size_t)i);
            }
            if (args) args[i] = c;
        }
        if (args) changed = 1;
    }
    if (t->nparams > 0) {
        for (i = 0; i < t->nparams; i++) {
            Type *p = t->params[i];
            if (t->kind == TY_DYN && p && p->kind == TY_PROJ) {
                /* a binding: keep args[0] (the dyn's own Self), substitute the bound type */
                Type *nr = p->ret ? isub(p->ret, s) : 0;
                c = p;
                if (nr != p->ret) {
                    c = copy_type(p);
                    c->ret = nr;
                }
            } else {
                c = isub(p, s);
            }
            if (c != p && !params) {
                params = tvec(t->nparams);
                memcpy(params, t->params, sizeof(Type *) * (size_t)i);
            }
            if (params) params[i] = c;
        }
        if (params) changed = 1;
    }
    if (!changed) return t;
    r = copy_type(t);
    r->elem = elem;
    r->ret = ret;
    r->len = len;
    r->len_param = lp;
    if (args) r->args = args;
    if (params) r->params = params;
    return r;
}

/* fresh inference arguments for the generic parameters gp[0..n) */
static Type **fresh_args(Sym **gp, int n) {
    Type **a = tvec(n);
    int i;
    for (i = 0; i < n; i++)
        a[i] = (gp[i] && gp[i]->kind == SYM_CONST_GENERIC) ? new_lenvar() : ty_infer(0);
    return a;
}

static void subst_init(Subst *s, Sym **gp, Type **args, int n, Type *self) {
    s->params = gp;
    s->args = args;
    s->n = n;
    s->self = self;
}

/* ================================================================== type predicates */

static Type *res(Type *t) {
    return ty_resolve(t);
}

static int is_var(Type *t) {
    t = res(t);
    return t && t->kind == TY_INFER && t->ik == 0;
}

static int is_floatvar(Type *t) {
    t = res(t);
    return t && t->kind == TY_INFER && t->ik == INFER_FLOAT;
}

/* integer, float, bool, char (or a numeric variable): types with built-in operators */
static int is_scalar(Type *t) {
    t = res(t);
    if (!t) return 0;
    return t->kind == TY_INT || t->kind == TY_FLOAT || t->kind == TY_BOOL || t->kind == TY_CHAR ||
           (t->kind == TY_INFER && t->ik != 0);
}

/* ---------------------------------------------------------------- lang items (cached) */

static Sym *lang_cache_get(const char *name) {
    return lang_item(name);
}

static Sym *L_deref, *L_deref_mut, *L_index, *L_index_mut, *L_fn, *L_fn_mut, *L_fn_once;
static Sym *L_copy, *L_sized, *L_iterator, *L_into_iterator, *L_from, *L_box, *L_option, *L_result;
static Sym *L_range, *L_range_inclusive, *L_range_from, *L_range_to, *L_range_to_inclusive, *L_range_full;
static Sym *L_eq, *L_partial_ord, *L_clone;
static Sym *L_ops[24];
static Sym *L_assign_ops[24];

static const char *op_lang_name(int op) {
    switch (op) {
    case OP_ADD: return "add";
    case OP_SUB: return "sub";
    case OP_MUL: return "mul";
    case OP_DIV: return "div";
    case OP_REM: return "rem";
    case OP_BITAND: return "bitand";
    case OP_BITOR: return "bitor";
    case OP_BITXOR: return "bitxor";
    case OP_SHL: return "shl";
    case OP_SHR: return "shr";
    case OP_NEG: return "neg";
    case OP_NOT: return "not";
    }
    return 0;
}

static const char *op_method_name(int op) {
    return op_lang_name(op);
}

static void init_lang(void) {
    int op;
    L_deref = lang_cache_get("deref");
    L_deref_mut = lang_cache_get("deref_mut");
    L_index = lang_cache_get("index");
    L_index_mut = lang_cache_get("index_mut");
    L_fn = lang_cache_get("fn");
    L_fn_mut = lang_cache_get("fn_mut");
    L_fn_once = lang_cache_get("fn_once");
    L_copy = lang_cache_get("copy");
    L_sized = lang_cache_get("sized");
    L_iterator = lang_cache_get("iterator");
    L_into_iterator = lang_cache_get("into_iterator");
    L_from = lang_cache_get("from");
    L_box = lang_cache_get("owned_box");
    L_option = lang_cache_get("option");
    L_result = lang_cache_get("result");
    L_range = lang_cache_get("range");
    L_range_inclusive = lang_cache_get("range_inclusive");
    L_range_from = lang_cache_get("range_from");
    L_range_to = lang_cache_get("range_to");
    L_range_to_inclusive = lang_cache_get("range_to_inclusive");
    L_range_full = lang_cache_get("range_full");
    L_eq = lang_cache_get("eq");
    L_partial_ord = lang_cache_get("partial_ord");
    L_clone = lang_cache_get("clone");
    for (op = 0; op < 24; op++) {
        const char *nm = op_lang_name(op);
        L_ops[op] = 0;
        L_assign_ops[op] = 0;
        if (nm) {
            char buf[40];
            L_ops[op] = lang_cache_get(nm);
            if (op != OP_NEG && op != OP_NOT) {
                strcpy(buf, nm);
                strcat(buf, "_assign");
                L_assign_ops[op] = lang_cache_get(buf);
            }
        }
    }
}

static int is_fn_trait(Sym *t) {
    return t && (t == L_fn || t == L_fn_mut || t == L_fn_once);
}

/* ================================================================== callee side table */

static Callee *callee_tab[1024];

static unsigned callee_hash(Node *n, int slot) {
    return (unsigned)(((size_t)n >> 4) + (size_t)slot * 7) & 1023u;
}

Callee *check_callee(Node *n, int slot) {
    Callee *c;
    for (c = callee_tab[callee_hash(n, slot)]; c; c = c->next)
        if (c->node == n && c->slot == slot) return c;
    return 0;
}

static Callee **body_callees = 0;
static int nbody_callees = 0, cbody_callees = 0;

static void rec_callee(Node *n, int slot, Sym *fn, Type *fnty, Type *self) {
    Callee *c = check_callee(n, slot);
    unsigned h;
    if (!c) {
        c = (Callee *)ast_alloc(sizeof(Callee));
        c->node = n;
        c->slot = slot;
        h = callee_hash(n, slot);
        c->next = callee_tab[h];
        callee_tab[h] = c;
        if (nbody_callees >= cbody_callees) {
            cbody_callees = cbody_callees ? cbody_callees * 2 : 64;
            body_callees = (Callee **)xrealloc(body_callees, sizeof(Callee *) * (size_t)cbody_callees);
        }
        body_callees[nbody_callees++] = c;
    }
    c->fn = fn;
    c->fnty = fnty;
    c->self = self;
}

/* ================================================================== tracking (for the final write-back) */

static Node **tracked = 0;
static int ntracked = 0, ctracked = 0;
static Sym **tracked_syms = 0;
static int ntsyms = 0, ctsyms = 0;

static Type *setty(Node *n, Type *t) {
    n->ty = t;
    if (ntracked >= ctracked) {
        ctracked = ctracked ? ctracked * 2 : 512;
        tracked = (Node **)xrealloc(tracked, sizeof(Node *) * (size_t)ctracked);
    }
    tracked[ntracked++] = n;
    return t;
}

static void set_local_ty(Sym *s, Type *t) {
    s->ty = t;
    if (ntsyms >= ctsyms) {
        ctsyms = ctsyms ? ctsyms * 2 : 128;
        tracked_syms = (Sym **)xrealloc(tracked_syms, sizeof(Sym *) * (size_t)ctsyms);
    }
    tracked_syms[ntsyms++] = s;
}

/* ================================================================== indexes of traits, impls and associated items */

typedef struct NEnt {
    Name *name;
    Sym **v;
    int n, cap;
    struct NEnt *next;
} NEnt;
static NEnt *ntab[512];

static NEnt *nent(Name *nm, int create) {
    unsigned h = (unsigned)(nm->hash) & 511u;
    NEnt *e;
    for (e = ntab[h]; e; e = e->next)
        if (e->name == nm) return e;
    if (!create) return 0;
    e = (NEnt *)ast_alloc(sizeof(NEnt));
    e->name = nm;
    e->next = ntab[h];
    ntab[h] = e;
    return e;
}

static void nent_add(Name *nm, Sym *s) {
    NEnt *e = nent(nm, 1);
    if (e->n >= e->cap) {
        int nc = e->cap ? e->cap * 2 : 4;
        Sym **nv = (Sym **)ast_alloc(sizeof(Sym *) * (size_t)nc);
        if (e->n) memcpy(nv, e->v, sizeof(Sym *) * (size_t)e->n);
        e->v = nv;
        e->cap = nc;
    }
    e->v[e->n++] = s;
}

typedef struct IEnt {
    Sym *trait;
    Sym **v;
    int n, cap;
    struct IEnt *next;
} IEnt;
static IEnt *itab[256];

static IEnt *ient(Sym *tr, int create) {
    unsigned h = (unsigned)((size_t)tr >> 4) & 255u;
    IEnt *e;
    for (e = itab[h]; e; e = e->next)
        if (e->trait == tr) return e;
    if (!create) return 0;
    e = (IEnt *)ast_alloc(sizeof(IEnt));
    e->trait = tr;
    e->next = itab[h];
    itab[h] = e;
    return e;
}

static void ient_add(Sym *tr, Sym *impl) {
    IEnt *e = ient(tr, 1);
    if (e->n >= e->cap) {
        int nc = e->cap ? e->cap * 2 : 8;
        Sym **nv = (Sym **)ast_alloc(sizeof(Sym *) * (size_t)nc);
        if (e->n) memcpy(nv, e->v, sizeof(Sym *) * (size_t)e->n);
        e->v = nv;
        e->cap = nc;
    }
    e->v[e->n++] = impl;
}

static void index_trait(Sym *tr) {
    int i, n;
    if (!tr->members) return;
    n = scope_member_count(tr->members);
    for (i = 0; i < n; i++) {
        Sym *m = scope_member(tr->members, i);
        if (m && (m->kind == SYM_FN || m->kind == SYM_CONST) && m->name) nent_add(m->name, m);
    }
}

static void index_mod(Sym *m, int depth) {
    int i, n;
    if (!m || !m->members || depth > 12) return;
    n = scope_member_count(m->members);
    for (i = 0; i < n; i++) {
        Sym *s = scope_member(m->members, i);
        if (!s || s->parent != m) continue;
        if (s->kind == SYM_MOD) index_mod(s, depth + 1);
        else if (s->kind == SYM_TRAIT) index_trait(s);
    }
}

static void index_items(void) {
    int i, j, n;
    for (i = 0; i < resolve_nimpls; i++) {
        Sym *impl = resolve_impls[i];
        if (impl->trait) {
            ient_add(impl->trait, impl);
        } else if (impl->members) {
            n = scope_member_count(impl->members);
            for (j = 0; j < n; j++) {
                Sym *m = scope_member(impl->members, j);
                if (m && (m->kind == SYM_FN || m->kind == SYM_CONST) && m->name) nent_add(m->name, m);
            }
        }
    }
    if (core_root) index_mod(core_root, 0);
    if (crate_root) index_mod(crate_root, 0);
}

/* ================================================================== the generic environment of the body */

static Bound **env = 0;
static int nenv = 0, cenv = 0;
static Sym *cur_item = 0;

static void env_push(Bound *b) {
    if (nenv >= cenv) {
        cenv = cenv ? cenv * 2 : 32;
        env = (Bound **)xrealloc(env, sizeof(Bound *) * (size_t)cenv);
    }
    env[nenv++] = b;
}

static void env_add_sym(Sym *s) {
    int i;
    for (i = 0; i < s->nbounds; i++) env_push(&s->bounds[i]);
}

static void set_env(Sym *item) {
    Sym *p;
    nenv = 0;
    cur_item = item;
    if (!item) return;
    collect_set_env(item);
    env_add_sym(item);
    p = item->parent;
    if (p && (p->kind == SYM_IMPL || p->kind == SYM_TRAIT)) {
        if (p->kind == SYM_TRAIT) {
            Bound *b = (Bound *)ast_alloc(sizeof(Bound));
            int i;
            b->ty = ty_self();
            b->trait = p;
            b->nargs = p->ngparams;
            b->args = tvec(p->ngparams);
            for (i = 0; i < p->ngparams; i++) b->args[i] = ty_param(p->gparams[i]);
            env_push(b);
        }
    }
}

/* t is `target` or has it among its supertraits */
static int trait_is(Sym *t, Sym *target, int depth) {
    Node *n;
    long i;
    if (t == target) return 1;
    if (!t || depth > 8 || !t->node) return 0;
    n = t->node->c;
    if (!n || n->kind != N_BOUNDS) return 0;
    for (i = 0; i < n->list.n; i++) {
        Node *b = n->list.v[i];
        Sym *s;
        if (b->kind != N_BOUND_TRAIT || !b->a) continue;
        s = (Sym *)b->a->sym;
        if (s && s->kind == SYM_TRAIT && trait_is(s, target, depth + 1)) return 1;
    }
    return 0;
}

/* does the module of the current item see the trait by name (use, definition or prelude)? */
static int trait_in_scope(Sym *tr) {
    Sym *m = cur_item;
    while (m && m->kind != SYM_MOD) m = m->parent;
    if (!m || !m->members || !tr->name) return 1;
    return scope_lookup(m->members, NS_TYPE, tr->name) == tr;
}

/* bounds a trait declares on its own associated types (`type IntoIter: Iterator<Item = Self::Item>`),
   instantiated for the projection `self` */
static Bound **implied_for(Type *self, int *n) {
    Sym *tr;
    Subst S;
    Bound **v = 0;
    int i, cnt = 0, np;
    *n = 0;
    if (self->kind != TY_PROJ || !self->def || self->nargs < 1) return 0;
    tr = self->def;
    np = self->nargs - 1;
    if (np > tr->ngparams) np = tr->ngparams;
    subst_init(&S, tr->gparams, self->args + 1, np, self->args[0]);
    v = (Bound **)ast_alloc(sizeof(Bound *) * (size_t)(tr->nbounds + 1));
    for (i = 0; i < tr->nbounds; i++) {
        Bound *b = &tr->bounds[i], *nb;
        Type *bt = res(b->ty);
        int j;
        if (!bt || bt->kind != TY_PROJ || bt->def != tr || bt->name != self->name) continue;
        nb = (Bound *)ast_alloc(sizeof(Bound));
        nb->ty = self;
        nb->trait = b->trait;
        nb->nargs = b->nargs;
        nb->args = tvec(b->nargs);
        for (j = 0; j < b->nargs; j++) nb->args[j] = isub(b->args[j], &S);
        nb->assoc_name = b->assoc_name;
        nb->assoc_ty = b->assoc_ty ? isub(b->assoc_ty, &S) : 0;
        v[cnt++] = nb;
    }
    *n = cnt;
    return v;
}

/* ================================================================== trait selection */

static int head_ok(Type *impl_self, Type *self) {
    impl_self = res(impl_self);
    self = res(self);
    if (!impl_self || !self) return 1;
    if (impl_self->kind == TY_PARAM || impl_self->kind == TY_PROJ) return 1;
    if (self->kind == TY_INFER) {
        if (self->ik == INFER_INT) return impl_self->kind == TY_INT;
        if (self->ik == INFER_FLOAT) return impl_self->kind == TY_FLOAT;
        return 1;
    }
    if (self->kind == TY_ERR || self->kind == TY_NEVER) return 1;
    if (impl_self->kind != self->kind) return 0;
    switch (self->kind) {
    case TY_INT: case TY_FLOAT: return impl_self->ik == self->ik;
    case TY_ADT: case TY_DYN: return impl_self->def == self->def;
    case TY_REF: case TY_PTR: return impl_self->mut == self->mut;
    case TY_TUPLE: return impl_self->nargs == self->nargs;
    }
    return 1;
}

typedef struct {
    Sym *impl;
    Subst sub;
} ImplSel;

static int solve_pred_d(Type *self, Sym *trait, Type **args, int nargs, int depth);
static Type *norm(Type *t);
static Type *proj_try(Type *self, Sym *trait, Name *name, Type **targs, int nt, int *st, int depth);

/* the impl's bounds hold for the substitution (ambiguous counts as holding) */
static int probing = 0;

static int bounds_ok(Sym *impl, Subst *S, int depth) {
    int i, j, r;
    for (i = 0; i < impl->nbounds; i++) {
        Bound *b = &impl->bounds[i];
        Type *bt = norm(isub(b->ty, S));
        Type **ba = tvec(b->nargs);
        for (j = 0; j < b->nargs; j++) ba[j] = norm(isub(b->args[j], S));
        r = solve_pred_d(bt, b->trait, ba, b->nargs, depth + 1);
        if (r == 0) return 0;
        if (b->assoc_name && b->assoc_ty && r == 1) {
            int st = 2;
            Type *pr = proj_try(bt, b->trait, b->assoc_name, ba, b->nargs, &st, depth + 1);
            if (st == 0 && pr && !uni(norm(pr), norm(isub(b->assoc_ty, S)))) return 0;
        }
    }
    return 1;
}

static int try_impl(Sym *impl, Type *self, Type **args, int nargs, int depth, ImplSel *out) {
    Subst S;
    Type **ia;
    int j;
    if (impl->ngparams > 0) ia = fresh_args(impl->gparams, impl->ngparams);
    else ia = 0;
    subst_init(&S, impl->gparams, ia, impl->ngparams, 0);
    if (!cu(isub(impl->impl_self, &S), self)) return 0;
    for (j = 0; j < nargs && j < impl->ntrait_args; j++)
        if (!cu(isub(impl->trait_args[j], &S), args[j])) return 0;
    if (!bounds_ok(impl, &S, depth)) return 0;
    if (out) {
        out->impl = impl;
        out->sub = S;
    }
    return 1;
}

/* 1 = exactly one impl applies (bound, *out filled); 0 = none; 2 = several (nothing bound) */
static int select_impl_inner(Sym *trait, Type *self, Type **args, int nargs, ImplSel *out, int depth) {
    IEnt *L = ient(trait, 0);
    Sym *found = 0;
    int i, nfound = 0, m;
    if (!L) return 0;
    for (i = 0; i < L->n; i++) {
        Sym *impl = L->v[i];
        if (!head_ok(impl->impl_self, self)) continue;
        m = snap();
        if (try_impl(impl, self, args, nargs, depth, 0)) {
            if (!found) found = impl;
            nfound++;
        }
        rollback(m);
        if (nfound > 1) break;
    }
    if (nfound == 0) return 0;
    if (nfound > 1) {
        int k;
        int concrete = !ty_has_infer(self);
        for (k = 0; k < nargs; k++)
            if (ty_has_infer(args[k])) concrete = 0;
        if (!concrete) return 2;
    }
    try_impl(found, self, args, nargs, depth, out);
    return 1;
}

static int select_impl(Sym *trait, Type *self, Type **args, int nargs, ImplSel *out, int depth) {
    int r;
    probing++;
    r = select_impl_inner(trait, self, args, nargs, out, depth);
    probing--;
    return r;
}

static int is_marker_trait(Sym *tr) {
    const char *s;
    if (tr == L_sized) return 1;
    if (!tr->name) return 0;
    s = tr->name->s;
    return strcmp(s, "Send") == 0 || strcmp(s, "Sync") == 0 || strcmp(s, "Unpin") == 0;
}

static Type *fn_like_tuple(Type *f) {
    return ty_tuple(f->params, f->nparams);
}

int ck_closure_impl(Type *t, Sym *trait) {
    t = res(t);
    if (!t) return 0;
    if (t->kind == TY_CLOSURE || t->kind == TY_FN_DEF || t->kind == TY_FN_PTR) return is_fn_trait(trait);
    return 0;
}

/* 1 holds (impl selection / unification done), 0 does not, 2 not known yet */
static int solve_pred_d(Type *self, Sym *trait, Type **args, int nargs, int depth) {
    int i, j, r;
    self = res(self);
    if (!self || !trait) return 1;
    if (self->kind == TY_ERR || self->kind == TY_NEVER) return 1;
    if (is_marker_trait(trait)) return 1;
    if (depth > 10) return 2;
    if (self->kind == TY_INFER) {
        if (self->ik == 0) return 2;
        /* a numeric literal variable: the default type decides, nothing is bound */
        {
            Type *d = self->ik == INFER_INT ? ty_int(IK_I32) : ty_float(64);
            int m = snap();
            r = solve_pred_d(d, trait, args, nargs, depth + 1);
            rollback(m);
            return r == 0 ? 0 : 1;
        }
    }
    /* bounds in the environment (and those a trait declares for its associated types) */
    {
    int nimp = 0, tot;
    Bound **imp = implied_for(self, &nimp);
    tot = nenv + nimp;
    for (i = 0; i < tot; i++) {
        Bound *b = i < nenv ? env[i] : imp[i - nenv];
        if (!b->trait || !ty_equal(b->ty, self)) continue;
        if (b->trait == trait) {
            int m = snap(), ok = 1;
            for (j = 0; j < nargs && j < b->nargs; j++)
                if (!cu(b->args[j], args[j])) { ok = 0; break; }
            if (ok) return 1;
            rollback(m);
        } else if (trait_is(b->trait, trait, 0)) {
            return 1;
        }
    }
    }
    if (self->kind == TY_DYN && trait_is(self->def, trait, 0)) {
        if (self->def == trait) {
            int m = snap(), ok = 1;
            for (j = 0; j < nargs && j < self->nargs; j++)
                if (!cu(self->args[j], args[j])) { ok = 0; break; }
            if (ok) return 1;
            rollback(m);
        } else {
            return 1;
        }
    }
    if (is_fn_trait(trait)) {
        Type *f = self;
        while (f->kind == TY_REF || (f->kind == TY_ADT && f->def == L_box && f->nargs > 0)) {
            f = res(f->kind == TY_REF ? f->elem : f->args[0]);
        }
        if (f->kind == TY_CLOSURE || f->kind == TY_FN_DEF || f->kind == TY_FN_PTR) {
            if (nargs >= 1 && !uni(args[0], fn_like_tuple(f))) return 0;
            return 1;
        }
        if (f->kind == TY_DYN && is_fn_trait(f->def)) {
            if (nargs >= 1 && f->nargs >= 1 && !uni(args[0], f->args[0])) return 0;
            return 1;
        }
    }
    {
        ImplSel sel;
        r = select_impl(trait, self, args, nargs, &sel, depth);
        if (r) return r;
    }
    if (trait == L_copy) return is_copy(self) ? 1 : 0;
    if (nargs == 0 && implements(self, trait)) return 1;
    return 0;
}

static int solve_pred(Type *self, Sym *trait, Type **args, int nargs) {
    return solve_pred_d(self, trait, args, nargs, 0);
}

/* ================================================================== obligations */

typedef struct Ob {
    int kind;                /* 0: self: trait<args> [, assoc = aty]   1: <self as trait<args>>::aname == aty */
    Type *self;
    Sym *trait;
    Type **args;
    int nargs;
    Name *aname;
    Type *aty;
    Node *at;
    int done;
} Ob;

static Ob *obs = 0;
static int nobs = 0, cobs = 0;
static Node *ob_node = 0;       /* the node being checked: where an unsatisfied obligation is reported */

static void ob_add(Ob *o) {
    if (nobs >= cobs) {
        cobs = cobs ? cobs * 2 : 64;
        obs = (Ob *)xrealloc(obs, sizeof(Ob) * (size_t)cobs);
    }
    obs[nobs++] = *o;
}

static void ob_pred(Type *self, Sym *trait, Type **args, int nargs, Name *aname, Type *aty) {
    Ob o;
    memset(&o, 0, sizeof o);
    o.kind = 0;
    o.self = self;
    o.trait = trait;
    o.args = args;
    o.nargs = nargs;
    o.aname = aname;
    o.aty = aty;
    o.at = ob_node;
    ob_add(&o);
}

static void ob_proj(Type *self, Sym *trait, Name *name, Type **targs, int nt, Type *var) {
    Ob o;
    memset(&o, 0, sizeof o);
    o.kind = 1;
    o.self = self;
    o.trait = trait;
    o.args = targs;
    o.nargs = nt;
    o.aname = name;
    o.aty = var;
    o.at = ob_node;
    ob_add(&o);
}

/* One try at <self as trait<targs>>::name.  *st: 0 found, 1 not known yet, 2 no such thing (rigid). */
static Type *proj_try(Type *self, Sym *trait, Name *name, Type **targs, int nt, int *st, int depth) {
    int i;
    self = res(self);
    *st = 1;
    if (!self || self->kind == TY_INFER) return 0;
    if (self->kind == TY_ERR) { *st = 0; return self; }
    {
    int nimp = 0, tot;
    Bound **imp = implied_for(self, &nimp);
    tot = nenv + nimp;
    for (i = 0; i < tot; i++) {
        Bound *b = i < nenv ? env[i] : imp[i - nenv];
        if (b->assoc_name == name && b->assoc_ty && b->trait && ty_equal(b->ty, self) &&
            trait_is(b->trait, trait, 0)) {
            *st = 0;
            return b->assoc_ty;
        }
    }
    }
    if (self->kind == TY_DYN) {
        for (i = 0; i < self->nparams; i++)
            if (self->params[i]->name == name) { *st = 0; return self->params[i]->ret; }
    }
    if (name == nm_output && is_fn_trait(trait)) {
        Type *f = self;
        while (f->kind == TY_REF || (f->kind == TY_ADT && f->def == L_box && f->nargs > 0))
            f = res(f->kind == TY_REF ? f->elem : f->args[0]);
        if (f->kind == TY_CLOSURE || f->kind == TY_FN_DEF || f->kind == TY_FN_PTR) {
            *st = 0;
            return f->ret;
        }
        if (f->kind == TY_DYN) {
            for (i = 0; i < f->nparams; i++)
                if (f->params[i]->name == name) { *st = 0; return f->params[i]->ret; }
        }
    }
    {
        ImplSel sel;
        int r = select_impl(trait, self, targs, nt, &sel, depth);
        if (r == 2) return 0;
        if (r == 1) {
            Sym *item = assoc_item(sel.impl, NS_TYPE, name);
            if (item) {
                if (!item->ty && item->node && item->node->b) item->ty = ty_from_ast(item->node->b);
                if (item->ty) {
                    *st = 0;
                    return isub(item->ty, &sel.sub);
                }
            }
        }
    }
    *st = 2;
    return 0;
}

static int has_proj(Type *t) {
    int i;
    t = res(t);
    if (!t) return 0;
    if (t->kind == TY_PROJ) return 1;
    if (t->elem && has_proj(t->elem)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (has_proj(t->args[i])) return 1;
    if (t->kind == TY_DYN) {
        for (i = 0; i < t->nparams; i++)
            if (t->params[i]->ret && has_proj(t->params[i]->ret)) return 1;
        return 0;
    }
    for (i = 0; i < t->nparams; i++)
        if (has_proj(t->params[i])) return 1;
    if (t->ret && has_proj(t->ret)) return 1;
    return 0;
}

static int norm_depth = 0;

static Type *proj_norm(Type *self, Sym *trait, Name *name, Type **targs, int nt) {
    int st;
    Type *r = proj_try(self, trait, name, targs, nt, &st, 0);
    if (st == 0) return norm(r);
    if (st == 1) {
        Type *v = ty_infer(0);
        if (!probing) ob_proj(self, trait, name, targs, nt, v);
        return v;
    }
    return ty_proj(self, trait, name, targs, nt);
}

/* replaces every projection that can be resolved; one whose self type is not known yet
   becomes a variable with a pending obligation */
static Type *norm(Type *t) {
    Type *r, *c, *elem, *ret;
    Type **args = 0, **params = 0;
    int i, changed = 0;
    t = res(t);
    if (!t || !has_proj(t)) return t;
    if (norm_depth > 20) return t;
    norm_depth++;
    if (t->kind == TY_PROJ) {
        Type **na = tvec(t->nargs);
        for (i = 0; i < t->nargs; i++) na[i] = norm(t->args[i]);
        r = proj_norm(na[0], t->def, t->name, na + 1, t->nargs - 1);
        norm_depth--;
        return r;
    }
    elem = t->elem ? norm(t->elem) : 0;
    if (elem != t->elem) changed = 1;
    ret = t->ret ? norm(t->ret) : 0;
    if (ret != t->ret) changed = 1;
    if (t->nargs > 0) {
        for (i = 0; i < t->nargs; i++) {
            c = norm(t->args[i]);
            if (c != t->args[i] && !args) {
                args = tvec(t->nargs);
                memcpy(args, t->args, sizeof(Type *) * (size_t)i);
            }
            if (args) args[i] = c;
        }
        if (args) changed = 1;
    }
    if (t->nparams > 0) {
        for (i = 0; i < t->nparams; i++) {
            Type *p = t->params[i];
            if (t->kind == TY_DYN && p->kind == TY_PROJ) {
                Type *nr = p->ret ? norm(p->ret) : 0;
                c = p;
                if (nr != p->ret) {
                    c = copy_type(p);
                    c->ret = nr;
                }
            } else {
                c = norm(p);
            }
            if (c != p && !params) {
                params = tvec(t->nparams);
                memcpy(params, t->params, sizeof(Type *) * (size_t)i);
            }
            if (params) params[i] = c;
        }
        if (params) changed = 1;
    }
    norm_depth--;
    if (!changed) return t;
    r = copy_type(t);
    r->elem = elem;
    r->ret = ret;
    if (args) r->args = args;
    if (params) r->params = params;
    return r;
}

/* 1 = finished */
static int try_ob(Ob *o) {
    int r, st;
    Type *p;
    ob_node = o->at;
    if (o->kind == 1) {
        p = proj_try(o->self, o->trait, o->aname, o->args, o->nargs, &st, 0);
        if (st == 0) {
            Type *n = norm(p);
            if (!uni(o->aty, n) && o->at)
                mismatch(o->at, o->aty, n);
            return 1;
        }
        if (st == 2) {
            uni(o->aty, ty_proj(o->self, o->trait, o->aname, o->args, o->nargs));
            return 1;
        }
        return 0;
    }
    r = solve_pred(o->self, o->trait, o->args, o->nargs);
    if (r == 2) return 0;
    if (r == 0) {
        if (!in_core && o->at && !is_var(o->self)) {
            const char *ts = tn(o->self);
            char *tmp = xstrdup(ts);
            const char *trn = o->trait->name ? o->trait->name->s : "?";
            if (getenv("CK_DEBUG")) { int q; fprintf(stderr, "ob %s: %s<", tn(o->self), trn); for (q = 0; q < o->nargs; q++) fprintf(stderr, "%s,", tn(o->args[q])); fprintf(stderr, ">\n"); }
            if (strcmp(trn, "Display") == 0)
                error_at(o->at->file, o->at->line, "`%s` doesn't implement `std::fmt::Display`", tmp);
            if (strcmp(trn, "Debug") == 0)
                error_at(o->at->file, o->at->line, "`%s` doesn't implement `Debug`", tmp);
            error_at(o->at->file, o->at->line, "the trait bound `%s: %s` is not satisfied", tmp, trn);
        }
        return 1;
    }
    if (o->aname && o->aty) {
        p = proj_try(o->self, o->trait, o->aname, o->args, o->nargs, &st, 0);
        if (st == 0) {
            Type *n = norm(p);
            if (!uni(o->aty, n) && !in_core && o->at) mismatch(o->at, o->aty, n);
        } else if (st == 1) {
            return 0;
        }
    }
    return 1;
}

static void solve_obs(void) {
    int again, i;
    do {
        again = 0;
        for (i = 0; i < nobs; i++) {
            Ob o;
            if (obs[i].done) continue;
            o = obs[i];
            if (try_ob(&o)) {
                obs[i].done = 1;
                again = 1;
            }
        }
    } while (again);
    /* drop finished entries */
    {
        int k = 0;
        for (i = 0; i < nobs; i++)
            if (!obs[i].done) obs[k++] = obs[i];
        nobs = k;
    }
}

/* ================================================================== function context, loops */

typedef struct Loop {
    Name *label;
    Type *brk;               /* the type of `break value` / of a labeled block */
    int kind;                /* 0 loop, 1 while / for, 2 labeled block */
    int has_break;
} Loop;

static Loop *loops = 0;
static int nloops = 0, cloops = 0, loop_base = 0;

typedef struct FnCx {
    Type *ret;
    Node *closure;
    int saved_loop_base;
} FnCx;
static FnCx *fcx = 0;
static int nfcx = 0, cfcx = 0;
static int closure_depth = 0;

static void push_fcx(Type *ret, Node *closure) {
    if (nfcx >= cfcx) {
        cfcx = cfcx ? cfcx * 2 : 8;
        fcx = (FnCx *)xrealloc(fcx, sizeof(FnCx) * (size_t)cfcx);
    }
    fcx[nfcx].ret = ret;
    fcx[nfcx].closure = closure;
    fcx[nfcx].saved_loop_base = loop_base;
    nfcx++;
    loop_base = nloops;
}

static void pop_fcx(void) {
    nfcx--;
    loop_base = fcx[nfcx].saved_loop_base;
}

static Loop *push_loop(Name *label, int kind, Type *brk) {
    Loop *l;
    if (nloops >= cloops) {
        cloops = cloops ? cloops * 2 : 16;
        loops = (Loop *)xrealloc(loops, sizeof(Loop) * (size_t)cloops);
    }
    l = &loops[nloops++];
    l->label = label;
    l->kind = kind;
    l->brk = brk;
    l->has_break = 0;
    return l;
}

/* ================================================================== forward declarations */

static Type *ck(Node *e, Type *expect, int strict);
static Type *ck_expr(Node *e, Type *hint);
static Type *ck_coerce(Node *e, Type *target);
static Type *ck_block(Node *b, Type *expect, int strict);
static void ck_pat(Node *p, Type *ty, int bm);
static Type *lower_ty(Node *t);
static void ck_cond(Node *c);
static void check_match_exhaustive(Node *m, Type *scrut);
static void check_irrefutable(Node *pat, Type *ty, Node *at, const char *what);
static void queue_item(Node *n);

static Name *nm_target, *nm_item_name;

/* ================================================================== autoderef */

/* 0: no deref; 1: built-in (&T); 2: overloaded through Deref */
static int deref_once(Type *t, Type **out) {
    t = res(t);
    if (!t) return 0;
    if (t->kind == TY_REF) {
        *out = t->elem;
        return 1;
    }
    if (t->kind == TY_ADT || t->kind == TY_PARAM || t->kind == TY_SELF || t->kind == TY_PROJ) {
        if (solve_pred(t, L_deref, 0, 0) == 1) {
            *out = proj_norm(t, L_deref, nm_target, 0, 0);
            return 2;
        }
    }
    return 0;
}

typedef struct {
    Type *ty;
    int nderef;
    int unsize;
} Step;
#define MAXSTEPS 12

static int make_steps(Type *t, Step *st) {
    int n = 0, k;
    Type *cur = res(t), *next, *last;
    st[n].ty = cur;
    st[n].nderef = 0;
    st[n].unsize = 0;
    n++;
    for (k = 1; k < MAXSTEPS - 1; k++) {
        if (!deref_once(cur, &next)) break;
        cur = res(next);
        if (!cur || cur->kind == TY_INFER) break;
        st[n].ty = cur;
        st[n].nderef = k;
        st[n].unsize = 0;
        n++;
    }
    last = res(st[n - 1].ty);
    if (last && last->kind == TY_ARRAY && last->elem) {
        st[n].ty = ty_slice(last->elem);
        st[n].nderef = st[n - 1].nderef;
        st[n].unsize = 1;
        n++;
    }
    return n;
}

/* ================================================================== coercion */

static int coerce_ref(Node *e, Type *f, Type *t) {
    Type *fe = res(f->elem), *te = res(t->elem), *cur, *next;
    int k, r;
    if (t->mut && !f->mut) return 0;
    if (te->kind == TY_INFER || fe->kind == TY_INFER) {
        if (!uni(fe, te)) return 0;
        if (f->mut && !t->mut && e) e->adj |= ADJ_REBORROW;
        return 1;
    }
    if (uni(fe, te)) {
        if (f->mut && !t->mut && e) e->adj |= ADJ_REBORROW;
        return 1;
    }
    if (te->kind == TY_DYN && fe->kind != TY_DYN) {
        r = solve_pred(fe, te->def, te->args, te->nargs);
        if (r != 0) {
            if (r == 2) ob_pred(fe, te->def, te->args, te->nargs, 0, 0);
            if (e) e->adj |= ADJ_UNSIZE;
            return 1;
        }
        return 0;
    }
    if (te->kind == TY_SLICE && fe->kind == TY_ARRAY && fe->elem) {
        if (uni(fe->elem, te->elem)) {
            if (e) e->adj |= ADJ_UNSIZE;
            return 1;
        }
        return 0;
    }
    cur = fe;
    for (k = 1; k < 8; k++) {
        if (!deref_once(cur, &next)) break;
        cur = res(next);
        if (!cur || cur->kind == TY_INFER) break;
        if (uni(cur, te)) {
            if (e) e->adj |= (k + 1) | (t->mut ? ADJ_AUTOREF_MUT : ADJ_AUTOREF);
            return 1;
        }
        if (te->kind == TY_SLICE && cur->kind == TY_ARRAY && cur->elem && uni(cur->elem, te->elem)) {
            if (e) e->adj |= (k + 1) | (t->mut ? ADJ_AUTOREF_MUT : ADJ_AUTOREF) | ADJ_UNSIZE;
            return 1;
        }
    }
    return 0;
}

/* converts a value of type `from` (expression e, may be 0) to `to`; records adjustments */
static int coerce(Node *e, Type *from, Type *to) {
    Type *f = res(from), *t = res(to);
    if (!f || !t) return 1;
    if (f->kind == TY_NEVER) {
        if (e) e->adj |= ADJ_NEVER;
        return 1;
    }
    if (f->kind == TY_ERR || t->kind == TY_ERR) return 1;
    if (t->kind == TY_INFER || f->kind == TY_INFER) return uni(f, t);
    if (t->kind == TY_REF && f->kind == TY_REF) return coerce_ref(e, f, t);
    if (t->kind == TY_PTR && (f->kind == TY_REF || f->kind == TY_PTR)) {
        if (t->mut && !f->mut) return 0;
        return uni(f->elem, t->elem);
    }
    if (t->kind == TY_ADT && f->kind == TY_ADT && t->def == f->def && t->def == L_box &&
        t->nargs > 0 && f->nargs > 0) {
        Type *te = res(t->args[0]), *fe = res(f->args[0]);
        if (te->kind == TY_DYN && fe->kind != TY_DYN && fe->kind != TY_INFER) {
            int r = solve_pred(fe, te->def, te->args, te->nargs);
            if (r != 0) {
                if (r == 2) ob_pred(fe, te->def, te->args, te->nargs, 0, 0);
                if (e) e->adj |= ADJ_UNSIZE;
                return 1;
            }
            return 0;
        }
        if (te->kind == TY_SLICE && fe->kind == TY_ARRAY && fe->elem && uni(fe->elem, te->elem)) {
            if (e) e->adj |= ADJ_UNSIZE;
            return 1;
        }
    }
    if (t->kind == TY_FN_PTR && (f->kind == TY_FN_DEF || f->kind == TY_CLOSURE)) {
        int i;
        if (f->nparams != t->nparams) return 0;
        for (i = 0; i < f->nparams; i++)
            if (!uni(f->params[i], t->params[i])) return 0;
        return uni(f->ret, t->ret);
    }
    return uni(f, t);
}

static int is_blocklike(Node *e) {
    while (e && e->kind == N_E_PAREN) e = e->a;
    return e && (e->kind == N_BLOCK || e->kind == N_E_IF || e->kind == N_E_MATCH);
}

/* check e and coerce it to target (a coercion site) */
static Type *ck_coerce(Node *e, Type *target) {
    Type *t;
    if (!target) return ck_expr(e, 0);
    t = ck(e, target, is_blocklike(e));
    if (!coerce(e, t, target)) mismatch(e, target, t);
    return target;
}

/* ================================================================== items: fields, constructors */

static Sym *find_field(Sym *owner, Name *name, int index) {
    int i, n;
    if (!owner || !owner->members) return 0;
    n = scope_member_count(owner->members);
    for (i = 0; i < n; i++) {
        Sym *f = scope_member(owner->members, i);
        if (!f || f->kind != SYM_FIELD) continue;
        if (name) {
            if (f->name == name) return f;
        } else if (i == index) {
            return f;
        }
    }
    return 0;
}

static int count_fields(Sym *owner) {
    int i, n, c = 0;
    if (!owner || !owner->members) return 0;
    n = scope_member_count(owner->members);
    for (i = 0; i < n; i++) {
        Sym *f = scope_member(owner->members, i);
        if (f && f->kind == SYM_FIELD) c++;
    }
    return c;
}

static Sym *adt_owner(Sym *vs) {
    return vs->kind == SYM_VARIANT ? vs->parent : vs;
}

static Type *adt_fresh(Sym *adt) {
    Type **args = adt->ngparams > 0 ? fresh_args(adt->gparams, adt->ngparams) : 0;
    return ty_adt(adt, args, adt->ngparams);
}

/* the type of a field of the ADT type `adt` (a struct / union, or the enum for a variant's field) */
static Type *field_ty(Sym *field, Type *adt) {
    Sym *owner = adt->def;
    Subst S;
    if (!field->ty) {
        if (field->node && field->node->a) field->ty = ty_from_ast(field->node->a);
        else error_at(field->node ? field->node->file : 0, field->node ? field->node->line : 0,
                      "internal error: field `%s` has no type", field->name ? field->name->s : "?");
    }
    subst_init(&S, owner->gparams, adt->args, owner->ngparams, 0);
    return norm(isub(field->ty, &S));
}

static Type *ctor_fn_type(Sym *vs, Type *adt) {
    int n = count_fields(vs), i;
    Type **params = tvec(n);
    for (i = 0; i < n; i++) params[i] = field_ty(find_field(vs, 0, i), adt);
    return ty_fn_def(vs, adt->args, adt->nargs, params, n, adt);
}

static int is_unit_like(Sym *vs) {
    return vs->node && !(vs->node->flags & (F_NAMED | F_TUPLE));
}

static int is_tuple_like(Sym *vs) {
    return vs->node && (vs->node->flags & F_TUPLE);
}

static Sym *find_variant(Sym *en, Name *name) {
    int i, n;
    if (!en || !en->members) return 0;
    n = scope_member_count(en->members);
    for (i = 0; i < n; i++) {
        Sym *v = scope_member(en->members, i);
        if (v && v->kind == SYM_VARIANT && v->name == name) return v;
    }
    return 0;
}

/* ================================================================== generics instantiation */

static Type *lower_ty(Node *t) {
    return norm(ty_from_ast(t));
}

static int sym_in(Sym **v, int n, Sym *s) {
    int i;
    for (i = 0; i < n; i++)
        if (v[i] == s) return 1;
    return 0;
}

/* a substitution over the fn's generics and the owner's (impl / trait), all fresh */
static void build_subst(Sym *fn, Sym *owner, Subst *S, Type *self) {
    int nf = fn ? fn->ngparams : 0, no = owner ? owner->ngparams : 0, i, n = 0;
    Sym **gp = (Sym **)ast_alloc(sizeof(Sym *) * (size_t)(nf + no + 1));
    for (i = 0; i < nf; i++) gp[n++] = fn->gparams[i];
    for (i = 0; i < no; i++)
        if (!sym_in(gp, n, owner->gparams[i])) gp[n++] = owner->gparams[i];
    subst_init(S, gp, fresh_args(gp, n), n, self);
}

static Type **trait_args_of(Subst *S, Sym *trait) {
    Type **a = tvec(trait->ngparams);
    int i, j;
    for (i = 0; i < trait->ngparams; i++) {
        a[i] = 0;
        for (j = 0; j < S->n; j++)
            if (S->params[j] == trait->gparams[i]) a[i] = S->args[j];
        if (!a[i]) a[i] = ty_infer(0);
    }
    return a;
}

static void regb(Bound *bs, int nb, Subst *S) {
    int i, j;
    for (i = 0; i < nb; i++) {
        Bound *b = &bs[i];
        Type *bt;
        Type **ba;
        if (!b->trait || is_marker_trait(b->trait)) continue;
        bt = norm(isub(b->ty, S));
        ba = tvec(b->nargs);
        for (j = 0; j < b->nargs; j++) ba[j] = norm(isub(b->args[j], S));
        ob_pred(bt, b->trait, ba, b->nargs, b->assoc_name,
                b->assoc_ty ? norm(isub(b->assoc_ty, S)) : 0);
    }
}

/* the obligations of calling fn with substitution S (Self given for trait items) */
static void reg_bounds(Sym *fn, Subst *S, Type *self, Node *at) {
    Sym *p = fn->parent;
    ob_node = at;
    regb(fn->bounds, fn->nbounds, S);
    if (p && p->kind == SYM_TRAIT && self)
        ob_pred(self, p, trait_args_of(S, p), p->ngparams, 0, 0);
}

static Type *lower_targ(Node *a, Sym *param) {
    Type *t;
    if (a->kind == N_E_LIT_INT) return ty_const((long)a->ival);
    t = lower_ty(a);
    if (param && param->kind == SYM_CONST_GENERIC && res(t)->kind == TY_INFER) return new_lenvar();
    return t;
}

/* explicit generic arguments (turbofish) of the fn's own parameters */
static void apply_turbofish(Sym *fn, Subst *S, Node *seg, Node *at) {
    Sym *owner = fn->parent;
    int off = 0, k, given = 0, own, i;
    if (!seg) return;
    if (owner && (owner->kind == SYM_IMPL || owner->kind == SYM_TRAIT)) off = owner->ngparams;
    own = fn->ngparams - off;
    for (k = 0; k < seg->list.n; k++) {
        Node *a = seg->list.v[k];
        Type *lt;
        int idx;
        if (a->kind == N_LIFETIME || a->kind == N_ASSOC_BIND) continue;
        if (given >= own)
            error_at(at->file, at->line, "function takes %d generic argument%s but %d generic arguments were supplied",
                     own, own == 1 ? "" : "s", given + 1);
        idx = off + given;
        for (i = 0; i < S->n; i++)
            if (S->params[i] == fn->gparams[idx]) break;
        lt = lower_targ(a, fn->gparams[idx]);
        if (i < S->n && !uni(S->args[i], lt)) mismatch(at, S->args[i], lt);
        given++;
    }
}

/* the signature of fn under S, as a TY_FN_DEF; bounds are registered as obligations */
static Type *inst_sig(Sym *fn, Subst *S, Type *self, Node *at, int reg) {
    Type *sig = fn->ty, **params, **targs;
    int i, np, nt = fn->ngparams;
    if (!sig || sig->kind != TY_FN_DEF)
        error_at(at->file, at->line, "internal error: function `%s` has no signature",
                 fn->name ? fn->name->s : "?");
    np = sig->nparams;
    params = tvec(np);
    for (i = 0; i < np; i++) params[i] = norm(isub(sig->params[i], S));
    targs = tvec(nt);
    for (i = 0; i < nt; i++) {
        int j;
        targs[i] = 0;
        for (j = 0; j < S->n; j++)
            if (S->params[j] == fn->gparams[i]) targs[i] = S->args[j];
    }
    if (reg) reg_bounds(fn, S, self, at);
    return ty_fn_def(fn, targs, nt, params, np, norm(isub(sig->ret, S)));
}

/* a free fn / path to a fn with its own generics only */
static Type *inst_fn_path(Sym *fn, Node *seg, Node *at, Type *self) {
    Subst S;
    Sym *owner = fn->parent;
    if (owner && owner->kind != SYM_IMPL && owner->kind != SYM_TRAIT) owner = 0;
    build_subst(fn, owner, &S, self);
    apply_turbofish(fn, &S, seg, at);
    return inst_sig(fn, &S, self, at, 1);
}

/* ================================================================== constant evaluation (array lengths) */

static int ck_const_eval(Node *e, long *out, int depth) {
    long x, y;
    Sym *s;
    if (!e || depth > 24) return 0;
    switch (e->kind) {
    case N_E_LIT_INT: *out = (long)e->ival; return 1;
    case N_E_PAREN: return ck_const_eval(e->a, out, depth + 1);
    case N_E_CAST: return ck_const_eval(e->a, out, depth + 1);
    case N_E_PATH:
        s = (Sym *)e->sym;
        if (!s && e->a) s = (Sym *)e->a->sym;
        if (s && s->kind == SYM_CONST && s->node && s->node->b) return ck_const_eval(s->node->b, out, depth + 1);
        return 0;
    case N_E_BINARY:
        if (!ck_const_eval(e->a, &x, depth + 1) || !ck_const_eval(e->b, &y, depth + 1)) return 0;
        switch (e->op) {
        case OP_ADD: *out = x + y; return 1;
        case OP_SUB: *out = x - y; return 1;
        case OP_MUL: *out = x * y; return 1;
        case OP_DIV: if (!y) return 0; *out = x / y; return 1;
        case OP_REM: if (!y) return 0; *out = x % y; return 1;
        case OP_SHL: *out = x << y; return 1;
        }
        return 0;
    }
    return 0;
}

/* ================================================================== associated item lookup */

typedef struct {
    Sym *item;
    Sym *owner;              /* impl or trait */
    Subst S;
    Type *self;              /* the Self type for trait items */
    int pred;                /* trait items: solve_pred result */
} Pick;

static int owner_is_inherent(Sym *s) {
    Sym *p = s->parent;
    return p && p->kind == SYM_IMPL && !p->trait;
}

static int try_inherent(Sym *s, Type *self, Pick *pk) {
    Sym *impl = s->parent;
    int m = snap();
    Subst S;
    if (!owner_is_inherent(s)) return 0;
    if (!head_ok(impl->impl_self, self)) return 0;
    build_subst(s->kind == SYM_FN ? s : 0, impl, &S, 0);
    if (!cu(isub(impl->impl_self, &S), self)) {
        rollback(m);
        return 0;
    }
    pk->item = s;
    pk->owner = impl;
    pk->S = S;
    pk->self = 0;
    pk->pred = 1;
    return 1;
}

static int try_trait_item(Sym *s, Type *self, Pick *pk) {
    Sym *tr = s->parent;
    int m = snap(), r;
    Subst S;
    if (!tr || tr->kind != SYM_TRAIT) return 0;
    build_subst(s->kind == SYM_FN ? s : 0, tr, &S, self);
    r = solve_pred(self, tr, trait_args_of(&S, tr), tr->ngparams);
    if (r == 0) {
        rollback(m);
        return 0;
    }
    pk->item = s;
    pk->owner = tr;
    pk->S = S;
    pk->self = self;
    pk->pred = r;
    return 1;
}

/* Type::name for a value item: inherent first, then the traits the type implements */
static int lookup_assoc(Type *self, Name *name, Pick *pk) {
    NEnt *ne = nent(name, 0);
    int i, pass;
    if (!ne) return 0;
    for (i = 0; i < ne->n; i++)
        if (try_inherent(ne->v[i], self, pk)) return 1;
    for (pass = 0; pass < 2; pass++)
        for (i = 0; i < ne->n; i++) {
            Sym *s = ne->v[i];
            if (!s->parent || s->parent->kind != SYM_TRAIT) continue;
            if ((pass == 0) != (trait_in_scope(s->parent) != 0)) continue;
            if (try_trait_item(s, self, pk)) return 1;
        }
    return 0;
}

/* ================================================================== constructor paths and patterns */

static int path_nres(Node *path) {
    int n = 0;
    while (n < path->list.n && path->list.v[n]->sym) n++;
    return n;
}

static Type *variant_adt(Node *path, int n, Sym *v) {
    Sym *en = v->parent;
    Type *t;
    Node *seg;
    long k;
    int j = 0;
    if (n >= 2 && path->list.v[n - 2]->sym == en) return res(ty_from_path_prefix(path, n - 1));
    t = adt_fresh(en);
    seg = path->list.v[n - 1];
    for (k = 0; k < seg->list.n; k++) {
        Node *a = seg->list.v[k];
        if (a->kind == N_LIFETIME || a->kind == N_ASSOC_BIND) continue;
        if (j < t->nargs) uni(t->args[j], lower_targ(a, 0));
        j++;
    }
    return t;
}

/* does the path name a struct or an enum variant constructor?  Fills the constructor symbol
   (struct or variant) and the ADT type (with inference variables for missing arguments). */
static int resolve_ctor(Node *path, Sym **vsp, Type **adtp) {
    Sym *s = (Sym *)path->sym;
    int n, total = (int)path->list.n;
    Type *t;
    if (path->flags & F_QUALIFIED) return 0;
    n = path_nres(path);
    if (n == total) {
        if (!s) return 0;
        switch (s->kind) {
        case SYM_STRUCT: case SYM_UNION:
            *vsp = s;
            *adtp = res(ty_from_path_prefix(path, n));
            return 1;
        case SYM_SELF_TY: case SYM_TYPEALIAS:
            t = res(ty_from_path_prefix(path, n));
            if (t->kind == TY_ADT && (t->def->kind == SYM_STRUCT || t->def->kind == SYM_UNION)) {
                *vsp = t->def;
                *adtp = t;
                return 1;
            }
            return 0;
        case SYM_VARIANT:
            *vsp = s;
            *adtp = variant_adt(path, n, s);
            return 1;
        }
        return 0;
    }
    if (n >= 1 && total == n + 1) {
        Sym *ls = (Sym *)path->list.v[n - 1]->sym;
        if (ls && (ls->kind == SYM_SELF_TY || ls->kind == SYM_ENUM || ls->kind == SYM_TYPEALIAS)) {
            Sym *v;
            t = res(ty_from_path_prefix(path, n));
            if (t->kind == TY_ADT && t->def->kind == SYM_ENUM) {
                v = find_variant(t->def, path->list.v[n]->name);
                if (v) {
                    *vsp = v;
                    *adtp = t;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void pat_mismatch(Node *p, Type *expected, const char *found) {
    error_at(p->file, p->line, "mismatched types: expected `%s`, found %s", tn(expected), found);
}

static int is_rest_pat(Node *p) {
    return p->kind == N_P_REST;
}

/* binds the type of a P_IDENT binding */
static void bind_local(Node *p, Type *bt) {
    Sym *s = (Sym *)p->sym;
    if (s->ty) {
        if (!uni(s->ty, bt)) mismatch(p, s->ty, bt);
    } else {
        set_local_ty(s, bt);
    }
    s->prim = closure_depth;
}

static void ck_subpats(Node *p, Sym *vs, Type *adt, int bm, const char *what) {
    int nf = count_fields(vs), i, nsub = (int)p->list.n, rest = -1, fixed;
    for (i = 0; i < nsub; i++)
        if (is_rest_pat(p->list.v[i])) rest = i;
    fixed = rest >= 0 ? nsub - 1 : nsub;
    if ((rest < 0 && fixed != nf) || (rest >= 0 && fixed > nf))
        error_at(p->file, p->line,
                 "this pattern has %d field%s, but the corresponding %s has %d field%s", fixed,
                 fixed == 1 ? "" : "s", what, nf, nf == 1 ? "" : "s");
    for (i = 0; i < nsub; i++) {
        Node *sp = p->list.v[i];
        int fi;
        if (i == rest) continue;
        fi = (rest >= 0 && i > rest) ? nf - (nsub - i) : i;
        ck_pat(sp, field_ty(find_field(vs, 0, fi), adt), bm);
    }
}

static void ck_pat(Node *p, Type *ty, int bm) {
    Type *t = res(ty);
    int peeled = 0, peel = 1;
    Sym *s;
    if (!t) t = ty_infer(0);

    switch (p->kind) {
    case N_P_WILD: case N_P_REST: case N_P_REF: case N_P_OR:
        peel = 0;
        break;
    case N_P_IDENT:
        s = (Sym *)p->sym;
        if (!s || s->kind == SYM_LOCAL) peel = 0;
        else if (s->kind == SYM_CONST || s->kind == SYM_STATIC) peel = 0;
        break;
    case N_P_LIT:
        if (p->a && (p->a->kind == N_E_LIT_STR || p->a->kind == N_E_LIT_BYTESTR)) peel = 0;
        break;
    case N_P_PATH:
        s = (Sym *)p->sym;
        if (s && (s->kind == SYM_CONST || s->kind == SYM_STATIC)) peel = 0;
        break;
    }
    if (peel) {
        while (t->kind == TY_REF) {
            bm = (t->mut && bm != 1) ? 2 : 1;
            t = res(t->elem);
            peeled++;
        }
    }
    setty(p, t);
    p->adj = peeled;

    switch (p->kind) {
    case N_P_WILD:
    case N_P_REST:
        break;
    case N_P_IDENT:
        s = (Sym *)p->sym;
        if (s && s->kind != SYM_LOCAL) {
            Sym *vs = 0;
            Type *adt = 0;
            if (s->kind == SYM_CONST || s->kind == SYM_STATIC) {
                Type *ct = s->ty ? norm(s->ty) : ty_infer(0);
                if (!uni(ct, t)) mismatch(p, t, ct);
            } else {
                Node *tmp = p->a;
                (void)tmp;
                if (s->kind == SYM_VARIANT) {
                    adt = adt_fresh(s->parent);
                } else {
                    adt = adt_fresh(s);
                }
                vs = s;
                (void)vs;
                if (!uni(adt, t)) mismatch(p, t, adt);
            }
            break;
        }
        {
            Type *bt = t;
            if (p->flags & F_REF) {
                bt = ty_ref(t, (p->flags & F_MUT) != 0);
                p->adj |= (p->flags & F_MUT) ? ADJ_AUTOREF_MUT : ADJ_AUTOREF;
            } else if (bm == 1) {
                bt = ty_ref(t, 0);
                p->adj |= ADJ_AUTOREF;
            } else if (bm == 2) {
                bt = ty_ref(t, 1);
                p->adj |= ADJ_AUTOREF_MUT;
            }
            if (p->a && p->a->kind == N_P_REST) {
                /* `rest @ ..` inside a slice pattern: the caller gave the rest type as ty */
                setty(p->a, t);
            } else if (p->a) {
                ck_pat(p->a, t, bm);
            }
            bind_local(p, bt);
        }
        break;
    case N_P_LIT: {
        Type *lt;
        Node *lit = p->a;
        lt = ck_expr(lit, t);
        if ((p->flags & F_NEG) && !ty_is_numeric(lt))
            error_at(p->file, p->line, "cannot apply unary operator `-` to type `%s`", tn(lt));
        if (!uni(lt, t)) mismatch(p, t, lt);
        break;
    }
    case N_P_RANGE:
        if (p->a) ck_pat(p->a, t, bm);
        if (p->b) ck_pat(p->b, t, bm);
        break;
    case N_P_REF: {
        Type *inner;
        if (t->kind == TY_INFER) {
            inner = ty_infer(0);
            uni(t, ty_ref(inner, (p->flags & F_MUT) != 0));
        } else if (t->kind == TY_REF) {
            inner = t->elem;
            if ((p->flags & F_MUT) && !t->mut) pat_mismatch(p, t, "`&mut _`");
        } else {
            pat_mismatch(p, t, (p->flags & F_MUT) ? "`&mut _`" : "`&_`");
            return;
        }
        ck_pat(p->a, inner, 0);
        break;
    }
    case N_P_TUPLE: {
        int nsub = (int)p->list.n, rest = -1, i, fixed;
        Type **elems;
        for (i = 0; i < nsub; i++)
            if (is_rest_pat(p->list.v[i])) rest = i;
        fixed = rest >= 0 ? nsub - 1 : nsub;
        if (t->kind == TY_INFER) {
            if (rest >= 0) error_at(p->file, p->line, "type annotations needed");
            elems = tvec(nsub);
            for (i = 0; i < nsub; i++) elems[i] = ty_infer(0);
            uni(t, ty_tuple(elems, nsub));
            t = res(t);
            setty(p, t);
        }
        if (t->kind != TY_TUPLE) {
            pat_mismatch(p, t, "tuple");
            return;
        }
        if ((rest < 0 && fixed != t->nargs) || (rest >= 0 && fixed > t->nargs))
            error_at(p->file, p->line,
                     "mismatched types: expected a tuple with %d element%s, found one with %d element%s",
                     t->nargs, t->nargs == 1 ? "" : "s", fixed, fixed == 1 ? "" : "s");
        for (i = 0; i < nsub; i++) {
            int fi;
            if (i == rest) continue;
            fi = (rest >= 0 && i > rest) ? t->nargs - (nsub - i) : i;
            ck_pat(p->list.v[i], t->args[fi], bm);
        }
        break;
    }
    case N_P_SLICE: {
        int nsub = (int)p->list.n, i, rest = -1, fixed;
        Type *elem;
        for (i = 0; i < nsub; i++) {
            Node *sp = p->list.v[i];
            if (is_rest_pat(sp) || (sp->kind == N_P_IDENT && sp->a && sp->a->kind == N_P_REST)) rest = i;
        }
        fixed = rest >= 0 ? nsub - 1 : nsub;
        if (t->kind == TY_INFER) {
            elem = ty_infer(0);
            uni(t, rest >= 0 ? ty_slice(elem) : ty_array(elem, fixed));
            t = res(t);
            setty(p, t);
        }
        if (t->kind == TY_ARRAY && t->elem) {
            elem = t->elem;
            if (rest < 0 && t->len_param == 0 && t->len != fixed)
                error_at(p->file, p->line,
                         "mismatched types: expected an array with a fixed size of %ld element%s, found one with %d element%s",
                         t->len, t->len == 1 ? "" : "s", fixed, fixed == 1 ? "" : "s");
        } else if (t->kind == TY_SLICE) {
            elem = t->elem;
        } else {
            pat_mismatch(p, t, "slice pattern");
            return;
        }
        for (i = 0; i < nsub; i++) {
            Node *sp = p->list.v[i];
            if (i == rest) {
                if (sp->kind == N_P_IDENT) {
                    Type *rt = t->kind == TY_ARRAY && t->len_param == 0 ? ty_array(elem, t->len - fixed)
                                                                           : ty_slice(elem);
                    setty(sp, rt);
                    ck_pat(sp, rt, bm);
                } else {
                    setty(sp, t);
                }
                continue;
            }
            ck_pat(sp, elem, bm);
        }
        break;
    }
    case N_P_PATH: {
        Sym *vs = 0;
        Type *adt = 0;
        s = (Sym *)p->sym;
        if (resolve_ctor(p->a, &vs, &adt)) {
            if (!uni(adt, t)) mismatch(p, t, adt);
        } else if (s && (s->kind == SYM_CONST || s->kind == SYM_STATIC)) {
            Type *ct = s->ty ? norm(s->ty) : ty_infer(0);
            if (!uni(ct, t)) mismatch(p, t, ct);
        } else {
            /* an associated const: Type::C */
            Pick pk;
            int n = path_nres(p->a);
            Type *self;
            if (n >= 1 && (p->a->flags & F_QUALIFIED) == 0 && p->a->list.n == n + 1) {
                self = ty_from_path_prefix(p->a, n);
                if (lookup_assoc(self, p->a->list.v[n]->name, &pk) && pk.item->kind == SYM_CONST) {
                    Type *ct = norm(isub(pk.item->ty, &pk.S));
                    p->sym = pk.item;
                    if (!uni(ct, t)) mismatch(p, t, ct);
                    break;
                }
            }
            error_at(p->file, p->line, "expected unit struct, unit variant or constant in pattern");
        }
        break;
    }
    case N_P_TUPLE_STRUCT: {
        Sym *vs = 0;
        Type *adt = 0;
        if (!resolve_ctor(p->a, &vs, &adt) || !is_tuple_like(vs))
            error_at(p->file, p->line, "expected tuple struct or tuple variant, found %s",
                     p->a->list.n ? p->a->list.v[p->a->list.n - 1]->name->s : "?");
        if (!uni(adt, t)) mismatch(p, t, adt);
        p->sym = vs;
        ck_subpats(p, vs, res(adt), bm, vs->kind == SYM_VARIANT ? "tuple variant" : "tuple struct");
        break;
    }
    case N_P_STRUCT: {
        Sym *vs = 0;
        Type *adt = 0;
        long k;
        int nf, seen = 0;
        if (!resolve_ctor(p->a, &vs, &adt))
            error_at(p->file, p->line, "expected struct, variant or union type");
        if (!uni(adt, t)) mismatch(p, t, adt);
        p->sym = vs;
        adt = res(adt);
        nf = count_fields(vs);
        for (k = 0; k < p->list.n; k++) {
            Node *fp = p->list.v[k];
            Sym *f;
            char *nm = fp->name ? fp->name->s : "";
            int idx = -1;
            if (nm[0] >= '0' && nm[0] <= '9') idx = atoi(nm);
            f = idx >= 0 ? find_field(vs, 0, idx) : find_field(vs, fp->name, 0);
            if (!f)
                error_at(fp->file, fp->line, "%s `%s` does not have a field named `%s`",
                         vs->kind == SYM_VARIANT ? "variant" : "struct", vs->name->s, nm);
            fp->sym = f;
            setty(fp, field_ty(f, adt));
            ck_pat(fp->a, (Type *)fp->ty, bm);
            seen++;
        }
        if (!(p->flags & F_REST) && seen < nf) {
            int i;
            for (i = 0; i < nf; i++) {
                Sym *f = find_field(vs, 0, i);
                int found = 0;
                for (k = 0; k < p->list.n; k++)
                    if (p->list.v[k]->sym == f) found = 1;
                if (!found)
                    error_at(p->file, p->line, "pattern does not mention field `%s`",
                             f->name ? f->name->s : "?");
            }
        }
        break;
    }
    case N_P_OR: {
        long k;
        for (k = 0; k < p->list.n; k++) ck_pat(p->list.v[k], t, bm);
        break;
    }
    default:
        error_at(p->file, p->line, "unsupported pattern");
    }
}

/* ================================================================== privacy */

static Sym *cur_module(void) {
    Sym *m = cur_item;
    while (m && m->kind != SYM_MOD) m = m->parent;
    return m;
}

static Sym *def_module(Sym *s) {
    while (s && s->kind != SYM_MOD) s = s->parent;
    return s;
}

static int vis_ok(Sym *item) {
    Sym *from, *def;
    Sym *p;
    if (in_core || (item->flags & SF_PUB)) return 1;
    p = item->parent;
    if (item->kind == SYM_FIELD && p && p->kind == SYM_VARIANT) return 1;
    from = cur_module();
    def = def_module(p ? p : item);
    if (!from || !def) return 1;
    for (; from; from = from->parent)
        if (from == def) return 1;
    return 0;
}

static void check_field_vis(Node *at, Sym *field, Type *adt) {
    if (!vis_ok(field))
        error_at(at->file, at->line, "field `%s` of %s `%s` is private", field->name ? field->name->s : "?",
                 field->parent && field->parent->kind == SYM_UNION ? "union" : "struct", tn(adt));
}

static void check_fn_vis(Node *at, Sym *fn, int method) {
    Sym *o = fn->parent;
    if (!o || o->kind != SYM_IMPL || o->trait) return;
    if (!vis_ok(fn))
        error_at(at->file, at->line, "%s `%s` is private", method ? "method" : "associated function",
                 fn->name ? fn->name->s : "?");
}

/* ================================================================== expressions: paths */

static int cur_div = 0;

static Type *ctor_value(Node *e, Sym *vs, Type *adt) {
    e->sym = vs;
    if (is_unit_like(vs)) return adt;
    if (is_tuple_like(vs)) return ctor_fn_type(vs, res(adt));
    error_at(e->file, e->line, "expected value, found struct %s `%s`",
             vs->kind == SYM_VARIANT ? "variant" : "", vs->name ? vs->name->s : "?");
    return 0;
}

static const char *assoc_kind_word(Type *t) {
    return kind_word(t);
}

static Type *ck_assoc_path(Node *e, Node *path, int nres) {
    Type *self, *ty;
    Sym *trait = 0, *item, *vs = 0;
    Node *seg;
    Name *name;
    Pick pk;
    Subst S;
    int total = (int)path->list.n;
    Type *adt = 0;

    if (resolve_ctor(path, &vs, &adt)) return ctor_value(e, vs, adt);
    if (path->flags & F_QUALIFIED) {
        self = lower_ty(path->a);
        trait = (Sym *)path->sym;
        if (total != 1) error_at(e->file, e->line, "unsupported qualified path");
        seg = path->list.v[0];
        name = seg->name;
    } else {
        Sym *ls;
        if (nres < 1 || total != nres + 1) error_at(e->file, e->line, "unsupported path");
        ls = (Sym *)path->list.v[nres - 1]->sym;
        if (ls->kind == SYM_TRAIT) {
            trait = ls;
            self = ty_infer(0);
        } else {
            self = norm(ty_from_path_prefix(path, nres));
        }
        seg = path->list.v[nres];
        name = seg->name;
    }
    if (trait) {
        item = assoc_item(trait, NS_VALUE, name);
        if (!item)
            error_at(e->file, e->line, "cannot find method or associated constant `%s` in trait `%s`",
                     name->s, trait->name ? trait->name->s : "?");
        build_subst(item->kind == SYM_FN ? item : 0, trait, &S, self);
        if (item->kind == SYM_FN) {
            apply_turbofish(item, &S, seg, e);
            ty = inst_sig(item, &S, self, e, 1);
            e->sym = item;
            rec_callee(e, 0, item, ty, self);
            return ty;
        }
        ob_node = e;
        ob_pred(self, trait, trait_args_of(&S, trait), trait->ngparams, 0, 0);
        e->sym = item;
        return norm(isub(item->ty, &S));
    }
    if (res(self)->kind == TY_INFER)
        error_at(e->file, e->line, "type annotations needed");
    if (!lookup_assoc(self, name, &pk)) {
        Type *r = res(self);
        if (r->kind == TY_ADT && r->def->kind == SYM_ENUM)
            error_at(e->file, e->line, "no variant or associated item named `%s` found for enum `%s` in the current scope",
                     name->s, tn(self));
        error_at(e->file, e->line, "no function or associated item named `%s` found for %s `%s` in the current scope",
                 name->s, assoc_kind_word(self), tn(self));
    }
    item = pk.item;
    e->sym = item;
    if (item->kind == SYM_FN) {
        check_fn_vis(e, item, (item->flags & SF_METHOD) != 0 && 0);
        apply_turbofish(item, &pk.S, seg, e);
        ty = inst_sig(item, &pk.S, pk.self, e, 1);
        rec_callee(e, 0, item, ty, pk.self);
        return ty;
    }
    if (pk.self) {
        ob_node = e;
        ob_pred(pk.self, pk.owner, trait_args_of(&pk.S, pk.owner), pk.owner->ngparams, 0, 0);
    }
    if (!item->ty) error_at(e->file, e->line, "internal error: constant `%s` has no type", name->s);
    return norm(isub(item->ty, &pk.S));
}

static Type *ck_path(Node *e, Type *expect) {
    Node *path = e->a;
    Sym *s = (Sym *)path->sym, *vs = 0;
    Type *adt = 0;
    int n = (int)path->list.n, nres;
    (void)expect;
    if (path->flags & F_QUALIFIED) return ck_assoc_path(e, path, 0);
    nres = path_nres(path);
    if (nres < n) return ck_assoc_path(e, path, nres);
    if (!s) error_at(e->file, e->line, "unresolved path");
    switch (s->kind) {
    case SYM_LOCAL:
        if (!s->ty) error_at(e->file, e->line, "internal error: local `%s` has no type", s->name->s);
        return s->ty;
    case SYM_FN:
        e->sym = s;
        {
            Type *t = inst_fn_path(s, path->list.v[n - 1], e, 0);
            rec_callee(e, 0, s, t, 0);
            return t;
        }
    case SYM_CONST: case SYM_STATIC:
        if (!s->ty) error_at(e->file, e->line, "internal error: `%s` has no type", s->name->s);
        return norm(s->ty);
    case SYM_CONST_GENERIC:
        return TUSIZE;
    case SYM_STRUCT: case SYM_VARIANT: case SYM_SELF_TY: case SYM_TYPEALIAS:
        if (resolve_ctor(path, &vs, &adt)) return ctor_value(e, vs, adt);
        break;
    }
    error_at(e->file, e->line, "expected value, found %s `%s`", sym_kind_name(s->kind),
             s->name ? s->name->s : "?");
    return 0;
}

/* ================================================================== expressions: arguments and calls */

static Type **fudge_hints(Type *ret, Type **params, int np, Type *hint) {
    Type **h;
    int m, i;
    Type *hh;
    if (!hint || np == 0) return 0;
    hh = res(hint);
    if (!hh || hh->kind == TY_INFER) return 0;
    if (!ty_has_infer(ret)) return 0;
    m = snap();
    h = tvec(np);
    if (cu(ret, hh)) {
        for (i = 0; i < np; i++) h[i] = zonk(params[i]);
    } else {
        h = 0;
    }
    rollback(m);
    return h;
}

static void check_args(Node *call, Node **args, int na, Type **params, int np, Type **hints,
                       int variadic, const char *what) {
    int pass, i;
    if (na != np && !(variadic && na >= np))
        error_at(call->file, call->line, "this %s takes %d argument%s but %d argument%s %s supplied", what,
                 np, np == 1 ? "" : "s", na, na == 1 ? "" : "s", na == 1 ? "was" : "were");
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < na; i++) {
            Node *a = args[i];
            int isclo = a->kind == N_E_CLOSURE;
            Type *t;
            if ((pass == 0) == isclo) continue;
            if (i >= np) {
                ck_expr(a, 0);
                continue;
            }
            if (hints && hints[i]) {
                t = ck(a, hints[i], 0);
                if (!coerce(a, t, params[i])) mismatch(a, params[i], t);
            } else {
                ck_coerce(a, params[i]);
            }
        }
    }
    ob_node = call;
    solve_obs();
}

static Sym *trait_fn(Sym *trait, const char *name) {
    Sym *f = assoc_item(trait, NS_VALUE, name_intern_str(name));
    return f;
}

/* the signature (parameter types, return type) a value of type U can be called with, through
   Fn bounds, dyn Fn, closures and fn types.  kind: 1 Fn, 2 FnMut, 3 FnOnce */
static int callable_sig(Type *u, Type ***ptys, int *np, Type **ret, int *kind) {
    int i;
    u = res(u);
    if (!u) return 0;
    switch (u->kind) {
    case TY_CLOSURE: case TY_FN_DEF: case TY_FN_PTR:
        *ptys = u->params;
        *np = u->nparams;
        *ret = u->ret;
        *kind = 1;
        return 1;
    case TY_DYN:
        if (is_fn_trait(u->def) && u->nargs >= 1) {
            Type *tup = res(u->args[0]);
            *np = tup->kind == TY_TUPLE ? tup->nargs : 0;
            *ptys = tup->kind == TY_TUPLE ? tup->args : 0;
            *ret = TUNIT;
            for (i = 0; i < u->nparams; i++)
                if (u->params[i]->name == nm_output) *ret = u->params[i]->ret;
            *kind = u->def == L_fn ? 1 : u->def == L_fn_mut ? 2 : 3;
            return 1;
        }
        return 0;
    case TY_PARAM: case TY_SELF: case TY_PROJ:
        for (i = 0; i < nenv; i++) {
            Bound *b = env[i];
            if (!is_fn_trait(b->trait) || !ty_equal(b->ty, u) || b->nargs < 1) continue;
            {
                Type *tup = res(b->args[0]);
                *np = tup->kind == TY_TUPLE ? tup->nargs : 0;
                *ptys = tup->kind == TY_TUPLE ? tup->args : 0;
                *ret = b->assoc_ty ? b->assoc_ty : TUNIT;
                *kind = b->trait == L_fn ? 1 : b->trait == L_fn_mut ? 2 : 3;
                return 1;
            }
        }
        return 0;
    case TY_INFER:
        for (i = 0; i < nobs; i++) {
            Ob *o = &obs[i];
            if (o->kind != 0 || o->done || !is_fn_trait(o->trait) || o->nargs < 1) continue;
            if (res(o->self) != u) continue;
            {
                Type *tup = res(o->args[0]);
                *np = tup->kind == TY_TUPLE ? tup->nargs : 0;
                *ptys = tup->kind == TY_TUPLE ? tup->args : 0;
                *ret = o->aty ? o->aty : TUNIT;
                *kind = o->trait == L_fn ? 1 : o->trait == L_fn_mut ? 2 : 3;
                return 1;
            }
        }
        return 0;
    }
    return 0;
}

static Type *ck_call(Node *e, Type *hint) {
    Node *callee = e->a;
    Type *ft = ck_expr(callee, 0), *f = res(ft);
    Step steps[MAXSTEPS];
    int n, i;
    Type **hints;

    if (f->kind == TY_INFER) {
        ob_node = e;
        solve_obs();
        f = res(ft);
    }
    if (f->kind == TY_FN_DEF) {
        Sym *d = f->def;
        const char *what = d->kind == SYM_FN ? "function" : d->kind == SYM_VARIANT ? "enum variant" : "struct";
        int variadic = d->kind == SYM_FN && d->node && (d->node->flags & F_VARIADIC);
        hints = fudge_hints(f->ret, f->params, f->nparams, hint);
        check_args(e, e->list.v, (int)e->list.n, f->params, f->nparams, hints, variadic, what);
        return f->ret;
    }
    if (f->kind == TY_FN_PTR) {
        hints = fudge_hints(f->ret, f->params, f->nparams, hint);
        check_args(e, e->list.v, (int)e->list.n, f->params, f->nparams, hints, 0, "function");
        return f->ret;
    }
    n = make_steps(f, steps);
    for (i = 0; i < n; i++) {
        Type **pt = 0, *ret = 0, *u = res(steps[i].ty);
        int np = 0, kind = 0;
        if (!callable_sig(u, &pt, &np, &ret, &kind)) continue;
        callee->adj |= steps[i].nderef;
        {
            Sym *fnsym = trait_fn(kind == 1 ? L_fn : kind == 2 ? L_fn_mut : L_fn_once,
                                  kind == 1 ? "call" : kind == 2 ? "call_mut" : "call_once");
            Type **pp = tvec(2);
            pp[0] = kind == 3 ? u : ty_ref(u, kind == 2);
            pp[1] = ty_tuple(pt, np);
            if (fnsym) rec_callee(e, 0, fnsym, ty_fn_def(fnsym, 0, 0, pp, 2, ret), u);
        }
        check_args(e, e->list.v, (int)e->list.n, pt, np, 0, 0, "function");
        return ret;
    }
    error_at(callee->file, callee->line, "expected function, found `%s`", tn(f));
    return 0;
}

/* ================================================================== method calls */

static int trial_method(Sym *fn, Type *ft, int trait_cand, Pick *pk) {
    Sym *owner = fn->parent;
    int m, r = 1;
    Subst S;
    Type *self = 0, *pt;
    if (!(fn->flags & SF_METHOD) || !fn->ty || fn->ty->nparams < 1 || !owner) return 0;
    m = snap();
    if (trait_cand) self = ty_infer(0);
    build_subst(fn, owner, &S, self);
    pt = isub(fn->ty->params[0], &S);
    if (!cu(pt, ft)) {
        rollback(m);
        return 0;
    }
    if (trait_cand) {
        r = solve_pred(self, owner, trait_args_of(&S, owner), owner->ngparams);
        if (r == 0) {
            rollback(m);
            return 0;
        }
    }
    pk->item = fn;
    pk->owner = owner;
    pk->S = S;
    pk->self = self;
    pk->pred = r;
    return 1;
}

static int probe_at(NEnt *ne, Type *ft, Pick *pk) {
    int i, pass;
    for (i = 0; i < ne->n; i++) {
        Sym *c = ne->v[i];
        if (c->kind != SYM_FN || !owner_is_inherent(c)) continue;
        if (trial_method(c, ft, 0, pk)) return 1;
    }
    for (pass = 0; pass < 2; pass++)
        for (i = 0; i < ne->n; i++) {
            Sym *c = ne->v[i];
            if (c->kind != SYM_FN || !c->parent || c->parent->kind != SYM_TRAIT) continue;
            if ((pass == 0) != (trait_in_scope(c->parent) != 0)) continue;
            if (trial_method(c, ft, 1, pk)) return 1;
        }
    return 0;
}

static Type *ck_method_call(Node *e, Type *hint) {
    Type *rt = ck_expr(e->a, 0), *r0;
    Step steps[MAXSTEPS];
    int n, i, form;
    NEnt *ne = nent(e->name, 0);
    Pick pk;
    Type *sig;
    Type **hints;

    r0 = res(rt);
    if (r0->kind == TY_INFER) {
        ob_node = e;
        solve_obs();
        r0 = res(rt);
        if (r0->kind == TY_INFER) {
            if (r0->ik == INFER_INT) uni(r0, ty_int(IK_I32));
            else if (r0->ik == INFER_FLOAT) uni(r0, ty_float(64));
            else error_at(e->file, e->line, "type annotations needed");
        }
    }
    n = make_steps(rt, steps);
    if (ne) {
        for (i = 0; i < n; i++) {
            Type *u = steps[i].ty;
            for (form = 0; form < 3; form++) {
                Type *ft = form == 0 ? u : ty_ref(u, form == 2);
                if (probe_at(ne, ft, &pk)) {
                    Node *recv = e->a;
                    recv->adj |= steps[i].nderef | (form == 1 ? ADJ_AUTOREF : form == 2 ? ADJ_AUTOREF_MUT : 0) |
                                 (steps[i].unsize ? ADJ_UNSIZE : 0);
                    goto picked;
                }
            }
        }
    }
    error_at(e->file, e->line, "no method named `%s` found for %s `%s` in the current scope", e->name->s,
             kind_word(r0), tn(r0));
picked:
    e->sym = pk.item;
    check_fn_vis(e, pk.item, 1);
    apply_turbofish(pk.item, &pk.S, e->b, e);
    sig = inst_sig(pk.item, &pk.S, pk.self, e, 1);
    rec_callee(e, 0, pk.item, sig, pk.self);
    hints = fudge_hints(sig->ret, sig->params + 1, sig->nparams - 1, hint);
    check_args(e, e->list.v, (int)e->list.n, sig->params + 1, sig->nparams - 1, hints, 0, "method");
    return sig->ret;
}

/* ================================================================== expressions: operators */

static int numeric_like(Type *t) {
    t = res(t);
    return t && (t->kind == TY_INT || t->kind == TY_FLOAT || (t->kind == TY_INFER && t->ik != 0));
}

static int int_like(Type *t) {
    t = res(t);
    return t && (t->kind == TY_INT || (t->kind == TY_INFER && t->ik == INFER_INT));
}

/* calls the operator trait method: records the callee, returns the method's return type or 0 */
static Type *op_trait_call(Node *e, Sym *trait, const char *mname, Type *self, Type *rhs) {
    Sym *fn;
    Subst S;
    Type **ta, *sig;
    int pr;
    if (!trait) return 0;
    fn = trait_fn(trait, mname);
    if (!fn) error_at(e->file, e->line, "internal error: trait method `%s` missing", mname);
    {
        int m = snap();
        build_subst(fn, trait, &S, self);
        ta = trait_args_of(&S, trait);
        pr = 0;
        if (!rhs || trait->ngparams == 0 || uni(ta[0], rhs)) pr = solve_pred(self, trait, ta, trait->ngparams);
        if (pr == 0 && rhs && res(rhs)->kind == TY_REF && trait->ngparams > 0 && e->b) {
            /* the right operand is a coercion site: `string + &other_string` */
            rollback(m);
            build_subst(fn, trait, &S, self);
            ta = trait_args_of(&S, trait);
            pr = solve_pred(self, trait, ta, trait->ngparams);
            if (pr == 1 && !is_var(ta[0]) && coerce(e->b, rhs, ta[0])) {
                /* ok */
            } else {
                pr = 0;
            }
        }
        if (pr == 0) {
            rollback(m);
            return 0;
        }
    }
    ob_node = e;
    if (pr == 2) ob_pred(self, trait, ta, trait->ngparams, 0, 0);
    sig = inst_sig(fn, &S, self, e, 0);
    e->sym = fn;
    rec_callee(e, 0, fn, sig, self);
    return sig->ret;
}

static void bad_binop(Node *e, int op, Type *l, Type *r) {
    const char *ls = tn(l), *rs = tn(r);
    char *lc = xstrdup(ls);
    switch (op) {
    case OP_ADD: error_at(e->file, e->line, "cannot add `%s` to `%s`", rs, lc); break;
    case OP_SUB: error_at(e->file, e->line, "cannot subtract `%s` from `%s`", rs, lc); break;
    case OP_MUL: error_at(e->file, e->line, "cannot multiply `%s` by `%s`", lc, rs); break;
    case OP_DIV: error_at(e->file, e->line, "cannot divide `%s` by `%s`", lc, rs); break;
    case OP_REM: error_at(e->file, e->line, "cannot mod `%s` by `%s`", lc, rs); break;
    case OP_EQ: case OP_NE: case OP_LT: case OP_LE: case OP_GT: case OP_GE:
        if (strcmp(lc, rs) == 0)
            error_at(e->file, e->line, "binary operation `%s` cannot be applied to type `%s`", op_spelling(op), lc);
        error_at(e->file, e->line, "can't compare `%s` with `%s`", lc, rs);
        break;
    default:
        error_at(e->file, e->line, "no implementation for `%s %s %s`", lc, op_spelling(op), rs);
    }
}

static Type *ck_binary(Node *e, Type *hint) {
    int op = e->op;
    Type *l, *r, *lr, *rr, *out;
    int cmp = op >= OP_EQ && op <= OP_GE;
    (void)hint;
    if (op == OP_LAND || op == OP_LOR) {
        ck_cond(e->a);
        ck_cond(e->b);
        return TBOOL;
    }
    l = ck_expr(e->a, 0);
    lr = res(l);
    r = ck_expr(e->b, numeric_like(lr) ? lr : (cmp ? lr : 0));
    rr = res(r);
    if (rr->kind == TY_INFER && rr->ik == 0 && lr->kind != TY_INFER) {
        /* an unconstrained right operand: assume the same type for built-in operations */
        if (numeric_like(lr) && op != OP_SHL && op != OP_SHR) { uni(r, l); rr = res(r); }
    }
    if (lr->kind == TY_INFER && lr->ik == 0) {
        ob_node = e;
        solve_obs();
        lr = res(l);
        if (lr->kind == TY_INFER && lr->ik == 0 && (numeric_like(rr) || rr->kind == TY_BOOL)) {
            if (op != OP_SHL && op != OP_SHR) uni(l, r);
            lr = res(l);
        }
    }
    if (op == OP_SHL || op == OP_SHR) {
        if (int_like(lr) && int_like(rr)) return l;
    } else if (cmp) {
        if (is_scalar(lr) && is_scalar(rr)) {
            if (!uni(l, r)) mismatch(e->b, l, r);
            return TBOOL;
        }
        if (ty_is_unit(lr) && ty_is_unit(rr)) return TBOOL;
    } else if (numeric_like(lr) && numeric_like(rr)) {
        if (!uni(l, r)) mismatch(e->b, l, r);
        lr = res(l);
        if ((op == OP_BITAND || op == OP_BITOR || op == OP_BITXOR) && lr->kind == TY_FLOAT) bad_binop(e, op, l, r);
        return l;
    } else if (lr->kind == TY_BOOL && rr->kind == TY_BOOL &&
               (op == OP_BITAND || op == OP_BITOR || op == OP_BITXOR)) {
        return TBOOL;
    }
    /* overloaded */
    if (int_like(lr) && rr->kind == TY_REF && int_like(rr->elem)) uni(l, rr->elem);
    if (is_floatvar(lr) && rr->kind == TY_REF && res(rr->elem)->kind == TY_FLOAT) uni(l, rr->elem);
    if (cmp) {
        Sym *tr = (op == OP_EQ || op == OP_NE) ? L_eq : L_partial_ord;
        const char *mn = op == OP_EQ ? "eq" : op == OP_NE ? "ne" : op == OP_LT ? "lt" : op == OP_LE ? "le" :
                         op == OP_GT ? "gt" : "ge";
        out = op_trait_call(e, tr, mn, l, r);
        if (!out) {
            int m = snap();
            if (uni(l, r)) out = op_trait_call(e, tr, mn, l, r);
            if (!out) { rollback(m); bad_binop(e, op, l, r); }
        }
        return TBOOL;
    }
    out = op_trait_call(e, L_ops[op], op_method_name(op), l, r);
    if (!out) bad_binop(e, op, l, r);
    out = norm(out);
    /* `&{integer} * {integer}`: the result is the pointee type (as for the std impls) */
    if (lr->kind == TY_REF && numeric_like(lr->elem) && numeric_like(rr) &&
        (is_var(lr->elem) || res(lr->elem)->kind == TY_INFER))
        uni(out, lr->elem);
    return out;
}

static Type *ck_assign_op(Node *e) {
    int op = e->op;
    Type *l = ck_expr(e->a, 0), *r, *lr = res(l), *rr, *out;
    char nm[40];
    r = ck_expr(e->b, numeric_like(lr) ? lr : 0);
    rr = res(r);
    if (lr->kind == TY_INFER && lr->ik == 0 && numeric_like(rr)) { uni(l, r); lr = res(l); }
    if (op == OP_SHL || op == OP_SHR) {
        if (int_like(lr) && int_like(rr)) return TUNIT;
    } else if (numeric_like(lr) && numeric_like(rr)) {
        if (!uni(l, r)) mismatch(e->b, l, r);
        return TUNIT;
    } else if (lr->kind == TY_BOOL && rr->kind == TY_BOOL &&
               (op == OP_BITAND || op == OP_BITOR || op == OP_BITXOR)) {
        return TUNIT;
    }
    if (int_like(lr) && rr->kind == TY_REF && int_like(rr->elem)) uni(l, rr->elem);
    strcpy(nm, op_method_name(op));
    strcat(nm, "_assign");
    out = op_trait_call(e, L_assign_ops[op], nm, l, r);
    if (!out) {
        const char *ls = xstrdup(tn(l));
        error_at(e->file, e->line, "binary assignment operation `%s=` cannot be applied to type `%s`",
                 op_spelling(op), ls);
    }
    (void)rr;
    return TUNIT;
}

static void note_neg_lit(Node *lit);

static Type *ck_unary(Node *e) {
    Type *t = ck_expr(e->a, 0), *r = res(t), *out;
    if (e->op == OP_NEG) {
        if (e->a->kind == N_E_LIT_INT) note_neg_lit(e->a);
        if (r->kind == TY_FLOAT || r->kind == TY_INT || (r->kind == TY_INFER && r->ik != 0)) {
            if (r->kind == TY_INT && r->ik >= IK_U8 && r->ik != IK_ISIZE)
                error_at(e->file, e->line, "cannot apply unary operator `-` to type `%s`", tn(t));
            return t;
        }
    } else {
        if (r->kind == TY_BOOL || int_like(r)) return t;
    }
    out = op_trait_call(e, L_ops[e->op], op_method_name(e->op), t, 0);
    if (!out) error_at(e->file, e->line, "cannot apply unary operator `%s` to type `%s`", e->op == OP_NEG ? "-" : "!", tn(t));
    return norm(out);
}

static Type *ck_deref(Node *e) {
    Type *t = ck_expr(e->a, 0), *r = res(t), *next;
    int k;
    if (r->kind == TY_REF || r->kind == TY_PTR) return r->elem;
    if (r->kind == TY_INFER) {
        ob_node = e;
        solve_obs();
        r = res(t);
        if (r->kind == TY_INFER) error_at(e->file, e->line, "type annotations needed");
        if (r->kind == TY_REF || r->kind == TY_PTR) return r->elem;
    }
    k = deref_once(r, &next);
    if (k == 2) {
        Subst S;
        Sym *fn = trait_fn(L_deref, "deref");
        Type *sig;
        build_subst(fn, L_deref, &S, r);
        sig = inst_sig(fn, &S, r, e, 0);
        e->sym = fn;
        rec_callee(e, 0, fn, sig, r);
        return next;
    }
    error_at(e->file, e->line, "type `%s` cannot be dereferenced", tn(t));
    return 0;
}

static int is_range_adt(Type *t) {
    t = res(t);
    return t && t->kind == TY_ADT &&
           (t->def == L_range || t->def == L_range_inclusive || t->def == L_range_from ||
            t->def == L_range_to || t->def == L_range_to_inclusive || t->def == L_range_full);
}

static Type *ck_index(Node *e) {
    Type *bt = ck_expr(e->a, 0), *it, *ir;
    Step steps[MAXSTEPS];
    int n, i;
    bt = res(bt);
    if (bt->kind == TY_INFER) {
        ob_node = e;
        solve_obs();
        bt = res(bt);
        if (bt->kind == TY_INFER) error_at(e->file, e->line, "type annotations needed");
    }
    it = ck_expr(e->b, 0);
    ir = res(it);
    n = make_steps(bt, steps);
    for (i = 0; i < n; i++) {
        Type *u = res(steps[i].ty);
        if ((u->kind == TY_ARRAY && u->elem) || u->kind == TY_SLICE || u->kind == TY_STR) {
            int builtin = 0;
            Type *result = 0;
            if (u->kind != TY_STR && int_like(ir)) {
                if (!uni(it, TUSIZE)) {
                    error_at(e->file, e->line, "the type `%s` cannot be indexed by `%s`", tn(u), tn(it));
                }
                builtin = 1;
                result = u->elem;
            } else if (is_range_adt(ir)) {
                if (ir->nargs > 0 && !uni(ir->args[0], TUSIZE)) {
                    error_at(e->file, e->line, "the type `%s` cannot be indexed by `%s`", tn(u), tn(it));
                }
                builtin = 1;
                result = u->kind == TY_STR ? TSTR : ty_slice(u->elem);
            }
            if (builtin) {
                e->a->adj |= steps[i].nderef | (steps[i].unsize ? ADJ_UNSIZE : 0);
                return result;
            }
        }
        if (u->kind == TY_INFER) continue;
        {
            Sym *fn = trait_fn(L_index, "index");
            Subst S;
            Type **ta, *sig;
            int pr;
            int m = snap();
            build_subst(fn, L_index, &S, u);
            ta = trait_args_of(&S, L_index);
            if (L_index->ngparams > 0) uni(ta[0], it);
            pr = solve_pred(u, L_index, ta, L_index->ngparams);
            if (pr != 0) {
                if (pr == 2) { ob_node = e; ob_pred(u, L_index, ta, L_index->ngparams, 0, 0); }
                sig = inst_sig(fn, &S, u, e, 0);
                e->sym = fn;
                rec_callee(e, 0, fn, sig, u);
                e->a->adj |= steps[i].nderef;
                /* sig->ret is &Output */
                {
                    Type *rr = res(sig->ret);
                    return rr->kind == TY_REF ? rr->elem : sig->ret;
                }
            }
            rollback(m);
        }
    }
    error_at(e->file, e->line, "cannot index into a value of type `%s`", tn(bt));
    return 0;
}

static Type *ck_field(Node *e) {
    Type *bt = ck_expr(e->a, 0), *r;
    Step steps[MAXSTEPS];
    int n, i;
    r = res(bt);
    if (r->kind == TY_INFER) {
        ob_node = e;
        solve_obs();
        r = res(bt);
        if (r->kind == TY_INFER) error_at(e->file, e->line, "type annotations needed");
    }
    n = make_steps(bt, steps);
    for (i = 0; i < n; i++) {
        Type *u = res(steps[i].ty);
        Sym *f = 0;
        if (steps[i].unsize) continue;
        if (e->kind == N_E_FIELD) {
            if (u->kind == TY_ADT && (u->def->kind == SYM_STRUCT || u->def->kind == SYM_UNION)) {
                f = find_field(u->def, e->name, 0);
                if (f) {
                    check_field_vis(e, f, u);
                    e->sym = f;
                    e->a->adj |= steps[i].nderef;
                    return field_ty(f, u);
                }
            }
        } else {
            int idx = (int)e->ival;
            if (u->kind == TY_TUPLE && idx < u->nargs) {
                e->a->adj |= steps[i].nderef;
                return u->args[idx];
            }
            if (u->kind == TY_ADT && u->def->kind == SYM_STRUCT && (f = find_field(u->def, 0, idx)) != 0) {
                check_field_vis(e, f, u);
                e->sym = f;
                e->a->adj |= steps[i].nderef;
                return field_ty(f, u);
            }
        }
    }
    if (e->kind == N_E_FIELD)
        error_at(e->file, e->line, "no field `%s` on type `%s`", e->name->s, tn(bt));
    error_at(e->file, e->line, "no field `%d` on type `%s`", (int)e->ival, tn(bt));
    return 0;
}

/* ================================================================== casts */

static int is_c_like_enum(Type *t) {
    t = res(t);
    if (!t || t->kind != TY_ADT || t->def->kind != SYM_ENUM) return 0;
    {
        int i, n = scope_member_count(t->def->members);
        for (i = 0; i < n; i++) {
            Sym *v = scope_member(t->def->members, i);
            if (v && v->kind == SYM_VARIANT && !is_unit_like(v)) return 0;
        }
    }
    return 1;
}

static Type *ck_cast(Node *e) {
    Type *to = lower_ty(e->b), *from, *f, *t;
    t = res(to);
    from = ck_expr(e->a, 0);
    f = res(from);
    if (f->kind == TY_INFER && f->ik == INFER_INT && t->kind == TY_INT) { uni(f, t); return to; }
    if (f->kind == TY_INFER && f->ik == INFER_FLOAT && t->kind == TY_FLOAT) { uni(f, t); return to; }
    if (f->kind == TY_INFER && f->ik == 0) {
        ob_node = e;
        solve_obs();
        f = res(from);
        if (f->kind == TY_INFER && f->ik == 0) { uni(f, t); return to; }
    }
    if (numeric_like(f) && (t->kind == TY_INT || t->kind == TY_FLOAT)) return to;
    if ((f->kind == TY_BOOL || f->kind == TY_CHAR) && t->kind == TY_INT) return to;
    if (is_c_like_enum(f) && t->kind == TY_INT) return to;
    if (t->kind == TY_CHAR) {
        if (f->kind == TY_CHAR) return to;
        if (f->kind == TY_INT && f->ik == IK_U8) return to;
        if (f->kind == TY_INFER && f->ik == INFER_INT) { uni(f, ty_int(IK_U8)); return to; }
        if (numeric_like(f))
            error_at(e->file, e->line, "only `u8` can be cast as `char`, not `%s`", tn(f));
    }
    if (f->kind == t->kind && ty_equal(f, t)) return to;
    if (t->kind == TY_PTR && (f->kind == TY_PTR || f->kind == TY_REF)) return to;
    if (t->kind == TY_INT && (f->kind == TY_PTR || f->kind == TY_FN_DEF || f->kind == TY_FN_PTR)) return to;
    if (t->kind == TY_PTR && (int_like(f) || f->kind == TY_FN_DEF)) return to;
    if (t->kind == TY_FN_PTR && (f->kind == TY_FN_DEF || f->kind == TY_CLOSURE || f->kind == TY_FN_PTR)) {
        if (coerce(e->a, f, t)) return to;
    }
    if ((t->kind == TY_REF || t->kind == TY_ADT || t->kind == TY_FN_PTR) && coerce(e->a, f, t)) return to;
    if (numeric_like(f) && t->kind != TY_INT && t->kind != TY_FLOAT)
        error_at(e->file, e->line, "casting `%s` as `%s` is invalid", tn(f), tn(t));
    if ((f->kind == TY_BOOL || f->kind == TY_CHAR) && t->kind == TY_FLOAT)
        error_at(e->file, e->line, "casting `%s` as `%s` is invalid", tn(f), tn(t));
    error_at(e->file, e->line, "non-primitive cast: `%s` as `%s`", tn(f), tn(t));
    return to;
}

/* ================================================================== expressions: literals, structs, closures */

static int suffix_kind(Name *n, int *ik, int *isfloat) {
    static const char *names[] = {"i8", "i16", "i32", "i64", "i128", "isize",
                                  "u8", "u16", "u32", "u64", "u128", "usize"};
    int i;
    if (!n) return 0;
    for (i = 0; i < 12; i++)
        if (strcmp(n->s, names[i]) == 0) { *ik = IK_I8 + i; *isfloat = 0; return 1; }
    if (strcmp(n->s, "f32") == 0) { *ik = 32; *isfloat = 1; return 1; }
    if (strcmp(n->s, "f64") == 0) { *ik = 64; *isfloat = 1; return 1; }
    return 0;
}

static Type *ck_lit_int(Node *e, Type *expect) {
    int ik = 0, isf = 0;
    Type *x;
    if (suffix_kind(e->name, &ik, &isf)) return isf ? ty_float(ik) : ty_int(ik);
    x = expect ? res(expect) : 0;
    if (x && x->kind == TY_INT) return x;
    return ty_infer(INFER_INT);
}

static Type *ck_lit_float(Node *e, Type *expect) {
    int ik = 0, isf = 0;
    Type *x;
    if (suffix_kind(e->name, &ik, &isf) && isf) return ty_float(ik);
    x = expect ? res(expect) : 0;
    if (x && x->kind == TY_FLOAT) return x;
    return ty_infer(INFER_FLOAT);
}

static Type *ck_struct(Node *e, Type *expect) {
    Sym *vs = 0;
    Type *adt = 0, *x;
    long k;
    int nf, seen = 0, i;
    if (!resolve_ctor(e->a, &vs, &adt)) error_at(e->file, e->line, "expected struct, variant or union type");
    x = expect ? res(expect) : 0;
    if (x && x->kind == TY_ADT && x->def == adt_owner(vs)) uni(adt, x);
    adt = res(adt);
    e->sym = vs;
    nf = count_fields(vs);
    for (k = 0; k < e->list.n; k++) {
        Node *fi = e->list.v[k];
        Sym *f;
        char *nm = fi->name ? fi->name->s : "";
        int idx = -1;
        if (nm[0] >= '0' && nm[0] <= '9') idx = atoi(nm);
        f = idx >= 0 ? find_field(vs, 0, idx) : find_field(vs, fi->name, 0);
        if (!f)
            error_at(fi->file, fi->line, "%s `%s` has no field named `%s`",
                     vs->kind == SYM_VARIANT ? "variant" : "struct", vs->name->s, nm);
        for (i = 0; i < k; i++)
            if (e->list.v[i]->sym == f)
                error_at(fi->file, fi->line, "field `%s` specified more than once", nm);
        check_field_vis(fi, f, adt);
        fi->sym = f;
        setty(fi, ck_coerce(fi->a, field_ty(f, adt)));
        seen++;
    }
    if (vs->kind == SYM_UNION) return adt;
    if (e->b) {
        ck_coerce(e->b, adt);
    } else if (seen < nf) {
        StrBuf sb;
        int miss = 0;
        sb_init(&sb);
        for (i = 0; i < nf; i++) {
            Sym *f = find_field(vs, 0, i);
            int found = 0;
            for (k = 0; k < e->list.n; k++)
                if (e->list.v[k]->sym == f) found = 1;
            if (found) continue;
            if (miss) sb_puts(&sb, ", ");
            sb_putc(&sb, '`');
            sb_puts(&sb, f->name ? f->name->s : "?");
            sb_putc(&sb, '`');
            miss++;
        }
        error_at(e->file, e->line, "missing field%s %s in initializer of `%s`", miss == 1 ? "" : "s", sb.s,
                 tn(adt));
    }
    return adt;
}

static Type *ck_array(Node *e, Type *hint) {
    Type *he = 0, *elem = 0, *x = hint ? res(hint) : 0;
    long i;
    if (x && (x->kind == TY_ARRAY || x->kind == TY_SLICE) && x->elem) he = x->elem;
    if (he && res(he)->kind == TY_INFER) he = 0;
    if (e->list.n == 0) return ty_array(he ? he : ty_infer(0), 0);
    if (he) {
        elem = he;
        for (i = 0; i < e->list.n; i++) ck_coerce(e->list.v[i], elem);
    } else {
        elem = ck_expr(e->list.v[0], 0);
        for (i = 1; i < e->list.n; i++) ck_coerce(e->list.v[i], elem);
    }
    return ty_array(elem, (long)e->list.n);
}

static Type *ck_repeat(Node *e, Type *hint) {
    Type *he = 0, *elem, *x = hint ? res(hint) : 0;
    long len;
    Node *c = e->b;
    Sym *s;
    if (x && (x->kind == TY_ARRAY || x->kind == TY_SLICE) && x->elem) he = x->elem;
    elem = he ? ck_coerce(e->a, he) : ck_expr(e->a, 0);
    ck_coerce(e->b, TUSIZE);
    while (c->kind == N_E_PAREN) c = c->a;
    if (c->kind == N_E_PATH) {
        s = (Sym *)c->sym;
        if (s && s->kind == SYM_CONST_GENERIC) return ty_array_param(elem, s);
    }
    if (!ck_const_eval(e->b, &len, 0)) error_at(e->line ? e->file : 0, e->line, "array length must be a constant");
    return ty_array(elem, len);
}

static Type *ck_tuple(Node *e, Type *hint) {
    Type *x = hint ? res(hint) : 0, **v;
    long i;
    if (e->list.n == 0) return TUNIT;
    v = tvec((int)e->list.n);
    for (i = 0; i < e->list.n; i++) {
        Type *h = (x && x->kind == TY_TUPLE && i < x->nargs) ? x->args[i] : 0;
        v[i] = ck_expr(e->list.v[i], h);
    }
    return ty_tuple(v, (int)e->list.n);
}

static Type *ck_range(Node *e, Type *hint) {
    Type *t = 0, *a = 0, *b = 0, *x = hint ? res(hint) : 0, *eh = 0, *ty;
    Sym *def;
    if (x && is_range_adt(x) && x->nargs > 0) eh = x->args[0];
    if (e->a) a = ck_expr(e->a, eh);
    if (e->b) b = ck_expr(e->b, a ? a : eh);
    if (a && b && !uni(a, b)) mismatch(e->b, a, b);
    t = a ? a : b;
    if (a && b) def = (e->flags & F_INCLUSIVE) ? L_range_inclusive : L_range;
    else if (a) def = L_range_from;
    else if (b) def = (e->flags & F_INCLUSIVE) ? L_range_to_inclusive : L_range_to;
    else def = L_range_full;
    if (t) {
        Type *args[1];
        args[0] = t;
        ty = ty_adt(def, args, 1);
    } else {
        ty = ty_adt(def, 0, 0);
    }
    if (a && b && (e->flags & F_INCLUSIVE)) {
        Sym *fn = lang_item("range_inclusive_new");
        Subst S;
        Type *sig;
        int i;
        build_subst(fn, fn->parent, &S, 0);
        for (i = 0; i < S.n; i++)
            if (S.params[i] == fn->parent->gparams[0]) uni(S.args[i], t);
        sig = inst_sig(fn, &S, 0, e, 0);
        rec_callee(e, 0, fn, sig, 0);
    }
    return ty;
}

static int closure_sig_hint(Type *expect, Type ***ptys, int *np, Type **ret) {
    Type *x = expect ? res(expect) : 0;
    int i;
    if (!x) return 0;
    if (x->kind == TY_FN_PTR) {
        *ptys = x->params;
        *np = x->nparams;
        *ret = x->ret;
        return 1;
    }
    if (x->kind == TY_DYN && is_fn_trait(x->def) && x->nargs >= 1) {
        Type *tup = res(x->args[0]);
        if (tup->kind != TY_TUPLE) return 0;
        *ptys = tup->args;
        *np = tup->nargs;
        *ret = 0;
        for (i = 0; i < x->nparams; i++)
            if (x->params[i]->name == nm_output) *ret = x->params[i]->ret;
        if (!*ret) *ret = TUNIT;
        return 1;
    }
    if (x->kind == TY_INFER) {
        for (i = 0; i < nobs; i++) {
            Ob *o = &obs[i];
            Type *tup;
            if (o->kind != 0 || o->done || !is_fn_trait(o->trait) || o->nargs < 1) continue;
            if (res(o->self) != x) continue;
            tup = res(o->args[0]);
            if (tup->kind != TY_TUPLE) continue;
            *ptys = tup->args;
            *np = tup->nargs;
            *ret = o->aty ? o->aty : TUNIT;
            return 1;
        }
    }
    return 0;
}

static Node **closures = 0;
static int *closure_depths = 0;
static int nclosures = 0, cclosures = 0;

static Type *ck_closure(Node *e, Type *expect) {
    int np = (int)e->list.n, i, has, hn = 0;
    Type **hp = 0, *hret = 0, **params, *ret, *cty;
    has = closure_sig_hint(expect, &hp, &hn, &hret);
    if (has && hn != np) has = 0;
    params = tvec(np);
    for (i = 0; i < np; i++) {
        Node *p = e->list.v[i];
        Type *pt;
        if (p->b) {
            pt = lower_ty(p->b);
            if (has) uni(pt, hp[i]);
        } else {
            pt = has ? hp[i] : ty_infer(0);
        }
        params[i] = pt;
    }
    ret = e->a ? lower_ty(e->a) : ((has && hret) ? hret : ty_infer(0));
    closure_depth++;
    for (i = 0; i < np; i++)
        if (e->list.v[i]->a) ck_pat(e->list.v[i]->a, params[i], 0);
    cty = ty_closure(e, params, np, ret);
    setty(e, cty);
    push_fcx(ret, e);
    ck_coerce(e->b, ret);
    pop_fcx();
    closure_depth--;
    if (nclosures >= cclosures) {
        cclosures = cclosures ? cclosures * 2 : 16;
        closures = (Node **)xrealloc(closures, sizeof(Node *) * (size_t)cclosures);
        closure_depths = (int *)xrealloc(closure_depths, sizeof(int) * (size_t)cclosures);
    }
    closure_depths[nclosures] = closure_depth + 1;
    closures[nclosures++] = e;
    return cty;
}

/* ================================================================== control flow */

static void ck_cond(Node *c) {
    if (c->kind == N_E_LET) {
        Type *st = ck_expr(c->b, 0);
        ck_pat(c->a, st, 0);
        setty(c, TBOOL);
        return;
    }
    ck_coerce(c, TBOOL);
}

/* the common type of two branches: coerce b to a, or a to b */
static Type *join_branches(Node *an, Type *a, Node *bn, Type *b, const char *what) {
    Type *ra = res(a), *rb = res(b);
    if (ra->kind == TY_NEVER) return b;
    if (rb->kind == TY_NEVER) {
        if (bn) bn->adj |= ADJ_NEVER;
        return a;
    }
    if (coerce(bn, b, a)) return a;
    if (coerce(an, a, b)) return b;
    error_at(bn->file, bn->line, "%s: expected `%s`, found `%s`", what, tn(a), tn_found(b));
    return a;
}

static Type *ck_if(Node *e, Type *expect, int strict) {
    Type *t1, *t2;
    ck_cond(e->a);
    if (!e->c) {
        t1 = ck_block(e->b, TUNIT, 1);
        (void)t1;
        return TUNIT;
    }
    if (strict && expect) {
        Type *r1 = ck_block(e->b, expect, 1);
        Type *r2 = ck(e->c, expect, 1);
        if (res(r1)->kind == TY_NEVER && res(r2)->kind == TY_NEVER) return TNEVER;
        return expect;
    }
    t1 = ck_block(e->b, expect, 0);
    t2 = ck(e->c, expect, 0);
    return join_branches(e->b, t1, e->c, t2, "`if` and `else` have incompatible types");
}

static Type *ck_match(Node *e, Type *expect, int strict) {
    Type *st = ck_expr(e->a, 0), *result = 0;
    long i;
    int all_never = 1;
    for (i = 0; i < e->list.n; i++) {
        Node *arm = e->list.v[i];
        Type *at;
        ck_pat(arm->a, st, 0);
        if (arm->b) ck_cond(arm->b);
        if (strict && expect) {
            at = ck_coerce(arm->c, expect);
            if (res(arm->c->ty ? (Type *)arm->c->ty : at)->kind != TY_NEVER) all_never = 0;
            result = expect;
        } else {
            at = ck(arm->c, expect, 0);
            if (!result) {
                result = at;
                if (res(at)->kind != TY_NEVER) all_never = 0;
            } else {
                if (res(at)->kind != TY_NEVER) all_never = 0;
                result = join_branches(0, result, arm->c, at, "`match` arms have incompatible types");
            }
        }
    }
    check_match_exhaustive(e, st);
    if (e->list.n == 0 || all_never) return TNEVER;
    return result;
}

static Loop *find_loop(Node *e, Name *label) {
    int i;
    for (i = nloops - 1; i >= loop_base; i--) {
        if (label) {
            if (loops[i].label == label) return &loops[i];
        } else if (loops[i].kind != 2) {
            return &loops[i];
        }
    }
    if (nfcx > 0 && fcx[nfcx - 1].closure)
        error_at(e->file, e->line, "`%s` inside of a closure", e->kind == N_E_BREAK ? "break" : "continue");
    error_at(e->file, e->line, "`%s` outside of a loop or labeled block", e->kind == N_E_BREAK ? "break" : "continue");
    return 0;
}

static Type *ck_for(Node *e) {
    Type *it = ck_expr(e->b, 0), *item, *iter_ty;
    Sym *f_into, *f_next;
    Subst S;
    Type *sig;
    int pr;
    Loop *lp;
    ob_node = e->b;
    pr = solve_pred(it, L_into_iterator, 0, 0);
    if (pr == 0) error_at(e->b->file, e->b->line, "`%s` is not an iterator", tn(it));
    if (pr == 2) ob_pred(it, L_into_iterator, 0, 0, 0, 0);
    item = proj_norm(it, L_into_iterator, nm_item_name, 0, 0);
    iter_ty = proj_norm(it, L_into_iterator, name_intern_str("IntoIter"), 0, 0);
    f_into = trait_fn(L_into_iterator, "into_iter");
    build_subst(f_into, L_into_iterator, &S, it);
    sig = inst_sig(f_into, &S, it, e, 0);
    rec_callee(e, 0, f_into, sig, it);
    f_next = trait_fn(L_iterator, "next");
    build_subst(f_next, L_iterator, &S, iter_ty);
    sig = inst_sig(f_next, &S, iter_ty, e, 0);
    rec_callee(e, 1, f_next, sig, iter_ty);
    lp = push_loop(e->name, 1, 0);
    (void)lp;
    ck_pat(e->a, item, 0);
    ck_block(e->c, TUNIT, 1);
    nloops--;
    return TUNIT;
}

static Type *ck_try(Node *e) {
    Type *t = ck_expr(e->a, 0), *r = res(t), *fr, *frr;
    if (r->kind == TY_INFER) {
        ob_node = e;
        solve_obs();
        r = res(t);
        if (r->kind == TY_INFER) error_at(e->file, e->line, "type annotations needed");
    }
    if (nfcx == 0) error_at(e->file, e->line, "the `?` operator can only be used inside a function");
    fr = fcx[nfcx - 1].ret;
    frr = res(fr);
    if (r->kind == TY_ADT && r->def == L_result && r->nargs == 2) {
        Type *E = r->args[1], *F, *Er, *Fr;
        if (frr->kind == TY_INFER) {
            Type *a2[2];
            a2[0] = ty_infer(0);
            a2[1] = ty_infer(0);
            uni(frr, ty_adt(L_result, a2, 2));
            frr = res(fr);
        }
        if (frr->kind == TY_ADT && frr->def == L_option)
            error_at(e->file, e->line,
                     "the `?` operator can only be used on `Option`s, not `Result`s, in a function that returns `Option`");
        if (frr->kind != TY_ADT || frr->def != L_result || frr->nargs != 2)
            error_at(e->file, e->line,
                     "the `?` operator can only be used in a function that returns `Result` or `Option` (or another type that implements `FromResidual`)");
        F = frr->args[1];
        Er = res(E);
        Fr = res(F);
        if (Fr->kind == TY_INFER && Fr->ik == 0) {
            uni(E, F);
        } else if (Er->kind == TY_INFER || !ty_equal(Er, Fr)) {
            Sym *fn = trait_fn(L_from, "from");
            Subst S;
            Type **ta, *sig;
            int pr;
            build_subst(fn, L_from, &S, F);
            ta = trait_args_of(&S, L_from);
            uni(ta[0], E);
            pr = solve_pred(F, L_from, ta, 1);
            if (pr == 0)
                error_at(e->file, e->line, "`?` couldn't convert the error to `%s`: the trait `From<%s>` is not implemented for `%s`",
                         tn(F), tn(E), tn(F));
            ob_node = e;
            if (pr == 2) ob_pred(F, L_from, ta, 1, 0, 0);
            sig = inst_sig(fn, &S, F, e, 0);
            rec_callee(e, 0, fn, sig, F);
        }
        return r->args[0];
    }
    if (r->kind == TY_ADT && r->def == L_option && r->nargs == 1) {
        if (frr->kind == TY_INFER) {
            Type *a1[1];
            a1[0] = ty_infer(0);
            uni(frr, ty_adt(L_option, a1, 1));
            frr = res(fr);
        }
        if (frr->kind == TY_ADT && frr->def == L_result)
            error_at(e->file, e->line,
                     "the `?` operator can only be used on `Result`s, not `Option`s, in a function that returns `Result`");
        if (frr->kind != TY_ADT || frr->def != L_option)
            error_at(e->file, e->line,
                     "the `?` operator can only be used in a function that returns `Result` or `Option` (or another type that implements `FromResidual`)");
        return r->args[0];
    }
    error_at(e->file, e->line,
             "the `?` operator can only be applied to values that implement `Try`");
    return 0;
}

/* ================================================================== blocks and statements */

static int is_item_node(int k) {
    switch (k) {
    case N_ITEM_FN: case N_ITEM_STRUCT: case N_ITEM_ENUM: case N_ITEM_UNION: case N_ITEM_TRAIT:
    case N_ITEM_IMPL: case N_ITEM_MOD: case N_ITEM_CONST: case N_ITEM_STATIC: case N_ITEM_TYPE:
    case N_ITEM_EXTERN_BLOCK: case N_ITEM_USE: case N_ITEM_MACRO_RULES: case N_ITEM_EXTERN_CRATE:
        return 1;
    }
    return 0;
}

static Type *ck_block(Node *b, Type *expect, int strict) {
    long i;
    int sdiv = cur_div;
    Type *result;
    Loop *lp = 0;
    int lpidx = -1;
    cur_div = 0;
    if (b->name) {
        lp = push_loop(b->name, 2, ty_infer(0));
        lpidx = nloops - 1;
    }
    for (i = 0; i < b->list.n; i++) {
        Node *s = b->list.v[i];
        if (s->kind == N_STMT_LET) {
            Type *dt = s->b ? lower_ty(s->b) : 0, *ty = 0, *it;
            if (s->c) {
                if (dt) {
                    ck_coerce(s->c, dt);
                    ty = dt;
                } else {
                    it = ck_expr(s->c, 0);
                    ty = it;
                }
            }
            if (!ty) ty = ty_infer(0);
            if (res(ty)->kind == TY_NEVER) ty = ty_infer(0);
            ck_pat(s->a, ty, 0);
            if (s->d) {
                Type *dty = ck_block(s->d, 0, 0);
                if (res(dty)->kind != TY_NEVER)
                    error_at(s->d->file, s->d->line, "`else` clause of `let...else` does not diverge");
            } else if (s->c || 1) {
                check_irrefutable(s->a, ty, s, "local binding");
            }
        } else if (s->kind == N_STMT_EXPR) {
            if (s->flags & F_SEMI) {
                ck_expr(s->a, 0);
            } else {
                ck_coerce(s->a, TUNIT);
            }
        } else if (s->kind == N_STMT_EMPTY) {
            continue;
        } else if (is_item_node(s->kind)) {
            queue_item(s);
        } else if (s->kind == N_MACRO_CALL) {
            error_at(s->file, s->line, "cannot find macro in this scope (unexpanded macro)");
        }
    }
    if (b->a) {
        if (lp) {
            result = ck_coerce(b->a, loops[lpidx].brk);
        } else if (strict && expect) {
            result = ck_coerce(b->a, expect);
            if (res(b->a->ty ? (Type *)b->a->ty : result)->kind == TY_NEVER && !b->a->adj) result = expect;
        } else {
            result = ck(b->a, expect, 0);
        }
    } else if (cur_div) {
        result = TNEVER;
    } else if (lp) {
        if (!uni(loops[lpidx].brk, TUNIT)) mismatch(b, loops[lpidx].brk, TUNIT);
        result = loops[lpidx].brk;
    } else if (strict && expect) {
        if (!coerce(b, TUNIT, expect)) mismatch(b, expect, TUNIT);
        result = expect;
    } else {
        result = TUNIT;
    }
    if (lp) {
        if (res(result)->kind == TY_NEVER && !loops[lpidx].has_break) {
            /* stays never */
        } else {
            result = loops[lpidx].brk;
        }
        nloops = lpidx;
    }
    cur_div = sdiv;
    return result;
}

/* ================================================================== the dispatcher */

static Type *ck_inner(Node *e, Type *expect, int strict) {
    switch (e->kind) {
    case N_E_LIT_INT: return ck_lit_int(e, expect);
    case N_E_LIT_FLOAT: return ck_lit_float(e, expect);
    case N_E_LIT_STR: return ty_ref(TSTR, 0);
    case N_E_LIT_BYTESTR: return ty_ref(ty_array(ty_int(IK_U8), e->slen), 0);
    case N_E_LIT_CHAR: return TCHAR;
    case N_E_LIT_BYTE: return ty_int(IK_U8);
    case N_E_LIT_BOOL: return TBOOL;
    case N_E_PATH: return ck_path(e, expect);
    case N_E_UNDERSCORE: return ty_infer(0);
    case N_E_PAREN: {
        Type *t = ck(e->a, expect, strict);
        e->adj = 0;
        return t;
    }
    case N_E_UNARY: return ck_unary(e);
    case N_E_DEREF: return ck_deref(e);
    case N_E_REF: {
        Type *h = 0, *hh = expect ? res(expect) : 0, *t;
        if (hh && hh->kind == TY_REF) h = hh->elem;
        t = ck_expr(e->a, h);
        return ty_ref(t, (e->flags & F_MUT) != 0);
    }
    case N_E_TRY: return ck_try(e);
    case N_E_BINARY: return ck_binary(e, expect);
    case N_E_ASSIGN: {
        Type *lt = ck_expr(e->a, 0);
        ck_coerce(e->b, lt);
        return TUNIT;
    }
    case N_E_ASSIGN_OP: return ck_assign_op(e);
    case N_E_CAST: return ck_cast(e);
    case N_E_CALL: return ck_call(e, expect);
    case N_E_METHOD_CALL: return ck_method_call(e, expect);
    case N_E_FIELD: case N_E_TUPLE_FIELD: return ck_field(e);
    case N_E_INDEX: return ck_index(e);
    case N_E_RANGE: return ck_range(e, expect);
    case N_BLOCK: return ck_block(e, expect, strict);
    case N_E_IF: return ck_if(e, expect, strict);
    case N_E_LET:
        ck_cond(e);
        return TBOOL;
    case N_E_MATCH: return ck_match(e, expect, strict);
    case N_E_WHILE: {
        Loop *lp;
        ck_cond(e->a);
        lp = push_loop(e->name, 1, 0);
        (void)lp;
        ck_block(e->b, TUNIT, 1);
        nloops--;
        return TUNIT;
    }
    case N_E_LOOP: {
        int idx;
        Type *bv = ty_infer(0);
        int had;
        push_loop(e->name, 0, bv);
        idx = nloops - 1;
        ck_block(e->a, TUNIT, 1);
        had = loops[idx].has_break;
        nloops = idx;
        if (!had) return TNEVER;
        return bv;
    }
    case N_E_FOR: return ck_for(e);
    case N_E_BREAK: {
        Loop *lp = find_loop(e, e->name);
        lp->has_break = 1;
        if (e->a) {
            if (lp->kind == 1)
                error_at(e->file, e->line, "`break` with value from a `%s` loop", "while");
            ck_coerce(e->a, lp->brk);
        } else if (lp->kind != 1) {
            if (!uni(lp->brk, TUNIT)) mismatch(e, lp->brk, TUNIT);
        }
        return TNEVER;
    }
    case N_E_CONTINUE:
        find_loop(e, e->name);
        return TNEVER;
    case N_E_RETURN: {
        Type *fr;
        if (nfcx == 0) error_at(e->file, e->line, "return statement outside of function body");
        fr = fcx[nfcx - 1].ret;
        if (e->a) ck_coerce(e->a, fr);
        else if (!uni(fr, TUNIT)) mismatch(e, fr, TUNIT);
        return TNEVER;
    }
    case N_E_CLOSURE: return ck_closure(e, expect);
    case N_E_STRUCT: return ck_struct(e, expect);
    case N_E_TUPLE: return ck_tuple(e, expect);
    case N_E_ARRAY: return ck_array(e, expect);
    case N_E_REPEAT: return ck_repeat(e, expect);
    case N_MACRO_CALL:
        error_at(e->file, e->line, "cannot find macro in this scope (unexpanded macro)");
        break;
    case N_E_AWAIT:
        error_at(e->file, e->line, "`await` is not supported");
        break;
    }
    error_at(e->file, e->line, "unsupported expression (%s)", node_kind_name(e->kind));
    return 0;
}

static Type *ck(Node *e, Type *expect, int strict) {
    Type *t = ck_inner(e, expect, strict);
    setty(e, t);
    if (t && res(t)->kind == TY_NEVER) cur_div = 1;
    return t;
}

static Type *ck_expr(Node *e, Type *hint) {
    return ck(e, hint, 0);
}

/* ================================================================== exhaustiveness of patterns */

enum { DP_WILD, DP_CTOR, DP_OR };
enum { C_VARIANT, C_SINGLE, C_BOOL, C_INT, C_STR, C_SLICE, C_OPAQUE };

typedef struct DP {
    int kind;
    int ctor;
    long idx;                /* variant position / bool value / int low / slice length (fixed) */
    long hi;                 /* int high */
    int arity;
    struct DP **sub;
    struct DP **alts;
    int nalts;
    int rest;                /* C_SLICE: has `..` */
    int prefix, suffix;      /* C_SLICE with rest */
    Sym *vs;
    char *text;              /* C_STR: the value, for equality */
} DP;

typedef struct {
    DP **c;
} Row;

static DP dp_wild_obj;
static DP *dp_wild(void) {
    dp_wild_obj.kind = DP_WILD;
    return &dp_wild_obj;
}

static DP *dp_new(int kind, int ctor) {
    DP *d = (DP *)ast_alloc(sizeof(DP));
    d->kind = kind;
    d->ctor = ctor;
    return d;
}

static DP *dp_wrap_ref(DP *d, int n) {
    while (n-- > 0) {
        DP *w = dp_new(DP_CTOR, C_SINGLE);
        w->arity = 1;
        w->sub = (DP **)ast_alloc(sizeof(DP *));
        w->sub[0] = d;
        d = w;
    }
    return d;
}

static int variant_pos(Sym *v) {
    Sym *en = v->parent;
    int i, n, pos = 0;
    if (!en || !en->members) return 0;
    n = scope_member_count(en->members);
    for (i = 0; i < n; i++) {
        Sym *m = scope_member(en->members, i);
        if (!m || m->kind != SYM_VARIANT) continue;
        if (m == v) return pos;
        pos++;
    }
    return 0;
}

static Sym *variant_at(Sym *en, int pos) {
    int i, n, k = 0;
    n = scope_member_count(en->members);
    for (i = 0; i < n; i++) {
        Sym *m = scope_member(en->members, i);
        if (!m || m->kind != SYM_VARIANT) continue;
        if (k++ == pos) return m;
    }
    return 0;
}

static int count_variants(Sym *en) {
    int i, n, k = 0;
    if (!en->members) return 0;
    n = scope_member_count(en->members);
    for (i = 0; i < n; i++) {
        Sym *m = scope_member(en->members, i);
        if (m && m->kind == SYM_VARIANT) k++;
    }
    return k;
}

static long lit_value(Node *lit, int neg, int *ok) {
    *ok = 1;
    switch (lit->kind) {
    case N_E_LIT_INT: return neg ? -(long)lit->ival : (long)lit->ival;
    case N_E_LIT_CHAR: case N_E_LIT_BYTE: return (long)lit->ival;
    case N_E_LIT_BOOL: return (long)lit->ival;
    }
    *ok = 0;
    return 0;
}

static DP *dp_from(Node *p);

static DP *dp_ctor_of(Node *p, Sym *vs, Node **subpats, int nsub, int has_rest, Node *tuple_rest_host) {
    DP *d = dp_new(DP_CTOR, vs->kind == SYM_VARIANT ? C_VARIANT : C_SINGLE);
    int nf = count_fields(vs), i, rest = -1;
    (void)tuple_rest_host;
    d->vs = vs;
    d->idx = vs->kind == SYM_VARIANT ? variant_pos(vs) : 0;
    d->arity = nf;
    d->sub = nf ? (DP **)ast_alloc(sizeof(DP *) * (size_t)nf) : 0;
    for (i = 0; i < nf; i++) d->sub[i] = dp_wild();
    for (i = 0; i < nsub; i++)
        if (subpats[i]->kind == N_P_REST) rest = i;
    (void)has_rest;
    for (i = 0; i < nsub; i++) {
        int fi;
        if (i == rest) continue;
        fi = (rest >= 0 && i > rest) ? nf - (nsub - i) : i;
        if (fi >= 0 && fi < nf) d->sub[fi] = dp_from(subpats[i]);
    }
    return d;
}

static DP *dp_from(Node *p) {
    DP *d = 0;
    int peel = p->adj & 0xFF;
    Sym *s;
    long k;
    switch (p->kind) {
    case N_P_WILD: case N_P_REST:
        return dp_wild();
    case N_P_IDENT:
        s = (Sym *)p->sym;
        if (!s || s->kind == SYM_LOCAL) {
            if (p->a && p->a->kind != N_P_REST) return dp_from(p->a);
            return dp_wild();
        }
        if (s->kind == SYM_VARIANT || s->kind == SYM_STRUCT) d = dp_ctor_of(p, s, 0, 0, 0, 0);
        else d = dp_new(DP_CTOR, C_OPAQUE);
        return dp_wrap_ref(d, peel);
    case N_P_LIT: {
        int ok;
        long v = lit_value(p->a, (p->flags & F_NEG) != 0, &ok);
        if (p->a->kind == N_E_LIT_BOOL) {
            d = dp_new(DP_CTOR, C_BOOL);
            d->idx = v;
        } else if (ok) {
            d = dp_new(DP_CTOR, C_INT);
            d->idx = v;
            d->hi = v;
        } else if (p->a->kind == N_E_LIT_STR) {
            d = dp_new(DP_CTOR, C_STR);
            d->text = p->a->str;
            d->idx = p->a->slen;
        } else {
            d = dp_new(DP_CTOR, C_OPAQUE);
        }
        return dp_wrap_ref(d, peel);
    }
    case N_P_RANGE: {
        int ok1 = 1, ok2 = 1;
        long lo = 0, hi = 0;
        d = dp_new(DP_CTOR, C_OPAQUE);
        if (p->a && p->a->kind == N_P_LIT) lo = lit_value(p->a->a, (p->a->flags & F_NEG) != 0, &ok1); else ok1 = 0;
        if (p->b && p->b->kind == N_P_LIT) hi = lit_value(p->b->a, (p->b->flags & F_NEG) != 0, &ok2); else ok2 = 0;
        if (ok1 && ok2) {
            d->ctor = C_INT;
            d->idx = lo;
            d->hi = (p->flags & F_INCLUSIVE) ? hi : hi - 1;
        }
        return dp_wrap_ref(d, peel);
    }
    case N_P_REF:
        d = dp_new(DP_CTOR, C_SINGLE);
        d->arity = 1;
        d->sub = (DP **)ast_alloc(sizeof(DP *));
        d->sub[0] = dp_from(p->a);
        return dp_wrap_ref(d, peel);
    case N_P_TUPLE: {
        Type *t = p->ty ? res((Type *)p->ty) : 0;
        int nsub = (int)p->list.n, rest = -1, i, nf;
        d = dp_new(DP_CTOR, C_SINGLE);
        for (i = 0; i < nsub; i++)
            if (p->list.v[i]->kind == N_P_REST) rest = i;
        nf = t && t->kind == TY_TUPLE ? t->nargs : nsub - (rest >= 0 ? 1 : 0);
        d->arity = nf;
        d->sub = nf ? (DP **)ast_alloc(sizeof(DP *) * (size_t)nf) : 0;
        for (i = 0; i < nf; i++) d->sub[i] = dp_wild();
        for (i = 0; i < nsub; i++) {
            int fi;
            if (i == rest) continue;
            fi = (rest >= 0 && i > rest) ? nf - (nsub - i) : i;
            if (fi >= 0 && fi < nf) d->sub[fi] = dp_from(p->list.v[i]);
        }
        return dp_wrap_ref(d, peel);
    }
    case N_P_SLICE: {
        Type *t = p->ty ? res((Type *)p->ty) : 0;
        int nsub = (int)p->list.n, rest = -1, i;
        for (i = 0; i < nsub; i++) {
            Node *sp = p->list.v[i];
            if (sp->kind == N_P_REST || (sp->kind == N_P_IDENT && sp->a && sp->a->kind == N_P_REST)) rest = i;
        }
        if (t && t->kind == TY_ARRAY && t->elem && !t->len_param) {
            int nf = (int)t->len, fixed = rest >= 0 ? nsub - 1 : nsub;
            d = dp_new(DP_CTOR, C_SINGLE);
            d->arity = nf;
            d->sub = nf ? (DP **)ast_alloc(sizeof(DP *) * (size_t)nf) : 0;
            for (i = 0; i < nf; i++) d->sub[i] = dp_wild();
            (void)fixed;
            for (i = 0; i < nsub; i++) {
                int fi;
                if (i == rest) continue;
                fi = (rest >= 0 && i > rest) ? nf - (nsub - i) : i;
                if (fi >= 0 && fi < nf) d->sub[fi] = dp_from(p->list.v[i]);
            }
            return dp_wrap_ref(d, peel);
        }
        d = dp_new(DP_CTOR, C_SLICE);
        d->rest = rest >= 0;
        d->prefix = rest >= 0 ? rest : nsub;
        d->suffix = rest >= 0 ? nsub - rest - 1 : 0;
        d->idx = rest >= 0 ? d->prefix + d->suffix : nsub;
        d->arity = d->prefix + d->suffix;
        d->sub = d->arity ? (DP **)ast_alloc(sizeof(DP *) * (size_t)d->arity) : 0;
        for (i = 0; i < nsub; i++) {
            int fi;
            if (i == rest) continue;
            fi = (rest >= 0 && i > rest) ? d->prefix + (i - rest - 1) : i;
            d->sub[fi] = dp_from(p->list.v[i]);
        }
        return dp_wrap_ref(d, peel);
    }
    case N_P_PATH: {
        Sym *vs = 0;
        Type *adt = 0;
        if (resolve_ctor(p->a, &vs, &adt)) d = dp_ctor_of(p, vs, 0, 0, 0, 0);
        else d = dp_new(DP_CTOR, C_OPAQUE);
        return dp_wrap_ref(d, peel);
    }
    case N_P_TUPLE_STRUCT:
        s = (Sym *)p->sym;
        d = dp_ctor_of(p, s, p->list.v, (int)p->list.n, 0, 0);
        return dp_wrap_ref(d, peel);
    case N_P_STRUCT: {
        int nf, i;
        s = (Sym *)p->sym;
        d = dp_ctor_of(p, s, 0, 0, 0, 0);
        nf = count_fields(s);
        for (k = 0; k < p->list.n; k++) {
            Node *fp = p->list.v[k];
            for (i = 0; i < nf; i++)
                if (find_field(s, 0, i) == (Sym *)fp->sym) d->sub[i] = dp_from(fp->a);
        }
        return dp_wrap_ref(d, peel);
    }
    case N_P_OR: {
        d = dp_new(DP_OR, 0);
        d->nalts = (int)p->list.n;
        d->alts = (DP **)ast_alloc(sizeof(DP *) * (size_t)d->nalts);
        for (k = 0; k < p->list.n; k++) d->alts[k] = dp_from(p->list.v[k]);
        return d;
    }
    }
    return dp_wild();
}

/* ---- the signature of a type */

/* returns the number of constructors of the (finite) signature, -1 for infinite, -2 for slices */
static int sig_count(Type *t, int *kind) {
    t = res(t);
    if (!t) return -1;
    switch (t->kind) {
    case TY_BOOL: *kind = C_BOOL; return 2;
    case TY_NEVER: *kind = C_VARIANT; return 0;
    case TY_TUPLE: case TY_REF: *kind = C_SINGLE; return 1;
    case TY_ARRAY:
        if (t->elem && !t->len_param && t->len <= 64) { *kind = C_SINGLE; return 1; }
        return -1;
    case TY_SLICE: *kind = C_SLICE; return -2;
    case TY_ADT:
        if (t->def->kind == SYM_ENUM) { *kind = C_VARIANT; return count_variants(t->def); }
        *kind = C_SINGLE;
        return 1;
    }
    return -1;
}

static Type *adt_field_nn(Sym *field, Type *adt) {
    Subst S;
    if (!field->ty) return 0;
    subst_init(&S, adt->def->gparams, adt->args, adt->def->ngparams, 0);
    return isub(field->ty, &S);
}

/* arity and the types of the sub-columns of constructor (kind, idx) of type t */
static int ctor_subtypes(Type *t, int kind, long idx, Type ***out) {
    int n = 0, i;
    Type **v = 0;
    t = res(t);
    *out = 0;
    if (!t) return 0;
    if (kind == C_VARIANT && t->kind == TY_ADT) {
        Sym *vs = variant_at(t->def, (int)idx);
        if (!vs) return 0;
        n = count_fields(vs);
        v = tvec(n);
        for (i = 0; i < n; i++) v[i] = adt_field_nn(find_field(vs, 0, i), t);
    } else if (kind == C_SINGLE) {
        if (t->kind == TY_TUPLE) {
            n = t->nargs;
            v = tvec(n);
            for (i = 0; i < n; i++) v[i] = t->args[i];
        } else if (t->kind == TY_REF) {
            n = 1;
            v = tvec(1);
            v[0] = t->elem;
        } else if (t->kind == TY_ARRAY) {
            n = (int)t->len;
            v = tvec(n);
            for (i = 0; i < n; i++) v[i] = t->elem;
        } else if (t->kind == TY_ADT) {
            n = count_fields(t->def);
            v = tvec(n);
            for (i = 0; i < n; i++) v[i] = adt_field_nn(find_field(t->def, 0, i), t);
        }
    } else if (kind == C_SLICE && (t->kind == TY_SLICE || t->kind == TY_ARRAY)) {
        n = (int)idx;
        v = tvec(n);
        for (i = 0; i < n; i++) v[i] = t->elem;
    }
    *out = v;
    return n;
}

static int dp_matches(DP *h, int kind, long idx) {
    if (h->kind != DP_CTOR || h->ctor != kind) return 0;
    if (kind == C_VARIANT || kind == C_BOOL) return h->idx == idx;
    return 1;
}

static void row_push(Row **rows, int *n, int *cap, Row r) {
    if (*n >= *cap) {
        *cap = *cap ? *cap * 2 : 8;
        *rows = (Row *)xrealloc(*rows, sizeof(Row) * (size_t)*cap);
    }
    (*rows)[(*n)++] = r;
}

static void expand_row(Row r, int ncols, Row **rows, int *n, int *cap) {
    DP *h = r.c[0];
    if (h->kind == DP_OR) {
        int i;
        for (i = 0; i < h->nalts; i++) {
            Row r2;
            r2.c = (DP **)ast_alloc(sizeof(DP *) * (size_t)ncols);
            memcpy(r2.c, r.c, sizeof(DP *) * (size_t)ncols);
            r2.c[0] = h->alts[i];
            expand_row(r2, ncols, rows, n, cap);
        }
        return;
    }
    row_push(rows, n, cap, r);
}

/* rows specialised by constructor (kind, idx) with arity a; slices: idx is the length */
static Row *specialize(Row *rows, int nrows, int ncols, int kind, long idx, int a, int *nout) {
    Row *out = 0;
    int n = 0, cap = 0, i, j;
    for (i = 0; i < nrows; i++) {
        DP *h = rows[i].c[0];
        Row r;
        r.c = (DP **)ast_alloc(sizeof(DP *) * (size_t)(a + ncols - 1 + 1));
        if (h->kind == DP_WILD) {
            for (j = 0; j < a; j++) r.c[j] = dp_wild();
        } else if (kind == C_SLICE) {
            if (h->ctor != C_SLICE) continue;
            if (h->rest) {
                if (idx < h->prefix + h->suffix) continue;
                for (j = 0; j < h->prefix; j++) r.c[j] = h->sub[j];
                for (j = h->prefix; j < (int)idx - h->suffix; j++) r.c[j] = dp_wild();
                for (j = 0; j < h->suffix; j++) r.c[(int)idx - h->suffix + j] = h->sub[h->prefix + j];
            } else {
                if (h->idx != idx) continue;
                for (j = 0; j < a; j++) r.c[j] = h->sub[j];
            }
        } else {
            if (!dp_matches(h, kind, idx)) continue;
            for (j = 0; j < a; j++) r.c[j] = h->sub ? h->sub[j] : dp_wild();
        }
        for (j = 1; j < ncols; j++) r.c[a + j - 1] = rows[i].c[j];
        row_push(&out, &n, &cap, r);
    }
    *nout = n;
    return out;
}

static Row *default_rows(Row *rows, int nrows, int ncols, int *nout) {
    Row *out = 0;
    int n = 0, cap = 0, i, j;
    for (i = 0; i < nrows; i++) {
        Row r;
        if (rows[i].c[0]->kind != DP_WILD) continue;
        r.c = (DP **)ast_alloc(sizeof(DP *) * (size_t)ncols);
        for (j = 1; j < ncols; j++) r.c[j - 1] = rows[i].c[j];
        row_push(&out, &n, &cap, r);
    }
    *nout = n;
    return out;
}

static char *ctor_text(Type *t, int kind, long idx, char **subs, int nsub) {
    StrBuf sb;
    int i;
    Sym *vs = 0;
    sb_init(&sb);
    t = res(t);
    if (kind == C_BOOL) {
        sb_puts(&sb, idx ? "true" : "false");
    } else if (kind == C_VARIANT && t && t->kind == TY_ADT) {
        vs = variant_at(t->def, (int)idx);
        if (vs) {
            if (t->def != L_option && t->def != L_result) {
                sb_puts(&sb, t->def->name->s);
                sb_puts(&sb, "::");
            }
            sb_puts(&sb, vs->name->s);
            if (is_tuple_like(vs)) {
                sb_putc(&sb, '(');
                for (i = 0; i < nsub; i++) {
                    if (i) sb_puts(&sb, ", ");
                    sb_puts(&sb, subs[i]);
                }
                sb_putc(&sb, ')');
            } else if (!is_unit_like(vs)) {
                sb_puts(&sb, " { .. }");
            }
        }
    } else if (kind == C_SINGLE && t) {
        if (t->kind == TY_TUPLE) {
            sb_putc(&sb, '(');
            for (i = 0; i < nsub; i++) {
                if (i) sb_puts(&sb, ", ");
                sb_puts(&sb, subs[i]);
            }
            if (nsub == 1) sb_putc(&sb, ',');
            sb_putc(&sb, ')');
        } else if (t->kind == TY_REF) {
            sb_putc(&sb, '&');
            if (t->mut) sb_puts(&sb, "mut ");
            sb_puts(&sb, nsub ? subs[0] : "_");
        } else if (t->kind == TY_ARRAY) {
            sb_putc(&sb, '[');
            for (i = 0; i < nsub; i++) {
                if (i) sb_puts(&sb, ", ");
                sb_puts(&sb, subs[i]);
            }
            sb_putc(&sb, ']');
        } else if (t->kind == TY_ADT) {
            sb_puts(&sb, t->def->name->s);
            if (is_tuple_like(t->def)) {
                sb_putc(&sb, '(');
                for (i = 0; i < nsub; i++) {
                    if (i) sb_puts(&sb, ", ");
                    sb_puts(&sb, subs[i]);
                }
                sb_putc(&sb, ')');
            } else if (!is_unit_like(t->def)) {
                sb_puts(&sb, " { .. }");
            }
        } else {
            sb_putc(&sb, '_');
        }
    } else if (kind == C_SLICE) {
        sb_putc(&sb, '[');
        for (i = 0; i < nsub; i++) {
            if (i) sb_puts(&sb, ", ");
            sb_puts(&sb, subs[i]);
        }
        sb_putc(&sb, ']');
    } else {
        sb_putc(&sb, '_');
    }
    return sb_take(&sb);
}

static int dp_missing(Row *rows0, int nrows0, int n, Type **tys, char **out, int depth) {
    Row *rows = 0;
    int nrows = 0, cap = 0, i, kind = 0, nct, ci, k;
    if (n == 0) return nrows0 == 0;
    for (i = 0; i < nrows0; i++) expand_row(rows0[i], n, &rows, &nrows, &cap);
    nct = sig_count(tys[0], &kind);
    if (nct == -2) {
        long maxl = 0, len;
        for (i = 0; i < nrows; i++) {
            DP *h = rows[i].c[0];
            if (h->kind == DP_CTOR && h->ctor == C_SLICE) {
                long l = h->rest ? h->prefix + h->suffix : h->idx;
                if (l > maxl) maxl = l;
            }
        }
        for (len = 0; len <= maxl + 1; len++) {
            int nspec, a = (int)len;
            Type **sub = 0, **ctys;
            char **subout;
            Row *spec = specialize(rows, nrows, n, C_SLICE, len, a, &nspec);
            ctor_subtypes(tys[0], C_SLICE, len, &sub);
            ctys = tvec(a + n - 1 + 1);
            for (k = 0; k < a; k++) ctys[k] = sub ? sub[k] : 0;
            for (k = 1; k < n; k++) ctys[a + k - 1] = tys[k];
            subout = (char **)ast_alloc(sizeof(char *) * (size_t)(a + n));
            if (dp_missing(spec, nspec, a + n - 1, ctys, subout, depth + 1)) {
                out[0] = ctor_text(tys[0], C_SLICE, len, subout, a);
                for (k = 1; k < n; k++) out[k] = subout[a + k - 1];
                return 1;
            }
        }
        return 0;
    }
    if (nct >= 0) {
        int *has = (int *)ast_alloc(sizeof(int) * (size_t)(nct + 1)), all = 1;
        for (ci = 0; ci < nct; ci++) {
            has[ci] = 0;
            for (i = 0; i < nrows; i++)
                if (dp_matches(rows[i].c[0], kind, ci)) { has[ci] = 1; break; }
            if (!has[ci]) all = 0;
        }
        if (nct == 1 || all) {
            for (ci = 0; ci < nct; ci++) {
                Type **sub = 0, **ctys;
                char **subout;
                int a = ctor_subtypes(tys[0], kind, ci, &sub), nspec;
                Row *spec = specialize(rows, nrows, n, kind, ci, a, &nspec);
                ctys = tvec(a + n - 1 + 1);
                for (k = 0; k < a; k++) ctys[k] = sub ? sub[k] : 0;
                for (k = 1; k < n; k++) ctys[a + k - 1] = tys[k];
                subout = (char **)ast_alloc(sizeof(char *) * (size_t)(a + n));
                if (dp_missing(spec, nspec, a + n - 1, ctys, subout, depth + 1)) {
                    out[0] = ctor_text(tys[0], kind, ci, subout, a);
                    for (k = 1; k < n; k++) out[k] = subout[a + k - 1];
                    return 1;
                }
            }
            return 0;
        } else {
            int nd;
            Row *d = default_rows(rows, nrows, n, &nd);
            if (dp_missing(d, nd, n - 1, tys + 1, out + 1, depth + 1)) {
                StrBuf sb;
                int first = 1;
                sb_init(&sb);
                for (ci = 0; ci < nct; ci++) {
                    char **wild;
                    Type **sub = 0;
                    int a, j;
                    if (has[ci]) continue;
                    if (!first && !(depth == 0 && n == 1)) break;
                    a = ctor_subtypes(tys[0], kind, ci, &sub);
                    wild = (char **)ast_alloc(sizeof(char *) * (size_t)(a + 1));
                    for (j = 0; j < a; j++) wild[j] = "_";
                    if (!first) sb_putc(&sb, 1);
                    sb_puts(&sb, ctor_text(tys[0], kind, ci, wild, a));
                    first = 0;
                }
                out[0] = sb_take(&sb);
                return 1;
            }
            return 0;
        }
    }
    /* infinite signature: only the wildcard rows cover the rest */
    {
        int nd;
        Row *d = default_rows(rows, nrows, n, &nd);
        if (dp_missing(d, nd, n - 1, tys + 1, out + 1, depth + 1)) {
            Type *t = res(tys[0]);
            out[0] = (t && (t->kind == TY_REF)) ? "&_" : "_";
            return 1;
        }
    }
    return 0;
}

/* "`A`", "`A` and `B`", "`A`, `B` and `C`", "`A`, `B`, `C` and 2 more" */
static char *witness_list(char *w) {
    StrBuf sb;
    char *items[64];
    int n = 0, i, shown;
    char *p = w;
    sb_init(&sb);
    items[n++] = p;
    for (; *p; p++)
        if (*p == 1 && n < 64) {
            *p = 0;
            items[n++] = p + 1;
        }
    shown = n > 3 ? 3 : n;
    for (i = 0; i < shown; i++) {
        if (i > 0) sb_puts(&sb, (i == shown - 1 && n <= 3) ? " and " : ", ");
        sb_putc(&sb, '`');
        sb_puts(&sb, items[i]);
        sb_putc(&sb, '`');
    }
    if (n > 3) {
        char b2[32];
        sprintf(b2, " and %d more", n - 3);
        sb_puts(&sb, b2);
    }
    return sb_take(&sb);
}

static void check_match_exhaustive(Node *m, Type *scrut) {
    Row *rows = 0;
    int nrows = 0, cap = 0;
    long i;
    char *out[2];
    Type *tys[2];
    for (i = 0; i < m->list.n; i++) {
        Node *arm = m->list.v[i];
        Row r;
        if (arm->b) continue;           /* a guarded arm never counts */
        r.c = (DP **)ast_alloc(sizeof(DP *) * 2);
        r.c[0] = dp_from(arm->a);
        row_push(&rows, &nrows, &cap, r);
    }
    tys[0] = scrut;
    tys[1] = 0;
    out[0] = out[1] = 0;
    if (dp_missing(rows, nrows, 1, tys, out, 0))
        error_at(m->file, m->line, "non-exhaustive patterns: %s not covered", witness_list(out[0]));
}

static void check_irrefutable(Node *pat, Type *ty, Node *at, const char *what) {
    Row r;
    char *out[2];
    Type *tys[2];
    if (!pat) return;
    r.c = (DP **)ast_alloc(sizeof(DP *) * 2);
    r.c[0] = dp_from(pat);
    tys[0] = ty;
    tys[1] = 0;
    out[0] = out[1] = 0;
    if (dp_missing(&r, 1, 1, tys, out, 0))
        error_at(at->file, at->line, "refutable pattern in %s: %s not covered", what, witness_list(out[0]));
}

/* ================================================================== closure captures */

typedef struct CapInfo {
    Node *clo;
    Capture *v;
    int n, cap;
    struct CapInfo *next;
} CapInfo;
static CapInfo *capt_tab[256];

static CapInfo *cap_info(Node *clo, int create) {
    unsigned h = (unsigned)((size_t)clo >> 4) & 255u;
    CapInfo *c;
    for (c = capt_tab[h]; c; c = c->next)
        if (c->clo == clo) return c;
    if (!create) return 0;
    c = (CapInfo *)ast_alloc(sizeof(CapInfo));
    c->clo = clo;
    c->next = capt_tab[h];
    capt_tab[h] = c;
    return c;
}

int closure_captures(Node *closure, Capture **out) {
    CapInfo *c = cap_info(closure, 0);
    if (!c) {
        *out = 0;
        return 0;
    }
    *out = c->v;
    return c->n;
}

static CapInfo *cur_ci;
static int cur_cdepth, force_move;

static void cap_note(Sym *s, int mode) {
    int i;
    CapInfo *ci = cur_ci;
    if (s->prim >= cur_cdepth) return;
    if (force_move) mode = CAP_MOVE;
    for (i = 0; i < ci->n; i++)
        if (ci->v[i].local == s) {
            if (mode > ci->v[i].mode) ci->v[i].mode = mode;
            return;
        }
    if (ci->n >= ci->cap) {
        int nc = ci->cap ? ci->cap * 2 : 4;
        Capture *nv = (Capture *)ast_alloc(sizeof(Capture) * (size_t)nc);
        if (ci->n) memcpy(nv, ci->v, sizeof(Capture) * (size_t)ci->n);
        ci->v = nv;
        ci->cap = nc;
    }
    ci->v[ci->n].local = s;
    ci->v[ci->n].mode = mode;
    ci->n++;
}

static int value_mode(Node *n) {
    Type *t = n->ty ? (Type *)n->ty : 0;
    if (t && !ty_has_infer(t) && is_copy(t)) return CAP_REF;
    return CAP_MOVE;
}

static int pat_moves(Node *p) {
    long i;
    if (!p) return 0;
    switch (p->kind) {
    case N_P_IDENT: {
        Sym *s = (Sym *)p->sym;
        if (s && s->kind == SYM_LOCAL && !(p->flags & F_REF) && !(p->adj & (ADJ_AUTOREF | ADJ_AUTOREF_MUT)) &&
            s->ty && !is_copy((Type *)s->ty))
            return 1;
        return pat_moves(p->a);
    }
    case N_P_OR: case N_P_TUPLE: case N_P_SLICE: case N_P_TUPLE_STRUCT: case N_P_STRUCT:
        for (i = 0; i < p->list.n; i++)
            if (pat_moves(p->list.v[i])) return 1;
        return 0;
    case N_P_FIELD: case N_P_REF:
        return pat_moves(p->a);
    }
    return 0;
}

static void cap_walk(Node *n, int ctx) {
    long i;
    if (!n) return;
    if (is_item_node(n->kind)) return;
    switch (n->kind) {
    case N_E_PATH: {
        Sym *s = (Sym *)n->sym;
        if (s && s->kind == SYM_LOCAL) cap_note(s, ctx == 0 ? value_mode(n) : ctx);
        return;
    }
    case N_E_REF:
        cap_walk(n->a, (n->flags & F_MUT) ? CAP_MUT : CAP_REF);
        return;
    case N_E_ASSIGN: case N_E_ASSIGN_OP:
        cap_walk(n->a, CAP_MUT);
        cap_walk(n->b, 0);
        return;
    case N_E_FIELD: case N_E_TUPLE_FIELD:
        cap_walk(n->a, ctx == 0 ? value_mode(n) : ctx);
        return;
    case N_E_INDEX:
        cap_walk(n->a, ctx == CAP_MUT ? CAP_MUT : CAP_REF);
        cap_walk(n->b, 0);
        return;
    case N_E_DEREF:
        cap_walk(n->a, ctx == CAP_MUT ? CAP_MUT : CAP_REF);
        return;
    case N_E_METHOD_CALL: {
        int adj = n->a->adj;
        int mode = (adj & ADJ_AUTOREF_MUT) ? CAP_MUT : (adj & ADJ_AUTOREF) ? CAP_REF : ((adj & 0xFF) ? CAP_REF : 0);
        cap_walk(n->a, mode);
        for (i = 0; i < n->list.n; i++) cap_walk(n->list.v[i], 0);
        return;
    }
    case N_E_CALL:
        cap_walk(n->a, CAP_REF);
        for (i = 0; i < n->list.n; i++) cap_walk(n->list.v[i], 0);
        return;
    case N_E_MATCH: {
        int mv = 0;
        for (i = 0; i < n->list.n; i++)
            if (pat_moves(n->list.v[i]->a)) mv = 1;
        cap_walk(n->a, mv ? 0 : CAP_REF);
        for (i = 0; i < n->list.n; i++) {
            cap_walk(n->list.v[i]->b, 0);
            cap_walk(n->list.v[i]->c, 0);
        }
        return;
    }
    case N_E_LET:
        cap_walk(n->b, pat_moves(n->a) ? 0 : CAP_REF);
        return;
    case N_E_CLOSURE: {
        int fm = force_move;
        if (n->flags & F_MOVE) force_move = 1;
        cap_walk(n->b, 0);
        force_move = fm;
        return;
    }
    case N_STMT_LET:
        cap_walk(n->c, 0);
        cap_walk(n->d, 0);
        return;
    }
    cap_walk(n->a, 0);
    cap_walk(n->b, 0);
    cap_walk(n->c, 0);
    cap_walk(n->d, 0);
    for (i = 0; i < n->list.n; i++) cap_walk(n->list.v[i], 0);
}

static void compute_captures(Node *clo, int depth) {
    CapInfo *ci = cap_info(clo, 1);
    int i;
    ci->n = 0;
    cur_ci = ci;
    cur_cdepth = depth;
    force_move = (clo->flags & F_MOVE) != 0;
    cap_walk(clo->b, 0);
    if (clo->flags & F_MOVE)
        for (i = 0; i < ci->n; i++) ci->v[i].mode = CAP_MOVE;
    force_move = 0;
}

/* ================================================================== finishing a body */

static void default_ob_types(void) {
    int i, j;
    for (i = 0; i < nobs; i++) {
        ty_default(obs[i].self);
        for (j = 0; j < obs[i].nargs; j++) ty_default(obs[i].args[j]);
        if (obs[i].aty) ty_default(obs[i].aty);
    }
}

static void default_all(void) {
    int i;
    for (i = 0; i < ntracked; i++)
        if (tracked[i]->ty) ty_default((Type *)tracked[i]->ty);
    for (i = 0; i < ntsyms; i++)
        if (tracked_syms[i]->ty) ty_default(tracked_syms[i]->ty);
    for (i = 0; i < nbody_callees; i++) {
        if (body_callees[i]->fnty) ty_default(body_callees[i]->fnty);
        if (body_callees[i]->self) ty_default(body_callees[i]->self);
    }
    default_ob_types();
}

static long stat_bodies = 0, stat_nodes = 0, stat_obs = 0;

static Node **neg_lits = 0;
static int nneg = 0, cneg = 0;

static void note_neg_lit(Node *lit) {
    if (nneg >= cneg) {
        cneg = cneg ? cneg * 2 : 16;
        neg_lits = (Node **)xrealloc(neg_lits, sizeof(Node *) * (size_t)cneg);
    }
    neg_lits[nneg++] = lit;
}

static int is_neg_lit(Node *lit) {
    int i;
    for (i = 0; i < nneg; i++)
        if (neg_lits[i] == lit) return 1;
    return 0;
}

/* does the literal fit the integer type?  (a negated literal may reach -2^(n-1)) */
static int lit_in_range(Node *n, Type *t) {
    unsigned long long v = (unsigned long long)n->ival;
    int neg = is_neg_lit(n);
    switch (t->ik) {
    case IK_I8: return neg ? v <= 128ULL : v <= 127ULL;
    case IK_I16: return neg ? v <= 32768ULL : v <= 32767ULL;
    case IK_I32: return neg ? v <= 2147483648ULL : v <= 2147483647ULL;
    case IK_I64: case IK_ISIZE: return neg ? v <= 9223372036854775808ULL : v <= 9223372036854775807ULL;
    case IK_U8: return v <= 255ULL;
    case IK_U16: return v <= 65535ULL;
    case IK_U32: return v <= 4294967295ULL;
    }
    return 1;
}

/* a trait method whose Self type is now a known concrete type: point the callee at the impl's fn */
static void devirtualize(Callee *c) {
    Sym *fn = c->fn, *tr, *ifn;
    Type *self, **targs, **nargs, **np, *old, *ret;
    ImplSel sel;
    Subst S;
    int i, j, nt, ni;
    if (!fn || fn->kind != SYM_FN || !fn->parent || fn->parent->kind != SYM_TRAIT || !c->self || !c->fnty) return;
    tr = fn->parent;
    self = res(c->self);
    if (!self || ty_has_infer(self)) return;
    switch (self->kind) {
    case TY_PARAM: case TY_SELF: case TY_PROJ: case TY_DYN: case TY_CLOSURE: case TY_FN_DEF: case TY_FN_PTR:
    case TY_ERR: case TY_NEVER:
        return;
    }
    old = res(c->fnty);
    if (old->kind != TY_FN_DEF || old->nargs < tr->ngparams) return;
    targs = old->args;
    if (select_impl(tr, self, targs, tr->ngparams, &sel, 0) != 1) return;
    ifn = assoc_item(sel.impl, NS_VALUE, fn->name);
    if (!ifn || ifn->kind != SYM_FN || !ifn->ty || ifn->ty->kind != TY_FN_DEF) return;
    if (ifn->parent != sel.impl) return;   /* a provided (default) method: the trait's fn stays the callee */
    ni = ifn->ngparams;
    nt = tr->ngparams;
    nargs = tvec(ni);
    for (i = 0; i < ni; i++) {
        nargs[i] = 0;
        for (j = 0; j < sel.sub.n; j++)
            if (sel.sub.params[j] == ifn->gparams[i]) nargs[i] = zonk(sel.sub.args[j]);
        if (!nargs[i]) {
            int own = i - (sel.impl->ngparams);
            int ti = nt + own;
            nargs[i] = (own >= 0 && ti < old->nargs) ? old->args[ti] : ty_infer(0);
        }
    }
    subst_init(&S, ifn->gparams, nargs, ni, 0);
    np = tvec(ifn->ty->nparams);
    for (i = 0; i < ifn->ty->nparams; i++) np[i] = zonk(isub(ifn->ty->params[i], &S));
    ret = zonk(isub(ifn->ty->ret, &S));
    c->fnty = ty_fn_def(ifn, nargs, ni, np, ifn->ty->nparams, ret);
    if (c->slot == 0 && c->node->sym == fn) c->node->sym = ifn;
    c->fn = ifn;
}

static void finish_body(void) {
    int i;
    stat_bodies++;
    stat_nodes += ntracked;
    ob_node = 0;
    solve_obs();
    default_all();
    solve_obs();
    default_all();
    solve_obs();
    for (i = 0; i < ntracked; i++) {
        Node *n = tracked[i];
        if (n->ty && ty_has_infer((Type *)n->ty))
            error_at(n->file, n->line, "type annotations needed");
    }
    for (i = 0; i < ntracked; i++) {
        Node *n = tracked[i];
        if (n->kind == N_E_LIT_INT && n->ty && !in_core) {
            Type *t = res((Type *)n->ty);
            if (t->kind == TY_INT && !lit_in_range(n, t))
                error_at(n->file, n->line, "literal out of range for `%s`", tn(t));
        }
    }
    for (i = 0; i < ntracked; i++)
        if (tracked[i]->ty) tracked[i]->ty = zonk((Type *)tracked[i]->ty);
    for (i = 0; i < ntsyms; i++)
        if (tracked_syms[i]->ty) tracked_syms[i]->ty = zonk(tracked_syms[i]->ty);
    for (i = 0; i < nbody_callees; i++) {
        if (body_callees[i]->fnty) body_callees[i]->fnty = zonk(body_callees[i]->fnty);
        if (body_callees[i]->self) body_callees[i]->self = zonk(body_callees[i]->self);
    }
    for (i = 0; i < nbody_callees; i++) devirtualize(body_callees[i]);
    for (i = 0; i < nclosures; i++) compute_captures(closures[i], closure_depths ? closure_depths[i] : 1);
}

static void begin_body(Sym *item, Node *n) {
    nobs = 0;
    ntracked = 0;
    ntsyms = 0;
    nbody_callees = 0;
    nloops = 0;
    nfcx = 0;
    loop_base = 0;
    closure_depth = 0;
    cur_div = 0;
    nclosures = 0;
    nneg = 0;
    nct = 0;
    in_core = core_file >= 0 && n->file >= core_file;   /* core spans files 1.. (the user crate is file 0) */
    set_env(item);
}

/* ================================================================== items */

static Node **pending = 0;
static int npending = 0, cpending = 0;

static void queue_item(Node *n) {
    if (npending >= cpending) {
        cpending = cpending ? cpending * 2 : 16;
        pending = (Node **)xrealloc(pending, sizeof(Node *) * (size_t)cpending);
    }
    pending[npending++] = n;
}

static void check_fn_item(Node *n) {
    Sym *s = (Sym *)n->sym;
    Type *sig, *ret;
    long i;
    int pi = 0;
    if (!n->c || !s) return;
    if (!s->ty || s->ty->kind != TY_FN_DEF)
        error_at(n->file, n->line, "internal error: function `%s` has no signature", n->name ? n->name->s : "?");
    begin_body(s, n);
    sig = s->ty;
    ret = norm(sig->ret);
    push_fcx(ret, 0);
    for (i = 0; i < n->list.n; i++) {
        Node *p = n->list.v[i];
        Type *pt;
        if (pi >= sig->nparams) break;
        pt = norm(sig->params[pi++]);
        if (p->kind == N_SELF_PARAM) {
            Sym *l = (Sym *)p->sym;
            if (l) {
                set_local_ty(l, pt);
                l->prim = 0;
            }
        } else if (p->a) {
            ck_pat(p->a, pt, 0);
            check_irrefutable(p->a, pt, p, "function argument");
        }
    }
    ck_block(n->c, ret, 1);
    pop_fcx();
    finish_body();
}

static void check_const_item(Node *n) {
    Sym *s = (Sym *)n->sym;
    Type *t;
    if (!n->b || !s) return;
    begin_body(s, n);
    t = s->ty ? norm(s->ty) : lower_ty(n->a);
    push_fcx(t, 0);
    ck_coerce(n->b, t);
    pop_fcx();
    finish_body();
}

static void check_item(Node *n) {
    long i;
    switch (n->kind) {
    case N_ITEM_FN:
        check_fn_item(n);
        break;
    case N_ITEM_CONST: case N_ITEM_STATIC:
        check_const_item(n);
        break;
    case N_ITEM_MOD:
        for (i = 0; i < n->list.n; i++) check_item(n->list.v[i]);
        break;
    case N_ITEM_IMPL: case N_ITEM_TRAIT:
        for (i = 0; i < n->list.n; i++) {
            Node *m = n->list.v[i];
            if (m->kind == N_ITEM_FN || m->kind == N_ITEM_CONST) check_item(m);
        }
        break;
    }
}

static void drain_pending(void) {
    while (npending > 0) {
        Node *n = pending[--npending];
        check_item(n);
    }
}

static void check_crate_items(Node *crate) {
    long i;
    if (!crate) return;
    for (i = 0; i < crate->list.n; i++) {
        check_item(crate->list.v[i]);
        drain_pending();
    }
}

void check_crate(void) {
    int saved = ty_infer_missing_args;
    n_lenvar = name_intern_str("{len}");
    nm_output = name_intern_str("Output");
    nm_target = name_intern_str("Target");
    nm_item_name = name_intern_str("Item");
    core_file = (core_root && core_root->node) ? core_root->node->file : -1;
    ty_infer_missing_args = 1;
    init_lang();
    index_items();
    collect_closure_impl = ck_closure_impl;
    if (core_root && core_root->node) check_crate_items(core_root->node);
    if (crate_root && crate_root->node) check_crate_items(crate_root->node);
    ty_infer_missing_args = saved;
    if (getenv("RUSTC_CHECK_STATS"))
        fprintf(stderr, "check: %ld bodies, %ld typed nodes\n", stat_bodies, stat_nodes);
}
