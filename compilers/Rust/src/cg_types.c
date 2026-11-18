/*
 * cg_types.c - the driver of the code generator: output sections, C types,
 * monomorphization (the instance worklist), trait method resolution for an
 * instance, vtables, drop glue, statics, and the C main.  See cg.h and
 * design-codegen.md.
 *
 * Included by rustc.c after check.c / collect.c (it uses Callee, Capture,
 * closure_captures, collect_set_env, collect_builtin_impl) and before
 * cg_expr.c.
 *
 * Conventions beyond cg.h (cg_expr.c relies on them):
 *   - a closure struct has the fields c0, c1, ... one per entry of
 *     closure_captures() in that order: `long` for a by-reference capture
 *     (CAP_REF / CAP_MUT), the value for CAP_MOVE;
 *   - cg_mono() tags every TY_CLOSURE it returns with the generic context of
 *     the instance that created it (Type.len = 1 + number of generic args,
 *     Type.mut = has Self, Type.args = the args then Self), so that two
 *     instances of a generic fn get two closure types;
 *   - cg_closure_fn(closure_type) is the C function of a closure body;
 *   - a vtable entry of Fn / FnMut / FnOnce::call* takes (long data, spread
 *     arguments...), not a tuple;
 *   - an `extern "rust-intrinsic"` fn has no instance (cg_fn_instance gives 0);
 *     a trait method whose impl is built in (Fn* of closures / fn items / fn
 *     pointers, Clone) gives the closure function / fn item function / 0.
 * Everything here is C99 that the CPUSim64 C11 compiler also accepts: no
 * bit-fields, VLAs or designated initializers; never memset with a non-zero
 * byte.
 */
#include "cg.h"

/* ================================================================== output */

StrBuf cg_sect[CG_NSECT];
Subst *cg_subst = 0;
Sym *cg_cur_fn = 0;

static int cgt_ready = 0;
static int cgt_ids = 0;
static const char *cgt_src = "";
static int cgt_last_line = -1, cgt_last_file = -1;

static Subst cgt_empty;        /* no generics: used to resolve inference links deeply */

static void cgt_init(void) {
    int i;
    if (cgt_ready) return;
    cgt_ready = 1;
    for (i = 0; i < CG_NSECT; i++) sb_init(&cg_sect[i]);
}

static void cgt_append(StrBuf *b, const char *s, int n) {
    int i;
    if (n <= 0) return;
    if (b->len + n + 2 > b->cap) {
        while (b->len + n + 2 > b->cap) b->cap *= 2;
        b->s = (char *)xrealloc(b->s, (size_t)b->cap);
    }
    for (i = 0; i < n; i++) b->s[b->len + i] = s[i];
    b->len += n;
    b->s[b->len] = 0;
}

static void cgt_vprintf(StrBuf *b, const char *fmt, va_list ap) {
    char small[1024], *big;
    int n;
    va_list ap2;
    va_copy(ap2, ap);
    n = vsnprintf(small, sizeof small, fmt, ap);
    if (n < (int)sizeof small) {
        cgt_append(b, small, n);
    } else {
        big = (char *)xalloc((size_t)n + 2);
        vsnprintf(big, (size_t)n + 1, fmt, ap2);
        cgt_append(b, big, n);
        free(big);
    }
    va_end(ap2);
}

static void sbf(StrBuf *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    cgt_vprintf(b, fmt, ap);
    va_end(ap);
}

void cg_printf(int sect, const char *fmt, ...) {
    va_list ap;
    cgt_init();
    va_start(ap, fmt);
    cgt_vprintf(&cg_sect[sect], fmt, ap);
    va_end(ap);
}

/* file name for #line: a quoted C string body */
static void cgt_line_name(StrBuf *b, int file) {
    const char *nm = file == 0 ? cgt_src : (file > 0 && file < nfiles ? files[file].disp : "?");
    const char *p;
    for (p = nm; *p; p++) {
        if (*p == '\\' || *p == '"') sb_putc(b, '\\');
        sb_putc(b, *p);
    }
}

void cg_line(Node *n) {
    StrBuf b;
    if (!n || n->line <= 0) return;
    if (n->line == cgt_last_line && n->file == cgt_last_file) return;
    cgt_init();
    cgt_last_line = n->line;
    cgt_last_file = n->file;
    sb_init(&b);
    cgt_line_name(&b, n->file);
    cg_printf(CG_FUNCS, "#line %d \"%s\"\n", n->line, b.s);
    free(b.s);
}

/* The C11 compiler numbers every C line after a #line, but one Rust
   statement becomes several C lines: cg_expr.c calls this before each of
   them, so they all map back to the statement's line (the debugger's
   current line and breakpoints). */
void cg_line_resync(StrBuf *out) {
    StrBuf b;
    if (cgt_last_line <= 0) return;
    sb_init(&b);
    cgt_line_name(&b, cgt_last_file);
    sb_puts(out, "#line ");
    {
        char num[24];
        sprintf(num, "%d", cgt_last_line);
        sb_puts(out, num);
    }
    sb_puts(out, " \"");
    sb_puts(out, b.s);
    sb_puts(out, "\"\n");
    free(b.s);
}

int cg_new_id(void) {
    return ++cgt_ids;
}

static char *cgt_name(const char *prefix, int id) {
    char buf[40];
    sprintf(buf, "%s%d", prefix, id);
    return xstrdup(buf);
}

/* a Rust name or type text made safe inside a C comment */
static const char *cgt_cmt(const char *s) {
    char *r = xstrdup(s);
    int i;
    for (i = 0; r[i]; i++) {
        if (r[i] == '*' && r[i + 1] == '/') r[i] = '+';
        else if (r[i] == '/' && r[i + 1] == '*') r[i] = '+';
        else if (r[i] == '\n' || r[i] == '\r') r[i] = ' ';
    }
    return r;
}

static void cgt_fail(const char *what, Type *t) {
    error_at(-1, 0, "code generator: %s: %s", what, t ? ty_str(t) : "?");
}

/* ================================================================== per-symbol data */

typedef struct CgSym {
    const char *fname;       /* field name / variant member */
    int have_disc;
    long disc;
    const char *gname;       /* static */
} CgSym;

static CgSym *cgs(Sym *s) {
    if (!s->cg) s->cg = xalloc(sizeof(CgSym));
    return (CgSym *)s->cg;
}

/* lang items, looked up once */
static Sym *l_drop = 0, *l_box = 0, *l_fn = 0, *l_fn_mut = 0, *l_fn_once = 0, *l_sized = 0;

static void cgt_lang(void) {
    if (l_drop) return;
    l_drop = lang_item("drop");
    l_box = lang_item("owned_box");
    l_fn = lang_item("fn");
    l_fn_mut = lang_item("fn_mut");
    l_fn_once = lang_item("fn_once");
    l_sized = lang_item("sized");
}

static int cgt_is_fn_trait(Sym *tr) {
    cgt_lang();
    return tr && (tr == l_fn || tr == l_fn_mut || tr == l_fn_once);
}

/* ================================================================== monomorphization of types */

/* adds the generic context to every closure type that has none yet */
static Type *cgt_tag(Type *t) {
    Type *r, *elem, *ret, *c;
    Type **args = 0, **params = 0;
    int i, changed = 0;
    t = ty_resolve(t);
    if (!t) return t;
    elem = t->elem ? cgt_tag(t->elem) : 0;
    if (elem != t->elem) changed = 1;
    ret = t->ret ? cgt_tag(t->ret) : 0;
    if (ret != t->ret) changed = 1;
    for (i = 0; i < t->nargs; i++) {
        c = cgt_tag(t->args[i]);
        if (c != t->args[i] && !args) {
            args = (Type **)ast_alloc(sizeof(Type *) * (size_t)t->nargs);
            memcpy(args, t->args, sizeof(Type *) * (size_t)i);
        }
        if (args) args[i] = c;
    }
    if (args) changed = 1;
    for (i = 0; i < t->nparams; i++) {
        c = cgt_tag(t->params[i]);
        if (c != t->params[i] && !params) {
            params = (Type **)ast_alloc(sizeof(Type *) * (size_t)t->nparams);
            memcpy(params, t->params, sizeof(Type *) * (size_t)i);
        }
        if (params) params[i] = c;
    }
    if (params) changed = 1;
    if (changed || (t->kind == TY_CLOSURE && t->len == 0)) {
        r = (Type *)ast_alloc(sizeof(Type));
        *r = *t;
        r->elem = elem;
        r->ret = ret;
        if (args) r->args = args;
        if (params) r->params = params;
        t = r;
    }
    if (t->kind == TY_CLOSURE && t->len == 0) {
        int n = cg_subst ? cg_subst->n : 0, has_self = cg_subst && cg_subst->self ? 1 : 0;
        Type **a = 0;
        if (n + has_self > 0) {
            a = (Type **)ast_alloc(sizeof(Type *) * (size_t)(n + has_self));
            for (i = 0; i < n; i++) a[i] = cg_subst->args[i];
            if (has_self) a[n] = cg_subst->self;
        }
        t->len = 1 + n;
        t->mut = has_self;
        t->args = a;
        t->nargs = n + has_self;
    }
    return t;
}

Type *cg_mono(Type *t) {
    t = ty_resolve(t);
    if (!t) return t;
    if (cg_subst && (cg_subst->n > 0 || cg_subst->self)) t = ty_subst(t, cg_subst);
    else t = ty_subst(t, &cgt_empty);
    t = normalize(t);
    return cgt_tag(t);
}

/* a type of an item (field, signature) under an explicit substitution */
static Type *cgt_inst(Type *t, Subst *s) {
    if (!t) return t;
    return normalize(ty_subst(t, s));
}

static Type *cgt_zonk(Type *t) {
    return ty_subst(t, &cgt_empty);
}

/* ================================================================== type table */

typedef struct TInfo {
    Type *ty;
    struct TInfo *next;
    const char *cname;       /* C spelling; 0 until computed */
    const char *sname;       /* "S12" for structs */
    int state;               /* struct definition: 0 none, 1 emitting, 2 done */
    int zst, words, needs_drop;   /* -1 unknown, -2 in progress */
    const char *glue;
    /* closures */
    int caps_ready, ncaps;
    Capture *caps;
    Type **captys;
    Subst *ctx;
} TInfo;

#define TBUCK 1024
static TInfo *tbuck[TBUCK];
#define PCSIZE 4096
static Type *pc_key[PCSIZE];
static TInfo *pc_val[PCSIZE];

static unsigned long cgt_hash(Type *t) {
    unsigned long h;
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    h = (unsigned long)t->kind * 7 + (unsigned long)t->ik * 13 + (unsigned long)t->mut * 17 +
        (unsigned long)t->len * 19 + (unsigned long)t->nargs * 23 + (unsigned long)t->nparams * 29;
    h = h * 31 + ((unsigned long)(long)t->def >> 4);
    h = h * 31 + ((unsigned long)(long)t->node >> 4);
    h = h * 31 + ((unsigned long)(long)t->len_param >> 4);
    if (t->elem) h = h * 31 + cgt_hash(t->elem);
    for (i = 0; i < t->nargs; i++) h = h * 31 + cgt_hash(t->args[i]);
    for (i = 0; i < t->nparams; i++) h = h * 31 + cgt_hash(t->params[i]);
    if (t->ret) h = h * 31 + cgt_hash(t->ret);
    return h;
}

static TInfo *tinfo(Type *t) {
    unsigned long h;
    int pk;
    TInfo *e;
    t = ty_resolve(t);
    if (!t) error_at(-1, 0, "code generator: missing type");
    pk = (int)(((unsigned long)(long)t >> 4) & (PCSIZE - 1));
    if (pc_key[pk] == t) return pc_val[pk];
    h = cgt_hash(t) & (TBUCK - 1);
    for (e = tbuck[h]; e; e = e->next)
        if (e->ty == t || ty_equal(e->ty, t)) {
            pc_key[pk] = t;
            pc_val[pk] = e;
            return e;
        }
    e = (TInfo *)xalloc(sizeof(TInfo));
    e->ty = t;
    e->zst = -1;
    e->words = -1;
    e->needs_drop = -1;
    e->next = tbuck[h];
    tbuck[h] = e;
    pc_key[pk] = t;
    pc_val[pk] = e;
    return e;
}

/* ---------------------------------------------------------------- basic classification */

int cg_is_u8(Type *t) {
    t = ty_resolve(t);
    return t && t->kind == TY_INT && t->ik == IK_U8;
}

int cg_is_unsized(Type *t) {
    t = ty_resolve(t);
    return t && (t->kind == TY_STR || t->kind == TY_SLICE || t->kind == TY_DYN);
}

int cg_ref_kind(Type *pointee) {
    pointee = ty_resolve(pointee);
    if (!pointee) return REF_WORD;
    if (pointee->kind == TY_STR || pointee->kind == TY_SLICE) return REF_SLICE;
    if (pointee->kind == TY_DYN) return REF_DYN;
    if (cg_is_u8(pointee)) return REF_BYTE;
    return REF_WORD;
}

int cg_is_box(Type *t) {
    t = ty_resolve(t);
    cgt_lang();
    return t && t->kind == TY_ADT && t->def == l_box;
}

Type *cg_box_inner(Type *t) {
    t = ty_resolve(t);
    return t->args[0];
}

static const char *cgt_ref_ctype(Type *pointee) {
    switch (cg_ref_kind(pointee)) {
    case REF_SLICE: return "struct rs_slice";
    case REF_DYN: return "struct rs_dyn";
    }
    return "long";
}

/* ---------------------------------------------------------------- fields, variants */

const char *cg_field(Sym *f) {
    CgSym *c = cgs(f);
    if (!c->fname) {
        char buf[600];
        if (f->name && f->node && f->node->name) sprintf(buf, "_%s", f->name->s);
        else sprintf(buf, "f%d", f->index);
        c->fname = xstrdup(buf);
    }
    return c->fname;
}

const char *cg_variant_member(Sym *v) {
    CgSym *c = cgs(v);
    if (!c->fname) {
        char buf[40];
        sprintf(buf, "u.v%d", v->index);
        c->fname = xstrdup(buf);
    }
    return c->fname;
}

int cg_variant_index(Sym *v) {
    return v->index;
}

static long cgt_const_eval(Node *n) {
    if (!n) return 0;
    switch (n->kind) {
    case N_E_LIT_INT: case N_E_LIT_CHAR: case N_E_LIT_BYTE: case N_E_LIT_BOOL:
        return (long)n->ival;
    case N_E_PAREN: case N_E_CAST:
        return cgt_const_eval(n->a);
    case N_E_UNARY:
        if (n->op == OP_NEG) return -cgt_const_eval(n->a);
        return ~cgt_const_eval(n->a);
    case N_E_BINARY:
        switch (n->op) {
        case OP_ADD: return cgt_const_eval(n->a) + cgt_const_eval(n->b);
        case OP_SUB: return cgt_const_eval(n->a) - cgt_const_eval(n->b);
        case OP_MUL: return cgt_const_eval(n->a) * cgt_const_eval(n->b);
        case OP_SHL: return cgt_const_eval(n->a) << cgt_const_eval(n->b);
        case OP_BITOR: return cgt_const_eval(n->a) | cgt_const_eval(n->b);
        }
        break;
    }
    error_at(n->file, n->line, "unsupported enum discriminant expression");
    return 0;
}

long cg_discriminant(Sym *v) {
    CgSym *c = cgs(v);
    if (!c->have_disc) {
        Sym *en = v->parent;
        long prev = -1;
        int i, n = scope_member_count(en->members);
        for (i = 0; i < n; i++) {
            Sym *m = scope_member(en->members, i);
            CgSym *mc;
            if (!m || m->kind != SYM_VARIANT) continue;
            mc = cgs(m);
            mc->disc = m->node && m->node->a ? cgt_const_eval(m->node->a) : prev + 1;
            mc->have_disc = 1;
            prev = mc->disc;
        }
    }
    return c->disc;
}

/* the SYM_FIELD members of a struct / variant, in order */
static int cgt_fields(Sym *owner, Sym ***out) {
    int i, n = scope_member_count(owner->members), k = 0;
    Sym **v = (Sym **)ast_alloc(sizeof(Sym *) * (size_t)(n + 1));
    for (i = 0; i < n; i++) {
        Sym *m = scope_member(owner->members, i);
        if (m && m->kind == SYM_FIELD) v[k++] = m;
    }
    *out = v;
    return k;
}

/* the SYM_VARIANT members of an enum, in order */
static int cgt_variants(Sym *en, Sym ***out) {
    int i, n = scope_member_count(en->members), k = 0;
    Sym **v = (Sym **)ast_alloc(sizeof(Sym *) * (size_t)(n + 1));
    for (i = 0; i < n; i++) {
        Sym *m = scope_member(en->members, i);
        if (m && m->kind == SYM_VARIANT) v[k++] = m;
    }
    *out = v;
    return k;
}

/* the type of a field of the instantiated ADT t */
static Type *cgt_field_ty(Sym *f, Type *adt) {
    Sym *def = f->parent && f->parent->kind == SYM_VARIANT ? f->parent->parent : f->parent;
    Subst s;
    s.params = def->gparams;
    s.args = adt->args;
    s.n = adt->nargs < def->ngparams ? adt->nargs : def->ngparams;
    s.self = 0;
    return cgt_inst(f->ty, &s);
}

/* ---------------------------------------------------------------- closures */

/* the generic context of a tagged closure type */
static Subst *cgt_clo_ctx(TInfo *ti) {
    if (!ti->ctx) {
        Type *t = ti->ty;
        Subst *s = (Subst *)ast_alloc(sizeof(Subst));
        int n = t->len > 0 ? (int)t->len - 1 : 0;
        s->params = 0;
        s->args = t->args;
        s->n = n;
        s->self = t->mut && t->args ? t->args[n] : 0;
        ti->ctx = s;
    }
    return ti->ctx;
}

static void cgt_closure_caps(TInfo *ti) {
    Subst *save;
    Sym *savefn;
    int i;
    if (ti->caps_ready) return;
    ti->caps_ready = 1;
    ti->ncaps = closure_captures(ti->ty->node, &ti->caps);
    ti->captys = (Type **)ast_alloc(sizeof(Type *) * (size_t)(ti->ncaps + 1));
    save = cg_subst;
    savefn = cg_cur_fn;
    cg_subst = cgt_clo_ctx(ti);
    cg_cur_fn = 0;
    for (i = 0; i < ti->ncaps; i++)
        ti->captys[i] = ti->caps[i].mode == CAP_MOVE ? cg_mono(ti->caps[i].local->ty) : 0;
    cg_subst = save;
    cg_cur_fn = savefn;
}

/* ---------------------------------------------------------------- zst, words, drop */

int cg_is_zst(Type *t) {
    TInfo *ti;
    int i, r;
    t = ty_resolve(t);
    ti = tinfo(t);
    if (ti->zst >= 0) return ti->zst;
    r = 0;
    switch (t->kind) {
    case TY_NEVER: case TY_FN_DEF:
        r = 1;
        break;
    case TY_TUPLE:
        r = 1;
        for (i = 0; i < t->nargs; i++)
            if (!cg_is_zst(t->args[i])) r = 0;
        break;
    case TY_CLOSURE:
        cgt_closure_caps(ti);
        r = ti->ncaps == 0;
        break;
    case TY_ADT:
        if (cg_is_box(t) || t->def->kind != SYM_STRUCT) {
            r = 0;
        } else {
            Sym **fs;
            int nf = cgt_fields(t->def, &fs);
            ti->zst = 0;      /* guards a by-value cycle */
            r = 1;
            for (i = 0; i < nf; i++)
                if (!cg_is_zst(cgt_field_ty(fs[i], t))) r = 0;
        }
        break;
    }
    ti->zst = r;
    return r;
}

static Sym *cgt_drop_impl(Type *t) {
    Subst s;
    Sym *impl;
    cgt_lang();
    if (t->kind != TY_ADT || cg_is_box(t)) return 0;
    impl = find_impl(l_drop, t, 0, 0, &s);
    if (!impl || impl == collect_builtin_impl) return 0;
    return impl;
}

int cg_needs_drop(Type *t) {
    TInfo *ti;
    int i, r = 0;
    t = ty_resolve(t);
    ti = tinfo(t);
    if (ti->needs_drop >= 0) return ti->needs_drop;
    if (ti->needs_drop == -2) return 0;
    ti->needs_drop = -2;
    switch (t->kind) {
    case TY_DYN:
        r = 1;
        break;
    case TY_TUPLE:
        for (i = 0; i < t->nargs; i++)
            if (cg_needs_drop(t->args[i])) r = 1;
        break;
    case TY_ARRAY: case TY_SLICE:
        r = t->elem && cg_needs_drop(t->elem) && !(t->kind == TY_ARRAY && t->len == 0);
        break;
    case TY_CLOSURE:
        cgt_closure_caps(ti);
        for (i = 0; i < ti->ncaps; i++)
            if (ti->captys[i] && cg_needs_drop(ti->captys[i])) r = 1;
        break;
    case TY_ADT:
        if (cg_is_box(t) || cgt_drop_impl(t)) {
            r = 1;
        } else if (t->def->kind == SYM_STRUCT) {
            Sym **fs;
            int nf = cgt_fields(t->def, &fs);
            for (i = 0; i < nf; i++)
                if (cg_needs_drop(cgt_field_ty(fs[i], t))) r = 1;
        } else if (t->def->kind == SYM_ENUM) {
            Sym **vs;
            int nv = cgt_variants(t->def, &vs), v;
            for (v = 0; v < nv; v++) {
                Sym **fs;
                int nf = cgt_fields(vs[v], &fs);
                for (i = 0; i < nf; i++)
                    if (cg_needs_drop(cgt_field_ty(fs[i], t))) r = 1;
            }
        }
        break;
    }
    ti->needs_drop = r;
    return r;
}

int cg_words(Type *t) {
    TInfo *ti;
    int w = 1, i;
    t = ty_resolve(t);
    ti = tinfo(t);
    if (ti->words >= 0) return ti->words;
    switch (t->kind) {
    case TY_STR: case TY_SLICE: case TY_DYN:
        w = 0;
        break;
    case TY_REF: case TY_PTR:
        w = cg_ref_kind(t->elem) >= REF_SLICE ? 2 : 1;
        break;
    case TY_TUPLE:
        if (!cg_is_zst(t)) {
            w = 0;
            for (i = 0; i < t->nargs; i++) w += cg_words(t->args[i]);
        }
        break;
    case TY_ARRAY:
        if (t->len == 0) w = 1;
        else if (cg_is_u8(t->elem)) w = (int)((t->len + 7) / 8);
        else w = (int)t->len * cg_words(t->elem);
        break;
    case TY_CLOSURE:
        cgt_closure_caps(ti);
        if (ti->ncaps > 0) {
            w = 0;
            for (i = 0; i < ti->ncaps; i++) w += ti->captys[i] ? cg_words(ti->captys[i]) : 1;
        }
        break;
    case TY_ADT:
        if (cg_is_box(t)) {
            w = cg_ref_kind(cg_box_inner(t)) >= REF_SLICE ? 2 : 1;
        } else if (cg_is_zst(t)) {
            w = 1;
        } else if (t->def->kind == SYM_STRUCT) {
            Sym **fs;
            int nf = cgt_fields(t->def, &fs);
            w = 0;
            for (i = 0; i < nf; i++) w += cg_words(cgt_field_ty(fs[i], t));
        } else {
            Sym **vs;
            int nv = cgt_variants(t->def, &vs), v, best = 0;
            for (v = 0; v < nv; v++) {
                Sym **fs;
                int nf = cgt_fields(vs[v], &fs), vw = 0;
                for (i = 0; i < nf; i++) vw += cg_words(cgt_field_ty(fs[i], t));
                if (nf == 0) vw = 1;
                if (vw > best) best = vw;
            }
            {
                int payload = 0;
                for (v = 0; v < nv; v++) {
                    Sym **fs;
                    if (cgt_fields(vs[v], &fs) > 0) payload = 1;
                }
                w = payload ? 1 + best : 1;
            }
        }
        break;
    }
    if (w < 1 && t->kind != TY_STR && t->kind != TY_SLICE && t->kind != TY_DYN) w = 1;
    ti->words = w;
    return w;
}

/* ================================================================== C types */

static const char *cgt_new_sname(TInfo *ti) {
    if (!ti->sname) ti->sname = cgt_name("S", cg_new_id());
    return ti->sname;
}

static void cgt_emit_adt(TInfo *ti, Type *t) {
    StrBuf b;
    Sym *def = t->def;
    char *cn;
    int i;
    if (def->kind == SYM_UNION) cgt_fail("unions are not supported", t);
    cgt_new_sname(ti);
    sb_init(&b);
    sbf(&b, "/* %s */\n", cgt_cmt(ty_str(t)));
    if (def->kind == SYM_STRUCT) {
        Sym **fs;
        int nf = cgt_fields(def, &fs);
        sbf(&b, "struct %s {\n", ti->sname);
        for (i = 0; i < nf; i++) {
            const char *ct = cg_ctype(cgt_field_ty(fs[i], t));
            sbf(&b, "    %s %s;\n", ct, cg_field(fs[i]));
        }
        sbf(&b, "};\n");
    } else {
        Sym **vs;
        int nv = cgt_variants(def, &vs), v, payload = 0;
        for (v = 0; v < nv; v++) {
            Sym **fs;
            if (cgt_fields(vs[v], &fs) > 0) payload = 1;
        }
        if (payload) {
            for (v = 0; v < nv; v++) {
                Sym **fs;
                int nf = cgt_fields(vs[v], &fs);
                sbf(&b, "struct %s_v%d {\n", ti->sname, v);
                for (i = 0; i < nf; i++) {
                    const char *ct = cg_ctype(cgt_field_ty(fs[i], t));
                    sbf(&b, "    %s %s;\n", ct, cg_field(fs[i]));
                }
                if (nf == 0) sbf(&b, "    long _;\n");
                sbf(&b, "};\n");
            }
        }
        sbf(&b, "struct %s {\n    long tag;\n", ti->sname);
        if (payload) {
            sbf(&b, "    union {\n");
            for (v = 0; v < nv; v++) sbf(&b, "        struct %s_v%d v%d;\n", ti->sname, v, v);
            sbf(&b, "    } u;\n");
        }
        sbf(&b, "};\n");
    }
    cgt_append(&cg_sect[CG_TYPES], b.s, b.len);
    free(b.s);
    cn = (char *)xalloc(strlen(ti->sname) + 8);
    sprintf(cn, "struct %s", ti->sname);
    ti->cname = cn;
}

static void cgt_compute_ctype(TInfo *ti) {
    Type *t = ti->ty;
    char *cn;
    int i;
    cgt_init();
    if (ti->state == 1) cgt_fail("recursive type without indirection", t);
    ti->state = 1;
    switch (t->kind) {
    case TY_BOOL: case TY_CHAR: case TY_INT: case TY_NEVER: case TY_FN_PTR: case TY_FN_DEF:
        ti->cname = "long";
        break;
    case TY_FLOAT:
        ti->cname = "double";
        break;
    case TY_STR: case TY_SLICE:
        ti->cname = "struct rs_slice";
        break;
    case TY_DYN:
        ti->cname = "struct rs_dyn";
        break;
    case TY_REF: case TY_PTR:
        ti->cname = cgt_ref_ctype(t->elem);
        break;
    case TY_INFER:
        if (t->ik == INFER_FLOAT) ti->cname = "double";
        else if (t->ik == INFER_INT) ti->cname = "long";
        else cgt_fail("unresolved inference variable", t);
        break;
    case TY_TUPLE:
        if (cg_is_zst(t)) {
            ti->cname = "long";
        } else {
            StrBuf b;
            cgt_new_sname(ti);
            sb_init(&b);
            sbf(&b, "/* %s */\nstruct %s {\n", cgt_cmt(ty_str(t)), ti->sname);
            for (i = 0; i < t->nargs; i++) sbf(&b, "    %s f%d;\n", cg_ctype(t->args[i]), i);
            sbf(&b, "};\n");
            cgt_append(&cg_sect[CG_TYPES], b.s, b.len);
            free(b.s);
            cn = (char *)xalloc(strlen(ti->sname) + 8);
            sprintf(cn, "struct %s", ti->sname);
            ti->cname = cn;
        }
        break;
    case TY_ARRAY: {
        StrBuf b;
        if (!t->elem) cgt_fail("a const generic argument used as a type", t);
        if (t->len_param) cgt_fail("array with an unresolved const length", t);
        cgt_new_sname(ti);
        sb_init(&b);
        if (t->len == 0) {
            sbf(&b, "/* %s */\nstruct %s {\n    long _;\n};\n", cgt_cmt(ty_str(t)), ti->sname);
        } else if (cg_is_u8(t->elem)) {
            sbf(&b, "/* %s */\nstruct %s {\n    long w[%ld];\n};\n", cgt_cmt(ty_str(t)), ti->sname, (t->len + 7) / 8);
        } else {
            const char *et = cg_ctype(t->elem);
            sbf(&b, "/* %s */\nstruct %s {\n    %s e[%ld];\n};\n", cgt_cmt(ty_str(t)), ti->sname, et, t->len);
        }
        cgt_append(&cg_sect[CG_TYPES], b.s, b.len);
        free(b.s);
        cn = (char *)xalloc(strlen(ti->sname) + 8);
        sprintf(cn, "struct %s", ti->sname);
        ti->cname = cn;
        break;
    }
    case TY_CLOSURE:
        cgt_closure_caps(ti);
        if (ti->ncaps == 0) {
            ti->cname = "long";
        } else {
            StrBuf b;
            cgt_new_sname(ti);
            sb_init(&b);
            sbf(&b, "/* closure at line %d */\nstruct %s {\n", t->node ? t->node->line : 0, ti->sname);
            for (i = 0; i < ti->ncaps; i++) {
                if (ti->caps[i].mode == CAP_MOVE) sbf(&b, "    %s c%d;\n", cg_ctype(ti->captys[i]), i);
                else sbf(&b, "    long c%d;\n", i);
            }
            sbf(&b, "};\n");
            cgt_append(&cg_sect[CG_TYPES], b.s, b.len);
            free(b.s);
            cn = (char *)xalloc(strlen(ti->sname) + 8);
            sprintf(cn, "struct %s", ti->sname);
            ti->cname = cn;
        }
        break;
    case TY_ADT:
        if (cg_is_box(t)) ti->cname = cgt_ref_ctype(cg_box_inner(t));
        else if (cg_is_zst(t)) ti->cname = "long";
        else cgt_emit_adt(ti, t);
        break;
    default:
        cgt_fail("type is not monomorphic", t);
    }
    ti->state = 2;
}

const char *cg_ctype(Type *t) {
    TInfo *ti = tinfo(t);
    if (!ti->cname) cgt_compute_ctype(ti);
    return ti->cname;
}

/* ================================================================== string literals */

typedef struct StrLit {
    char *bytes;
    long len;
    const char *name;
    struct StrLit *next;
} StrLit;

#define SLBUCK 256
static StrLit *sl_tab[SLBUCK];

const char *cg_str_literal(const char *bytes, long len) {
    unsigned long h = 5381;
    long i, nw, w;
    StrLit *e;
    StrBuf b;
    for (i = 0; i < len; i++) h = h * 33 + ((unsigned long)bytes[i] & 255);
    h = (h + (unsigned long)len) % SLBUCK;
    for (e = sl_tab[h]; e; e = e->next) {
        if (e->len != len) continue;
        for (i = 0; i < len; i++)
            if (e->bytes[i] != bytes[i]) break;
        if (i == len) return e->name;
    }
    e = (StrLit *)xalloc(sizeof(StrLit));
    e->bytes = (char *)xalloc((size_t)len + 1);
    for (i = 0; i < len; i++) e->bytes[i] = bytes[i];
    e->len = len;
    e->name = cgt_name("L", cg_new_id());
    e->next = sl_tab[h];
    sl_tab[h] = e;
    cgt_init();
    sb_init(&b);
    sbf(&b, "static const long %s[] = {", e->name);
    nw = (len + 7) / 8;
    if (nw == 0) sbf(&b, " 0");
    for (w = 0; w < nw; w++) {
        unsigned long v = 0;
        long k;
        for (k = 0; k < 8; k++) {
            unsigned long c = 0;
            if (w * 8 + k < len) c = (unsigned long)bytes[w * 8 + k] & 255;
            v = (v << 8) | c;
        }
        if (w % 6 == 0) sbf(&b, "\n   ");
        if ((long)v == (-9223372036854775807L - 1)) sbf(&b, " (-9223372036854775807L-1),");
        else if ((long)v < 0) sbf(&b, " (%ld),", (long)v);
        else sbf(&b, " %ld,", (long)v);
    }
    sbf(&b, "\n};\n");
    cgt_append(&cg_sect[CG_DATA], b.s, b.len);
    free(b.s);
    return e->name;
}

/* ================================================================== instances */

enum { INST_FN = 1, INST_CLO, INST_DROP, INST_SHIM, INST_VT };

typedef struct Inst {
    int kind;
    Sym *fn;
    Type **args;
    int nargs;
    Type *self;
    Type *ty;
    Node *node;
    const char *name;
    Subst sub;
    struct Inst *chain;
} Inst;

#define IBUCK 1024
static Inst *inst_tab[IBUCK];
static Inst **queue = 0;
static int qn = 0, qcap = 0, qhead = 0;

static unsigned long cgt_inst_hash(int kind, Sym *fn, Type **args, int n, Type *self, Type *ty) {
    unsigned long h = (unsigned long)kind * 131 + ((unsigned long)(long)fn >> 4);
    int i;
    for (i = 0; i < n; i++) h = h * 31 + cgt_hash(args[i]);
    h = h * 31 + cgt_hash(self);
    h = h * 31 + cgt_hash(ty);
    return h;
}

static Inst *cgt_inst_find(int kind, Sym *fn, Type **args, int n, Type *self, Type *ty) {
    Inst *in;
    int i;
    for (in = inst_tab[cgt_inst_hash(kind, fn, args, n, self, ty) % IBUCK]; in; in = in->chain) {
        if (in->kind != kind || in->fn != fn || in->nargs != n) continue;
        for (i = 0; i < n; i++)
            if (!ty_equal(in->args[i], args[i])) break;
        if (i < n) continue;
        if (!ty_equal(in->self, self) || !ty_equal(in->ty, ty)) continue;
        return in;
    }
    return 0;
}

static Inst *cgt_inst_new(int kind, Sym *fn, Type **args, int n, Type *self, Type *ty, const char *prefix) {
    Inst *in = (Inst *)xalloc(sizeof(Inst));
    unsigned long h = cgt_inst_hash(kind, fn, args, n, self, ty) % IBUCK;
    in->kind = kind;
    in->fn = fn;
    in->args = args;
    in->nargs = n;
    in->self = self;
    in->ty = ty;
    in->name = cgt_name(prefix, cg_new_id());
    in->chain = inst_tab[h];
    inst_tab[h] = in;
    return in;
}

static void cgt_enqueue(Inst *in) {
    if (qn == qcap) {
        qcap = qcap ? qcap * 2 : 256;
        queue = (Inst **)xrealloc(queue, sizeof(Inst *) * (size_t)qcap);
    }
    queue[qn++] = in;
}

static Type **cgt_zonk_list(Type **v, int n) {
    Type **r;
    int i;
    if (n <= 0) return 0;
    r = (Type **)ast_alloc(sizeof(Type *) * (size_t)n);
    for (i = 0; i < n; i++) r[i] = v[i] ? cgt_zonk(v[i]) : 0;
    return r;
}

/* t contains a closure type without a generic context tag */
static int cgt_has_untagged(Type *t) {
    int i;
    t = ty_resolve(t);
    if (!t) return 0;
    if (t->kind == TY_CLOSURE && t->len == 0) return 1;
    if (t->elem && cgt_has_untagged(t->elem)) return 1;
    for (i = 0; i < t->nargs; i++)
        if (cgt_has_untagged(t->args[i])) return 1;
    for (i = 0; i < t->nparams; i++)
        if (cgt_has_untagged(t->params[i])) return 1;
    return t->ret && cgt_has_untagged(t->ret);
}

/* the signature of fn instantiated with args / self, as a TY_FN_DEF */
static Type *cgt_sig(Sym *fn, Type **args, int n, Type *self) {
    Type *sig = fn->ty, **params, *ret;
    Subst s;
    int i, np;
    if (!sig || sig->kind != TY_FN_DEF) error_at(-1, 0, "code generator: function `%s` has no signature", fn->name ? fn->name->s : "?");
    s.params = fn->gparams;
    s.args = args;
    s.n = n < fn->ngparams ? n : fn->ngparams;
    s.self = self;
    np = sig->nparams;
    params = (Type **)ast_alloc(sizeof(Type *) * (size_t)(np + 1));
    for (i = 0; i < np; i++) params[i] = cgt_inst(sig->params[i], &s);
    ret = cgt_inst(sig->ret, &s);
    /* a closure in the return type that is still untagged (the hidden type of `-> impl Trait`)
       was created by this fn's body: it gets the identity the body gives it, i.e. the context of
       this instance (cgt_instance; closures in the args were tagged by the caller already) */
    if (ret && cgt_has_untagged(ret)) {
        Subst *save = cg_subst, cs = s;
        Type **ta = n > 0 ? (Type **)ast_alloc(sizeof(Type *) * (size_t)n) : 0;
        for (i = 0; i < n; i++) ta[i] = args[i] ? cgt_tag(cgt_zonk(args[i])) : 0;
        cs.args = ta;
        if (!fn->parent || fn->parent->kind != SYM_TRAIT) cs.self = 0;
        else if (self) cs.self = cgt_tag(cgt_zonk(self));
        for (i = 0; i < cs.n; i++)
            if (!ta[i]) { cs.n = i; break; }
        ret = cgt_inst(sig->ret, &cs);
        cg_subst = &cs;
        ret = cgt_tag(ret);
        cg_subst = save;
    }
    return ty_fn_def(fn, args, n, params, np, ret);
}

static void cgt_proto_text(StrBuf *b, const char *cmt, Type *ret, const char *name, const char *first,
                           Type **params, int np) {
    int i;
    if (cmt) sbf(b, "/* %s */\n", cgt_cmt(cmt));
    sbf(b, "static %s %s(", cg_ctype(ret), name);
    if (first) sbf(b, "%s", first);
    for (i = 0; i < np; i++) {
        if (i || first) sbf(b, ", ");
        sbf(b, "%s a%d", cg_ctype(params[i]), i);
    }
    if (!first && np == 0) sbf(b, "void");
    sbf(b, ");\n");
}

static void cgt_emit_proto(Type *ret, const char *name, const char *first, Type **params, int np, const char *cmt) {
    StrBuf b;
    sb_init(&b);
    cgt_proto_text(&b, cmt, ret, name, first, params, np);
    cgt_append(&cg_sect[CG_PROTOS], b.s, b.len);
    free(b.s);
}

/* ---------------------------------------------------------------- trait method resolution */

/* Resolves fn called with (args, self): a trait method with a known Self is
 * replaced by the impl's method (or kept as the trait's default method).
 * Returns 0 when there is no function to call (built-in impl, dyn). */
static int cgt_resolve_fn(Sym *fn, Type **args, int nargs, Type *self,
                          Sym **ofn, Type ***oargs, int *on, Type **oself) {
    Sym *tr, *impl, *item;
    Subst sub;
    Type **na;
    int ntr, nown, i, nimpl;
    *ofn = fn;
    *oargs = args;
    *on = nargs;
    *oself = self;
    if (!fn->parent || fn->parent->kind != SYM_TRAIT || !self) return 1;
    tr = fn->parent;
    ntr = tr->ngparams < nargs ? tr->ngparams : nargs;
    impl = find_impl(tr, self, args, ntr, &sub);
    if (!impl || impl == collect_builtin_impl) return 0;
    item = assoc_item(impl, NS_VALUE, fn->name);
    if (!item || item->kind != SYM_FN) return 0;
    if (item->parent != impl) {
        /* the trait's default method: Self and the trait args stay */
        *ofn = item;
        return 1;
    }
    nimpl = impl->ngparams;
    nown = nargs - ntr;
    if (nown < 0) nown = 0;
    na = (Type **)ast_alloc(sizeof(Type *) * (size_t)(nimpl + nown + 1));
    for (i = 0; i < nimpl; i++) na[i] = i < sub.n && sub.args[i] ? cgt_zonk(sub.args[i]) : 0;
    for (i = 0; i < nown; i++) na[nimpl + i] = args[ntr + i];
    *ofn = item;
    *oargs = na;
    *on = nimpl + nown;
    *oself = self;
    return 1;
}

static const char *cgt_instance(Sym *fn, Type **args, int n, Type *self);
const char *cg_closure_fn(Type *closure_ty);

/* a trait method with a built-in impl: Fn* on a closure / fn item gives its function */
static const char *cgt_builtin_instance(Sym *fn, Type *self) {
    Sym *tr = fn->parent;
    if (!self || !tr || tr->kind != SYM_TRAIT) return 0;
    self = ty_resolve(self);
    if (cgt_is_fn_trait(tr)) {
        if (self->kind == TY_CLOSURE) return cg_closure_fn(self);
        if (self->kind == TY_FN_DEF && self->def && self->def->kind == SYM_FN)
            return cg_fn_instance(self->def, self->args, self->nargs, 0);
    }
    return 0;
}

const char *cg_fn_instance(Sym *fn, Type **args, int nargs, Type *self_ty) {
    Sym *ofn;
    Type **oargs, *oself;
    int on;
    cgt_init();
    if (!fn) return 0;
    /* callers may pass types still written in the current instance's terms
       (Self, generic params): substitute them before they become identities */
    if (self_ty) self_ty = cg_mono(self_ty);
    if (nargs > 0) {
        Type **ma = (Type **)ast_alloc(sizeof(Type *) * (size_t)nargs);
        int k;
        for (k = 0; k < nargs; k++) ma[k] = args[k] ? cg_mono(args[k]) : 0;
        args = ma;
    }
    if (self_ty) self_ty = cgt_zonk(self_ty);
    if (!cgt_resolve_fn(fn, args, nargs, self_ty, &ofn, &oargs, &on, &oself))
        return cgt_builtin_instance(fn, self_ty);
    return cgt_instance(ofn, oargs, on, oself);
}

static const char *cgt_instance(Sym *fn, Type **args, int n, Type *self) {
    Inst *in;
    StrBuf pb;
    Type *sig;
    int i;
    if (fn->flags & SF_INTRINSIC) return 0;
    if (!fn->node || fn->node->kind != N_ITEM_FN) return 0;
    if (!fn->node->c) return (fn->flags & SF_EXTERN) && fn->name ? fn->name->s : 0;
    args = cgt_zonk_list(args, n);
    /* closure types get their identity from the caller's context here, so the
       body (generated under this instance's subst) cannot re-tag them */
    for (i = 0; i < n; i++)
        if (args[i]) args[i] = cgt_tag(args[i]);
    if (!fn->parent || fn->parent->kind != SYM_TRAIT) self = 0;
    else if (self) self = cgt_tag(cgt_zonk(self));
    in = cgt_inst_find(INST_FN, fn, args, n, self, 0);
    if (in) return in->name;
    in = cgt_inst_new(INST_FN, fn, args, n, self, 0, "F");
    in->sub.params = fn->gparams;
    in->sub.args = args;
    in->sub.n = n < fn->ngparams ? n : fn->ngparams;
    in->sub.self = self;
    for (i = 0; i < in->sub.n; i++)
        if (!args[i]) { in->sub.n = i; break; }
    sig = cgt_sig(fn, args, n, self);
    sb_init(&pb);
    sym_path(fn, &pb);
    cgt_emit_proto(sig->ret, in->name, 0, sig->params, sig->nparams, pb.s);
    free(pb.s);
    cgt_enqueue(in);
    return in->name;
}

Sym *cg_resolve_callee(Callee *c, Type ***out_args, int *out_n, Type **out_sig) {
    Type *fnty, *self = 0;
    Sym *ofn;
    Type **oargs, *oself;
    int on;
    if (!c || !c->fnty) error_at(-1, 0, "code generator: call without a callee");
    fnty = cg_mono(c->fnty);
    if (fnty->kind != TY_FN_DEF) cgt_fail("callee is not a function type", fnty);
    if (c->self) self = cg_mono(c->self);
    if (!cgt_resolve_fn(c->fn, fnty->args, fnty->nargs, self, &ofn, &oargs, &on, &oself)) {
        *out_args = fnty->args;
        *out_n = fnty->nargs;
        *out_sig = fnty;
        return c->fn;
    }
    if (ofn == c->fn && oargs == fnty->args) {
        *out_args = fnty->args;
        *out_n = fnty->nargs;
        /* a trait's default method keeps its fn but Self must still be
           substituted in the signature */
        *out_sig = oself ? cgt_sig(ofn, oargs, on, oself) : fnty;
        return c->fn;
    }
    *out_args = oargs;
    *out_n = on;
    *out_sig = cgt_sig(ofn, oargs, on, oself);
    return ofn;
}

Sym *cg_resolve_fn_type(Type *fnty, Type ***out_args, int *out_n, Type **out_sig) {
    fnty = cg_mono(fnty);
    if (fnty->kind != TY_FN_DEF) cgt_fail("not a fn item type", fnty);
    *out_args = fnty->args;
    *out_n = fnty->nargs;
    *out_sig = fnty;
    return fnty->def;
}

/* ---------------------------------------------------------------- closures */

const char *cg_closure_fn(Type *t) {
    Inst *in;
    TInfo *ti;
    Type **params;
    int i, np;
    cgt_init();
    t = ty_resolve(t);
    if (!t || t->kind != TY_CLOSURE) cgt_fail("not a closure type", t);
    if (t->len == 0) t = cgt_tag(t);
    in = cgt_inst_find(INST_CLO, 0, 0, 0, 0, t);
    if (in) return in->name;
    in = cgt_inst_new(INST_CLO, 0, 0, 0, 0, t, "F");
    in->node = t->node;
    ti = tinfo(t);
    in->sub = *cgt_clo_ctx(ti);
    np = t->nparams;
    params = t->params;
    for (i = 0; i < np; i++) params[i] = cgt_zonk(params[i]);
    cgt_emit_proto(t->ret, in->name, "long env", params, np, "closure");
    cgt_enqueue(in);
    return in->name;
}

const char *cg_closure_instance(Node *closure) {
    return cg_closure_fn(cg_mono((Type *)closure->ty));
}

/* ---------------------------------------------------------------- vtables */

typedef struct {
    Sym *method;
    Sym *trait;
    Type **targs;
    int ntargs;
} VEnt;

typedef struct {
    VEnt *v;
    int n, cap;
    Sym **seen;
    int nseen, cseen;
} VList;

static void cgt_vt_add_trait(VList *l, Sym *tr, Type **targs, int nt, Type *self) {
    int i, n, k;
    for (i = 0; i < l->nseen; i++)
        if (l->seen[i] == tr) return;
    if (l->nseen == l->cseen) {
        l->cseen = l->cseen ? l->cseen * 2 : 8;
        l->seen = (Sym **)xrealloc(l->seen, sizeof(Sym *) * (size_t)l->cseen);
    }
    l->seen[l->nseen++] = tr;
    n = scope_member_count(tr->members);
    for (i = 0; i < n; i++) {
        Sym *m = scope_member(tr->members, i);
        if (!m || m->kind != SYM_FN) continue;
        if (l->n == l->cap) {
            l->cap = l->cap ? l->cap * 2 : 16;
            l->v = (VEnt *)xrealloc(l->v, sizeof(VEnt) * (size_t)l->cap);
        }
        l->v[l->n].method = m;
        l->v[l->n].trait = tr;
        l->v[l->n].targs = targs;
        l->v[l->n].ntargs = nt;
        l->n++;
    }
    for (k = 0; k < tr->nbounds; k++) {
        Bound *b = &tr->bounds[k];
        Type **sargs = 0;
        Subst s;
        if (!b->ty || ty_resolve(b->ty)->kind != TY_SELF || !b->trait || b->trait == tr || b->assoc_name) continue;
        s.params = tr->gparams;
        s.args = targs;
        s.n = nt < tr->ngparams ? nt : tr->ngparams;
        s.self = self;
        if (b->nargs > 0) {
            sargs = (Type **)ast_alloc(sizeof(Type *) * (size_t)b->nargs);
            for (i = 0; i < b->nargs; i++) sargs[i] = cgt_inst(b->args[i], &s);
        }
        cgt_vt_add_trait(l, b->trait, sargs, b->nargs, self);
    }
}

static void cgt_vt_collect(VList *l, Type *dyn_ty, Type *self) {
    memset(l, 0, sizeof(VList));
    cgt_vt_add_trait(l, dyn_ty->def, dyn_ty->args, dyn_ty->nargs, self);
}

int cg_vtable_slot(Type *dyn_ty, Sym *method) {
    VList l;
    int i;
    dyn_ty = ty_resolve(dyn_ty);
    cgt_lang();
    cgt_vt_collect(&l, dyn_ty, 0);
    for (i = 0; i < l.n; i++)
        if (l.v[i].method == method) return 2 + i;
    error_at(-1, 0, "code generator: method `%s` is not in the vtable of %s",
             method->name ? method->name->s : "?", ty_str(dyn_ty));
    return 0;
}

/* a vtable method eligible: has a receiver, no generics of its own, not `where Self: Sized` */
static int cgt_vt_eligible(VEnt *e) {
    Sym *m = e->method;
    int i;
    if (!(m->flags & SF_METHOD)) return 0;
    if (m->ngparams > e->trait->ngparams) return 0;
    for (i = 0; i < m->nbounds; i++) {
        Bound *b = &m->bounds[i];
        if (b->ty && ty_resolve(b->ty)->kind == TY_SELF && b->trait == l_sized) return 0;
    }
    return 1;
}

/* the C function a vtable slot of the Fn* traits points to for a fn item / fn pointer */
static const char *cgt_shim(Type *t) {
    Inst *in = cgt_inst_find(INST_SHIM, 0, 0, 0, 0, t);
    if (in) return in->name;
    in = cgt_inst_new(INST_SHIM, 0, 0, 0, 0, t, "V");
    cgt_emit_proto(t->ret, in->name, "long data", t->params, t->nparams, "vtable shim");
    cgt_enqueue(in);
    return in->name;
}

static const char *cgt_vt_entry(Type *concrete, VEnt *e) {
    Sym *ofn;
    Type **oargs, *oself;
    int on;
    const char *nm;
    concrete = ty_resolve(concrete);
    if (!cgt_vt_eligible(e)) return "0";
    if (cgt_is_fn_trait(e->trait)) {
        if (concrete->kind == TY_CLOSURE) return cg_closure_fn(concrete);
        if (concrete->kind == TY_FN_DEF || concrete->kind == TY_FN_PTR) return cgt_shim(concrete);
    }
    if (!cgt_resolve_fn(e->method, e->targs, e->ntargs, concrete, &ofn, &oargs, &on, &oself)) return "0";
    nm = cgt_instance(ofn, oargs, on, oself);
    return nm ? nm : "0";
}

const char *cg_vtable(Type *concrete, Type *dyn_ty) {
    Inst *in;
    VList l;
    StrBuf b;
    const char *glue = 0;
    int i;
    cgt_init();
    cgt_lang();
    concrete = cgt_zonk(concrete);
    dyn_ty = cgt_zonk(dyn_ty);
    in = cgt_inst_find(INST_VT, 0, 0, 0, dyn_ty, concrete);
    if (in) return in->name;
    in = cgt_inst_new(INST_VT, 0, 0, 0, dyn_ty, concrete, "VT");
    if (cg_needs_drop(concrete)) glue = cg_drop_glue(concrete);
    cgt_vt_collect(&l, dyn_ty, concrete);
    sb_init(&b);
    sbf(&b, "/* %s as %s */\n", cgt_cmt(ty_str(concrete)), cgt_cmt(ty_str(dyn_ty)));
    sbf(&b, "static long %s[] = { ", in->name);
    if (glue) sbf(&b, "(long)%s", glue);
    else sbf(&b, "0");
    sbf(&b, ", %d", cg_words(concrete));
    for (i = 0; i < l.n; i++) {
        const char *nm = cgt_vt_entry(concrete, &l.v[i]);
        if (nm[0] == '0') sbf(&b, ", 0");
        else sbf(&b, ", (long)%s", nm);
    }
    sbf(&b, " };\n");
    cgt_append(&cg_sect[CG_DATA], b.s, b.len);
    free(b.s);
    return in->name;
}

/* the body of a vtable shim for a fn item / fn pointer / constructor */
static void cgt_gen_shim(Inst *in) {
    Type *t = in->ty;
    StrBuf b;
    int i;
    sb_init(&b);
    sbf(&b, "static %s %s(long data", cg_ctype(t->ret), in->name);
    for (i = 0; i < t->nparams; i++) sbf(&b, ", %s a%d", cg_ctype(t->params[i]), i);
    sbf(&b, ") {\n");
    if (t->kind == TY_FN_PTR) {
        sbf(&b, "    return ((%s (*)(", cg_ctype(t->ret));
        if (t->nparams == 0) sbf(&b, "void");
        for (i = 0; i < t->nparams; i++) sbf(&b, "%s%s", i ? ", " : "", cg_ctype(t->params[i]));
        sbf(&b, "))(*(long *)data))(");
        for (i = 0; i < t->nparams; i++) sbf(&b, "%sa%d", i ? ", " : "", i);
        sbf(&b, ");\n");
    } else if (t->def && t->def->kind == SYM_FN) {
        const char *nm = cg_fn_instance(t->def, t->args, t->nargs, 0);
        if (!nm) {
            sbf(&b, "    return 0;\n");
        } else {
            sbf(&b, "    return %s(", nm);
            for (i = 0; i < t->nparams; i++) sbf(&b, "%sa%d", i ? ", " : "", i);
            sbf(&b, ");\n");
        }
    } else if (t->def) {
        /* a tuple struct / variant constructor used as a function */
        Type *rt = ty_resolve(t->ret);
        Sym **fs;
        int nf;
        if (cg_is_zst(rt)) {
            sbf(&b, "    return 0;\n");
        } else if (t->def->kind == SYM_VARIANT) {
            nf = cgt_fields(t->def, &fs);
            sbf(&b, "    %s r;\n    r.tag = %d;\n", cg_ctype(rt), t->def->index);
            for (i = 0; i < nf; i++) sbf(&b, "    r.%s.%s = a%d;\n", cg_variant_member(t->def), cg_field(fs[i]), i);
            sbf(&b, "    return r;\n");
        } else {
            nf = cgt_fields(t->def, &fs);
            sbf(&b, "    %s r;\n", cg_ctype(rt));
            for (i = 0; i < nf; i++) sbf(&b, "    r.%s = a%d;\n", cg_field(fs[i]), i);
            sbf(&b, "    return r;\n");
        }
    } else {
        sbf(&b, "    return 0;\n");
    }
    sbf(&b, "}\n");
    cgt_append(&cg_sect[CG_FUNCS], b.s, b.len);
    free(b.s);
    cgt_last_line = -1;
}

/* ================================================================== drop glue */

const char *cg_drop_glue(Type *t) {
    TInfo *ti;
    Inst *in;
    StrBuf b;
    cgt_init();
    t = cgt_zonk(t);
    if (t->kind == TY_DYN || t->kind == TY_STR || t->kind == TY_SLICE) return 0;
    if (!cg_needs_drop(t)) return 0;
    ti = tinfo(t);
    if (ti->glue) return ti->glue;
    in = cgt_inst_new(INST_DROP, 0, 0, 0, 0, t, "D");
    ti->glue = in->name;
    sb_init(&b);
    sbf(&b, "/* drop %s */\nstatic long %s(long p);\n", cgt_cmt(ty_str(t)), in->name);
    cgt_append(&cg_sect[CG_PROTOS], b.s, b.len);
    free(b.s);
    cgt_enqueue(in);
    return in->name;
}

/* emits a call dropping the value of type ft at address expression addr */
static void cgt_drop_call(StrBuf *b, Type *ft, const char *addr) {
    const char *g;
    if (!cg_needs_drop(ft)) return;
    g = cg_drop_glue(ft);
    if (g) sbf(b, "    %s(%s);\n", g, addr);
}

static const char *cgt_member_addr(Type *container, const char *member) {
    char buf[300];
    if (cg_is_zst(container)) return "p";
    sprintf(buf, "(long)&((%s *)p)->%s", cg_ctype(container), member);
    return xstrdup(buf);
}

static const char *cgt_member_addr_idx(Type *arr) {
    char buf[300];
    sprintf(buf, "(long)&((%s *)p)->e[i]", cg_ctype(arr));
    return xstrdup(buf);
}

static void cgt_gen_box_drop(StrBuf *b, Type *t) {
    Type *inner = ty_resolve(cg_box_inner(t));
    switch (cg_ref_kind(inner)) {
    case REF_DYN:
        sbf(b, "    {\n        struct rs_dyn *d = (struct rs_dyn *)p;\n");
        sbf(b, "        if (((long *)d->vt)[0]) ((long (*)(long))(((long *)d->vt)[0]))(d->data);\n");
        sbf(b, "        free((long *)d->data);\n    }\n");
        break;
    case REF_SLICE:
        sbf(b, "    {\n        struct rs_slice *s = (struct rs_slice *)p;\n");
        if (inner->kind == TY_SLICE && cg_needs_drop(inner->elem)) {
            const char *g = cg_drop_glue(inner->elem);
            sbf(b, "        long i;\n        for (i = 0; i < s->len; i++) %s(s->base + i * %d);\n", g, cg_words(inner->elem));
        }
        if (inner->kind == TY_STR || cg_is_u8(inner->elem))
            sbf(b, "        if (s->base) free((long *)(s->base >> 3));\n    }\n");
        else
            sbf(b, "        if (s->base) free((long *)s->base);\n    }\n");
        break;
    case REF_BYTE:
        sbf(b, "    free((long *)((*(long *)p) >> 3));\n");
        break;
    default:
        sbf(b, "    {\n        long a = *(long *)p;\n");
        if (cg_needs_drop(inner)) {
            const char *g = cg_drop_glue(inner);
            if (g) sbf(b, "        %s(a);\n", g);
        }
        sbf(b, "        free((long *)a);\n    }\n");
        break;
    }
}

static void cgt_gen_drop(Inst *in) {
    Type *t = in->ty;
    StrBuf b;
    int i;
    sb_init(&b);
    sbf(&b, "/* drop %s */\nstatic long %s(long p) {\n", cgt_cmt(ty_str(t)), in->name);
    switch (t->kind) {
    case TY_TUPLE:
        for (i = 0; i < t->nargs; i++) {
            char m[40];
            sprintf(m, "f%d", i);
            cgt_drop_call(&b, t->args[i], cgt_member_addr(t, m));
        }
        break;
    case TY_ARRAY:
        if (t->len > 0 && cg_needs_drop(t->elem)) {
            sbf(&b, "    {\n        long i;\n        for (i = 0; i < %ld; i++) {\n", t->len);
            cgt_drop_call(&b, t->elem, cgt_member_addr_idx(t));
            sbf(&b, "        }\n    }\n");
        }
        break;
    case TY_CLOSURE: {
        TInfo *ti = tinfo(t);
        for (i = 0; i < ti->ncaps; i++) {
            char m[40];
            if (ti->caps[i].mode != CAP_MOVE) continue;
            sprintf(m, "c%d", i);
            cgt_drop_call(&b, ti->captys[i], cgt_member_addr(t, m));
        }
        break;
    }
    case TY_ADT:
        if (cg_is_box(t)) {
            cgt_gen_box_drop(&b, t);
            break;
        }
        if (cgt_drop_impl(t)) {
            Sym *dfn = assoc_item(l_drop, NS_VALUE, name_intern_str("drop"));
            const char *nm = cg_fn_instance(dfn, 0, 0, t);
            if (nm) sbf(&b, "    %s(p);\n", nm);
        }
        if (t->def->kind == SYM_STRUCT) {
            Sym **fs;
            int nf = cgt_fields(t->def, &fs);
            for (i = 0; i < nf; i++)
                cgt_drop_call(&b, cgt_field_ty(fs[i], t), cgt_member_addr(t, cg_field(fs[i])));
        } else if (t->def->kind == SYM_ENUM) {
            Sym **vs;
            int nv = cgt_variants(t->def, &vs), v, first = 1;
            for (v = 0; v < nv; v++) {
                Sym **fs;
                int nf = cgt_fields(vs[v], &fs), any = 0;
                for (i = 0; i < nf; i++)
                    if (cg_needs_drop(cgt_field_ty(fs[i], t))) any = 1;
                if (!any) continue;
                if (first) sbf(&b, "    {\n        long tag = ((%s *)p)->tag;\n", cg_ctype(t));
                first = 0;
                sbf(&b, "        if (tag == %d) {\n", v);
                for (i = 0; i < nf; i++) {
                    Type *ft = cgt_field_ty(fs[i], t);
                    char m[300];
                    if (!cg_needs_drop(ft)) continue;
                    sprintf(m, "(long)&((%s *)p)->%s.%s", cg_ctype(t), cg_variant_member(vs[v]), cg_field(fs[i]));
                    sbf(&b, "    ");
                    cgt_drop_call(&b, ft, m);
                }
                sbf(&b, "        }\n");
            }
            if (!first) sbf(&b, "    }\n");
        }
        break;
    }
    sbf(&b, "    return 0;\n}\n");
    cgt_append(&cg_sect[CG_FUNCS], b.s, b.len);
    free(b.s);
    cgt_last_line = -1;
}

/* ================================================================== statics */

static Sym **pend_statics = 0;
static int npend = 0, cpend = 0, ndone_statics = 0;

const char *cg_static(Sym *s) {
    CgSym *c = cgs(s);
    StrBuf pb;
    Type *ty;
    cgt_init();
    if (c->gname) return c->gname;
    c->gname = cgt_name("G", cg_new_id());
    ty = cgt_inst(s->ty, &cgt_empty);
    sb_init(&pb);
    sym_path(s, &pb);
    cg_printf(CG_DATA, "/* static %s */\nstatic %s %s;\n", cgt_cmt(pb.s), cg_ctype(ty), c->gname);
    free(pb.s);
    if (npend == cpend) {
        cpend = cpend ? cpend * 2 : 16;
        pend_statics = (Sym **)xrealloc(pend_statics, sizeof(Sym *) * (size_t)cpend);
    }
    pend_statics[npend++] = s;
    return c->gname;
}

/* ================================================================== the driver */

static Sym *cgt_core_item(const char *mod, const char *name, int ns) {
    Sym *m;
    if (!core_root || !core_root->members) return 0;
    m = scope_get(core_root->members, NS_TYPE, name_intern_str(mod));
    if (!m || !m->members) return 0;
    return scope_get(m->members, ns, name_intern_str(name));
}

static Sym *cgt_inherent_fn(Sym *adt, const char *name) {
    int i;
    for (i = 0; i < resolve_nimpls; i++) {
        Sym *impl = resolve_impls[i], *item;
        Type *st;
        if (impl->trait || !impl->impl_self) continue;
        st = ty_resolve(impl->impl_self);
        if (st->kind != TY_ADT || st->def != adt) continue;
        item = scope_get(impl->members, NS_VALUE, name_intern_str(name));
        if (item && item->kind == SYM_FN) return item;
    }
    return 0;
}

static void cgt_run(Inst *in) {
    cgt_last_line = -1;
    switch (in->kind) {
    case INST_FN:
        cg_subst = &in->sub;
        cg_cur_fn = in->fn;
        collect_set_env(in->fn);
        cg_fn_body(in->fn, in->name);
        break;
    case INST_CLO:
        cg_subst = &in->sub;
        cg_cur_fn = 0;
        collect_set_env(0);
        cg_closure_body(in->node, in->name);
        break;
    case INST_DROP:
        cg_subst = 0;
        cg_cur_fn = 0;
        cgt_gen_drop(in);
        break;
    case INST_SHIM:
        cg_subst = 0;
        cg_cur_fn = 0;
        cgt_gen_shim(in);
        break;
    }
    cg_subst = 0;
    cg_cur_fn = 0;
    collect_set_env(0);
}

/* the C statements that call main and report an `Err` result */
static void cgt_main_tail(StrBuf *tail, const char *mname, Type *ret) {
    Sym *result = lang_item("result");
    Type *rt = ty_resolve(ret);
    if (rt->kind == TY_ADT && rt->def == result) {
        Sym **vs, *err = 0, **fs, *debug_tr, *write_tr, *rawout, *stderr_fn, *fmt_new, *fmt_write_str, *dbg_fmt;
        Type *et, *raw_ty, *fm_ty, *dyn_ty;
        const char *dbg, *sraw, *fnew, *fws, *vt, *l1, *l2;
        int nv = cgt_variants(rt->def, &vs), i;
        fs = 0;
        for (i = 0; i < nv; i++)
            if (vs[i]->name && strcmp(vs[i]->name->s, "Err") == 0) err = vs[i];
        debug_tr = cgt_core_item("fmt", "Debug", NS_TYPE);
        write_tr = cgt_core_item("fmt", "Write", NS_TYPE);
        rawout = cgt_core_item("io", "RawOut", NS_TYPE);
        stderr_fn = cgt_core_item("io", "stderr_raw", NS_VALUE);
        fmt_new = cgt_inherent_fn(lang_item("formatter"), "new");
        fmt_write_str = cgt_inherent_fn(lang_item("formatter"), "write_str");
        if (!err || !debug_tr || !write_tr || !rawout || !stderr_fn || !fmt_new || !fmt_write_str ||
            cgt_fields(err, &fs) < 1)
            error_at(-1, 0, "code generator: cannot report the error of `main` (core items missing)");
        dbg_fmt = assoc_item(debug_tr, NS_VALUE, name_intern_str("fmt"));
        et = cgt_field_ty(fs[0], rt);
        raw_ty = rawout->ty;
        fm_ty = lang_item("formatter")->ty;
        dyn_ty = ty_dyn(write_tr, 0, 0);
        dbg = cg_fn_instance(dbg_fmt, 0, 0, et);
        sraw = cg_fn_instance(stderr_fn, 0, 0, 0);
        fnew = cg_fn_instance(fmt_new, 0, 0, 0);
        fws = cg_fn_instance(fmt_write_str, 0, 0, 0);
        vt = cg_vtable(raw_ty, dyn_ty);
        l1 = cg_str_literal("Error: ", 7);
        l2 = cg_str_literal("\n", 1);
        if (!dbg || !sraw || !fnew || !fws)
            error_at(-1, 0, "code generator: cannot report the error of `main`");
        sbf(tail, "    {\n        %s r = %s();\n        if (r.tag == %d) {\n", cg_ctype(rt), mname, err->index);
        sbf(tail, "            %s raw = %s();\n", cg_ctype(raw_ty), sraw);
        sbf(tail, "            struct rs_dyn d;\n            d.data = (long)&raw;\n            d.vt = (long)%s;\n", vt);
        sbf(tail, "            %s fm = %s(d);\n", cg_ctype(fm_ty), fnew);
        sbf(tail, "            struct rs_slice s1;\n            s1.base = (long)%s * 8;\n            s1.len = 7;\n", l1);
        sbf(tail, "            %s((long)&fm, s1);\n", fws);
        sbf(tail, "            %s((long)&r.%s.%s, (long)&fm);\n", dbg, cg_variant_member(err), cg_field(fs[0]));
        sbf(tail, "            s1.base = (long)%s * 8;\n            s1.len = 1;\n", l2);
        sbf(tail, "            %s((long)&fm, s1);\n", fws);
        sbf(tail, "            return 1;\n        }\n    }\n");
    } else {
        sbf(tail, "    %s();\n", mname);
    }
}

static void cgt_drain(void) {
    for (;;) {
        while (qhead < qn) {
            Inst *in = queue[qhead++];
            cgt_run(in);
        }
        if (ndone_statics < npend) {
            Sym *s = pend_statics[ndone_statics++];
            cg_subst = 0;
            cg_cur_fn = 0;
            collect_set_env(0);
            cg_printf(CG_MAIN, "    {\n");
            cg_static_init(s, cgs(s)->gname);
            cg_printf(CG_MAIN, "    }\n");
            continue;
        }
        break;
    }
}

void cg_crate(FILE *out, const char *src_name, const char *rt_path) {
    Sym *mainfn;
    const char *mname;
    Type *mret;
    StrBuf tail;
    int f, i;
    cgt_init();
    cgt_src = src_name;
    cgt_lang();
    f = load_file(rt_path);
    if (f < 0) {
        fprintf(stderr, "rustc: can't open %s\n", display_path(rt_path));
        exit(2);
    }
    for (i = 0; i < files[f].len; i++) sb_put_cp(&cg_sect[CG_RT], files[f].text[i]);
    sb_putc(&cg_sect[CG_RT], '\n');

    mainfn = crate_root ? scope_get(crate_root->members, NS_VALUE, name_intern_str("main")) : 0;
    if (!mainfn || mainfn->kind != SYM_FN) error_at(-1, 0, "`main` function not found in crate");
    cg_subst = 0;
    cg_cur_fn = 0;
    collect_set_env(0);
    mname = cg_fn_instance(mainfn, 0, 0, 0);
    if (!mname) error_at(-1, 0, "`main` has no body");
    mret = cgt_inst(mainfn->ty->ret, &cgt_empty);
    cg_printf(CG_MAIN, "int main(int argc, char **argv) {\n");
    cg_printf(CG_MAIN, "    rs_argc = argc;\n    rs_argv = argv;\n");
    sb_init(&tail);
    cgt_main_tail(&tail, mname, mret);
    cgt_drain();
    cgt_append(&cg_sect[CG_MAIN], tail.s, tail.len);
    cg_printf(CG_MAIN, "    return 0;\n}\n");

    for (i = 0; i < CG_NSECT; i++) fputs(cg_sect[i].s, out);
}
