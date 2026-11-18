/*
 * moves.c - the move checker (see sema.h, stage 5).
 *
 * moves_crate() walks every fn body of the USER crate (core.rs is skipped)
 * after check_crate() and reports, with rustc's wording and at the line of
 * the offending use:
 *
 *   use of moved value: `x`            (also "borrow of moved value",
 *                                       "use of partially moved value")
 *   cannot move out of `*r` which is behind a shared reference
 *   cannot move out of index of `Vec<String>`
 *   cannot assign twice to immutable variable `x`
 *   cannot borrow `x` as mutable, as it is not declared as mutable
 *   cannot assign to `*r`, which is behind a `&` reference
 *   used binding `x` isn't initialized
 *
 * It is a flow-sensitive analysis over places: a place is a local plus a path
 * of fields (and Box derefs).  The state at a program point is
 *     u[local]   maybe uninitialised
 *     iv[local]  maybe initialised (ever assigned; for immutability)
 *     mv         the set of (maybe) moved places, as strings "local.f.g.*"
 * Branches merge by union, loops iterate to a fixpoint (so a move inside a
 * loop is seen by the next iteration), code after return / break / continue /
 * a call of type `!` is dead and is not checked.  Types, adjustments and
 * closure captures come from check.c.  References (even `&mut`) are never
 * moved: a by-value use of one is a copy or a reborrow.  There are no
 * lifetime or aliasing checks.
 */

typedef struct MvState {
    int dead;
    char *u;                 /* per local: maybe uninitialised */
    char *iv;                /* per local: maybe initialised */
    char **mv;               /* moved places */
    int nm, cm;
} MvState;

typedef struct MvPlace {
    int root;                /* local index, or -1 for an rvalue / static */
    char key[400];           /* "idx.a.b.*" */
    int frozen;              /* the path stopped at a deref of a reference / index */
    int mutable_;            /* the place can be written */
    int shared;              /* reached through a `&` reference */
    int bar;                 /* 0 or why it cannot be moved out of (MVB_xxx) */
    Type *barty;
    int nsteps;              /* fields / derefs applied to the local */
} MvPlace;

enum { MVB_SHARED = 1, MVB_MUTREF, MVB_INDEX, MVB_PTR, MVB_OTHER };
enum { MV_VALUE = 1, MV_BORROW, MV_BORROWMUT };

typedef struct MvLoop {
    Name *label;
    int isblock;
    MvState *brk;
    MvState *cont;
} MvLoop;

static Sym **mv_lsym;
static char *mv_lparam;
static int mv_nloc, mv_lcap;
static int *mv_lhead, *mv_lnext;
static MvState *mv_cur;
static MvLoop *mv_loops;
static int mv_nloops, mv_caploops, mv_loop_base;
static Node **mv_pend;
static int mv_npend, mv_cpend;

#define MV_HASH 1024

/* ------------------------------------------------------------ locals */

static int mv_find(Sym *s) {
    int i = mv_lhead[(int)(((size_t)s >> 4) & (MV_HASH - 1))];
    while (i >= 0) {
        if (mv_lsym[i] == s) return i;
        i = mv_lnext[i];
    }
    return -1;
}

static int mv_reg(Sym *s, int param) {
    int i = mv_find(s), h;
    if (i >= 0) {
        if (param) mv_lparam[i] = 1;
        return i;
    }
    if (mv_nloc >= mv_lcap) {
        int nc = mv_lcap ? mv_lcap * 2 : 64;
        mv_lsym = (Sym **)xrealloc(mv_lsym, sizeof(Sym *) * (size_t)nc);
        mv_lparam = (char *)xrealloc(mv_lparam, (size_t)nc);
        mv_lnext = (int *)xrealloc(mv_lnext, sizeof(int) * (size_t)nc);
        mv_lcap = nc;
    }
    i = mv_nloc++;
    mv_lsym[i] = s;
    mv_lparam[i] = (char)param;
    h = (int)(((size_t)s >> 4) & (MV_HASH - 1));
    mv_lnext[i] = mv_lhead[h];
    mv_lhead[h] = i;
    return i;
}

static int mv_is_item(int k) {
    switch (k) {
    case N_ITEM_FN: case N_ITEM_STRUCT: case N_ITEM_ENUM: case N_ITEM_UNION: case N_ITEM_TRAIT:
    case N_ITEM_IMPL: case N_ITEM_MOD: case N_ITEM_CONST: case N_ITEM_STATIC: case N_ITEM_TYPE:
    case N_ITEM_EXTERN_BLOCK: case N_ITEM_USE: case N_ITEM_MACRO_RULES: case N_ITEM_EXTERN_CRATE:
        return 1;
    }
    return 0;
}

static void mv_queue(Node *n) {
    if (mv_npend >= mv_cpend) {
        mv_cpend = mv_cpend ? mv_cpend * 2 : 16;
        mv_pend = (Node **)xrealloc(mv_pend, sizeof(Node *) * (size_t)mv_cpend);
    }
    mv_pend[mv_npend++] = n;
}

/* registers every binding below n (not descending into items) */
static void mv_prepass(Node *n, int param) {
    long i;
    if (!n) return;
    if (mv_is_item(n->kind)) {
        if (n->kind == N_ITEM_FN && n->c) mv_queue(n);
        return;
    }
    if (n->kind == N_P_IDENT) {
        Sym *s = (Sym *)n->sym;
        if (s && s->kind == SYM_LOCAL) mv_reg(s, param);
    } else if (n->kind == N_SELF_PARAM) {
        Sym *s = (Sym *)n->sym;
        if (s && s->kind == SYM_LOCAL) mv_reg(s, 1);
    }
    mv_prepass(n->a, param);
    mv_prepass(n->b, 0);
    mv_prepass(n->c, 0);
    mv_prepass(n->d, 0);
    for (i = 0; i < n->list.n; i++) mv_prepass(n->list.v[i], param && n->kind == N_PARAM ? param : 0);
}

/* ------------------------------------------------------------ states */

static MvState *mv_new(int dead) {
    MvState *s = (MvState *)ast_alloc(sizeof(MvState));
    s->u = (char *)ast_alloc((size_t)mv_nloc + 1);
    s->iv = (char *)ast_alloc((size_t)mv_nloc + 1);
    s->dead = dead;
    return s;
}

static MvState *mv_copy(MvState *a) {
    MvState *s = mv_new(a->dead);
    memcpy(s->u, a->u, (size_t)mv_nloc);
    memcpy(s->iv, a->iv, (size_t)mv_nloc);
    if (a->nm > 0) {
        s->mv = (char **)ast_alloc(sizeof(char *) * (size_t)a->nm);
        memcpy(s->mv, a->mv, sizeof(char *) * (size_t)a->nm);
        s->nm = s->cm = a->nm;
    }
    return s;
}

static int mv_has(MvState *s, const char *k) {
    int i;
    for (i = 0; i < s->nm; i++)
        if (strcmp(s->mv[i], k) == 0) return 1;
    return 0;
}

static void mv_addmove(MvState *s, const char *k) {
    char *d;
    if (mv_has(s, k)) return;
    if (s->nm >= s->cm) {
        int nc = s->cm ? s->cm * 2 : 4;
        char **nv = (char **)ast_alloc(sizeof(char *) * (size_t)nc);
        if (s->nm) memcpy(nv, s->mv, sizeof(char *) * (size_t)s->nm);
        s->mv = nv;
        s->cm = nc;
    }
    d = (char *)ast_alloc(strlen(k) + 1);
    strcpy(d, k);
    s->mv[s->nm++] = d;
}

/* dst |= src; returns 1 if dst changed */
static int mv_merge(MvState *dst, MvState *src) {
    int i, ch = 0;
    if (src->dead) return 0;
    if (dst->dead) {
        MvState *c = mv_copy(src);
        *dst = *c;
        return 1;
    }
    for (i = 0; i < mv_nloc; i++) {
        if (src->u[i] && !dst->u[i]) { dst->u[i] = 1; ch = 1; }
        if (src->iv[i] && !dst->iv[i]) { dst->iv[i] = 1; ch = 1; }
    }
    for (i = 0; i < src->nm; i++)
        if (!mv_has(dst, src->mv[i])) {
            mv_addmove(dst, src->mv[i]);
            ch = 1;
        }
    return ch;
}

/* a is a prefix of b at a component boundary */
static int mv_kpre(const char *a, const char *b) {
    size_t n = strlen(a);
    return strncmp(a, b, n) == 0 && (b[n] == 0 || b[n] == '.');
}

/* forget every moved place below (or at) key */
static void mv_clear(MvState *s, const char *key) {
    int i, k = 0;
    for (i = 0; i < s->nm; i++)
        if (!mv_kpre(key, s->mv[i])) s->mv[k++] = s->mv[i];
    s->nm = k;
}

static void mv_declare_local(int i, int has_init) {
    char key[24];
    if (i < 0 || mv_cur->dead) return;
    sprintf(key, "%d", i);
    mv_clear(mv_cur, key);
    mv_cur->u[i] = (char)(has_init ? 0 : 1);
    mv_cur->iv[i] = (char)(has_init ? 1 : 0);
}

static void mv_declare_pat(Node *p, int has_init) {
    long i;
    if (!p) return;
    if (p->kind == N_P_IDENT) {
        Sym *s = (Sym *)p->sym;
        if (s && s->kind == SYM_LOCAL) mv_declare_local(mv_find(s), has_init);
    }
    mv_declare_pat(p->a, has_init);
    for (i = 0; i < p->list.n; i++) mv_declare_pat(p->list.v[i], has_init);
}

/* ------------------------------------------------------------ types */

static int mv_is_box(Type *t) {
    return t && t->kind == TY_ADT && t->def && t->def->lang && strcmp(t->def->lang->s, "owned_box") == 0;
}

/* a by-value use of a value of this type does not move it */
static int mv_copyish(Type *t) {
    if (!t) return 1;
    t = ty_resolve(t);
    if (!t) return 1;
    switch (t->kind) {
    case TY_ERR: case TY_REF: case TY_NEVER: case TY_PTR: case TY_FN_DEF: case TY_FN_PTR:
        return 1;
    case TY_INFER:
        return 1;
    }
    return is_copy(t);
}

static int mv_never(Node *e) {
    Type *t = e->ty ? ty_resolve((Type *)e->ty) : 0;
    return t && t->kind == TY_NEVER;
}

/* ------------------------------------------------------------ messages */

static int mv_hidden(int idx) {
    Name *n;
    if (idx < 0) return 1;
    n = mv_lsym[idx]->name;
    return n && n->s && n->s[0] == '_' && n->s[1] == '_';
}

static const char *mv_lname(int idx) {
    Name *n = mv_lsym[idx]->name;
    return n && n->s ? n->s : "?";
}

static void mv_desc(Node *e, char *buf, size_t cap) {
    char tmp[200];
    size_t l;
    if (!e) {
        strcpy(buf, "_");
        return;
    }
    switch (e->kind) {
    case N_E_PAREN:
        mv_desc(e->a, buf, cap);
        return;
    case N_E_PATH:
        if (e->a && e->a->list.n > 0 && e->a->list.v[e->a->list.n - 1]->name)
            snprintf(buf, cap, "%s", e->a->list.v[e->a->list.n - 1]->name->s);
        else
            strcpy(buf, "_");
        return;
    case N_E_FIELD:
        mv_desc(e->a, tmp, sizeof tmp);
        snprintf(buf, cap, "%s.%s", tmp, e->name ? e->name->s : "?");
        return;
    case N_E_TUPLE_FIELD:
        mv_desc(e->a, tmp, sizeof tmp);
        snprintf(buf, cap, "%s.%d", tmp, (int)e->ival);
        return;
    case N_E_DEREF:
        mv_desc(e->a, tmp, sizeof tmp);
        snprintf(buf, cap, "*%s", tmp);
        return;
    case N_E_INDEX:
        mv_desc(e->a, tmp, sizeof tmp);
        snprintf(buf, cap, "%s[..]", tmp);
        return;
    }
    l = 5;
    memcpy(buf, "value", l + 1);
}

/* ------------------------------------------------------------ places */

static int mv_is_place(Node *e) {
    if (!e) return 0;
    switch (e->kind) {
    case N_E_PAREN:
        return mv_is_place(e->a);
    case N_E_PATH: {
        Sym *s = (Sym *)e->sym;
        return s && s->kind == SYM_LOCAL;
    }
    case N_E_FIELD: case N_E_TUPLE_FIELD: case N_E_DEREF: case N_E_INDEX:
        return 1;
    }
    return 0;
}

static void mv_ex(Node *e);
static void mv_block(Node *b);
static MvState *mv_cond(Node *e);

static void mv_append(MvPlace *p, const char *comp) {
    size_t n;
    if (p->frozen) return;
    n = strlen(p->key);
    if (n + strlen(comp) + 2 >= sizeof p->key) {
        p->frozen = 1;
        return;
    }
    p->key[n] = '.';
    strcpy(p->key + n + 1, comp);
}

/* one deref of a value of type t; returns the pointee type or 0 if unknown */
static Type *mv_deref1(MvPlace *p, Type *t, int overloaded) {
    t = t ? ty_resolve(t) : 0;
    p->nsteps++;
    if (!t) {
        p->frozen = 1;
        return 0;
    }
    if (t->kind == TY_REF) {
        if (!p->bar) {
            p->bar = t->mut ? MVB_MUTREF : MVB_SHARED;
            p->barty = t;
        }
        p->frozen = 1;
        if (!t->mut) {
            p->shared = 1;
            p->mutable_ = 0;
        } else if (!p->shared)
            p->mutable_ = 1;
        return t->elem;
    }
    if (mv_is_box(t) && !overloaded) {
        mv_append(p, "*");
        return t->nargs > 0 ? t->args[0] : 0;
    }
    if (t->kind == TY_PTR) {
        if (!p->bar) {
            p->bar = MVB_PTR;
            p->barty = t;
        }
        p->frozen = 1;
        p->mutable_ = 1;
        return t->elem;
    }
    if (!p->bar) {
        p->bar = MVB_OTHER;
        p->barty = t;
    }
    p->frozen = 1;
    return 0;
}

/* applies the auto-derefs recorded on e; returns the type afterwards */
static Type *mv_derefs(Node *e, MvPlace *p) {
    Type *t = e->ty ? (Type *)e->ty : 0;
    int i, n = e->adj & 0xFF;
    for (i = 0; i < n; i++) t = mv_deref1(p, t, 0);
    return t;
}

static void mv_walk(Node *e, MvPlace *p) {
    switch (e->kind) {
    case N_E_PAREN:
        mv_walk(e->a, p);
        return;
    case N_E_PATH: {
        Sym *s = (Sym *)e->sym;
        int i = (s && s->kind == SYM_LOCAL) ? mv_find(s) : -1;
        if (i >= 0) {
            p->root = i;
            sprintf(p->key, "%d", i);
            p->mutable_ = (s->flags & SF_MUT) != 0;
        } else {
            p->root = -1;
            p->frozen = 1;
            p->mutable_ = 1;
        }
        return;
    }
    case N_E_FIELD: case N_E_TUPLE_FIELD: {
        char comp[40];
        mv_walk(e->a, p);
        mv_derefs(e->a, p);
        p->nsteps++;
        if (e->kind == N_E_FIELD) mv_append(p, e->name ? e->name->s : "?");
        else {
            sprintf(comp, "%d", (int)e->ival);
            mv_append(p, comp);
        }
        return;
    }
    case N_E_DEREF:
        mv_walk(e->a, p);
        mv_deref1(p, (Type *)e->a->ty, e->sym != 0);
        return;
    case N_E_INDEX: {
        Type *t;
        mv_walk(e->a, p);
        t = mv_derefs(e->a, p);
        mv_ex(e->b);
        if (!p->bar) {
            p->bar = MVB_INDEX;
            p->barty = t;
        }
        p->frozen = 1;
        p->nsteps++;
        return;
    }
    }
    mv_ex(e);
    p->root = -1;
    p->frozen = 1;
    p->mutable_ = 1;
}

static void mv_place_init(MvPlace *p) {
    memset(p, 0, sizeof *p);
    p->root = -1;
    p->mutable_ = 1;
}

static void mv_bar_error(MvPlace *p, Node *e, Node *at, const char *variant) {
    char d[200];
    const char *ts;
    mv_desc(e, d, sizeof d);
    if (p->barty && p->bar != MVB_SHARED && p->bar != MVB_MUTREF) ts = ty_str(p->barty);
    else ts = "";
    switch (p->bar) {
    case MVB_SHARED:
        if (variant) error_at(at->file, at->line, "cannot move out of `%s` as enum variant `%s` which is behind a shared reference", d, variant);
        error_at(at->file, at->line, "cannot move out of `%s` which is behind a shared reference", d);
        break;
    case MVB_MUTREF:
        if (variant) error_at(at->file, at->line, "cannot move out of `%s` as enum variant `%s` which is behind a mutable reference", d, variant);
        error_at(at->file, at->line, "cannot move out of `%s` which is behind a mutable reference", d);
        break;
    case MVB_INDEX:
        error_at(at->file, at->line, "cannot move out of index of `%s`", ts);
        break;
    case MVB_PTR:
        error_at(at->file, at->line, "cannot move out of `%s` which is behind a raw pointer", d);
        break;
    default:
        error_at(at->file, at->line, "cannot move out of dereference of `%s`", ts);
        break;
    }
}

/* the place is read / borrowed: it must be initialised and not (partly) moved */
static void mv_check_use(MvPlace *p, Node *e, const char *verb) {
    int i;
    char d[200];
    if (mv_cur->dead || p->root < 0 || mv_hidden(p->root)) return;
    if (mv_cur->u[p->root]) {
        if (mv_cur->iv[p->root])
            error_at(e->file, e->line, "used binding `%s` is possibly-uninitialized", mv_lname(p->root));
        error_at(e->file, e->line, "used binding `%s` isn't initialized", mv_lname(p->root));
    }
    for (i = 0; i < mv_cur->nm; i++) {
        const char *m = mv_cur->mv[i];
        if (mv_kpre(m, p->key)) {
            mv_desc(e, d, sizeof d);
            error_at(e->file, e->line, "%s of moved value: `%s`", verb, d);
        }
        if (mv_kpre(p->key, m)) {
            mv_desc(e, d, sizeof d);
            error_at(e->file, e->line, "%s of partially moved value: `%s`", verb, d);
        }
    }
}

/* the place is mutably borrowed (assign = 0) or assigned to (assign = 1) */
static void mv_check_mut(MvPlace *p, Node *e, int assign) {
    char d[200], d2[210];
    int nd, i;
    if (mv_cur->dead || p->root < 0 || mv_hidden(p->root)) return;
    mv_desc(e, d, sizeof d);
    nd = e->adj & 0xFF;
    if (!assign && nd > 0 && nd < 3) {
        d2[0] = 0;
        for (i = 0; i < nd; i++) strcat(d2, "*");
        strcat(d2, d);
        strcpy(d, d2);
    }
    if (p->shared) {
        if (assign) error_at(e->file, e->line, "cannot assign to `%s`, which is behind a `&` reference", d);
        error_at(e->file, e->line, "cannot borrow `%s` as mutable, as it is behind a `&` reference", d);
    }
    if (!p->mutable_) {
        const char *r = mv_lname(p->root);
        if (assign) error_at(e->file, e->line, "cannot assign to `%s`, as `%s` is not declared as mutable", d, r);
        if (strcmp(d, r) == 0)
            error_at(e->file, e->line, "cannot borrow `%s` as mutable, as it is not declared as mutable", d);
        error_at(e->file, e->line, "cannot borrow `%s` as mutable, as `%s` is not declared as mutable", d, r);
    }
}

/* e is a place expression used with the given access */
static void mv_access(Node *e, int acc) {
    MvPlace p;
    int adj = e->adj;
    if (mv_cur->dead) return;
    mv_place_init(&p);
    mv_walk(e, &p);
    if (mv_cur->dead) return;
    if (acc == MV_VALUE && (adj & (0xFF | ADJ_AUTOREF | ADJ_AUTOREF_MUT)))
        acc = (adj & ADJ_AUTOREF_MUT) ? MV_BORROWMUT : MV_BORROW;
    if (adj & 0xFF) mv_derefs(e, &p);
    if (acc == MV_VALUE) {
        if (mv_copyish((Type *)e->ty)) mv_check_use(&p, e, "use");
        else {
            if (p.bar) mv_bar_error(&p, e, e, 0);
            mv_check_use(&p, e, "use");
            if (p.root >= 0 && !mv_hidden(p.root)) mv_addmove(mv_cur, p.key);
        }
    } else {
        mv_check_use(&p, e, "borrow");
        if (acc == MV_BORROWMUT) mv_check_mut(&p, e, 0);
    }
}

/* operand of & / &mut / comparison: borrowed if a place, else evaluated */
static void mv_operand_ref(Node *e, int mut) {
    if (mv_is_place(e)) {
        Node *saved_adj_holder = e;
        int adj = saved_adj_holder->adj;
        saved_adj_holder->adj = 0;
        mv_access(e, mut ? MV_BORROWMUT : MV_BORROW);
        saved_adj_holder->adj = adj;
    } else
        mv_ex(e);
}

/* ------------------------------------------------------------ assignment */

static void mv_assign(Node *lhs) {
    MvPlace p;
    int i;
    if (mv_cur->dead) return;
    if (lhs->kind == N_E_PAREN) {
        mv_assign(lhs->a);
        return;
    }
    if (lhs->kind == N_E_TUPLE) {
        for (i = 0; i < lhs->list.n; i++) mv_assign(lhs->list.v[i]);
        return;
    }
    if (lhs->kind == N_E_UNDERSCORE) return;
    if (!mv_is_place(lhs)) {
        mv_ex(lhs);
        return;
    }
    mv_place_init(&p);
    mv_walk(lhs, &p);
    if (mv_cur->dead || p.root < 0 || mv_hidden(p.root)) return;
    if (p.nsteps == 0) {
        Sym *s = mv_lsym[p.root];
        if (!(s->flags & SF_MUT) && mv_cur->iv[p.root]) {
            if (mv_lparam[p.root])
                error_at(lhs->file, lhs->line, "cannot assign to immutable argument `%s`", mv_lname(p.root));
            error_at(lhs->file, lhs->line, "cannot assign twice to immutable variable `%s`", mv_lname(p.root));
        }
        mv_clear(mv_cur, p.key);
        mv_cur->u[p.root] = 0;
        mv_cur->iv[p.root] = 1;
        return;
    }
    for (i = 0; i < mv_cur->nm; i++) {
        const char *m = mv_cur->mv[i];
        if (mv_kpre(m, p.key) && strcmp(m, p.key) != 0) {
            char d[200];
            mv_desc(lhs, d, sizeof d);
            error_at(lhs->file, lhs->line, "assign to part of moved value: `%s`", d);
        }
    }
    mv_check_mut(&p, lhs, 1);
    if (!p.frozen) mv_clear(mv_cur, p.key);
}

static int mv_prim(Type *t) {
    t = t ? ty_resolve(t) : 0;
    if (!t) return 1;
    switch (t->kind) {
    case TY_BOOL: case TY_CHAR: case TY_INT: case TY_FLOAT: case TY_ERR: case TY_PTR:
        return 1;
    case TY_INFER:
        return t->ik == INFER_INT || t->ik == INFER_FLOAT;
    }
    return 0;
}

static void mv_assign_op(Node *e) {
    MvPlace p;
    mv_ex(e->b);
    if (mv_cur->dead) return;
    if (!mv_is_place(e->a)) {
        mv_ex(e->a);
        return;
    }
    if (!mv_prim(e->a->ty)) {
        mv_operand_ref(e->a, 1);
        return;
    }
    mv_place_init(&p);
    mv_walk(e->a, &p);
    if (mv_cur->dead) return;
    mv_check_use(&p, e->a, "use");
    if (p.root < 0 || mv_hidden(p.root)) return;
    if (p.nsteps == 0) {
        if (!(mv_lsym[p.root]->flags & SF_MUT)) {
            if (mv_lparam[p.root])
                error_at(e->line ? e->file : e->file, e->line, "cannot assign to immutable argument `%s`", mv_lname(p.root));
            error_at(e->file, e->line, "cannot assign twice to immutable variable `%s`", mv_lname(p.root));
        }
        return;
    }
    mv_check_mut(&p, e->a, 1);
}

/* ------------------------------------------------------------ patterns */

static void mv_bm(Node *p, const char *path, Node *scrut, MvPlace *pl, const char *epath, const char *vname) {
    long i;
    char np[400];
    if (!p) return;
    if (p->adj & 0xFF) return;
    switch (p->kind) {
    case N_P_IDENT: {
        Sym *s = (Sym *)p->sym;
        if (s && s->kind == SYM_LOCAL && !(p->flags & F_REF) &&
            !(p->adj & (ADJ_AUTOREF | ADJ_AUTOREF_MUT)) && !mv_copyish((Type *)s->ty)) {
            if (pl->bar) mv_bar_error(pl, scrut, p, vname);
            else if (pl->root >= 0 && !mv_hidden(pl->root) && !mv_cur->dead) {
                if (epath) {
                    snprintf(np, sizeof np, "%s.(%s)", epath, vname ? vname : "?");
                    mv_addmove(mv_cur, np);
                } else
                    mv_addmove(mv_cur, path);
            }
        }
        mv_bm(p->a, path, scrut, pl, epath, vname);
        return;
    }
    case N_P_OR:
        for (i = 0; i < p->list.n; i++) mv_bm(p->list.v[i], path, scrut, pl, epath, vname);
        return;
    case N_P_TUPLE: {
        Type *t = p->ty ? ty_resolve((Type *)p->ty) : 0;
        long n = p->list.n, r = -1, total;
        for (i = 0; i < n; i++)
            if (p->list.v[i]->kind == N_P_REST) r = i;
        total = (t && t->kind == TY_TUPLE) ? t->nargs : n;
        for (i = 0; i < n; i++) {
            long ix;
            if (p->list.v[i]->kind == N_P_REST) continue;
            ix = (r >= 0 && i > r) ? total - (n - i) : i;
            snprintf(np, sizeof np, "%s.%ld", path, ix);
            mv_bm(p->list.v[i], np, scrut, pl, epath, vname);
        }
        return;
    }
    case N_P_TUPLE_STRUCT: {
        Sym *s = (Sym *)p->sym;
        long n = p->list.n, r = -1, total;
        int isenum = s && s->kind == SYM_VARIANT;
        const char *vn = isenum ? (epath ? vname : (s->name ? s->name->s : "?")) : vname;
        const char *ep = isenum && !epath ? path : epath;
        for (i = 0; i < n; i++)
            if (p->list.v[i]->kind == N_P_REST) r = i;
        total = (s && s->members) ? scope_member_count(s->members) : n;
        for (i = 0; i < n; i++) {
            long ix;
            if (p->list.v[i]->kind == N_P_REST) continue;
            ix = (r >= 0 && i > r) ? total - (n - i) : i;
            snprintf(np, sizeof np, "%s.%ld", path, ix);
            mv_bm(p->list.v[i], isenum ? path : np, scrut, pl, ep, vn);
        }
        return;
    }
    case N_P_STRUCT: {
        Sym *s = (Sym *)p->sym;
        int isenum = s && s->kind == SYM_VARIANT;
        const char *vn = isenum ? (epath ? vname : (s->name ? s->name->s : "?")) : vname;
        const char *ep = isenum && !epath ? path : epath;
        for (i = 0; i < p->list.n; i++) {
            Node *f = p->list.v[i];
            if (f->kind != N_P_FIELD) continue;
            snprintf(np, sizeof np, "%s.%s", path, f->name ? f->name->s : "?");
            mv_bm(f->a, isenum ? path : np, scrut, pl, ep, vn);
        }
        return;
    }
    }
}

/* evaluates a scrutinee; returns 1 and fills pl if it is a place */
static int mv_scrut(Node *s, MvPlace *pl) {
    mv_place_init(pl);
    if (mv_is_place(s)) {
        mv_walk(s, pl);
        if (!mv_cur->dead) mv_check_use(pl, s, "use");
        return 1;
    }
    mv_ex(s);
    return 0;
}

static void mv_pat_moves(Node *pat, Node *scrut, MvPlace *pl) {
    if (!pat || mv_cur->dead) return;
    mv_bm(pat, pl->key, scrut, pl, 0, 0);
}

/* ------------------------------------------------------------ conditions */

/* evaluates the condition on the current state, which becomes the state when
   it is true; returns the state when it is false */
static MvState *mv_cond(Node *e) {
    MvState *f, *f2, *t1;
    if (mv_cur->dead) return mv_copy(mv_cur);
    switch (e->kind) {
    case N_E_PAREN:
        return mv_cond(e->a);
    case N_E_BINARY:
        if (e->op == OP_LAND) {
            f = mv_cond(e->a);
            f2 = mv_cond(e->b);
            mv_merge(f, f2);
            return f;
        }
        if (e->op == OP_LOR) {
            f = mv_cond(e->a);
            t1 = mv_cur;
            mv_cur = f;
            f2 = mv_cond(e->b);
            mv_merge(t1, mv_cur);
            mv_cur = t1;
            return f2;
        }
        break;
    case N_E_UNARY:
        if (e->op == OP_NOT) {
            f = mv_cond(e->a);
            t1 = mv_cur;
            mv_cur = f;
            return t1;
        }
        break;
    case N_E_LET: {
        MvPlace pl;
        int isp = mv_scrut(e->b, &pl);
        f = mv_copy(mv_cur);
        mv_declare_pat(e->a, 1);
        if (isp) mv_pat_moves(e->a, e->b, &pl);
        return f;
    }
    }
    mv_ex(e);
    return mv_copy(mv_cur);
}

/* ------------------------------------------------------------ loops */

static MvLoop *mv_push_loop(Name *label, int isblock) {
    MvLoop *l;
    if (mv_nloops >= mv_caploops) {
        mv_caploops = mv_caploops ? mv_caploops * 2 : 16;
        mv_loops = (MvLoop *)xrealloc(mv_loops, sizeof(MvLoop) * (size_t)mv_caploops);
    }
    l = &mv_loops[mv_nloops++];
    l->label = label;
    l->isblock = isblock;
    l->brk = mv_new(1);
    l->cont = mv_new(1);
    return l;
}

static MvLoop *mv_find_loop(Name *label) {
    int i;
    for (i = mv_nloops - 1; i >= mv_loop_base; i--) {
        if (label) {
            if (mv_loops[i].label == label) return &mv_loops[i];
        } else if (!mv_loops[i].isblock)
            return &mv_loops[i];
    }
    return 0;
}

static void mv_loop(Node *e) {
    MvState *head, *fs = 0;
    MvLoop *l;
    Node *body;
    int iter, idx;
    if (e->kind == N_E_FOR) {
        mv_ex(e->b);
        body = e->c;
    } else
        body = e->kind == N_E_WHILE ? e->b : e->a;
    if (mv_cur->dead) return;
    head = mv_copy(mv_cur);
    idx = mv_nloops;
    mv_push_loop(e->name, 0);
    for (iter = 0; iter < 8; iter++) {
        l = &mv_loops[idx];
        l->brk = mv_new(1);
        l->cont = mv_new(1);
        mv_cur = mv_copy(head);
        fs = 0;
        if (e->kind == N_E_WHILE) fs = mv_cond(e->a);
        else if (e->kind == N_E_FOR) {
            fs = mv_copy(head);
            mv_declare_pat(e->a, 1);
        }
        mv_block(body);
        l = &mv_loops[idx];
        mv_merge(mv_cur, l->cont);
        if (!mv_merge(head, mv_cur)) break;
    }
    l = &mv_loops[idx];
    mv_cur = fs ? fs : mv_new(1);
    mv_merge(mv_cur, l->brk);
    mv_nloops = idx;
}

/* ------------------------------------------------------------ closures */

static void mv_closure(Node *e) {
    Capture *caps = 0;
    int n, i, saved_base;
    MvState *inner, *outer;
    if (mv_cur->dead) return;
    n = closure_captures(e, &caps);
    inner = mv_copy(mv_cur);
    for (i = 0; i < n; i++) {
        int idx = mv_find(caps[i].local);
        MvPlace p;
        Type *t;
        if (idx < 0 || mv_hidden(idx)) continue;
        t = (Type *)caps[i].local->ty;
        mv_place_init(&p);
        p.root = idx;
        sprintf(p.key, "%d", idx);
        p.mutable_ = (caps[i].local->flags & SF_MUT) != 0;
        if (caps[i].mode == CAP_MOVE || (e->flags & F_MOVE)) {
            mv_check_use(&p, e, mv_copyish(t) ? "use" : "use");
            if (!mv_copyish(t)) mv_addmove(mv_cur, p.key);
        } else
            mv_check_use(&p, e, "borrow");
        if (caps[i].mode == CAP_MUT && !p.mutable_) {
            Type *rt = t ? ty_resolve(t) : 0;
            if (!(rt && rt->kind == TY_REF))
                error_at(e->file, e->line, "cannot borrow `%s` as mutable, as it is not declared as mutable",
                         mv_lname(idx), "");
        }
    }
    outer = mv_cur;
    mv_cur = inner;
    saved_base = mv_loop_base;
    mv_loop_base = mv_nloops;
    for (i = 0; i < e->list.n; i++) mv_declare_pat(e->list.v[i]->a, 1);
    mv_ex(e->b);
    mv_loop_base = saved_base;
    mv_cur = outer;
}

/* ------------------------------------------------------------ expressions */

static void mv_stmt(Node *s) {
    if (mv_cur->dead) return;
    switch (s->kind) {
    case N_STMT_EXPR:
        mv_ex(s->a);
        return;
    case N_STMT_LET: {
        Node *init = s->c;
        MvPlace pl;
        int isp = 0;
        if (!init) {
            mv_declare_pat(s->a, 0);
            return;
        }
        if (mv_is_place(init) && s->a && s->a->kind != N_P_WILD) isp = mv_scrut(init, &pl);
        else if (mv_is_place(init)) {
            mv_place_init(&pl);
            mv_walk(init, &pl);
        } else
            mv_ex(init);
        if (s->d && !mv_cur->dead) {
            MvState *saved = mv_cur;
            mv_cur = mv_copy(saved);
            mv_block(s->d);
            mv_cur = saved;
        }
        mv_declare_pat(s->a, 1);
        if (isp) mv_pat_moves(s->a, init, &pl);
        return;
    }
    }
}

static void mv_block(Node *b) {
    long i;
    int idx = -1;
    if (!b || mv_cur->dead) return;
    if (b->kind != N_BLOCK) {
        mv_ex(b);
        return;
    }
    if (b->name) {
        idx = mv_nloops;
        mv_push_loop(b->name, 1);
    }
    for (i = 0; i < b->list.n && !mv_cur->dead; i++) mv_stmt(b->list.v[i]);
    if (b->a) mv_ex(b->a);
    if (idx >= 0) {
        mv_merge(mv_cur, mv_loops[idx].brk);
        mv_nloops = idx;
    }
}

static void mv_args(Node *e) {
    long i;
    for (i = 0; i < e->list.n; i++) mv_ex(e->list.v[i]);
}

static void mv_ex(Node *e) {
    long i;
    if (!e || mv_cur->dead) return;
    switch (e->kind) {
    case N_E_PATH: case N_E_FIELD: case N_E_TUPLE_FIELD: case N_E_DEREF: case N_E_INDEX:
        mv_access(e, MV_VALUE);
        break;
    case N_E_PAREN:
        if (mv_is_place(e->a)) mv_access(e, MV_VALUE);
        else mv_ex(e->a);
        break;
    case N_E_REF:
        mv_operand_ref(e->a, (e->flags & F_MUT) != 0);
        break;
    case N_E_UNARY: case N_E_CAST: case N_E_TRY: case N_E_AWAIT:
        mv_ex(e->a);
        break;
    case N_E_BINARY:
        if (e->op == OP_LAND || e->op == OP_LOR) {
            MvState *f = mv_cond(e);
            mv_merge(mv_cur, f);
        } else if (e->op == OP_EQ || e->op == OP_NE || e->op == OP_LT || e->op == OP_LE ||
                   e->op == OP_GT || e->op == OP_GE) {
            mv_operand_ref(e->a, 0);
            mv_operand_ref(e->b, 0);
        } else {
            mv_ex(e->a);
            mv_ex(e->b);
        }
        break;
    case N_E_ASSIGN:
        mv_ex(e->b);
        mv_assign(e->a);
        break;
    case N_E_ASSIGN_OP:
        mv_assign_op(e);
        break;
    case N_E_CALL:
        if (mv_is_place(e->a)) mv_operand_ref(e->a, 0);
        else mv_ex(e->a);
        mv_args(e);
        break;
    case N_E_METHOD_CALL:
        mv_ex(e->a);
        mv_args(e);
        break;
    case N_E_RANGE:
        mv_ex(e->a);
        mv_ex(e->b);
        break;
    case N_E_TUPLE: case N_E_ARRAY:
        mv_args(e);
        break;
    case N_E_REPEAT:
        mv_ex(e->a);
        mv_ex(e->b);
        break;
    case N_E_STRUCT:
        for (i = 0; i < e->list.n; i++) mv_ex(e->list.v[i]->a);
        if (e->b) {
            if (mv_is_place(e->b)) mv_operand_ref(e->b, 0);
            else mv_ex(e->b);
        }
        break;
    case N_E_CLOSURE:
        mv_closure(e);
        break;
    case N_BLOCK:
        mv_block(e);
        break;
    case N_E_IF: {
        MvState *f = mv_cond(e->a), *t;
        mv_block(e->b);
        t = mv_cur;
        mv_cur = f;
        if (e->c) mv_block(e->c);
        mv_merge(mv_cur, t);
        break;
    }
    case N_E_LET: {
        MvState *f = mv_cond(e);
        mv_merge(mv_cur, f);
        break;
    }
    case N_E_MATCH: {
        MvPlace pl;
        int isp = mv_scrut(e->a, &pl);
        MvState *base, *end;
        if (mv_cur->dead) break;
        base = mv_cur;
        end = mv_new(1);
        for (i = 0; i < e->list.n; i++) {
            Node *arm = e->list.v[i];
            mv_cur = mv_copy(base);
            mv_declare_pat(arm->a, 1);
            if (isp) mv_pat_moves(arm->a, e->a, &pl);
            if (arm->b) {
                MvState *f = mv_cond(arm->b);
                (void)f;
            }
            mv_ex(arm->c);
            mv_merge(end, mv_cur);
        }
        mv_cur = end;
        break;
    }
    case N_E_LOOP: case N_E_WHILE: case N_E_FOR:
        mv_loop(e);
        break;
    case N_E_BREAK: {
        MvLoop *l;
        mv_ex(e->a);
        l = mv_find_loop(e->name);
        if (l && !mv_cur->dead) mv_merge(l->brk, mv_cur);
        mv_cur->dead = 1;
        break;
    }
    case N_E_CONTINUE: {
        MvLoop *l = mv_find_loop(e->name);
        if (l && !mv_cur->dead) mv_merge(l->cont, mv_cur);
        mv_cur->dead = 1;
        break;
    }
    case N_E_RETURN:
        mv_ex(e->a);
        mv_cur->dead = 1;
        break;
    default:
        break;
    }
    if (!mv_cur->dead && mv_never(e)) mv_cur->dead = 1;
}

/* ------------------------------------------------------------ bodies */

static void mv_body(Node *fn) {
    long i;
    Sym *fs = (Sym *)fn->sym;
    if (!fn->c) return;
    if (fs) collect_set_env(fs);
    mv_nloc = 0;
    /* a loop, not memset(0xff): under the C11 compiler an int is one word
       and memset stores 255 in each, not -1 */
    {
        int h;
        for (h = 0; h < MV_HASH; h++) mv_lhead[h] = -1;
    }
    mv_nloops = 0;
    mv_loop_base = 0;
    for (i = 0; i < fn->list.n; i++) mv_prepass(fn->list.v[i], 1);
    mv_prepass(fn->c, 0);
    mv_cur = mv_new(0);
    for (i = 0; i < fn->list.n; i++) {
        Node *p = fn->list.v[i];
        if (p->kind == N_SELF_PARAM) {
            Sym *s = (Sym *)p->sym;
            if (s && s->kind == SYM_LOCAL) mv_declare_local(mv_find(s), 1);
        } else
            mv_declare_pat(p->a, 1);
    }
    mv_block(fn->c);
}

static void mv_item(Node *n) {
    long i;
    switch (n->kind) {
    case N_ITEM_FN:
        mv_body(n);
        break;
    case N_ITEM_MOD:
        for (i = 0; i < n->list.n; i++) mv_item(n->list.v[i]);
        break;
    case N_ITEM_IMPL: case N_ITEM_TRAIT:
        for (i = 0; i < n->list.n; i++)
            if (n->list.v[i]->kind == N_ITEM_FN) mv_item(n->list.v[i]);
        break;
    }
}

void moves_crate(void) {
    Node *crate = crate_root ? crate_root->node : 0;
    long i;
    if (!crate) return;
    mv_lhead = (int *)xalloc(sizeof(int) * MV_HASH);
    for (i = 0; i < crate->list.n; i++) {
        mv_item(crate->list.v[i]);
        while (mv_npend > 0) {
            Node *n = mv_pend[--mv_npend];
            mv_item(n);
        }
    }
}
