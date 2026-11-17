/*
 * collect.c - item collection: signatures, generics, bounds, impls, and the
 * trait queries (find_impl, implements, is_copy, assoc_item, normalize).
 *
 * collect_crate() runs after resolve_crate() and fills the fields of Sym that
 * sema.h marks "filled by collect.c":
 *
 *   gparams / ngparams  the SYM_GENERIC / SYM_CONST_GENERIC symbols in order.
 *                       An associated fn / const / type starts with the
 *                       generics of its impl or trait, then its own.  The
 *                       index of a generic is renumbered to its position in
 *                       gparams, so ty_subst() works with Subst.params == 0
 *                       and with Subst.params == sym->gparams alike.
 *   bounds / nbounds    inline and where-clause bounds, `?Sized` dropped (the
 *                       parameter gets an internal flag), supertraits as
 *                       bounds on TY_SELF, `type X: B;` in a trait as a bound
 *                       on TY_PROJ(Self, Trait, X), `Trait<Item = T>` as one
 *                       Bound per binding (assoc_name / assoc_ty).  An
 *                       associated item's bounds start with those of its
 *                       impl or trait, like gparams.
 *   ty                  fn: TY_FN_DEF (args = its own params as TY_PARAM,
 *                       params include self, ret); struct / enum / union:
 *                       TY_ADT with its params as args; variant: the enum's
 *                       type; field, const, static: the declared type; type
 *                       alias and associated type of an impl: the aliased
 *                       type (0 for a trait's associated type); generic
 *                       parameter: TY_PARAM; const generic: its type; impl:
 *                       its self type.
 *   trait, trait_item, impl_self, trait_args, ntrait_args, lang, SF_COPY,
 *   SF_METHOD           as in sema.h (lang and SF_METHOD are set by
 *                       resolve.c already).
 *   next_impl           impls are chained per type constructor (inherent and
 *                       trait impls in source order); see collect_impls_for().
 *
 * Internals use the Sym.index field of structs, enums, unions, traits and
 * impls as an ordinal (nothing else uses it for those kinds).
 *
 * Everything static here is prefixed cl_ because the translator is one
 * translation unit.
 */
#include "sema.h"

/* the parts of types.c that sema.h does not declare */
Type *ty_fn_def(Sym *def, Type **targs, int nt, Type **params, int np, Type *ret);
Type *ty_dyn(Sym *trait, Type **args, int n);
Type *ty_proj(Type *self, Sym *trait, Name *name, Type **targs, int nt);
Type *ty_self(void);
Type *ty_const(long v);
Type *ty_from_path_prefix(Node *path, int nseg);

/* ================================================================ public extras */

/* find_impl() returns this symbol (kind SYM_IMPL, no members) when the trait
 * holds because of a built-in rule (Copy / Clone / Fn* of primitives, fn items,
 * closures, refs, tuples, arrays; dyn Trait; a where-clause of the generic
 * environment).  There are no associated items to look up in it. */
Sym *collect_builtin_impl = 0;

/* set by check.c: does the closure type implement the Fn / FnMut / FnOnce /
 * Clone / Copy trait?  (0 = hook not set: closures implement all of them) */
int (*collect_closure_impl)(Type *closure, Sym *trait) = 0;

/* find_impl() sets this to 1 when it returned 0 because several impls match a
 * query that still contains inference variables, and to 0 otherwise. */
int find_impl_ambiguous = 0;

void collect_set_env(Sym *owner);
Sym *collect_env(void);
int implements_with(Type *t, Sym *trait, Type **args, int nargs);
Sym *collect_impls_for(Type *self);
Sym *collect_blanket_impls(void);
Sym **collect_trait_impls(Sym *trait, int *n);
Sym **collect_all_traits(int *n);
void collect_dump(FILE *f, int include_core);

/* ================================================================ state */

#define CL_REG      ((long)1 << 20)   /* registered in cl_syms */
#define CL_GEN      ((long)1 << 21)   /* gparams computed */
#define CL_UNSIZED  ((long)1 << 22)   /* type parameter declared ?Sized */
#define CL_BDONE    ((long)1 << 23)   /* bounds lowering started */
#define CL_MAXDEPTH 12

typedef struct {
    Sym **v;
    int n, cap;
} ClSymList;

static Sym **cl_syms = 0;
static int cl_nsyms = 0, cl_csyms = 0;
static Sym **cl_traits = 0;
static int cl_ntraits = 0, cl_ctraits = 0;
static int cl_nadts = 0;
static ClSymList *cl_tinfo = 0;          /* per trait ordinal: its impls */
static int *cl_ikey = 0;                 /* per impl ordinal: constructor key (-1 = any) */
static Sym **cl_head = 0, **cl_tail = 0; /* per constructor bucket */
static int cl_nbuckets = 0;
static int cl_ready = 0;
static Sym *cl_env = 0;

static Sym *cl_lang_copy, *cl_lang_clone, *cl_lang_sized, *cl_lang_send, *cl_lang_sync;
static Sym *cl_lang_fn, *cl_lang_fn_mut, *cl_lang_fn_once, *cl_lang_box;
static Name *cl_n_output = 0;

static const char *cl_nm(Sym *s) {
    return s && s->name ? s->name->s : "_";
}

static Type **cl_tvec(int n) {
    return n > 0 ? (Type **)ast_alloc(sizeof(Type *) * (size_t)n) : 0;
}

static void cl_sym_push(Sym ***v, int *n, int *cap, Sym *s) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 64;
        *v = (Sym **)xrealloc(*v, sizeof(Sym *) * (size_t)*cap);
    }
    (*v)[(*n)++] = s;
}

/* ================================================================ unification */

/* A copy of types.c's unifier with an undo log that this file controls, and
 * with matching of the length of `[T; N]` against a const parameter N of the
 * impl being matched (cl_cp_*).  Bindings are made on TY_INFER variables
 * only, so undoing resets their link. */

static Type **cl_trail = 0;
static int cl_ntrail = 0, cl_captrail = 0;

static Sym **cl_cp_params = 0;
static Type **cl_cp_args = 0;
static int cl_cp_n = 0;

static int cl_mark(void) {
    return cl_ntrail;
}

static void cl_undo(int mark) {
    while (cl_ntrail > mark) cl_trail[--cl_ntrail]->link = 0;
}

static void cl_bind(Type *v, Type *to) {
    if (cl_ntrail >= cl_captrail) {
        cl_captrail = cl_captrail ? cl_captrail * 2 : 128;
        cl_trail = (Type **)xrealloc(cl_trail, sizeof(Type *) * (size_t)cl_captrail);
    }
    cl_trail[cl_ntrail++] = v;
    v->link = to;
}

static int cl_occurs(Type *v, Type *t) {
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t == v) return 1;
    if (t->elem && cl_occurs(v, t->elem)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (cl_occurs(v, t->args[i])) return 1;
    for (i = 0; i < t->nparams; i++)
        if (cl_occurs(v, t->params[i])) return 1;
    if (t->ret && cl_occurs(v, t->ret)) return 1;
    return 0;
}

static int cl_bind_var(Type *a, Type *b) {
    if (b->kind == TY_INFER) {
        if (a->ik == 0) { cl_bind(a, b); return 1; }
        if (b->ik == 0) { cl_bind(b, a); return 1; }
        if (a->ik == b->ik) { cl_bind(a, b); return 1; }
        return 0;
    }
    if (a->ik == INFER_INT) {
        if (b->kind != TY_INT) return 0;
    } else if (a->ik == INFER_FLOAT) {
        if (b->kind != TY_FLOAT) return 0;
    } else if (cl_occurs(a, b)) {
        return 0;
    }
    cl_bind(a, b);
    return 1;
}

static int cl_scalar_eq(Type *a, Type *b) {
    return a->kind == b->kind && a->ik == b->ik && a->mut == b->mut && a->len == b->len &&
           a->len_param == b->len_param && a->def == b->def && a->name == b->name &&
           a->node == b->node && a->nargs == b->nargs && a->nparams == b->nparams &&
           (a->elem != 0) == (b->elem != 0) && (a->ret != 0) == (b->ret != 0);
}

/* the variable standing for the const parameter p of the impl being matched, or 0 */
static Type *cl_cp_var(Sym *p) {
    int i;
    if (!p) return 0;
    for (i = 0; i < cl_cp_n; i++)
        if (cl_cp_params[i] == p && cl_cp_args[i]) return cl_cp_args[i];
    return 0;
}

static int cl_unify_rec(Type *a, Type *b);

static int cl_unify_len(Type *a, Type *b) {
    Type *va = cl_cp_var(a->len_param), *vb = cl_cp_var(b->len_param), *rep;
    if (va) {
        rep = b->len_param ? ty_param(b->len_param) : ty_const(b->len);
        if (vb && b->len_param) rep = vb;
        return cl_unify_rec(va, rep);
    }
    if (vb) {
        rep = a->len_param ? ty_param(a->len_param) : ty_const(a->len);
        return cl_unify_rec(vb, rep);
    }
    return a->len_param == b->len_param && a->len == b->len;
}

static int cl_unify_rec(Type *a, Type *b) {
    int i;
    a = ty_resolve(a);
    b = ty_resolve(b);
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->kind == TY_ERR || b->kind == TY_ERR) return 1;
    if (a->kind == TY_INFER) return cl_bind_var(a, b);
    if (b->kind == TY_INFER) return cl_bind_var(b, a);
    if (a->kind != b->kind) return 0;
    if (a->kind == TY_ARRAY && a->elem && b->elem) {
        if (!cl_unify_rec(a->elem, b->elem)) return 0;
        return cl_unify_len(a, b);
    }
    if (!cl_scalar_eq(a, b)) return 0;
    if (a->elem && !cl_unify_rec(a->elem, b->elem)) return 0;
    for (i = 0; i < a->nargs; i++)
        if (!cl_unify_rec(a->args[i], b->args[i])) return 0;
    for (i = 0; i < a->nparams; i++)
        if (!cl_unify_rec(a->params[i], b->params[i])) return 0;
    if (a->ret && !cl_unify_rec(a->ret, b->ret)) return 0;
    return 1;
}

/* atomic: on failure nothing stays bound */
static int cl_unify(Type *a, Type *b) {
    int mark = cl_mark(), r;
    r = cl_unify_rec(a, b);
    if (!r) cl_undo(mark);
    return r;
}

/* ================================================================ type mapping */

typedef Type *(*ClMapFn)(Type *t, void *ud);

/* rebuilds t bottom-up, applying f to every node (children first); dyn
 * bindings (params of TY_DYN) are left alone */
static Type *cl_map(Type *t, ClMapFn f, void *ud) {
    Type *elem, *ret, *r, *c;
    Type **args = 0, **params = 0;
    int i, changed = 0;
    t = ty_resolve(t);
    if (!t) return t;
    elem = t->elem ? cl_map(t->elem, f, ud) : 0;
    if (elem != t->elem) changed = 1;
    ret = t->ret ? cl_map(t->ret, f, ud) : 0;
    if (ret != t->ret) changed = 1;
    for (i = 0; i < t->nargs; i++) {
        c = cl_map(t->args[i], f, ud);
        if (c != t->args[i] && !args) {
            args = cl_tvec(t->nargs);
            memcpy(args, t->args, sizeof(Type *) * (size_t)i);
        }
        if (args) args[i] = c;
    }
    if (args) changed = 1;
    if (t->kind != TY_DYN) {
        for (i = 0; i < t->nparams; i++) {
            c = cl_map(t->params[i], f, ud);
            if (c != t->params[i] && !params) {
                params = cl_tvec(t->nparams);
                memcpy(params, t->params, sizeof(Type *) * (size_t)i);
            }
            if (params) params[i] = c;
        }
        if (params) changed = 1;
    }
    if (changed) {
        r = (Type *)ast_alloc(sizeof(Type));
        *r = *t;
        r->elem = elem;
        r->ret = ret;
        if (args) r->args = args;
        if (params) r->params = params;
        t = r;
    }
    return f(t, ud);
}

static int cl_has_proj(Type *t) {
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t->kind == TY_PROJ) return 1;
    if (t->elem && cl_has_proj(t->elem)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (cl_has_proj(t->args[i])) return 1;
    if (t->kind != TY_DYN)
        for (i = 0; i < t->nparams; i++)
            if (cl_has_proj(t->params[i])) return 1;
    if (t->ret && cl_has_proj(t->ret)) return 1;
    return 0;
}

static int cl_occurs_param(Type *t, Sym *g) {
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t->kind == TY_PARAM && t->def == g) return 1;
    if (t->kind == TY_ARRAY && t->len_param == g) return 1;
    if (t->elem && cl_occurs_param(t->elem, g)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (cl_occurs_param(t->args[i], g)) return 1;
    for (i = 0; i < t->nparams; i++)
        if (cl_occurs_param(t->params[i], g)) return 1;
    if (t->ret && cl_occurs_param(t->ret, g)) return 1;
    return 0;
}

/* identity arguments of a generic owner: its params as TY_PARAM */
static Type **cl_identity(Sym *s) {
    Type **v = cl_tvec(s->ngparams);
    int i;
    for (i = 0; i < s->ngparams; i++) v[i] = ty_param(s->gparams[i]);
    return v;
}

static void cl_lower_bounds(Sym *s);
static int cl_super_args(Type *bty, Sym *btrait, Type **bargs, int nb, Sym *want, Type ***oargs,
                         int *on, int depth);

/* `<S as Trait>::X` written without the trait's generic arguments (T::X, Self::X):
 * supply them from the impl / trait we are in, or from a bound on S */
static Type *cl_fill_fn(Type *t, void *ud) {
    Sym *ctx = (Sym *)ud, *o;
    Type **na = 0, *r;
    int n, i, nn = 0;
    if (t->kind != TY_PROJ || t->nargs != 1 || !t->def || t->def->ngparams == 0) return t;
    n = t->def->ngparams;
    o = ctx;
    if (o && o->kind != SYM_IMPL && o->kind != SYM_TRAIT) o = o->parent;
    if (o && o->kind == SYM_TRAIT && t->args[0]->kind == TY_SELF) {
        if (o == t->def) {
            na = cl_identity(o);
            nn = n;
        } else {
            Type **ids = cl_identity(o);
            if (cl_super_args(ty_self(), o, ids, o->ngparams, t->def, &na, &nn, 0) && nn != n) na = 0;
        }
    } else if (o && o->kind == SYM_IMPL && o->trait == t->def && o->impl_self &&
               o->ntrait_args == n && ty_equal(t->args[0], o->impl_self)) {
        na = o->trait_args;
        nn = n;
    }
    if (!na && ctx) {
        for (i = 0; i < ctx->nbounds && !na; i++) {
            Bound *b = &ctx->bounds[i];
            if (!ty_equal(b->ty, t->args[0])) continue;
            if (b->trait == t->def && b->nargs == n) {
                na = b->args;
                nn = n;
            } else if (cl_super_args(b->ty, b->trait, b->args, b->nargs, t->def, &na, &nn, 0) &&
                       nn != n) {
                na = 0;
            }
        }
    }
    if (!na) return t;
    r = ty_proj(t->args[0], t->def, t->name, na, n);
    r->ret = t->ret;
    return r;
}

static Type *cl_fill_projs(Type *t, Sym *ctx) {
    if (!t || !cl_has_proj(t)) return t;
    return cl_map(t, cl_fill_fn, ctx);
}

static Type *cl_lower(Node *tn, Sym *ctx) {
    return cl_fill_projs(ty_from_ast(tn), ctx);
}

/* ================================================================ registration */

static int cl_item_kind(int k) {
    switch (k) {
    case N_ITEM_FN: case N_ITEM_STRUCT: case N_ITEM_ENUM: case N_ITEM_UNION: case N_ITEM_TRAIT:
    case N_ITEM_IMPL: case N_ITEM_MOD: case N_ITEM_CONST: case N_ITEM_STATIC: case N_ITEM_TYPE:
        return 1;
    }
    return 0;
}

static void cl_register(Node *n) {
    Sym *s = (Sym *)n->sym;
    if (!s || (s->flags & CL_REG) || s->node != n) return;
    switch (s->kind) {
    case SYM_MOD: case SYM_STRUCT: case SYM_UNION: case SYM_ENUM: case SYM_TRAIT: case SYM_FN:
    case SYM_CONST: case SYM_STATIC: case SYM_TYPEALIAS: case SYM_ASSOC_TYPE: case SYM_IMPL:
        break;
    default:
        return;
    }
    s->flags |= CL_REG;
    cl_sym_push(&cl_syms, &cl_nsyms, &cl_csyms, s);
    if (s->kind == SYM_STRUCT || s->kind == SYM_ENUM || s->kind == SYM_UNION) s->index = cl_nadts++;
    if (s->kind == SYM_TRAIT) {
        s->index = cl_ntraits;
        cl_sym_push(&cl_traits, &cl_ntraits, &cl_ctraits, s);
    }
}

static void cl_scan(Node *n) {
    long i;
    if (!n) return;
    if (cl_item_kind(n->kind)) cl_register(n);
    cl_scan(n->a);
    cl_scan(n->b);
    cl_scan(n->c);
    cl_scan(n->d);
    for (i = 0; i < n->list.n; i++) cl_scan(n->list.v[i]);
}

/* ================================================================ generics */

static Node *cl_generics_node(Sym *s) {
    switch (s->kind) {
    case SYM_FN: case SYM_STRUCT: case SYM_UNION: case SYM_ENUM: case SYM_TRAIT: case SYM_IMPL:
    case SYM_TYPEALIAS: case SYM_ASSOC_TYPE:
        if (s->node && s->node->a && s->node->a->kind == N_GENERICS) return s->node->a;
        break;
    }
    return 0;
}

static int cl_is_assoc(Sym *s) {
    return s->parent && (s->parent->kind == SYM_IMPL || s->parent->kind == SYM_TRAIT) &&
           (s->kind == SYM_FN || s->kind == SYM_CONST || s->kind == SYM_ASSOC_TYPE);
}

static void cl_set_gparams(Sym *s) {
    Node *g = cl_generics_node(s);
    Sym *par = 0;
    long i;
    int np = 0, no = 0, k;
    if (s->flags & CL_GEN) return;
    s->flags |= CL_GEN;
    if (cl_is_assoc(s)) {
        par = s->parent;
        cl_set_gparams(par);
        np = par->ngparams;
    }
    if (g) {
        for (i = 0; i < g->list.n; i++) {
            Node *p = g->list.v[i];
            if ((p->kind == N_GEN_TYPE || p->kind == N_GEN_CONST) && p->sym) no++;
        }
    }
    if (np + no == 0) return;
    s->gparams = (Sym **)ast_alloc(sizeof(Sym *) * (size_t)(np + no));
    for (k = 0; k < np; k++) s->gparams[k] = par->gparams[k];
    k = np;
    if (g) {
        for (i = 0; i < g->list.n; i++) {
            Node *p = g->list.v[i];
            if ((p->kind == N_GEN_TYPE || p->kind == N_GEN_CONST) && p->sym) {
                Sym *gs = (Sym *)p->sym;
                gs->index = k;
                s->gparams[k++] = gs;
            }
        }
    }
    s->ngparams = np + no;
}

/* ================================================================ impl Trait in argument position */

/* `fn f(x: impl Add<i32>)` is `fn f<impl Add<i32>>(x: impl Add<i32>)`: the
 * T_IMPL node becomes a path to a new anonymous type parameter, appended to
 * the fn's generics together with its bounds */
static void cl_impl_arg(Node *t, Node *fnn, Sym *fn) {
    long i;
    Node *g, *gp, *path, *seg;
    Sym *gs;
    Name *nm;
    if (!t) return;
    switch (t->kind) {
    case N_T_REF: case N_T_PTR: case N_T_SLICE: case N_T_ARRAY:
        cl_impl_arg(t->a, fnn, fn);
        return;
    case N_T_TUPLE:
        for (i = 0; i < t->list.n; i++) cl_impl_arg(t->list.v[i], fnn, fn);
        return;
    case N_T_PATH:
        if (t->a && t->a->kind == N_PATH) {
            for (i = 0; i < t->a->list.n; i++) {
                Node *sg = t->a->list.v[i];
                long k;
                for (k = 0; k < sg->list.n; k++)
                    if (sg->list.v[k]->kind >= N_T_PATH && sg->list.v[k]->kind <= N_T_INFER)
                        cl_impl_arg(sg->list.v[k], fnn, fn);
            }
        }
        return;
    case N_T_IMPL:
        break;
    default:
        return;
    }
    if (!fnn->a || fnn->a->kind != N_GENERICS) {
        g = mk(N_GENERICS, fnn->line, fnn->col);
        g->file = fnn->file;
        fnn->a = g;
    }
    g = fnn->a;
    nm = name_intern_str("impl Trait");
    gp = mk(N_GEN_TYPE, t->line, t->col);
    gp->file = t->file;
    gp->name = nm;
    gp->b = t->a;
    gs = sym_new(SYM_GENERIC, nm, gp, fn);
    gp->sym = gs;
    list_push(&g->list, gp);
    path = mk(N_PATH, t->line, t->col);
    path->file = t->file;
    seg = mk(N_PATH_SEG, t->line, t->col);
    seg->file = t->file;
    seg->name = nm;
    seg->sym = gs;
    list_push(&path->list, seg);
    path->sym = gs;
    t->kind = N_T_PATH;
    t->a = path;
    t->b = 0;
    t->sym = gs;
}

static void cl_desugar_impl_args(Sym *fn) {
    Node *n = fn->node;
    long i;
    for (i = 0; i < n->list.n; i++) {
        Node *p = n->list.v[i];
        if (p->kind == N_PARAM && p->b) cl_impl_arg(p->b, n, fn);
    }
}

/* ================================================================ associated type paths */

/* `T::Item`, `Self::Item`: the resolver leaves the last segment unresolved
 * (and gives the T_PATH the symbol of T).  Find the associated type from the
 * traits that bound T (or from the impl / trait for Self) and put it on the
 * path, so that ty_from_ast() builds the projection. */

static Sym *cl_trait_assoc(Sym *tr, Name *name, int depth) {
    Sym *a;
    Node *c;
    long i;
    if (!tr || tr->kind != SYM_TRAIT || depth > 16) return 0;
    a = scope_get(tr->members, NS_TYPE, name);
    if (a && a->kind == SYM_ASSOC_TYPE) return a;
    c = tr->node ? tr->node->c : 0;
    if (c) {
        for (i = 0; i < c->list.n; i++) {
            Node *b = c->list.v[i];
            if (b->kind != N_BOUND_TRAIT || (b->flags & F_MAYBE) || !b->sym) continue;
            a = cl_trait_assoc((Sym *)b->sym, name, depth + 1);
            if (a) return a;
        }
    }
    return 0;
}

static Sym *cl_bounds_assoc(Node *bs, Name *name) {
    long i;
    Sym *a;
    if (!bs) return 0;
    for (i = 0; i < bs->list.n; i++) {
        Node *b = bs->list.v[i];
        if (b->kind != N_BOUND_TRAIT || (b->flags & F_MAYBE) || !b->sym) continue;
        a = cl_trait_assoc((Sym *)b->sym, name, 0);
        if (a) return a;
    }
    return 0;
}

static Sym *cl_where_assoc(Sym *holder, Sym *param, Name *name) {
    Node *w;
    long i;
    Sym *a;
    if (!holder || !holder->node) return 0;
    switch (holder->kind) {
    case SYM_FN: case SYM_STRUCT: case SYM_UNION: case SYM_ENUM: case SYM_TRAIT: case SYM_IMPL:
    case SYM_TYPEALIAS:
        break;
    default:
        return 0;
    }
    w = holder->node->d;
    if (!w || w->kind != N_WHERE) return 0;
    for (i = 0; i < w->list.n; i++) {
        Node *c = w->list.v[i];
        if (c->kind != N_WHERE_TYPE || !c->a || c->a->kind != N_T_PATH) continue;
        if ((Sym *)c->a->sym != param) continue;
        a = cl_bounds_assoc(c->b, name);
        if (a) return a;
    }
    return 0;
}

static Sym *cl_param_assoc(Sym *p, Name *name, Sym *ctx) {
    Sym *owner = p->parent, *a;
    Node *g;
    long i;
    if (owner) {
        g = cl_generics_node(owner);
        if (g) {
            for (i = 0; i < g->list.n; i++) {
                Node *gp = g->list.v[i];
                if (gp->kind == N_GEN_TYPE && (Sym *)gp->sym == p) {
                    a = cl_bounds_assoc(gp->b, name);
                    if (a) return a;
                }
            }
        }
        a = cl_where_assoc(owner, p, name);
        if (a) return a;
    }
    if (ctx) {
        a = cl_where_assoc(ctx, p, name);
        if (a) return a;
        if (ctx->parent && (ctx->parent->kind == SYM_IMPL || ctx->parent->kind == SYM_TRAIT)) {
            a = cl_where_assoc(ctx->parent, p, name);
            if (a) return a;
        }
    }
    return 0;
}

static Sym *cl_self_assoc(Sym *owner, Name *name) {
    Sym *a;
    if (!owner) return 0;
    if (owner->kind == SYM_IMPL) {
        a = scope_get(owner->members, NS_TYPE, name);
        if (a && a->kind == SYM_ASSOC_TYPE) return a;
        if (owner->trait) return cl_trait_assoc(owner->trait, name, 0);
        return 0;
    }
    if (owner->kind == SYM_TRAIT) return cl_trait_assoc(owner, name, 0);
    return 0;
}

static void cl_fix_tpath(Node *t, Sym *ctx) {
    Node *path = t->a, *last;
    Sym *prefix, *a = 0;
    int n;
    if (!path || path->kind != N_PATH || (path->flags & F_QUALIFIED)) return;
    n = (int)path->list.n;
    if (n < 2) return;
    last = path->list.v[n - 1];
    if (last->sym) return;
    prefix = (Sym *)path->list.v[n - 2]->sym;
    if (!prefix) error_at(t->file, t->line, "ambiguous associated type `%s`", cl_nm((Sym *)0));
    switch (prefix->kind) {
    case SYM_SELF_TY:
        a = cl_self_assoc(prefix->parent, last->name);
        if (!a)
            error_at(t->file, t->line, "associated type `%s` not found for `Self`", last->name->s);
        break;
    case SYM_GENERIC:
        a = cl_param_assoc(prefix, last->name, ctx);
        if (!a)
            error_at(t->file, t->line, "associated type `%s` not found for `%s`", last->name->s,
                     cl_nm(prefix));
        break;
    default:
        error_at(t->file, t->line, "ambiguous associated type");
    }
    last->sym = a;
    t->sym = a;
    path->sym = a;
}

static void cl_fix_walk(Node *n, Sym *ctx) {
    long i;
    if (!n) return;
    if (cl_item_kind(n->kind) && n->sym) ctx = n->kind == N_ITEM_MOD ? 0 : (Sym *)n->sym;
    if (n->kind == N_T_PATH) cl_fix_tpath(n, ctx);
    cl_fix_walk(n->a, ctx);
    cl_fix_walk(n->b, ctx);
    cl_fix_walk(n->c, ctx);
    cl_fix_walk(n->d, ctx);
    for (i = 0; i < n->list.n; i++) cl_fix_walk(n->list.v[i], ctx);
}

/* ================================================================ bounds */

static void cl_push_bound(Sym *s, Type *ty, Sym *trait, Type **args, int nargs, Name *an, Type *at) {
    Bound *nb = (Bound *)ast_alloc(sizeof(Bound) * (size_t)(s->nbounds + 1));
    if (s->nbounds) memcpy(nb, s->bounds, sizeof(Bound) * (size_t)s->nbounds);
    nb[s->nbounds].ty = ty;
    nb[s->nbounds].trait = trait;
    nb[s->nbounds].args = args;
    nb[s->nbounds].nargs = nargs;
    nb[s->nbounds].assoc_name = an;
    nb[s->nbounds].assoc_ty = at;
    s->bounds = nb;
    s->nbounds++;
}

/* how many generic arguments the last segment of a trait path writes */
static int cl_given_args(Node *path) {
    Node *seg;
    long k;
    int n = 0;
    if (!path || path->list.n == 0) return 0;
    seg = path->list.v[path->list.n - 1];
    if (seg->flags & F_PARENS) return 1;
    for (k = 0; k < seg->list.n; k++)
        if (seg->list.v[k]->kind != N_LIFETIME && seg->list.v[k]->kind != N_ASSOC_BIND) n++;
    return n;
}

static void cl_add_bounds(Sym *s, Type *bty, Node *bounds) {
    long i;
    if (!bounds) return;
    for (i = 0; i < bounds->list.n; i++) {
        Node *b = bounds->list.v[i];
        Type *d, **args;
        Subst sub;
        int k, given;
        if (b->kind != N_BOUND_TRAIT) continue;
        if (b->flags & F_MAYBE) {
            if (bty->kind == TY_PARAM && bty->def) bty->def->flags |= CL_UNSIZED;
            continue;
        }
        if (!b->sym || !b->a) continue;
        d = ty_from_path_prefix(b->a, (int)b->a->list.n);
        if (!d || d->kind != TY_DYN)
            error_at(b->file, b->line, "expected a trait, found type");
        sub.params = 0;
        sub.args = 0;
        sub.n = 0;
        sub.self = bty;
        given = cl_given_args(b->a);
        args = cl_tvec(d->nargs);
        for (k = 0; k < d->nargs; k++)
            args[k] = cl_fill_projs(k < given ? d->args[k] : ty_subst(d->args[k], &sub), s);
        if (d->nparams == 0) {
            cl_push_bound(s, bty, d->def, args, d->nargs, 0, 0);
        } else {
            for (k = 0; k < d->nparams; k++) {
                Type *p = d->params[k];
                cl_push_bound(s, bty, d->def, args, d->nargs, p->name, cl_fill_projs(p->ret, s));
            }
        }
    }
}

static void cl_lower_bounds(Sym *s) {
    Node *n = s->node, *g, *w;
    long i;
    if (s->flags & CL_BDONE) return;
    s->flags |= CL_BDONE;
    if (cl_is_assoc(s) && s->parent->nbounds > 0) {
        s->bounds = (Bound *)ast_alloc(sizeof(Bound) * (size_t)s->parent->nbounds);
        memcpy(s->bounds, s->parent->bounds, sizeof(Bound) * (size_t)s->parent->nbounds);
        s->nbounds = s->parent->nbounds;
    }
    g = cl_generics_node(s);
    if (g) {
        for (i = 0; i < g->list.n; i++) {
            Node *p = g->list.v[i];
            if (p->kind == N_GEN_TYPE && p->sym) cl_add_bounds(s, ty_param((Sym *)p->sym), p->b);
            else if (p->kind == N_GEN_CONST && p->sym) ((Sym *)p->sym)->ty = cl_lower(p->a, s);
        }
    }
    if (s->kind == SYM_TRAIT) cl_add_bounds(s, ty_self(), n->c);
    if (s->kind == SYM_FN || s->kind == SYM_STRUCT || s->kind == SYM_UNION || s->kind == SYM_ENUM ||
        s->kind == SYM_TRAIT || s->kind == SYM_IMPL || s->kind == SYM_TYPEALIAS) {
        w = n->d;
        if (w && w->kind == N_WHERE) {
            for (i = 0; i < w->list.n; i++) {
                Node *c = w->list.v[i];
                if (c->kind == N_WHERE_TYPE) cl_add_bounds(s, cl_lower(c->a, s), c->b);
            }
        }
    }
    if (s->kind == SYM_TRAIT) {
        /* `type X: Bound;` */
        int k, cnt = scope_member_count(s->members);
        for (k = 0; k < cnt; k++) {
            Sym *m = scope_member(s->members, k);
            if (m->kind == SYM_ASSOC_TYPE && m->node && m->node->c) {
                Type *pt = ty_proj(ty_self(), s, m->name, s->ngparams ? cl_identity(s) : 0, s->ngparams);
                cl_add_bounds(s, pt, m->node->c);
            }
        }
    }
}

/* ================================================================ signatures */

static Type *cl_self_type_of(Sym *s) {
    Sym *p = s->parent;
    if (!p) return 0;
    if (p->kind == SYM_TRAIT) return ty_self();
    if (p->kind == SYM_IMPL) return p->impl_self;
    return 0;
}

static void cl_lower_fn(Sym *s) {
    Node *n = s->node;
    Type *selfty = cl_self_type_of(s), **ps, *ret, *t;
    long i;
    int np = 0;
    ps = cl_tvec((int)n->list.n);
    for (i = 0; i < n->list.n; i++) {
        Node *p = n->list.v[i];
        if (p->kind == N_SELF_PARAM) {
            if (!selfty)
                error_at(p->file, p->line, "`self` parameter is only allowed in associated functions");
            if (p->flags & F_TYPED) t = cl_lower(p->b, s);
            else if (p->flags & F_REF) t = ty_ref(selfty, (p->flags & F_MUT) != 0);
            else t = selfty;
        } else {
            t = cl_lower(p->b, s);
        }
        ps[np++] = t;
    }
    ret = n->b ? cl_lower(n->b, s) : ty_unit();
    s->ty = ty_fn_def(s, s->ngparams ? cl_identity(s) : 0, s->ngparams, ps, np, ret);
    if (np > 0 && n->list.n > 0 && n->list.v[0]->kind == N_SELF_PARAM) s->flags |= SF_METHOD;
}

static void cl_lower_adt(Sym *s) {
    int i, j, cnt = scope_member_count(s->members);
    for (i = 0; i < cnt; i++) {
        Sym *m = scope_member(s->members, i);
        if (s->kind == SYM_ENUM) {
            int fc = scope_member_count(m->members);
            m->ty = s->ty;
            for (j = 0; j < fc; j++) {
                Sym *f = scope_member(m->members, j);
                f->ty = cl_lower(f->node->a, s);
            }
        } else {
            m->ty = cl_lower(m->node->a, s);
        }
    }
}

static void cl_lower_impl_header(Sym *s) {
    Node *n = s->node;
    Type *d, **args;
    Subst sub;
    int k;
    s->impl_self = cl_lower(n->b, s);
    s->ty = s->impl_self;
    if (!n->c) {
        s->trait = 0;
        return;
    }
    d = ty_from_path_prefix(n->c, (int)n->c->list.n);
    if (!d || d->kind != TY_DYN) error_at(n->c->file, n->c->line, "expected a trait, found type");
    s->trait = d->def;
    sub.params = 0;
    sub.args = 0;
    sub.n = 0;
    sub.self = s->impl_self;
    args = cl_tvec(d->nargs);
    for (k = 0; k < d->nargs; k++) args[k] = ty_subst(d->args[k], &sub);
    s->trait_args = args;
    s->ntrait_args = d->nargs;
    if (d->def == cl_lang_copy) s->flags |= SF_COPY;
}

static void cl_lower_sig(Sym *s) {
    Node *n = s->node;
    switch (s->kind) {
    case SYM_FN:
        cl_lower_fn(s);
        break;
    case SYM_CONST: case SYM_STATIC:
        s->ty = cl_lower(n->a, s);
        break;
    case SYM_TYPEALIAS:
        if (!(s->flags & SF_EXTERN) && n->b) s->ty = cl_fill_projs(s->ty ? s->ty : ty_from_ast(n->b), s);
        break;
    case SYM_ASSOC_TYPE:
        if (s->parent && s->parent->kind == SYM_IMPL && n->b)
            s->ty = cl_fill_projs(s->ty ? s->ty : ty_from_ast(n->b), s);
        break;
    case SYM_STRUCT: case SYM_UNION: case SYM_ENUM:
        cl_lower_adt(s);
        break;
    default:
        break;
    }
}

/* ================================================================ constructor keys */

static int cl_ctor_key(Type *t) {
    t = ty_resolve(t);
    if (!t) return 0;
    switch (t->kind) {
    case TY_BOOL: return 1;
    case TY_CHAR: return 2;
    case TY_STR: return 3;
    case TY_NEVER: return 4;
    case TY_TUPLE: return 5;
    case TY_ARRAY: return 6;
    case TY_SLICE: return 7;
    case TY_REF: return 8;
    case TY_PTR: return 9;
    case TY_FN_PTR: return 10;
    case TY_FN_DEF: return 11;
    case TY_CLOSURE: return 12;
    case TY_PROJ: return 13;
    case TY_SELF: return 14;
    case TY_PARAM: return 15;
    case TY_INT: return 16 + t->ik;
    case TY_FLOAT: return t->ik == 32 ? 30 : 31;
    case TY_ADT: return 64 + 2 * (t->def ? t->def->index : 0);
    case TY_DYN: return 64 + 2 * (t->def ? t->def->index : 0) + 1;
    }
    return 0;
}

/* the key of a query type: -2 any (unbound variable), -3 integer variable, -4 float variable */
static int cl_query_key(Type *t) {
    t = ty_resolve(t);
    if (t->kind == TY_INFER) {
        if (t->ik == INFER_INT) return -3;
        if (t->ik == INFER_FLOAT) return -4;
        return -2;
    }
    return cl_ctor_key(t);
}

static int cl_key_ok(int ik, int qkey) {
    if (ik == -1 || qkey == -2) return 1;
    if (qkey == -3) return ik >= 17 && ik <= 28;
    if (qkey == -4) return ik == 30 || ik == 31;
    return ik == qkey;
}

Sym *collect_impls_for(Type *self) {
    int k;
    if (!cl_ready || !self) return 0;
    k = cl_ctor_key(self);
    if (k < 0 || k + 1 >= cl_nbuckets) return 0;
    return cl_head[k + 1];
}

Sym *collect_blanket_impls(void) {
    return cl_ready ? cl_head[0] : 0;
}

Sym **collect_trait_impls(Sym *trait, int *n) {
    if (!cl_ready || !trait || trait->kind != SYM_TRAIT || trait->index < 0 ||
        trait->index >= cl_ntraits) {
        *n = 0;
        return 0;
    }
    *n = cl_tinfo[trait->index].n;
    return cl_tinfo[trait->index].v;
}

Sym **collect_all_traits(int *n) {
    *n = cl_ntraits;
    return cl_traits;
}

/* ================================================================ environment */

void collect_set_env(Sym *owner) {
    cl_env = owner;
}

Sym *collect_env(void) {
    return cl_env;
}

static Sym *cl_env_trait(Sym *env) {
    if (!env) return 0;
    if (env->kind == SYM_TRAIT) return env;
    if (env->parent && env->parent->kind == SYM_TRAIT) return env->parent;
    return 0;
}

/* ================================================================ queries */

static int cl_prove(Type *t, Sym *trait, Type **wargs, int nw, int depth);
static Type *cl_norm_depth(Type *t, int depth);
static Sym *cl_find(Sym *trait, Type *self, Type **targs, int nt, Subst *out, int depth);

static int cl_infer_in(Type **v, int n) {
    int i;
    for (i = 0; i < n; i++)
        if (v[i] && ty_has_infer(v[i])) return 1;
    return 0;
}

/* The bound (bty: btrait<bargs>, with the binding ban = bat) implies
 * want<wargs> (and, when aname is given, the binding aname = *aout), directly
 * or through supertraits.  On success the arguments are unified (bindings
 * stay). */
static int cl_expand(Type *bty, Sym *btrait, Type **bargs, int nb, Name *ban, Type *bat,
                     Sym *want, Type **wargs, int nw, Name *aname, Type **aout, int depth) {
    int mark, i, k;
    Subst s;
    Type **sargs;
    if (btrait == want) {
        mark = cl_mark();
        for (i = 0; i < nw && i < nb; i++) {
            if (!cl_unify(bargs[i], wargs[i])) {
                cl_undo(mark);
                return 0;
            }
        }
        if (aname) {
            if (ban == aname && bat) {
                if (aout) *aout = bat;
                return 1;
            }
            cl_undo(mark);
            return 0;
        }
        return 1;
    }
    if (depth > 8 || btrait->nbounds == 0) return 0;
    s.params = btrait->gparams;
    s.args = bargs;
    s.n = nb < btrait->ngparams ? nb : btrait->ngparams;
    s.self = bty;
    for (k = 0; k < btrait->nbounds; k++) {
        Bound *sb = &btrait->bounds[k];
        Name *an = ban;
        Type *at = bat;
        if (!sb->ty || sb->ty->kind != TY_SELF) continue;
        sargs = cl_tvec(sb->nargs);
        for (i = 0; i < sb->nargs; i++) sargs[i] = ty_subst(sb->args[i], &s);
        if (sb->assoc_name) {
            an = sb->assoc_name;
            at = ty_subst(sb->assoc_ty, &s);
        }
        if (cl_expand(bty, sb->trait, sargs, sb->nargs, an, at, want, wargs, nw, aname, aout,
                      depth + 1))
            return 1;
    }
    return 0;
}

/* the arguments of supertrait `want` implied by btrait<bargs> for bty */
static int cl_super_args(Type *bty, Sym *btrait, Type **bargs, int nb, Sym *want, Type ***oargs,
                         int *on, int depth) {
    int i, k;
    Subst s;
    Type **sargs;
    if (btrait == want) {
        *oargs = bargs;
        *on = nb;
        return 1;
    }
    if (depth > 8) return 0;
    if (!(btrait->flags & CL_BDONE)) cl_lower_bounds(btrait);
    s.params = btrait->gparams;
    s.args = bargs;
    s.n = nb < btrait->ngparams ? nb : btrait->ngparams;
    s.self = bty;
    for (k = 0; k < btrait->nbounds; k++) {
        Bound *sb = &btrait->bounds[k];
        if (!sb->ty || sb->ty->kind != TY_SELF) continue;
        sargs = cl_tvec(sb->nargs);
        for (i = 0; i < sb->nargs; i++) sargs[i] = ty_subst(sb->args[i], &s);
        if (cl_super_args(bty, sb->trait, sargs, sb->nargs, want, oargs, on, depth + 1)) return 1;
    }
    return 0;
}

/* where-clauses of the current environment */
static int cl_env_find(Type *t, Sym *trait, Type **wargs, int nw, Name *aname, Type **aout) {
    Sym *env = cl_env, *tr;
    int i, k;
    if (!env) return 0;
    tr = cl_env_trait(env);
    if (tr && t->kind == TY_SELF) {
        Type **ids = tr->ngparams ? cl_identity(tr) : 0;
        if (cl_expand(t, tr, ids, tr->ngparams, 0, 0, trait, wargs, nw, aname, aout, 0)) return 1;
    }
    for (i = 0; i < env->nbounds; i++) {
        Bound *b = &env->bounds[i];
        if (!ty_equal(b->ty, t)) continue;
        if (cl_expand(t, b->trait, b->args, b->nargs, b->assoc_name, b->assoc_ty, trait, wargs, nw,
                      aname, aout, 0))
            return 1;
    }
    if (t->kind == TY_PROJ && t->def && t->nargs > 0) {
        Sym *pt = t->def;
        for (i = 0; i < pt->nbounds; i++) {
            Bound *b = &pt->bounds[i];
            Subst s;
            Type **bargs;
            if (!b->ty || b->ty->kind != TY_PROJ || b->ty->name != t->name || b->ty->def != pt) continue;
            s.params = pt->gparams;
            s.args = t->args + 1;
            s.n = t->nargs - 1 < pt->ngparams ? t->nargs - 1 : pt->ngparams;
            s.self = t->args[0];
            bargs = cl_tvec(b->nargs);
            for (k = 0; k < b->nargs; k++) bargs[k] = ty_subst(b->args[k], &s);
            if (cl_expand(t, b->trait, bargs, b->nargs, b->assoc_name,
                          b->assoc_ty ? ty_subst(b->assoc_ty, &s) : 0, trait, wargs, nw, aname, aout, 0))
                return 1;
        }
    }
    return 0;
}

static int cl_is_fn_trait(Sym *tr) {
    return tr && (tr == cl_lang_fn || tr == cl_lang_fn_mut || tr == cl_lang_fn_once);
}

static int cl_and(int a, int b) {
    if (a == 0 || b == 0) return 0;
    if (a == 2 || b == 2) return 2;
    return 1;
}

/* the built-in impls: 1 holds, 2 maybe, 0 does not hold, -1 not a built-in case */
static int cl_builtin(Type *t, Sym *trait, Type **wargs, int nw, int depth) {
    int i, r;
    if (trait == cl_lang_copy || trait == cl_lang_clone) {
        switch (t->kind) {
        case TY_BOOL: case TY_CHAR: case TY_INT: case TY_FLOAT: case TY_NEVER:
        case TY_PTR: case TY_FN_PTR: case TY_FN_DEF:
            return 1;
        case TY_INFER:
            return t->ik == INFER_INT || t->ik == INFER_FLOAT ? 1 : -1;
        case TY_REF:
            return t->mut ? (trait == cl_lang_copy ? 0 : -1) : 1;
        case TY_TUPLE:
            r = 1;
            for (i = 0; i < t->nargs; i++) r = cl_and(r, cl_prove(t->args[i], trait, 0, -1, depth + 1));
            return r;
        case TY_ARRAY:
            if (!t->elem) return 1;
            return cl_prove(t->elem, trait, 0, -1, depth + 1);
        case TY_CLOSURE:
            if (collect_closure_impl) return collect_closure_impl(t, trait) ? 1 : 0;
            return 1;
        case TY_STR: case TY_SLICE: case TY_DYN:
            return trait == cl_lang_copy ? 0 : -1;
        default:
            return -1;
        }
    }
    if (cl_is_fn_trait(trait) &&
        (t->kind == TY_FN_DEF || t->kind == TY_FN_PTR || t->kind == TY_CLOSURE)) {
        if (t->kind == TY_CLOSURE && collect_closure_impl && !collect_closure_impl(t, trait)) return 0;
        if (nw > 0 && wargs && wargs[0]) {
            if (!cl_unify(wargs[0], ty_tuple(t->params, t->nparams))) return 0;
        }
        return 1;
    }
    return -1;
}

static int cl_sized(Type *t) {
    switch (t->kind) {
    case TY_STR: case TY_SLICE: case TY_DYN:
        return 0;
    case TY_PARAM:
        return t->def && (t->def->flags & CL_UNSIZED) ? 0 : 1;
    case TY_INFER:
        return t->ik == 0 ? 2 : 1;
    default:
        return 1;
    }
}

static int cl_auto_trait(Sym *tr) {
    if (!tr) return 0;
    if (tr == cl_lang_send || tr == cl_lang_sync) return 1;
    return tr->node && (tr->node->flags & F_AUTO) ? 1 : 0;
}

static int cl_bound_holds(Bound *b, Subst *s, int depth) {
    Type *bt = ty_subst(b->ty, s), **args = cl_tvec(b->nargs), *norm, *want;
    Sym *a, *dt;
    Type **dargs;
    int k, r, nd;
    for (k = 0; k < b->nargs; k++) args[k] = ty_subst(b->args[k], s);
    r = cl_prove(bt, b->trait, args, b->nargs, depth + 1);
    if (r == 0) return 0;
    if (b->assoc_name && b->assoc_ty) {
        a = cl_trait_assoc(b->trait, b->assoc_name, 0);
        dt = a && a->parent && a->parent->kind == SYM_TRAIT ? a->parent : b->trait;
        dargs = args;
        nd = b->nargs;
        if (dt != b->trait) cl_super_args(bt, b->trait, args, b->nargs, dt, &dargs, &nd, 0);
        norm = cl_norm_depth(ty_proj(bt, dt, b->assoc_name, dargs, nd), depth + 1);
        if (norm && norm->kind != TY_PROJ) {
            want = ty_subst(b->assoc_ty, s);
            if (!cl_unify(norm, want)) return 0;
        }
    }
    return r;
}

/* the impl's header and where-clauses against the query; leaves bindings.
 * 0 no, 1 yes, 2 maybe */
static int cl_match(Sym *impl, Type *self, Type **targs, int nt, int depth, Subst *out) {
    int n = impl->ngparams, i, ok, val = 1, r;
    Type **fresh = cl_tvec(n), *pat;
    Subst s;
    Sym **sp = cl_cp_params;
    Type **sa = cl_cp_args;
    int sn = cl_cp_n;
    if (!impl->impl_self) return 0;
    for (i = 0; i < n; i++) fresh[i] = ty_infer(0);
    s.params = impl->gparams;
    s.args = fresh;
    s.n = n;
    s.self = 0;
    cl_cp_params = impl->gparams;
    cl_cp_args = fresh;
    cl_cp_n = n;
    pat = n > 0 ? ty_subst(impl->impl_self, &s) : impl->impl_self;
    ok = cl_unify(pat, self);
    for (i = 0; ok && i < nt && i < impl->ntrait_args; i++)
        ok = cl_unify(n > 0 ? ty_subst(impl->trait_args[i], &s) : impl->trait_args[i], targs[i]);
    cl_cp_params = sp;
    cl_cp_args = sa;
    cl_cp_n = sn;
    for (i = 0; ok && i < impl->nbounds; i++) {
        r = cl_bound_holds(&impl->bounds[i], &s, depth);
        if (r == 0) ok = 0;
        else if (r == 2) val = 2;
    }
    if (!ok) return 0;
    if (out) {
        *out = s;
        out->self = self;
    }
    return val;
}

static Sym *cl_select(Sym *trait, Type *self, Type **targs, int nt, int depth, int stop_first,
                      int *nmatch, int *val) {
    ClSymList *tl;
    int qkey, i, m, r;
    Sym *first = 0;
    Subst tmp;
    *nmatch = 0;
    *val = 0;
    if (trait->index < 0 || trait->index >= cl_ntraits) return 0;
    tl = &cl_tinfo[trait->index];
    qkey = cl_query_key(self);
    for (i = 0; i < tl->n; i++) {
        Sym *impl = tl->v[i];
        if (!cl_key_ok(cl_ikey[impl->index], qkey)) continue;
        m = cl_mark();
        r = cl_match(impl, self, targs, nt, depth, &tmp);
        cl_undo(m);
        if (r) {
            (*nmatch)++;
            if (!first) {
                first = impl;
                *val = r;
            }
            if (stop_first) break;
        }
    }
    return first;
}

/* does t implement trait<wargs> (nw < 0: any arguments)?  0 no, 1 yes, 2 maybe.
 * Leaves the inference variables as they were. */
static int cl_prove_in(Type *t, Sym *trait, Type **wargs, int nw, int depth) {
    int r, nmatch, val, has_inf;
    Sym *impl;
    if (depth > CL_MAXDEPTH) return 2;
    t = cl_norm_depth(t, depth);
    switch (t->kind) {
    case TY_ERR: case TY_NEVER:
        return 1;
    case TY_INFER:
        if (t->ik == 0) return 2;
        break;
    default:
        break;
    }
    if (cl_auto_trait(trait)) return 1;
    if (trait == cl_lang_sized) return cl_sized(t);
    if (cl_env_find(t, trait, wargs, nw, 0, 0)) return 1;
    r = cl_builtin(t, trait, wargs, nw, depth);
    if (r > 0) return r;
    if (t->kind == TY_DYN &&
        cl_expand(t, t->def, t->args, t->nargs, 0, 0, trait, wargs, nw, 0, 0, 0))
        return 1;
    has_inf = ty_has_infer(t) || (nw > 0 && cl_infer_in(wargs, nw));
    impl = cl_select(trait, t, wargs, nw < 0 ? 0 : nw, depth, !has_inf, &nmatch, &val);
    if (!impl) return r == 0 ? 0 : 0;
    if (nmatch > 1 && has_inf) return 2;
    return val;
}

static int cl_prove(Type *t, Sym *trait, Type **wargs, int nw, int depth) {
    int mark = cl_mark(), r;
    r = cl_prove_in(t, trait, wargs, nw, depth);
    cl_undo(mark);
    return r;
}

static Sym *cl_builtin_result(Subst *out, Type *self) {
    if (out) {
        out->params = 0;
        out->args = 0;
        out->n = 0;
        out->self = self;
    }
    return collect_builtin_impl;
}

static Sym *cl_find(Sym *trait, Type *self, Type **targs, int nt, Subst *out, int depth) {
    Sym *impl;
    int nmatch, val, r, mark, has_inf;
    Subst tmp;
    if (!cl_ready || !trait || trait->kind != SYM_TRAIT) return 0;
    if (depth > CL_MAXDEPTH) return 0;
    self = cl_norm_depth(self, depth);
    if (self->kind == TY_ERR) return cl_builtin_result(out, self);
    if (cl_env_find(self, trait, targs, nt, 0, 0)) return cl_builtin_result(out, self);
    r = cl_builtin(self, trait, targs, nt, depth);
    if (r > 0) return cl_builtin_result(out, self);
    if (self->kind == TY_DYN &&
        cl_expand(self, self->def, self->args, self->nargs, 0, 0, trait, targs, nt, 0, 0, 0))
        return cl_builtin_result(out, self);
    if (self->kind == TY_INFER && self->ik == 0 && nt == 0 && 0) return 0;
    has_inf = ty_has_infer(self) || cl_infer_in(targs, nt);
    impl = cl_select(trait, self, targs, nt, depth, !has_inf, &nmatch, &val);
    if (!impl) return 0;
    if (nmatch > 1) {
        find_impl_ambiguous = 1;
        return 0;
    }
    mark = cl_mark();
    if (!cl_match(impl, self, targs, nt, depth, &tmp)) {
        cl_undo(mark);
        return 0;
    }
    if (out) *out = tmp;
    return impl;
}

Sym *find_impl(Sym *trait, Type *self, Type **trait_args, int ntrait_args, Subst *out) {
    int i;
    Type **targs = 0;
    find_impl_ambiguous = 0;
    if (!self) return 0;
    if (ntrait_args > 0) {
        targs = cl_tvec(ntrait_args);
        for (i = 0; i < ntrait_args; i++) targs[i] = cl_norm_depth(trait_args[i], 0);
    }
    return cl_find(trait, self, targs, ntrait_args, out, 0);
}

int implements_with(Type *t, Sym *trait, Type **args, int nargs) {
    if (!t || !trait) return 0;
    if (!cl_ready) return 0;
    return cl_prove(t, trait, args, nargs, 0);
}

int implements(Type *t, Sym *trait) {
    return implements_with(t, trait, 0, -1) != 0;
}

int is_copy(Type *t) {
    int r;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t->kind == TY_INFER) return t->ik == INFER_INT || t->ik == INFER_FLOAT;
    if (t->kind == TY_ADT && t->def && !(t->def->flags & SF_COPY)) return 0;
    if (!cl_lang_copy || !cl_ready) {
        switch (t->kind) {
        case TY_BOOL: case TY_CHAR: case TY_INT: case TY_FLOAT: case TY_NEVER: case TY_PTR:
        case TY_FN_PTR: case TY_FN_DEF:
            return 1;
        case TY_REF:
            return !t->mut;
        default:
            return 0;
        }
    }
    r = cl_prove(t, cl_lang_copy, 0, -1, 0);
    return r == 1;
}

Sym *assoc_item(Sym *owner, int ns, Name *name) {
    Sym *a;
    int k;
    if (!owner || !owner->members) return 0;
    a = scope_get(owner->members, ns, name);
    if (a) return a;
    if (owner->kind == SYM_IMPL && owner->trait) return assoc_item(owner->trait, ns, name);
    if (owner->kind == SYM_TRAIT) {
        for (k = 0; k < owner->nbounds; k++) {
            Bound *b = &owner->bounds[k];
            if (b->ty && b->ty->kind == TY_SELF && b->trait != owner) {
                a = assoc_item(b->trait, ns, name);
                if (a) return a;
            }
        }
    }
    return 0;
}

/* `<self as trait<targs>>::name`, or 0 when it cannot be resolved yet */
static Type *cl_project(Type *self, Sym *trait, Type **targs, int nt, Name *name, int depth) {
    Type *res = 0;
    Sym *impl, *item;
    Subst s;
    int k;
    self = ty_resolve(self);
    if (!self || depth > CL_MAXDEPTH) return 0;
    if (self->kind == TY_INFER && self->ik == 0) return 0;
    if (name == cl_n_output && cl_is_fn_trait(trait) &&
        (self->kind == TY_FN_DEF || self->kind == TY_FN_PTR || self->kind == TY_CLOSURE))
        return self->ret;
    if (cl_env_find(self, trait, targs, nt, name, &res)) return res;
    if (self->kind == TY_DYN) {
        for (k = 0; k < self->nparams; k++) {
            Type *p = self->params[k];
            if (p && p->name == name) return p->ret;
        }
    }
    impl = cl_find(trait, self, targs, nt, &s, depth + 1);
    if (!impl || impl == collect_builtin_impl) return 0;
    item = scope_get(impl->members, NS_TYPE, name);
    if (!item || item->kind != SYM_ASSOC_TYPE || !item->ty) return 0;
    return ty_subst(item->ty, &s);
}

static Type *cl_norm_fn(Type *t, void *ud) {
    int depth = *(int *)ud;
    Type *r;
    if (t->kind != TY_PROJ || t->nargs == 0 || !t->def) return t;
    r = cl_project(t->args[0], t->def, t->args + 1, t->nargs - 1, t->name, depth + 1);
    if (!r) return t;
    return cl_norm_depth(r, depth + 1);
}

static Type *cl_norm_depth(Type *t, int depth) {
    t = ty_resolve(t);
    if (!t || !cl_has_proj(t)) return t;
    if (depth > CL_MAXDEPTH) return t;
    return cl_map(t, cl_norm_fn, &depth);
}

Type *normalize(Type *t) {
    if (!cl_ready) return t;
    return cl_norm_depth(t, 0);
}

/* ================================================================ impl checks */

static void cl_type_text(StrBuf *sb, Type *t) {
    sb_puts(sb, ty_str(t));
}

static const char *cl_trait_text(Sym *tr, Type **args, int n) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    sb_puts(&sb, cl_nm(tr));
    if (n > 0) {
        sb_putc(&sb, '<');
        for (i = 0; i < n; i++) {
            if (i) sb_puts(&sb, ", ");
            cl_type_text(&sb, args[i]);
        }
        sb_putc(&sb, '>');
    }
    return sb_take(&sb);
}

static int cl_trait_item_required(Sym *ti) {
    Node *n = ti->node;
    if (!n) return 0;
    switch (ti->kind) {
    case SYM_FN: return n->c == 0;
    case SYM_CONST: return n->b == 0;
    case SYM_ASSOC_TYPE: return n->b == 0;
    }
    return 0;
}

static int cl_count_type_params(Sym *fn, int from) {
    int i, n = 0;
    for (i = from; i < fn->ngparams; i++)
        if (fn->gparams[i]->kind == SYM_GENERIC) n++;
    return n;
}

static const char *cl_self_text(Node *fn) {
    Node *p = fn->list.n > 0 ? fn->list.v[0] : 0;
    if (!p || p->kind != N_SELF_PARAM) return "self";
    if (p->flags & F_TYPED) return "self: _";
    if (p->flags & F_REF) return (p->flags & F_MUT) ? "&mut self" : "&self";
    return (p->flags & F_MUT) ? "mut self" : "self";
}

static int cl_sig_same(Type *a, Type *b) {
    a = ty_resolve(a);
    b = ty_resolve(b);
    if (!a || !b) return 1;
    if (cl_has_proj(a) || cl_has_proj(b)) return 1;
    if (a->kind == TY_ERR || b->kind == TY_ERR) return 1;
    return ty_equal(a, b);
}

/* an impl fn / const against the trait's declaration */
static void cl_check_compat(Sym *item, Sym *ti, Sym *impl) {
    Node *in = item->node, *tn = ti->node;
    int tp = impl->trait->ngparams, ip = impl->ngparams, i, nown, ntrait;
    Type **args, *a, *b;
    Subst s;
    if (item->kind == SYM_FN) {
        int ihas = (item->flags & SF_METHOD) != 0, thas = (ti->flags & SF_METHOD) != 0;
        if (thas && !ihas)
            error_at(in->file, in->line,
                     "method `%s` has a `%s` declaration in the trait, but not in the impl",
                     cl_nm(item), cl_self_text(tn));
        if (ihas && !thas)
            error_at(in->file, in->line,
                     "method `%s` has a `%s` declaration in the impl, but not in the trait",
                     cl_nm(item), cl_self_text(in));
        if (item->ty && ti->ty && item->ty->nparams != ti->ty->nparams)
            error_at(in->file, in->line,
                     "method `%s` has %d parameter%s but the declaration in trait `%s::%s` has %d",
                     cl_nm(item), item->ty->nparams, item->ty->nparams == 1 ? "" : "s",
                     cl_nm(impl->trait), cl_nm(ti), ti->ty->nparams);
        nown = cl_count_type_params(item, ip);
        ntrait = cl_count_type_params(ti, tp);
        if (nown != ntrait)
            error_at(in->file, in->line,
                     "method `%s` has %d type parameter%s but its trait declaration has %d type parameter%s",
                     cl_nm(item), nown, nown == 1 ? "" : "s", ntrait, ntrait == 1 ? "" : "s");
        if (item->ngparams - ip != ti->ngparams - tp || impl->ntrait_args != tp) return;
        if (!item->ty || !ti->ty) return;
        args = cl_tvec(ti->ngparams);
        for (i = 0; i < tp; i++) args[i] = impl->trait_args[i];
        for (i = tp; i < ti->ngparams; i++) args[i] = ty_param(item->gparams[ip + i - tp]);
        s.params = ti->gparams;
        s.args = args;
        s.n = ti->ngparams;
        s.self = impl->impl_self;
        collect_set_env(item);
        for (i = 0; i < item->ty->nparams; i++) {
            a = cl_norm_depth(ty_subst(ti->ty->params[i], &s), 0);
            b = cl_norm_depth(item->ty->params[i], 0);
            if (!cl_sig_same(a, b))
                error_at(in->file, in->line, "method `%s` has an incompatible type for trait", cl_nm(item));
        }
        a = cl_norm_depth(ty_subst(ti->ty->ret, &s), 0);
        b = cl_norm_depth(item->ty->ret, 0);
        if (!cl_sig_same(a, b))
            error_at(in->file, in->line, "method `%s` has an incompatible type for trait", cl_nm(item));
        collect_set_env(0);
    } else if (item->kind == SYM_CONST && item->ty && ti->ty && impl->ntrait_args == tp) {
        s.params = impl->trait->gparams;
        s.args = impl->trait_args;
        s.n = tp;
        s.self = impl->impl_self;
        collect_set_env(item);
        a = cl_norm_depth(ty_subst(ti->ty, &s), 0);
        b = cl_norm_depth(item->ty, 0);
        if (!cl_sig_same(a, b))
            error_at(in->file, in->line, "implemented const `%s` has an incompatible type for trait",
                     cl_nm(item));
        collect_set_env(0);
    }
}

static const char *cl_assoc_kind_word(Sym *s) {
    switch (s->kind) {
    case SYM_FN: return "method";
    case SYM_CONST: return "const";
    default: return "type";
    }
}

static void cl_match_items(Sym *impl) {
    Sym *tr = impl->trait;
    int nm = scope_member_count(impl->members), nt = scope_member_count(tr->members), i, j, nmiss = 0;
    char *seen = (char *)xalloc((size_t)nt + 1);
    StrBuf miss;
    for (i = 0; i < nm; i++) {
        Sym *it = scope_member(impl->members, i), *ti = 0;
        int ns = it->kind == SYM_ASSOC_TYPE ? NS_TYPE : NS_VALUE;
        ti = scope_get(tr->members, ns, it->name);
        if (!ti) {
            error_at(it->node->file, it->node->line, "%s `%s` is not a member of trait `%s`",
                     cl_assoc_kind_word(it), cl_nm(it), cl_nm(tr));
        }
        if (ti->kind != it->kind) {
            error_at(it->node->file, it->node->line,
                     "item `%s` is an associated %s, which doesn't match its trait `%s`", cl_nm(it),
                     cl_assoc_kind_word(it), cl_nm(tr));
        }
        it->trait = tr;
        it->trait_item = ti;
        for (j = 0; j < nt; j++)
            if (scope_member(tr->members, j) == ti) seen[j] = 1;
    }
    sb_init(&miss);
    for (j = 0; j < nt; j++) {
        Sym *ti = scope_member(tr->members, j);
        if (seen[j] || !cl_trait_item_required(ti)) continue;
        if (nmiss++) sb_puts(&miss, ", ");
        sb_putc(&miss, '`');
        sb_puts(&miss, cl_nm(ti));
        sb_putc(&miss, '`');
    }
    if (nmiss)
        error_at(impl->node->file, impl->node->line, "not all trait items implemented, missing: %s",
                 miss.s);
    free(seen);
}

static void cl_check_item_compat(Sym *impl) {
    int nm = scope_member_count(impl->members), i;
    for (i = 0; i < nm; i++) {
        Sym *it = scope_member(impl->members, i);
        if (it->trait_item && (it->kind == SYM_FN || it->kind == SYM_CONST))
            cl_check_compat(it, it->trait_item, impl);
    }
}

static void cl_check_supers(Sym *impl) {
    Sym *tr = impl->trait;
    Subst s;
    int k, i;
    if (impl->ntrait_args != tr->ngparams) return;
    s.params = tr->gparams;
    s.args = impl->trait_args;
    s.n = tr->ngparams;
    s.self = impl->impl_self;
    collect_set_env(impl);
    for (k = 0; k < tr->nbounds; k++) {
        Bound *sb = &tr->bounds[k];
        Type **args;
        if (!sb->ty || sb->ty->kind != TY_SELF) continue;
        args = cl_tvec(sb->nargs);
        for (i = 0; i < sb->nargs; i++) args[i] = ty_subst(sb->args[i], &s);
        if (cl_prove(impl->impl_self, sb->trait, args, sb->nargs, 0) == 0)
            error_at(impl->node->file, impl->node->line, "the trait bound `%s: %s` is not satisfied",
                     ty_str(impl->impl_self), cl_trait_text(sb->trait, args, sb->nargs));
    }
    collect_set_env(0);
}

static void cl_check_copy(Sym *impl) {
    Type *self = ty_resolve(impl->impl_self);
    Sym *def;
    Subst s;
    int i, j, cnt, ok = 1;
    if (self->kind != TY_ADT || !self->def) return;
    def = self->def;
    def->flags |= SF_COPY;
    if (def->kind == SYM_UNION) return;
    s.params = def->gparams;
    s.args = self->args;
    s.n = self->nargs < def->ngparams ? self->nargs : def->ngparams;
    s.self = 0;
    collect_set_env(impl);
    cnt = scope_member_count(def->members);
    for (i = 0; ok && i < cnt; i++) {
        Sym *m = scope_member(def->members, i);
        if (def->kind == SYM_ENUM) {
            int fc = scope_member_count(m->members);
            for (j = 0; ok && j < fc; j++) {
                Sym *f = scope_member(m->members, j);
                if (f->ty && cl_prove(ty_subst(f->ty, &s), cl_lang_copy, 0, -1, 0) == 0) ok = 0;
            }
        } else if (m->ty && cl_prove(ty_subst(m->ty, &s), cl_lang_copy, 0, -1, 0) == 0) {
            ok = 0;
        }
    }
    collect_set_env(0);
    if (!ok)
        error_at(impl->node->file, impl->node->line, "the trait `Copy` cannot be implemented for this type");
}

static void cl_check_constrained(Sym *impl) {
    int i, k, found;
    for (i = 0; i < impl->ngparams; i++) {
        Sym *g = impl->gparams[i];
        found = cl_occurs_param(impl->impl_self, g);
        for (k = 0; !found && k < impl->ntrait_args; k++) found = cl_occurs_param(impl->trait_args[k], g);
        for (k = 0; !found && k < impl->nbounds; k++)
            if (impl->bounds[k].assoc_ty) found = cl_occurs_param(impl->bounds[k].assoc_ty, g);
        if (!found)
            error_at(impl->node->file, impl->node->line,
                     "the %s parameter `%s` is not constrained by the impl trait, self type, or predicates",
                     g->kind == SYM_CONST_GENERIC ? "const" : "type", cl_nm(g));
    }
}

static int cl_local_ty(Type *t) {
    t = ty_resolve(t);
    if (!t) return 0;
    switch (t->kind) {
    case TY_ADT:
        if (t->def && !(t->def->flags & SF_CORE)) return 1;
        if (t->def && t->def == cl_lang_box && t->nargs > 0) return cl_local_ty(t->args[0]);
        return 0;
    case TY_DYN:
        return t->def && !(t->def->flags & SF_CORE);
    case TY_REF:
        return cl_local_ty(t->elem);
    }
    return 0;
}

static void cl_check_orphan(Sym *impl) {
    Type *self = ty_resolve(impl->impl_self), *peeled;
    int i;
    if (impl->flags & SF_CORE) return;
    if (!impl->trait) {
        switch (self->kind) {
        case TY_ADT:
            if (self->def->flags & SF_CORE)
                error_at(impl->node->file, impl->node->line,
                         "cannot define inherent `impl` for a type outside of the crate where the type is defined");
            break;
        case TY_DYN:
            if (self->def->flags & SF_CORE)
                error_at(impl->node->file, impl->node->line,
                         "cannot define inherent `impl` for a type outside of the crate where the type is defined");
            break;
        case TY_PARAM:
            error_at(impl->node->file, impl->node->line,
                     "no nominal type found for inherent implementation");
            break;
        default:
            error_at(impl->node->file, impl->node->line,
                     "cannot define inherent `impl` for primitive types");
        }
        return;
    }
    if (!(impl->trait->flags & SF_CORE)) return;
    if (cl_local_ty(self)) return;
    for (i = 0; i < impl->ntrait_args; i++)
        if (cl_local_ty(impl->trait_args[i])) return;
    peeled = self;
    while (peeled->kind == TY_REF) peeled = ty_resolve(peeled->elem);
    if (peeled->kind == TY_PARAM)
        error_at(impl->node->file, impl->node->line,
                 "type parameter `%s` must be used as the type parameter for some local type (e.g., `MyStruct<%s>`)",
                 cl_nm(peeled->def), cl_nm(peeled->def));
    error_at(impl->node->file, impl->node->line,
             "only traits defined in the current crate can be implemented for types defined outside of the crate");
}

/* could one type be both impls' self type?  (bounds consulted, but only a
 * definite "no" separates them) */
static int cl_overlap(Sym *a, Sym *b) {
    Type **fa = cl_tvec(a->ngparams), **fb = cl_tvec(b->ngparams), *pa, *pb;
    Subst sa, sb;
    int i, ok, mark, r;
    Sym **sp = cl_cp_params;
    Type **sargs = cl_cp_args;
    int sn = cl_cp_n;
    if (!a->impl_self || !b->impl_self || a->ntrait_args != b->ntrait_args) return 0;
    for (i = 0; i < a->ngparams; i++) fa[i] = ty_infer(0);
    for (i = 0; i < b->ngparams; i++) fb[i] = ty_infer(0);
    sa.params = a->gparams; sa.args = fa; sa.n = a->ngparams; sa.self = 0;
    sb.params = b->gparams; sb.args = fb; sb.n = b->ngparams; sb.self = 0;
    mark = cl_mark();
    cl_cp_params = a->gparams;
    cl_cp_args = fa;
    cl_cp_n = a->ngparams;
    pa = ty_subst(a->impl_self, &sa);
    pb = ty_subst(b->impl_self, &sb);
    /* const parameters of both impls are only compared by position */
    ok = cl_unify(pa, pb);
    for (i = 0; ok && i < a->ntrait_args; i++)
        ok = cl_unify(ty_subst(a->trait_args[i], &sa), ty_subst(b->trait_args[i], &sb));
    cl_cp_params = sp;
    cl_cp_args = sargs;
    cl_cp_n = sn;
    for (i = 0; ok && i < a->nbounds; i++) {
        Bound *bd = &a->bounds[i];
        Type **args = cl_tvec(bd->nargs);
        int k;
        for (k = 0; k < bd->nargs; k++) args[k] = ty_subst(bd->args[k], &sa);
        r = cl_prove(ty_subst(bd->ty, &sa), bd->trait, args, bd->nargs, 1);
        if (r == 0) ok = 0;
    }
    for (i = 0; ok && i < b->nbounds; i++) {
        Bound *bd = &b->bounds[i];
        Type **args = cl_tvec(bd->nargs);
        int k;
        for (k = 0; k < bd->nargs; k++) args[k] = ty_subst(bd->args[k], &sb);
        r = cl_prove(ty_subst(bd->ty, &sb), bd->trait, args, bd->nargs, 1);
        if (r == 0) ok = 0;
    }
    cl_undo(mark);
    return ok;
}

static void cl_check_overlap(Sym *impl, int upto) {
    ClSymList *tl = &cl_tinfo[impl->trait->index];
    int i;
    if (impl->flags & SF_CORE) return;
    for (i = 0; i < tl->n; i++) {
        Sym *o = tl->v[i];
        if (o == impl) break;
        if (!cl_key_ok(cl_ikey[o->index], cl_ikey[impl->index] == -1 ? -2 : cl_ikey[impl->index]))
            continue;
        if (cl_overlap(o, impl))
            error_at(impl->node->file, impl->node->line,
                     "conflicting implementations of trait `%s` for type `%s`", cl_nm(impl->trait),
                     ty_str(impl->impl_self));
    }
    (void)upto;
}

/* ================================================================ collect_crate */

static void cl_find_lang(void) {
    int i;
    cl_lang_copy = cl_lang_clone = cl_lang_sized = cl_lang_send = cl_lang_sync = 0;
    cl_lang_fn = cl_lang_fn_mut = cl_lang_fn_once = cl_lang_box = 0;
    for (i = 0; i < cl_nsyms; i++) {
        Sym *s = cl_syms[i];
        if (!s->lang) continue;
        if (strcmp(s->lang->s, "copy") == 0) cl_lang_copy = s;
        else if (strcmp(s->lang->s, "clone") == 0) cl_lang_clone = s;
        else if (strcmp(s->lang->s, "sized") == 0) cl_lang_sized = s;
        else if (strcmp(s->lang->s, "send") == 0) cl_lang_send = s;
        else if (strcmp(s->lang->s, "sync") == 0) cl_lang_sync = s;
        else if (strcmp(s->lang->s, "fn") == 0) cl_lang_fn = s;
        else if (strcmp(s->lang->s, "fn_mut") == 0) cl_lang_fn_mut = s;
        else if (strcmp(s->lang->s, "fn_once") == 0) cl_lang_fn_once = s;
        else if (strcmp(s->lang->s, "owned_box") == 0) cl_lang_box = s;
    }
}

static void cl_mark_prims_copy(void) {
    static const char *names[] = {"bool", "char", "i8", "i16", "i32", "i64", "i128", "isize",
                                  "u8", "u16", "u32", "u64", "u128", "usize", "f32", "f64"};
    Scope *sc = crate_root ? crate_root->members : 0;
    int i;
    if (!sc) return;
    for (i = 0; i < 16; i++) {
        Sym *p = scope_lookup(sc, NS_TYPE, name_intern_str(names[i]));
        if (p && p->kind == SYM_PRIM) p->flags |= SF_COPY;
    }
}

static void cl_register_impls(void) {
    int i, nb;
    Sym *impl;
    cl_tinfo = (ClSymList *)xalloc(sizeof(ClSymList) * (size_t)(cl_ntraits + 1));
    cl_ikey = (int *)xalloc(sizeof(int) * (size_t)(resolve_nimpls + 1));
    nb = 64 + 2 * (cl_nadts > cl_ntraits ? cl_nadts : cl_ntraits) + 4;
    cl_nbuckets = nb;
    cl_head = (Sym **)xalloc(sizeof(Sym *) * (size_t)nb);
    cl_tail = (Sym **)xalloc(sizeof(Sym *) * (size_t)nb);
    for (i = 0; i < resolve_nimpls; i++) {
        int key, bucket;
        impl = resolve_impls[i];
        impl->index = i;
        impl->next_impl = 0;
        if (impl->node->flags & F_NEG) {
            cl_ikey[i] = -3;
            continue;
        }
        key = ty_resolve(impl->impl_self)->kind == TY_PARAM ? -1 : cl_ctor_key(impl->impl_self);
        cl_ikey[i] = key;
        bucket = key + 1;
        if (bucket < 0 || bucket >= nb) bucket = 0;
        if (cl_tail[bucket]) cl_tail[bucket]->next_impl = impl;
        else cl_head[bucket] = impl;
        cl_tail[bucket] = impl;
        if (impl->trait) {
            ClSymList *tl = &cl_tinfo[impl->trait->index];
            cl_sym_push(&tl->v, &tl->n, &tl->cap, impl);
        }
    }
}

void collect_crate(void) {
    int i;
    Sym *dummy;
    cl_ready = 0;
    cl_nsyms = cl_ntraits = cl_nadts = 0;
    cl_n_output = name_intern_str("Output");
    dummy = sym_new(SYM_IMPL, name_intern_str("{builtin}"), 0, 0);
    dummy->members = scope_new(0);
    collect_builtin_impl = dummy;
    cl_env = 0;

    /* 1. every item of both crates, nested items included */
    if (core_root && core_root->node) cl_scan(core_root->node);
    if (crate_root && crate_root->node) cl_scan(crate_root->node);
    cl_find_lang();

    for (i = 0; i < cl_nsyms; i++)
        if (cl_syms[i]->kind == SYM_FN) cl_desugar_impl_args(cl_syms[i]);

    /* 2. generics, and the type of every struct / enum / union */
    for (i = 0; i < cl_nsyms; i++) cl_set_gparams(cl_syms[i]);
    for (i = 0; i < cl_nsyms; i++) {
        Sym *s = cl_syms[i];
        if (s->kind == SYM_STRUCT || s->kind == SYM_ENUM || s->kind == SYM_UNION)
            s->ty = ty_adt(s, s->ngparams ? cl_identity(s) : 0, s->ngparams);
    }
    for (i = 0; i < cl_nsyms; i++) {
        Sym *s = cl_syms[i];
        int k;
        for (k = 0; k < s->ngparams; k++)
            if (s->gparams[k]->kind == SYM_GENERIC) s->gparams[k]->ty = ty_param(s->gparams[k]);
    }

    /* 3. associated type paths (T::Item, Self::Item) in every type of every body */
    if (core_root && core_root->node) cl_fix_walk(core_root->node, 0);
    if (crate_root && crate_root->node) cl_fix_walk(crate_root->node, 0);

    /* 4. impl headers, then bounds (traits and impls first: items inherit theirs) */
    for (i = 0; i < resolve_nimpls; i++) cl_lower_impl_header(resolve_impls[i]);
    for (i = 0; i < cl_nsyms; i++)
        if (cl_syms[i]->kind == SYM_TRAIT) cl_lower_bounds(cl_syms[i]);
    for (i = 0; i < cl_nsyms; i++)
        if (cl_syms[i]->kind == SYM_IMPL) cl_lower_bounds(cl_syms[i]);
    for (i = 0; i < cl_nsyms; i++) {
        Sym *s = cl_syms[i];
        if (s->kind != SYM_TRAIT && s->kind != SYM_IMPL && s->kind != SYM_MOD) cl_lower_bounds(s);
    }

    /* 5. signatures, field types, constants, aliases */
    for (i = 0; i < cl_nsyms; i++) cl_lower_sig(cl_syms[i]);

    /* 6. impl tables */
    cl_register_impls();
    cl_ready = 1;
    cl_mark_prims_copy();

    /* 7. impl items against their trait, then the semantic checks */
    for (i = 0; i < resolve_nimpls; i++) {
        Sym *impl = resolve_impls[i];
        if (impl->node->flags & F_NEG) continue;
        if (impl->trait) cl_match_items(impl);
    }
    for (i = 0; i < resolve_nimpls; i++) {
        Sym *impl = resolve_impls[i];
        if (impl->node->flags & F_NEG) continue;
        cl_check_orphan(impl);
        cl_check_constrained(impl);
        if (!impl->trait) continue;
        if (impl->trait == cl_lang_copy) cl_check_copy(impl);
        cl_check_supers(impl);
        cl_check_item_compat(impl);
        cl_check_overlap(impl, i);
    }
    collect_set_env(0);
}

/* ================================================================ dump */

static void cl_puts(FILE *f, const char *s) {
#ifdef __CPUSIM64__
    while (*s) fputc(*s++, f);
#else
    fputs(s, f);
#endif
}

static void cl_generics_text(StrBuf *sb, Sym *s) {
    int from = 0, k, first = 1;
    if (cl_is_assoc(s)) from = s->parent->ngparams;
    for (k = from; k < s->ngparams; k++) {
        Sym *g = s->gparams[k];
        sb_puts(sb, first ? "<" : ", ");
        first = 0;
        if (g->kind == SYM_CONST_GENERIC) {
            sb_puts(sb, "const ");
            sb_puts(sb, cl_nm(g));
            sb_puts(sb, ": ");
            sb_puts(sb, g->ty ? ty_str(g->ty) : "?");
        } else {
            sb_puts(sb, cl_nm(g));
        }
    }
    if (!first) sb_putc(sb, '>');
}

static void cl_bound_text(StrBuf *sb, Bound *b) {
    int i;
    cl_type_text(sb, b->ty);
    sb_puts(sb, ": ");
    sb_puts(sb, cl_nm(b->trait));
    if (b->nargs > 0 || b->assoc_name) {
        sb_putc(sb, '<');
        for (i = 0; i < b->nargs; i++) {
            if (i) sb_puts(sb, ", ");
            cl_type_text(sb, b->args[i]);
        }
        if (b->assoc_name) {
            if (b->nargs) sb_puts(sb, ", ");
            sb_puts(sb, b->assoc_name->s);
            sb_puts(sb, " = ");
            cl_type_text(sb, b->assoc_ty);
        }
        sb_putc(sb, '>');
    }
}

static void cl_where_sep(StrBuf *sb, int *first, int indent) {
    int k;
    if (*first) {
        for (k = 0; k <= indent; k++) sb_puts(sb, "  ");
        sb_puts(sb, "where ");
    } else {
        sb_puts(sb, ", ");
    }
    *first = 0;
}

static void cl_dump_where(FILE *f, Sym *s, int indent) {
    int from = cl_is_assoc(s) ? s->parent->nbounds : 0, k, first = 1, g;
    StrBuf sb;
    sb_init(&sb);
    for (g = 0; g < s->ngparams; g++) {
        Sym *p = s->gparams[g];
        if (p->kind == SYM_GENERIC && (p->flags & CL_UNSIZED) && !(cl_is_assoc(s) && g < s->parent->ngparams)) {
            cl_where_sep(&sb, &first, indent);
            sb_puts(&sb, cl_nm(p));
            sb_puts(&sb, ": ?Sized");
        }
    }
    for (k = from; k < s->nbounds; k++) {
        cl_where_sep(&sb, &first, indent);
        cl_bound_text(&sb, &s->bounds[k]);
    }
    if (!first) {
        cl_puts(f, sb.s);
        fputc('\n', f);
    }
    free(sb.s);
}

static void cl_dump_item(FILE *f, Node *n) {
    Sym *s = (Sym *)n->sym;
    StrBuf sb;
    int indent = 0, k;
    Sym *p;
    if (!s || s->node != n) return;
    for (p = s->parent; p && p->kind != SYM_MOD; p = p->parent) indent++;
    sb_init(&sb);
    for (k = 0; k < indent; k++) sb_puts(&sb, "  ");
    switch (s->kind) {
    case SYM_MOD:
        sb_puts(&sb, "mod ");
        sym_path(s, &sb);
        break;
    case SYM_STRUCT: case SYM_UNION: case SYM_ENUM:
        sb_puts(&sb, s->kind == SYM_STRUCT ? "struct " : s->kind == SYM_UNION ? "union " : "enum ");
        sym_path(s, &sb);
        cl_generics_text(&sb, s);
        sb_puts(&sb, ": ");
        sb_puts(&sb, s->ty ? ty_str(s->ty) : "?");
        break;
    case SYM_TRAIT:
        sb_puts(&sb, "trait ");
        sym_path(s, &sb);
        cl_generics_text(&sb, s);
        break;
    case SYM_IMPL: {
        int a;
        sb_puts(&sb, "impl");
        cl_generics_text(&sb, s);
        sb_putc(&sb, ' ');
        if (s->trait) {
            sb_puts(&sb, cl_trait_text(s->trait, s->trait_args, s->ntrait_args));
            sb_puts(&sb, " for ");
        }
        sb_puts(&sb, s->impl_self ? ty_str(s->impl_self) : "?");
        if (s->flags & SF_COPY) sb_puts(&sb, "  [copy]");
        (void)a;
        break;
    }
    case SYM_FN:
        sb_puts(&sb, "fn ");
        sym_path(s, &sb);
        cl_generics_text(&sb, s);
        sb_puts(&sb, ": ");
        sb_puts(&sb, s->ty ? ty_str(s->ty) : "?");
        break;
    case SYM_CONST: case SYM_STATIC:
        sb_puts(&sb, s->kind == SYM_CONST ? "const " : "static ");
        sym_path(s, &sb);
        sb_puts(&sb, ": ");
        sb_puts(&sb, s->ty ? ty_str(s->ty) : "?");
        break;
    case SYM_TYPEALIAS: case SYM_ASSOC_TYPE:
        sb_puts(&sb, "type ");
        sym_path(s, &sb);
        cl_generics_text(&sb, s);
        if (s->ty) {
            sb_puts(&sb, " = ");
            sb_puts(&sb, ty_str(s->ty));
        }
        break;
    default:
        free(sb.s);
        return;
    }
    if (s->trait_item) {
        sb_puts(&sb, "  (impl of ");
        sym_path(s->trait_item, &sb);
        sb_putc(&sb, ')');
    }
    cl_puts(f, sb.s);
    fputc('\n', f);
    free(sb.s);
    if (s->kind != SYM_MOD) cl_dump_where(f, s, indent);
    if (s->kind == SYM_STRUCT || s->kind == SYM_UNION || s->kind == SYM_ENUM) {
        int i, j, cnt = scope_member_count(s->members);
        for (i = 0; i < cnt; i++) {
            Sym *m = scope_member(s->members, i);
            StrBuf lb;
            sb_init(&lb);
            for (k = 0; k <= indent; k++) sb_puts(&lb, "  ");
            if (s->kind == SYM_ENUM) {
                int fc = scope_member_count(m->members);
                sb_puts(&lb, "variant ");
                sym_path(m, &lb);
                cl_puts(f, lb.s);
                fputc('\n', f);
                for (j = 0; j < fc; j++) {
                    Sym *fl = scope_member(m->members, j);
                    StrBuf fb;
                    sb_init(&fb);
                    for (k = 0; k <= indent + 1; k++) sb_puts(&fb, "  ");
                    sb_puts(&fb, "field ");
                    sym_path(fl, &fb);
                    sb_puts(&fb, ": ");
                    sb_puts(&fb, fl->ty ? ty_str(fl->ty) : "?");
                    cl_puts(f, fb.s);
                    fputc('\n', f);
                    free(fb.s);
                }
            } else {
                sb_puts(&lb, "field ");
                sym_path(m, &lb);
                sb_puts(&lb, ": ");
                sb_puts(&lb, m->ty ? ty_str(m->ty) : "?");
                cl_puts(f, lb.s);
                fputc('\n', f);
            }
            free(lb.s);
        }
    }
}

static void cl_dump_walk(FILE *f, Node *n) {
    long i;
    if (!n) return;
    if (cl_item_kind(n->kind)) cl_dump_item(f, n);
    cl_dump_walk(f, n->a);
    cl_dump_walk(f, n->b);
    cl_dump_walk(f, n->c);
    cl_dump_walk(f, n->d);
    for (i = 0; i < n->list.n; i++) cl_dump_walk(f, n->list.v[i]);
}

/* prints every item of the user crate (and of core when include_core) with
 * its collected type, generics and bounds, impls with their trait */
void collect_dump(FILE *f, int include_core) {
    if (include_core && core_root && core_root->node) cl_dump_walk(f, core_root->node);
    if (crate_root && crate_root->node) cl_dump_walk(f, crate_root->node);
}
