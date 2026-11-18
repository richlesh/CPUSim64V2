/*
 * cg_expr.c - code generation of function bodies (see design-codegen.md).
 *
 * cg_fn_body / cg_closure_body / cg_static_init lower the checked AST of one
 * fn instance to C.  A Rust expression becomes C statements appended to the
 * body buffer plus a C expression for its value (a literal, a local or a
 * temporary: never something with side effects), so the C evaluation order is
 * never relied on.  Locals and temporaries are declared at the top of the C
 * function.  Places (lvalues) are Place records; reads and writes of a `u8`
 * in packed memory go through __ldb / __stb on a byte address.
 *
 * Assumptions about cg_types.c beyond cg.h (reported to its author):
 *   - a closure struct has the fields c0, c1, ... one per entry of
 *     closure_captures() in that order (by reference: `long` address, by move:
 *     the value);
 *   - cg_closure_fn(Type *closure_ty) gives the body function of a closure
 *     TYPE (the creation site and the call site may be in different instances);
 *   - vtable entries of Fn/FnMut/FnOnce for closures, fn items and fn pointers
 *     take (long data, spread arguments...), not a tuple.
 *
 * Runtime functions called (lib/rust_rt.c): see RT_API at the end of this file.
 *
 * Everything here is C99 that the CPUSim64 C11 compiler also accepts.
 */

/* ================================================================== state */

#define GE_LINEBUF 16384

static StrBuf *ge_out;            /* where statements are appended */
static StrBuf ge_body;            /* the body of the C function being generated */
static StrBuf ge_decl;            /* its declarations: locals, temporaries, drop flags */
static char ge_lbuf[GE_LINEBUF];
static int ge_gen = 0;            /* generation counter: invalidates the Local table */

const char *cg_closure_fn(Type *closure_ty);   /* cg_types.c: the body function of a closure type */

typedef struct Local Local;
typedef struct Place Place;

enum { LK_VAR = 1, LK_CAPREF, LK_CAPVAL };

struct Local {
    Sym *sym;
    Local *next;
    const char *name;      /* LK_VAR: the C variable */
    Type *ty;              /* mono type of the variable */
    int kind;
    const char *field;     /* LK_CAP*: "c<i>" */
    const char *flag;      /* drop flag (a C variable) or 0 */
};

#define GE_LHASH 256
static Local *ge_locals[GE_LHASH];

typedef struct {
    const char *addr;      /* C lvalue of the value to drop */
    Type *ty;
    const char *flag;      /* 0: always drop */
} DropEnt;

enum { SC_FN = 1, SC_BLOCK, SC_STMT };

typedef struct {
    int kind;
    DropEnt *v;
    int n, cap;
} GScope;

static GScope *ge_sc;
static int ge_nsc, ge_csc;

typedef struct {
    Name *label;
    int id;
    int sdepth;            /* number of scopes outside the loop */
    const char *res;       /* result temp of `loop` / labeled block, or 0 */
    Type *rty;
    int is_loop;           /* 0: labeled block (continue not allowed) */
    int is_while;          /* `break` with a value not allowed */
} GLoop;

static GLoop *ge_loops;
static int ge_nloops, ge_cloops;

static Node *ge_closure;          /* the closure being generated (0 in a fn) */
static Capture *ge_caps;
static int ge_ncaps;
static Type *ge_ret;              /* mono return type of the C function */
static int ge_ret_scope;          /* scope depth of the fn scope (0) */
static const char *ge_envtype;    /* closure struct type for env accesses */
static Node *ge_extend;           /* the operand of `let x = &temp`: lives to the end of the block */

static const char ge_never_s[] = "/*never*/0";
#define GE_NEVER ((const char *)ge_never_s)

/* ================================================================== output helpers */

static void E(const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(ge_lbuf, GE_LINEBUF, fmt, ap);
    va_end(ap);
    if (n < 0 || n >= GE_LINEBUF) fatal("code generator: output line too long");
    /* every C line of a statement gets the statement's #line (see cg_line_resync),
       unless the previous line already is that directive */
    if (n > 0 && (ge_out->len == 0 || ge_out->s[ge_out->len - 1] == '\n')) {
        int p = ge_out->len - 1;
        while (p > 0 && ge_out->s[p - 1] != '\n') p--;
        if (!(ge_out->len > 0 && p >= 0 && ge_out->len - p > 6 && strncmp(ge_out->s + p, "#line ", 6) == 0))
            cg_line_resync(ge_out);
    }
    sb_puts(ge_out, ge_lbuf);
}

static char *S(const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(ge_lbuf, GE_LINEBUF, fmt, ap);
    va_end(ap);
    if (n < 0 || n >= GE_LINEBUF) fatal("code generator: expression too long");
    return xstrdup(ge_lbuf);
}

static void ge_decl_line(const char *ctype, const char *name, const char *init) {
    sb_puts(&ge_decl, "    ");
    sb_puts(&ge_decl, ctype);
    sb_putc(&ge_decl, ' ');
    sb_puts(&ge_decl, name);
    if (init) {
        sb_puts(&ge_decl, " = ");
        sb_puts(&ge_decl, init);
    }
    sb_puts(&ge_decl, ";\n");
}

static const char *ge_tmp(const char *ctype) {
    char *n = S("t%d", cg_new_id());
    ge_decl_line(ctype, n, 0);
    return n;
}

static Type *gm(Type *t) {
    return t ? cg_mono(t) : 0;
}

static Type *gnt(Node *n) {
    return n && n->ty ? cg_mono((Type *)n->ty) : 0;
}

static const char *ge_ct(Type *t) {
    return cg_ctype(t);
}

static const char *ge_tmp_ty(Type *t) {
    return ge_tmp(ge_ct(t));
}

static int ge_is_never(Type *t) {
    return t && t->kind == TY_NEVER;
}

/* a value that can be reread without surprise: a literal or a temporary */
static int ge_stable(const char *v) {
    const char *p = v;
    if (v == GE_NEVER) return 1;
    if (*p == 't') {
        p++;
        if (*p < '0' || *p > '9') return 0;
        while (*p >= '0' && *p <= '9') p++;
        return *p == 0;
    }
    if (*p == '-') p++;
    if (*p < '0' || *p > '9') return 0;
    while (*p >= '0' && *p <= '9') p++;
    return *p == 0;
}

/* a value of the (unsized or sized) type t for a diverged computation */
static const char *ge_fix(const char *v, Type *want) {
    const char *c;
    if (v != GE_NEVER) return v;
    if (!want) return "0";
    c = ge_ct(want);
    if (strcmp(c, "long") == 0 || strcmp(c, "double") == 0) return "0";
    return ge_tmp(c);
}

static const char *ge_snap(Type *t, const char *v) {
    const char *tmp;
    if (ge_stable(v)) return v;
    if (!t || cg_is_zst(t)) return "0";
    tmp = ge_tmp_ty(t);
    E("%s = %s;\n", tmp, v);
    return tmp;
}

/* ================================================================== names, locals */

static char *ge_cname(Name *n) {
    char buf[96];
    int i = 0;
    const char *s = n ? n->s : "_";
    while (*s && i < 60) {
        int c = *s++ & 0xFF;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') buf[i++] = (char)c;
        else buf[i++] = '_';
    }
    buf[i] = 0;
    return S("l_%s_%d", buf, cg_new_id());
}

static unsigned ge_lh(Sym *s) {
    return (unsigned)(((size_t)s >> 4) & (GE_LHASH - 1));
}

static Local *ge_local_find(Sym *s) {
    Local *l;
    for (l = ge_locals[ge_lh(s)]; l; l = l->next)
        if (l->sym == s) return l;
    return 0;
}

static Local *ge_local_new(Sym *s, Type *t, int kind) {
    Local *l = (Local *)xalloc(sizeof(Local));
    unsigned h = ge_lh(s);
    l->sym = s;
    l->ty = t;
    l->kind = kind;
    l->next = ge_locals[h];
    ge_locals[h] = l;
    return l;
}

static void ge_locals_reset(void) {
    int i;
    for (i = 0; i < GE_LHASH; i++) ge_locals[i] = 0;
}

/* ================================================================== scopes and drops */

static void ge_push_scope(int kind) {
    if (ge_nsc >= ge_csc) {
        ge_csc = ge_csc ? ge_csc * 2 : 32;
        ge_sc = (GScope *)xrealloc(ge_sc, sizeof(GScope) * (size_t)ge_csc);
    }
    ge_sc[ge_nsc].kind = kind;
    ge_sc[ge_nsc].v = 0;
    ge_sc[ge_nsc].n = 0;
    ge_sc[ge_nsc].cap = 0;
    ge_nsc++;
}

static void ge_scope_add(int si, const char *addr, Type *t, const char *flag) {
    GScope *s = &ge_sc[si];
    if (s->n >= s->cap) {
        s->cap = s->cap ? s->cap * 2 : 4;
        s->v = (DropEnt *)xrealloc(s->v, sizeof(DropEnt) * (size_t)s->cap);
    }
    s->v[s->n].addr = addr;
    s->v[s->n].ty = t;
    s->v[s->n].flag = flag;
    s->n++;
}

static void ge_emit_drop(DropEnt *d) {
    const char *glue = cg_drop_glue(d->ty);
    if (!glue) return;
    if (d->flag) E("if (%s) { %s = 0; %s((long)&%s); }\n", d->flag, d->flag, glue, d->addr);
    else E("%s((long)&%s);\n", glue, d->addr);
}

/* drops every entry of the scopes with index >= depth, innermost first (no pop) */
static void ge_drops_to(int depth) {
    int i, j;
    for (i = ge_nsc - 1; i >= depth; i--)
        for (j = ge_sc[i].n - 1; j >= 0; j--) ge_emit_drop(&ge_sc[i].v[j]);
}

static void ge_pop_scope(int emit) {
    int j;
    GScope *s = &ge_sc[ge_nsc - 1];
    if (emit)
        for (j = s->n - 1; j >= 0; j--) ge_emit_drop(&s->v[j]);
    ge_nsc--;
}

/* the innermost scope of the given kind that is a block (for extended temporaries) */
static int ge_block_scope(void) {
    int i;
    for (i = ge_nsc - 1; i >= 0; i--)
        if (ge_sc[i].kind == SC_BLOCK || ge_sc[i].kind == SC_FN) return i;
    return 0;
}

/* a value that needs dropping is held in `name` (a C variable) with its own drop flag */
static const char *ge_new_flag(void) {
    char *f = S("df%d", cg_new_id());
    ge_decl_line("long", f, "0");
    return f;
}

/* ================================================================== integer helpers */

static int ge_bits(Type *t) {
    switch (t->ik) {
    case IK_I8: case IK_U8: return 8;
    case IK_I16: case IK_U16: return 16;
    case IK_I32: case IK_U32: return 32;
    }
    return 64;
}

static int ge_signed(Type *t) {
    return t->ik >= IK_I8 && t->ik <= IK_ISIZE;
}

static int ge_is_int(Type *t) {
    return t && t->kind == TY_INT;
}

static int ge_is_float(Type *t) {
    return t && t->kind == TY_FLOAT;
}

static const char *ge_imin(Type *t) {
    if (!ge_signed(t)) return "0";
    switch (ge_bits(t)) {
    case 8: return "(-128)";
    case 16: return "(-32768)";
    case 32: return "(-2147483648)";
    }
    return "(-9223372036854775807-1)";
}

static const char *ge_imax(Type *t) {
    int s = ge_signed(t);
    switch (ge_bits(t)) {
    case 8: return s ? "127" : "255";
    case 16: return s ? "32767" : "65535";
    case 32: return s ? "2147483647" : "4294967295";
    }
    return s ? "9223372036854775807" : "(-1)";
}

/* wraps the long x to the integer type t (as an `as` cast does) */
static const char *ge_norm(Type *t, const char *x) {
    int b = ge_bits(t);
    if (b == 64) return x;
    if (ge_signed(t)) {
        if (b == 8) return S("((((%s) & 255) ^ 128) - 128)", x);
        if (b == 16) return S("((((%s) & 65535) ^ 32768) - 32768)", x);
        return S("((((%s) & 4294967295) ^ 2147483648) - 2147483648)", x);
    }
    if (b == 8) return S("((%s) & 255)", x);
    if (b == 16) return S("((%s) & 65535)", x);
    return S("((%s) & 4294967295)", x);
}

static int ge_unsigned64(Type *t) {
    return ge_is_int(t) && !ge_signed(t) && ge_bits(t) == 64;
}

/* ================================================================== panics */

static const char *ge_file_name(Node *n) {
    int f = n ? n->file : 0;
    if (f < 0 || f >= nfiles) f = 0;
    return display_path(files[f].path);
}

static const char *ge_strlit(const char *s) {
    long len = (long)strlen(s);
    const char *l = cg_str_literal(s, len);
    return S("(long)%s * 8, %ld", l, len);
}

static void ge_panic(Node *at, const char *msg) {
    E("rs_panic(%s, %d, %d, %s);\n", ge_strlit(ge_file_name(at)), at ? at->line : 0, at ? at->col : 0, ge_strlit(msg));
}

static void ge_panic_if(Node *at, const char *cond, const char *msg) {
    E("if (%s) ", cond);
    ge_panic(at, msg);
}

/* ================================================================== arithmetic */

static const char *ge_arith_name(int op) {
    switch (op) {
    case OP_ADD: return "add";
    case OP_SUB: return "subtract";
    case OP_MUL: return "multiply";
    }
    return "?";
}

/* integer or float binary operation with rustc's debug-build checks; a, b are stable values */
static const char *ge_binop(Node *at, int op, Type *t, const char *a, const char *b, Type *bt) {
    const char *r;
    char *msg;
    if (ge_is_float(t)) {
        switch (op) {
        case OP_ADD: return S("(%s + %s)", a, b);
        case OP_SUB: return S("(%s - %s)", a, b);
        case OP_MUL: return S("(%s * %s)", a, b);
        case OP_DIV: return S("(%s / %s)", a, b);
        case OP_REM: return S("rs_fmod(%s, %s)", a, b);
        }
        return "0";
    }
    if (t->kind == TY_BOOL) {
        switch (op) {
        case OP_BITAND: return S("(%s & %s)", a, b);
        case OP_BITOR: return S("(%s | %s)", a, b);
        case OP_BITXOR: return S("(%s ^ %s)", a, b);
        }
        return "0";
    }
    switch (op) {
    case OP_BITAND: return S("(%s & %s)", a, b);
    case OP_BITOR: return S("(%s | %s)", a, b);
    case OP_BITXOR: return S("(%s ^ %s)", a, b);
    case OP_ADD: case OP_SUB: case OP_MUL: {
        int b64 = ge_bits(t) == 64, sg = ge_signed(t);
        r = ge_tmp("long");
        msg = S("attempt to %s with overflow", ge_arith_name(op));
        if (op == OP_ADD) {
            if (!b64) {
                E("%s = %s + %s;\n", r, a, b);
            } else {
                E("%s = (long)((unsigned long)%s + (unsigned long)%s);\n", r, a, b);
                if (sg) ge_panic_if(at, S("((%s ^ %s) & (%s ^ %s)) < 0", a, r, b, r), msg);
                else ge_panic_if(at, S("(unsigned long)%s < (unsigned long)%s", r, a), msg);
                return r;
            }
        } else if (op == OP_SUB) {
            if (!b64) {
                E("%s = %s - %s;\n", r, a, b);
            } else if (sg) {
                E("%s = (long)((unsigned long)%s - (unsigned long)%s);\n", r, a, b);
                ge_panic_if(at, S("((%s ^ %s) & (%s ^ %s)) < 0", a, b, a, r), msg);
                return r;
            } else {
                ge_panic_if(at, S("(unsigned long)%s < (unsigned long)%s", a, b), msg);
                E("%s = %s - %s;\n", r, a, b);
                return r;
            }
        } else {
            if (b64) {
                E("%s = rs_wmul(%s, %s);\n", r, a, b);
                ge_panic_if(at, S("rs_mul_ovf_%s(%s, %s)", sg ? "i64" : "u64", a, b), msg);
                return r;
            }
            if (sg) E("%s = %s * %s;\n", r, a, b);
            else E("%s = (long)((unsigned long)%s * (unsigned long)%s);\n", r, a, b);
        }
        /* narrow: range check */
        if (sg) ge_panic_if(at, S("%s < %s || %s > %s", r, ge_imin(t), r, ge_imax(t)), msg);
        else if (op == OP_SUB) ge_panic_if(at, S("%s < 0", r), msg);
        else ge_panic_if(at, S("(unsigned long)%s > %s", r, ge_imax(t)), msg);
        return r;
    }
    case OP_DIV: case OP_REM: {
        int dv = op == OP_DIV;
        r = ge_tmp("long");
        ge_panic_if(at, S("%s == 0", b), dv ? "attempt to divide by zero" : "attempt to calculate the remainder with a divisor of zero");
        if (ge_signed(t))
            ge_panic_if(at, S("%s == -1 && %s == %s", b, a, ge_imin(t)),
                        dv ? "attempt to divide with overflow" : "attempt to calculate the remainder with overflow");
        if (ge_unsigned64(t)) E("%s = (long)((unsigned long)%s %s (unsigned long)%s);\n", r, a, dv ? "/" : "%", b);
        else E("%s = %s %s %s;\n", r, a, dv ? "/" : "%", b);
        return r;
    }
    case OP_SHL: case OP_SHR: {
        int bits = ge_bits(t);
        int shl = op == OP_SHL;
        (void)bt;
        r = ge_tmp("long");
        ge_panic_if(at, S("(unsigned long)%s >= %d", b, bits), shl ? "attempt to shift left with overflow" : "attempt to shift right with overflow");
        if (shl) {
            E("%s = (long)((unsigned long)%s << %s);\n", r, a, b);
            if (bits < 64) E("%s = %s;\n", r, ge_norm(t, r));
        } else if (ge_unsigned64(t)) {
            E("%s = (long)((unsigned long)%s >> %s);\n", r, a, b);
        } else {
            E("%s = %s >> %s;\n", r, a, b);
        }
        return r;
    }
    }
    return "0";
}

/* comparison of scalars; a, b are values of type t */
static const char *ge_cmp(int op, Type *t, const char *a, const char *b) {
    const char *o = op == OP_EQ ? "==" : op == OP_NE ? "!=" : op == OP_LT ? "<" : op == OP_LE ? "<=" : op == OP_GT ? ">" : ">=";
    if (ge_unsigned64(t) && op != OP_EQ && op != OP_NE)
        return S("((unsigned long)%s %s (unsigned long)%s)", a, o, b);
    return S("(%s %s %s)", a, o, b);
}

static const char *ge_neg(Node *at, Type *t, const char *a) {
    const char *r;
    if (ge_is_float(t)) return S("(-%s)", a);
    r = ge_tmp("long");
    ge_panic_if(at, S("%s == %s", a, ge_imin(t)), "attempt to negate with overflow");
    E("%s = -%s;\n", r, a);
    return r;
}

static const char *ge_not(Type *t, const char *a) {
    if (t->kind == TY_BOOL) return S("(%s ^ 1)", a);
    if (ge_signed(t) || ge_bits(t) == 64) return S("(~%s)", a);
    return S("((~%s) & %s)", a, ge_imax(t));
}

/* ================================================================== casts */

static const char *ge_enum_disc(Type *from, const char *v) {
    Sym *en = from->def;
    int n = count_variants(en), i, plain = 1;
    for (i = 0; i < n; i++) {
        Sym *vs = variant_at(en, i);
        if (cg_discriminant(vs) != (long)i) plain = 0;
    }
    if (plain) return S("%s.tag", v);
    {
        const char *r = ge_tmp("long");
        for (i = 0; i < n; i++) {
            Sym *vs = variant_at(en, i);
            E("%sif (%s.tag == %d) %s = %ld;\n", i ? "else " : "", v, i, r, cg_discriminant(vs));
        }
        return r;
    }
}

static const char *ge_cast(Node *at, Type *from, Type *to, const char *v) {
    (void)at;
    if (to->kind == TY_INT) {
        if (from->kind == TY_INT || from->kind == TY_BOOL || from->kind == TY_CHAR) return ge_norm(to, v);
        if (from->kind == TY_FLOAT) {
            return S("rs_f64_to_int(%s, %d, %d)", v, ge_bits(to), ge_signed(to) ? 1 : 0);
        }
        if (from->kind == TY_ADT && from->def->kind == SYM_ENUM) return ge_norm(to, ge_enum_disc(from, v));
        if (from->kind == TY_PTR || from->kind == TY_REF || from->kind == TY_FN_PTR) {
            if (from->kind != TY_FN_PTR && cg_ref_kind(from->elem) >= REF_SLICE) return ge_norm(to, S("%s.base", v));
            return ge_norm(to, v);
        }
        return ge_norm(to, v);
    }
    if (to->kind == TY_FLOAT) {
        if (from->kind == TY_FLOAT) return v;
        if (ge_unsigned64(from)) return S("rs_u64_to_f64(%s)", v);
        return S("((double)%s)", v);
    }
    if (to->kind == TY_CHAR) return v;
    if (to->kind == TY_BOOL) return v;
    if (to->kind == TY_PTR || to->kind == TY_REF) {
        int fk = (from->kind == TY_PTR || from->kind == TY_REF) ? cg_ref_kind(from->elem) : REF_WORD;
        int tk = cg_ref_kind(to->elem);
        if (from->kind == TY_INT && tk >= REF_SLICE) return v;
        if (fk == tk) return v;
        /* fat -> thin: keep the data pointer */
        if (fk >= REF_SLICE && tk < REF_SLICE) return S("%s.%s", v, fk == REF_DYN ? "data" : "base");
        return v;
    }
    return v;
}

/* ================================================================== places */

enum { PK_LV = 1, PK_BYTE, PK_FAT };

struct Place {
    int kind;
    const char *e;         /* PK_LV: a C lvalue; PK_BYTE: a byte address; PK_FAT: the rs_slice / rs_dyn value */
    Type *ty;              /* mono type of the place */
    Local *root;           /* the variable whose drop flag a move out of this place clears */
    const char *addr;      /* PK_LV: the reference value of the place if known */
};

static const char *ex0(Node *e, Type *want);
static const char *ex_to(Node *e, Type *want);
static const char *ex(Node *e);
static Place ge_place_adj(Node *e, int mut);
static Place ge_place0(Node *e, int mut);
static const char *ge_block_value(Node *b, Type *want);
static const char *ge_call_callee(Node *at, Callee *c, const char **argv, int argc);

static int ge_is_name(const char *v) {
    const char *p = v;
    if (!*p) return 0;
    for (; *p; p++)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')) return 0;
    return 1;
}

static const char *ge_mk_slice(const char *base, const char *len) {
    const char *t = ge_tmp("struct rs_slice");
    E("%s.base = %s; %s.len = %s;\n", t, base, t, len);
    return t;
}

static const char *ge_mk_dyn(const char *data, const char *vt) {
    const char *t = ge_tmp("struct rs_dyn");
    E("%s.data = %s; %s.vt = %s;\n", t, data, t, vt);
    return t;
}

static Place ge_pl_make(int kind, const char *e, Type *ty, Local *root) {
    Place p;
    p.kind = kind;
    p.e = e;
    p.ty = ty;
    p.root = root;
    p.addr = 0;
    return p;
}

/* the place a reference value r (of type &pointee) points to */
static Place ge_deref_ref(const char *r, Type *pointee, Local *root) {
    int k = cg_ref_kind(pointee);
    Place p;
    if (k == REF_WORD) {
        if (!ge_is_name(r)) r = ge_snap(ty_int(IK_USIZE), r);
        p = ge_pl_make(PK_LV, S("(*(%s *)(%s))", ge_ct(pointee), r), pointee, root);
        p.addr = r;
        return p;
    }
    if (!ge_is_name(r)) r = ge_snap(k == REF_BYTE ? ty_int(IK_USIZE) : ty_ref(pointee, 0), r);
    if (k == REF_BYTE) return ge_pl_make(PK_BYTE, r, pointee, root);
    return ge_pl_make(PK_FAT, r, pointee, root);
}

static const char *ge_load(Place p) {
    if (p.kind == PK_BYTE) return S("__ldb(%s)", p.e);
    if (cg_is_zst(p.ty)) return "0";
    return p.e;
}

/* a read of a place as a value: a move for a type that needs dropping */
static const char *ge_take(Place p) {
    const char *v = ge_load(p);
    if (p.root && p.root->flag && cg_needs_drop(p.ty)) E("%s = 0;\n", p.root->flag);
    return v;
}

static void ge_store(Place p, const char *v) {
    if (p.kind == PK_BYTE) {
        E("__stb(%s, %s);\n", p.e, v);
        return;
    }
    if (p.kind == PK_FAT || cg_is_zst(p.ty) || v == GE_NEVER) return;
    E("%s = %s;\n", p.e, v);
}

/* the reference value (&place) */
static const char *ge_addr(Place p) {
    if (p.kind == PK_BYTE || p.kind == PK_FAT) return p.e;
    if (p.addr) return p.addr;
    if (cg_is_u8(p.ty)) return S("((long)&(%s) * 8 + 7)", p.e);
    return S("((long)&(%s))", p.e);
}

/* ---------------------------------------------------------------- locals as places */

static Place ge_local_place(Local *l) {
    Place p;
    if (l->kind == LK_VAR) return ge_pl_make(PK_LV, l->name, l->ty, l);
    if (l->kind == LK_CAPVAL) return ge_pl_make(PK_LV, S("(*(%s *)env).%s", ge_envtype, l->field), l->ty, 0);
    p = ge_deref_ref(S("(*(%s *)env).%s", ge_envtype, l->field), l->ty, 0);
    if (p.kind == PK_LV && !ge_is_name(p.addr)) {
        /* keep the env access as the address: it is pure */
        p.addr = S("(*(%s *)env).%s", ge_envtype, l->field);
        p.e = S("(*(%s *)(%s))", ge_ct(l->ty), p.addr);
    }
    return p;
}

/* declares a variable for the binding sym with the mono type t; a drop flag when the type needs dropping */
static Local *ge_declare(Sym *s, Type *t) {
    Local *l = ge_local_new(s, t, LK_VAR);
    l->name = ge_cname(s->name);
    ge_decl_line(ge_ct(t), l->name, 0);
    if (cg_needs_drop(t)) {
        l->flag = ge_new_flag();
        ge_scope_add(ge_block_scope(), l->name, t, l->flag);
    }
    return l;
}

/* a nameless variable holding a value, with a drop flag when needed; registered in the scope si */
static Local *ge_temp_local(const char *name, Type *t, int si) {
    Local *l = (Local *)xalloc(sizeof(Local));
    l->name = name;
    l->ty = t;
    l->kind = LK_VAR;
    if (cg_needs_drop(t)) {
        l->flag = ge_new_flag();
        ge_scope_add(si, name, t, l->flag);
    }
    return l;
}

static void ge_set_init(Local *l) {
    if (l && l->flag) E("%s = 1;\n", l->flag);
}

/* ---------------------------------------------------------------- calls to the Deref / Index traits */

static Sym *ge_trait_fn(Sym *trait, const char *name) {
    return assoc_item(trait, NS_VALUE, name_intern_str(name));
}

static const char *ge_call_trait(Node *at, Sym *trait, const char *name, Type *self, Type **targs, int nt,
                                 const char **argv, int argc) {
    Callee c;
    Sym *fn = ge_trait_fn(trait, name);
    memset(&c, 0, sizeof c);
    c.node = at;
    c.fn = fn;
    c.self = self;
    c.fnty = ty_fn_def(fn, targs, nt, 0, 0, 0);
    return ge_call_callee(at, &c, argv, argc);
}

/* Deref::deref / DerefMut::deref_mut on the place p: the reference to the target */
static const char *ge_overloaded_deref(Node *at, Place p, int mut) {
    const char *a[1];
    a[0] = ge_addr(p);
    return ge_call_trait(at, mut ? L_deref_mut : L_deref, mut ? "deref_mut" : "deref", p.ty, 0, 0, a, 1);
}

/* one auto-deref step of a place */
static Place ge_deref_place(Node *at, Place p, int mut) {
    Type *t = p.ty;
    if (t->kind == TY_REF || t->kind == TY_PTR) return ge_deref_ref(ge_load(p), t->elem, 0);
    if (cg_is_box(t)) return ge_deref_ref(ge_load(p), cg_box_inner(t), p.root);
    {
        /* overloaded: the target type is the pointee of the returned reference */
        Callee c;
        Sym *fn = ge_trait_fn(mut ? L_deref_mut : L_deref, mut ? "deref_mut" : "deref");
        Type *rt, *tgt;
        const char *r;
        memset(&c, 0, sizeof c);
        c.node = at;
        c.fn = fn;
        c.self = t;
        c.fnty = ty_fn_def(fn, 0, 0, 0, 0, 0);
        {
            Type **ta = 0;
            int nt = 0;
            Type *sig = 0;
            Sym *rf = cg_resolve_callee(&c, &ta, &nt, &sig);
            (void)rf;
            rt = sig ? gm(sig->ret) : 0;
        }
        tgt = rt && rt->kind == TY_REF ? rt->elem : 0;
        r = ge_overloaded_deref(at, p, mut);
        if (!tgt) fatal("code generator: overloaded deref without a target type");
        return ge_deref_ref(r, tgt, 0);
    }
}

/* the place of e after the auto-derefs recorded in e->adj */
static Place ge_place_adj(Node *e, int mut) {
    Place p = ge_place0(e, mut);
    int n = e->adj & 0xFF, i;
    for (i = 0; i < n; i++) p = ge_deref_place(e, p, mut);
    return p;
}

/* ---------------------------------------------------------------- struct / enum member access */

static const char *ge_field_member(Sym *f) {
    return cg_field(f);
}

static Sym *ge_find_named(Sym *owner, const char *name) {
    return find_field(owner, name_intern_str(name), 0);
}

static long ge_array_len(Type *t) {
    if (t->len_param) {
        Subst *s = cg_subst;
        int i;
        if (s)
            for (i = 0; i < s->n; i++)
                if (s->params[i] == t->len_param) {
                    Type *c = ty_resolve(s->args[i]);
                    if (c) return c->len;
                }
        return 0;
    }
    return t->len;
}

/* the value of a const generic parameter */
static long ge_const_generic(Sym *p) {
    Subst *s = cg_subst;
    int i;
    if (s)
        for (i = 0; i < s->n; i++)
            if (s->params[i] == p) {
                Type *c = ty_resolve(s->args[i]);
                if (c) return c->len;
            }
    return 0;
}

static const char *ge_arr_base(Place a, Type *at) {
    Type *el = at->elem;
    if (cg_is_u8(el)) return S("((long)&(%s).w[0] * 8)", a.e);
    return S("((long)&(%s).e[0])", a.e);
}

static void ge_panic_bounds(Node *at, const char *idx, const char *len) {
    E("rs_panic_bounds(%s, %d, %d, %s, %s);\n", ge_strlit(ge_file_name(at)), at->line, at->col, idx, len);
}

/* base[idx] with a usize index on an array or slice place */
static Place ge_index_int(Node *at, Place base, const char *idx) {
    Type *t = base.ty, *el = t->elem;
    if (t->kind == TY_ARRAY) {
        long n = ge_array_len(t);
        E("if ((unsigned long)%s >= %ld) ", idx, n);
        ge_panic_bounds(at, idx, S("%ld", n));
        if (cg_is_u8(el)) return ge_pl_make(PK_BYTE, S("((long)&(%s).w[0] * 8 + %s)", base.e, idx), el, 0);
        return ge_pl_make(PK_LV, S("(%s).e[%s]", base.e, idx), el, base.root);
    }
    E("if ((unsigned long)%s >= (unsigned long)%s.len) ", idx, base.e);
    ge_panic_bounds(at, idx, S("%s.len", base.e));
    if (cg_is_u8(el)) return ge_pl_make(PK_BYTE, S("(%s.base + %s)", base.e, idx), el, 0);
    return ge_pl_make(PK_LV, S("((%s *)(%s.base))[%s]", ge_ct(el), base.e, idx), el, 0);
}

/* base[range] on an array, slice or str place: a new fat place */
static Place ge_index_range(Node *at, Place base, Node *ix) {
    Type *rt = gnt(ix), *t = base.ty;
    const char *rv = ex_to(ix, rt), *lo = "0", *hi, *blen, *bbase, *res;
    Sym *def = rt->def;
    int incl = def == L_range_inclusive || def == L_range_to_inclusive;
    int has_lo = def == L_range || def == L_range_inclusive || def == L_range_from;
    int has_hi = def != L_range_from && def != L_range_full;
    Type *us = ty_int(IK_USIZE);
    rv = ge_snap(rt, rv);
    if (t->kind == TY_ARRAY) {
        blen = S("%ld", ge_array_len(t));
        bbase = ge_arr_base(base, t);
    } else {
        blen = S("%s.len", base.e);
        bbase = S("%s.base", base.e);
    }
    if (has_lo) lo = S("%s.%s", rv, ge_field_member(ge_find_named(def, "start")));
    hi = blen;
    if (has_hi) hi = S("%s.%s", rv, ge_field_member(ge_find_named(def, "end")));
    if (incl) {
        const char *h2 = ge_tmp("long");
        ge_panic_if(at, S("%s == -1", hi), "attempt to add with overflow");
        E("%s = %s + 1;\n", h2, hi);
        hi = h2;
    }
    lo = ge_snap(us, lo);
    hi = ge_snap(us, hi);
    if (t->kind == TY_STR)
        E("rs_check_str_range(%s, %d, %d, %s, %s, %s, %s);\n", ge_strlit(ge_file_name(at)), at->line, at->col, bbase, blen, lo, hi);
    else
        E("rs_check_range(%s, %d, %d, %s, %s, %s);\n", ge_strlit(ge_file_name(at)), at->line, at->col, lo, hi, blen);
    if (t->kind == TY_STR || cg_is_u8(t->elem)) res = ge_mk_slice(S("(%s + %s)", bbase, lo), S("(%s - %s)", hi, lo));
    else res = ge_mk_slice(S("((long)((%s *)(%s) + %s))", ge_ct(t->elem), bbase, lo), S("(%s - %s)", hi, lo));
    return ge_pl_make(PK_FAT, res, t->kind == TY_STR ? t : ty_slice(t->elem), 0);
}

static int ge_is_range_adt(Type *t) {
    return t && t->kind == TY_ADT &&
           (t->def == L_range || t->def == L_range_inclusive || t->def == L_range_from || t->def == L_range_to ||
            t->def == L_range_to_inclusive || t->def == L_range_full);
}

static Place ge_index_place(Node *e, int mut) {
    Callee *c = check_callee(e, 0);
    Place base = ge_place_adj(e->a, mut);
    Type *bt = base.ty;
    if (!c && (bt->kind == TY_ARRAY || bt->kind == TY_SLICE || bt->kind == TY_STR)) {
        Type *it = gnt(e->b);
        if (ge_is_range_adt(it)) return ge_index_range(e, base, e->b);
        {
            const char *idx = ge_snap(it, ex_to(e->b, it));
            return ge_index_int(e, base, idx);
        }
    }
    /* Index::index / IndexMut::index_mut */
    {
        Type *it = gnt(e->b), *ta[1], *et = gnt(e);
        const char *a[2], *r;
        ta[0] = it;
        a[0] = ge_addr(base);
        a[1] = ge_snap(it, ex_to(e->b, it));
        r = ge_call_trait(e, mut ? L_index_mut : L_index, mut ? "index_mut" : "index", bt, ta, 1, a, 2);
        return ge_deref_ref(r, et, 0);
    }
}

static Place ge_field_place(Node *e, int mut) {
    Place base = ge_place_adj(e->a, mut);
    Type *t = base.ty, *ft = gnt(e);
    const char *m;
    Place p;
    /* a zero-sized struct is a plain `long` in C: its fields are zero-sized
       too and share its address */
    if (cg_is_zst(t)) return ge_pl_make(PK_LV, base.e, ft, base.root);
    if (e->kind == N_E_TUPLE_FIELD && t->kind == TY_TUPLE) m = S("f%d", (int)e->ival);
    else m = ge_field_member((Sym *)e->sym);
    p = ge_pl_make(PK_LV, S("(%s).%s", base.e, m), ft, base.root);
    return p;
}

static Place ge_rvalue_place(Node *e, Type *t, const char *v) {
    const char *tmp;
    Local *l;
    Place p;
    if (v == GE_NEVER) v = ge_fix(v, t);
    tmp = ge_tmp_ty(t);
    if (!cg_is_zst(t)) E("%s = %s;\n", tmp, v);
    l = ge_temp_local(tmp, t, (e && e == ge_extend) ? ge_block_scope() : ge_nsc - 1);
    ge_set_init(l);
    p = ge_pl_make(PK_LV, tmp, t, l);
    return p;
}

static Place ge_place0(Node *e, int mut) {
    Type *t = gnt(e);
    switch (e->kind) {
    case N_E_PAREN:
        return ge_place_adj(e->a, mut);
    case N_E_PATH: {
        Sym *s = (Sym *)e->sym;
        if (s && s->kind == SYM_LOCAL) {
            Local *l = ge_local_find(s);
            if (!l) fatal("code generator: unknown local variable");
            return ge_local_place(l);
        }
        if (s && s->kind == SYM_STATIC) return ge_pl_make(PK_LV, cg_static(s), t, 0);
        break;
    }
    case N_E_FIELD: case N_E_TUPLE_FIELD:
        return ge_field_place(e, mut);
    case N_E_INDEX:
        return ge_index_place(e, mut);
    case N_E_DEREF: {
        Type *ot = gnt(e->a);
        Callee *c = check_callee(e, 0);
        if (ot->kind == TY_REF || ot->kind == TY_PTR) {
            const char *v = ex(e->a);
            return ge_deref_ref(v, ot->elem, 0);
        }
        if (cg_is_box(ot)) {
            Place bp = ge_place_adj(e->a, mut);
            return ge_deref_ref(ge_load(bp), cg_box_inner(ot), bp.root);
        }
        (void)c;
        {
            Place bp = ge_place_adj(e->a, mut);
            return ge_deref_place(e, bp, mut);
        }
    }
    }
    return ge_rvalue_place(e, t, ex0(e, t));
}

/* ================================================================== calls */

static const char *ge_join(const char **argv, int argc) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    for (i = 0; i < argc; i++) {
        if (i) sb_puts(&sb, ", ");
        sb_puts(&sb, argv[i]);
    }
    return sb.s;
}

/* emits the call `fexpr(args)` and returns its value */
static const char *ge_emit_call(const char *fexpr, const char **argv, int argc, Type *ret) {
    const char *call = S("%s(%s)", fexpr, ge_join(argv, argc)), *t;
    if (!ret || ge_is_never(ret)) {
        E("%s;\n", call);
        return GE_NEVER;
    }
    if (cg_is_zst(ret)) {
        E("%s;\n", call);
        return "0";
    }
    t = ge_tmp_ty(ret);
    E("%s = %s;\n", t, call);
    return t;
}

/* the C function pointer type for a call: ret (*)(first, params...) */
static const char *ge_fnptr_type(Type *ret, const char *first, Type **params, int np) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    sb_puts(&sb, ge_ct(ret ? ret : ty_unit()));
    sb_puts(&sb, " (*)(");
    if (first) sb_puts(&sb, first);
    for (i = 0; i < np; i++) {
        if (i || first) sb_puts(&sb, ", ");
        sb_puts(&sb, ge_ct(params[i]));
    }
    if (!np && !first) sb_puts(&sb, "void");
    sb_putc(&sb, ')');
    return sb.s;
}

static int ge_is_fn_trait(Sym *t) {
    return t && (t == L_fn || t == L_fn_mut || t == L_fn_once);
}

static int ge_is_callable_ty(Type *t) {
    return t && (t->kind == TY_CLOSURE || t->kind == TY_FN_DEF || t->kind == TY_FN_PTR);
}

/* ---------------------------------------------------------------- intrinsics */

static const char *ge_elem_addr(Type *T, const char *base, const char *i) {
    if (cg_is_u8(T)) return S("(%s + %s)", base, i);
    return S("((long)((%s *)(%s) + %s))", ge_ct(T), base, i);
}

static Place ge_elem_place(Type *T, const char *base, const char *i) {
    if (cg_is_u8(T)) return ge_pl_make(PK_BYTE, S("(%s + %s)", base, i), T, 0);
    return ge_pl_make(PK_LV, S("(*((%s *)(%s) + %s))", ge_ct(T), base, i), T, 0);
}

static const char *ge_with_overflow(Node *at, const char *name, Type *T, const char *a, const char *b, Type *rt) {
    const char *tup = ge_tmp_ty(rt), *r = ge_tmp("long"), *ovf = ge_tmp("long");
    int add = name[0] == 'a', sub = name[0] == 's', b64 = ge_bits(T) == 64, sg = ge_signed(T);
    const char *op = add ? "+" : sub ? "-" : "*";
    (void)at;
    if (b64) {
        if (!add && !sub) {
            E("%s = rs_wmul(%s, %s);\n", r, a, b);
            E("%s = rs_mul_ovf_%s(%s, %s);\n", ovf, sg ? "i64" : "u64", a, b);
        } else {
            E("%s = (long)((unsigned long)%s %s (unsigned long)%s);\n", r, a, op, b);
            if (add && sg) E("%s = ((%s ^ %s) & (%s ^ %s)) < 0;\n", ovf, a, r, b, r);
            else if (add) E("%s = (unsigned long)%s < (unsigned long)%s;\n", ovf, r, a);
            else if (sg) E("%s = ((%s ^ %s) & (%s ^ %s)) < 0;\n", ovf, a, b, a, r);
            else E("%s = (unsigned long)%s < (unsigned long)%s;\n", ovf, a, b);
        }
    } else {
        if (sg) E("%s = %s %s %s;\n", r, a, op, b);
        else E("%s = (long)((unsigned long)%s %s (unsigned long)%s);\n", r, a, op, b);
        if (sg) E("%s = %s < %s || %s > %s;\n", ovf, r, ge_imin(T), r, ge_imax(T));
        else if (sub) E("%s = %s < 0;\n", ovf, r);
        else E("%s = (unsigned long)%s > %s;\n", ovf, r, ge_imax(T));
        E("%s = %s;\n", r, ge_norm(T, r));
    }
    E("%s.f0 = %s; %s.f1 = %s;\n", tup, r, tup, ovf);
    return tup;
}

static const char *ge_intrinsic(Node *at, const char *name, Type **targs, int nt, Type *sig, const char **a, int argc) {
    Type *T = nt > 0 ? gm(targs[0]) : 0, *ret = sig ? gm(sig->ret) : ty_unit();
    const char *t;
    (void)argc;
    if (!strcmp(name, "elem_alloc")) {
        if (cg_is_u8(T)) return S("rs_alloc_bytes(%s)", a[0]);
        return S("rs_alloc(%s * %d)", a[0], cg_words(T));
    }
    if (!strcmp(name, "elem_realloc")) {
        if (cg_is_u8(T)) return S("rs_realloc_bytes(%s, %s, %s)", a[0], a[1], a[2]);
        return S("rs_realloc(%s, %s * %d, %s * %d)", a[0], a[1], cg_words(T), a[2], cg_words(T));
    }
    if (!strcmp(name, "elem_free")) {
        E("%s(%s);\n", cg_is_u8(T) ? "rs_free_bytes" : "rs_free", a[0]);
        return "0";
    }
    if (!strcmp(name, "elem_read")) {
        Place p = ge_elem_place(T, a[0], a[1]);
        if (cg_is_zst(T)) return "0";
        return ge_snap(T, ge_load(p));
    }
    if (!strcmp(name, "elem_write")) {
        Place p = ge_elem_place(T, a[0], a[1]);
        ge_store(p, a[2]);
        return "0";
    }
    if (!strcmp(name, "elem_offset")) {
        if (cg_is_u8(T)) return S("(%s + %s)", a[0], a[1]);
        return S("((long)((%s *)(%s) + %s))", ge_ct(T), a[0], a[1]);
    }
    if (!strcmp(name, "elem_ref") || !strcmp(name, "elem_ref_mut")) return ge_elem_addr(T, a[0], a[1]);
    if (!strcmp(name, "read_ref")) {
        Place p = ge_deref_ref(a[0], T, 0);
        if (cg_is_zst(T)) return "0";
        return ge_snap(T, ge_load(p));
    }
    if (!strcmp(name, "write_ref")) {
        Place p = ge_deref_ref(a[0], T, 0);
        ge_store(p, a[1]);
        return "0";
    }
    if (!strcmp(name, "forget")) return "0";
    if (!strcmp(name, "size_of")) return S("%d", cg_words(T) * 8);
    if (!strcmp(name, "transmute")) {
        Type *U = nt > 1 ? gm(targs[1]) : ret;
        if (T->kind == TY_FLOAT && U->kind != TY_FLOAT) return S("rs_f64_to_bits(%s)", a[0]);
        if (T->kind != TY_FLOAT && U->kind == TY_FLOAT) return S("rs_f64_from_bits(%s)", a[0]);
        return a[0];
    }
    if (!strcmp(name, "slice_base") || !strcmp(name, "str_base")) return S("%s.base", a[0]);
    if (!strcmp(name, "slice_len") || !strcmp(name, "str_len")) return S("%s.len", a[0]);
    if (!strcmp(name, "slice_from_raw") || !strcmp(name, "slice_from_raw_mut") || !strcmp(name, "str_from_raw"))
        return ge_mk_slice(a[0], a[1]);
    if (!strcmp(name, "box_new")) {
        t = ge_tmp("long");
        if (cg_is_u8(T)) {
            E("%s = rs_alloc(1) * 8 + 7;\n", t);
            E("__stb(%s, %s);\n", t, a[0]);
        } else {
            E("%s = rs_alloc(%d);\n", t, cg_words(T));
            if (!cg_is_zst(T)) E("*(%s *)%s = %s;\n", ge_ct(T), t, a[0]);
        }
        return t;
    }
    if (!strcmp(name, "box_deref") || !strcmp(name, "box_deref_mut")) {
        int k = cg_ref_kind(T);
        if (k == REF_SLICE) return ge_snap(ty_ref(T, 0), S("(*(struct rs_slice *)(%s))", a[0]));
        if (k == REF_DYN) return ge_snap(ty_ref(T, 0), S("(*(struct rs_dyn *)(%s))", a[0]));
        return ge_snap(ty_int(IK_USIZE), S("(*(long *)(%s))", a[0]));
    }
    if (!strcmp(name, "box_drop")) {
        int k = cg_ref_kind(T);
        if (k == REF_WORD || k == REF_BYTE) {
            const char *p = ge_tmp("long"), *g = cg_is_u8(T) ? 0 : cg_drop_glue(T);
            E("%s = *(long *)(%s);\n", p, a[0]);
            if (g) E("%s(%s);\n", g, p);
            E("rs_free(%s);\n", k == REF_BYTE ? S("%s / 8", p) : p);
        } else if (k == REF_SLICE) {
            const char *s = ge_tmp("struct rs_slice");
            Type *el = T->kind == TY_SLICE ? T->elem : 0;
            const char *g = el && !cg_is_u8(el) ? cg_drop_glue(el) : 0;
            E("%s = *(struct rs_slice *)(%s);\n", s, a[0]);
            if (g) {
                const char *i = ge_tmp("long");
                E("for (%s = 0; %s < %s.len; %s++) %s((long)((%s *)(%s.base) + %s));\n", i, i, s, i, g, ge_ct(el), s, i);
            }
            if (!el || cg_is_u8(el)) E("rs_free(%s.base / 8);\n", s);
            else E("rs_free(%s.base);\n", s);
        } else {
            const char *d = ge_tmp("struct rs_dyn");
            E("%s = *(struct rs_dyn *)(%s);\n", d, a[0]);
            E("if (((long *)%s.vt)[0]) ((long (*)(long))(((long *)%s.vt)[0]))(%s.data);\n", d, d, d);
            E("rs_free(%s.data);\n", d);
        }
        return "0";
    }
    if (!strcmp(name, "wrapping_add") || !strcmp(name, "wrapping_sub") || !strcmp(name, "wrapping_mul")) {
        const char *op = name[9] == 'a' ? "+" : name[9] == 's' ? "-" : "*";
        return ge_norm(T, S("(long)((unsigned long)%s %s (unsigned long)%s)", a[0], op, a[1]));
    }
    if (!strcmp(name, "add_with_overflow") || !strcmp(name, "sub_with_overflow") || !strcmp(name, "mul_with_overflow"))
        return ge_with_overflow(at, name, T, a[0], a[1], ret);
    if (!strcmp(name, "f64_to_bits")) return S("rs_f64_to_bits(%s)", a[0]);
    if (!strcmp(name, "f64_from_bits")) return S("rs_f64_from_bits(%s)", a[0]);
    if (!strcmp(name, "sqrtf64")) return S("rs_sqrt(%s)", a[0]);
    if (!strcmp(name, "floorf64")) return S("rs_floor(%s)", a[0]);
    if (!strcmp(name, "ceilf64")) return S("rs_ceil(%s)", a[0]);
    if (!strcmp(name, "fmt_f64")) return ge_snap(ty_int(IK_USIZE), S("rs_fmt_f64(%s, %s, %s, %s, %s)", a[0], a[1], a[2], a[3], a[4]));
    if (!strcmp(name, "parse_f64")) return ge_snap(ty_int(IK_USIZE), S("rs_parse_f64(%s, %s, %s)", a[0], a[1], a[2]));
    if (!strcmp(name, "write_bytes")) {
        E("rs_write_bytes(%s, %s, %s);\n", a[0], a[1], a[2]);
        return "0";
    }
    if (!strcmp(name, "read_byte")) return ge_snap(ty_int(IK_USIZE), S("rs_read_byte(%s)", a[0]));
    if (!strcmp(name, "arg_count")) return ge_snap(ty_int(IK_USIZE), S("rs_argc"));
    if (!strcmp(name, "arg_char")) return ge_snap(ty_int(IK_USIZE), S("rs_arg_char(%s, %s)", a[0], a[1]));
    if (!strcmp(name, "exit")) {
        E("rs_exit(%s);\n", a[0]);
        return GE_NEVER;
    }
    if (!strcmp(name, "abort")) {
        E("rs_abort();\n");
        return GE_NEVER;
    }
    fatal(S("code generator: unknown intrinsic `%s`", name));
    return "0";
}

/* ---------------------------------------------------------------- calling closures, fn items, fn pointers */

/* calls a value of callable type u (a closure, fn item or fn pointer) given as the place fp, with spread args */
static const char *ge_call_callable(Node *at, Type *u, Place fp, int by_value, const char **argv, int argc) {
    if (u->kind == TY_CLOSURE) {
        const char *env, **av = (const char **)xalloc(sizeof(char *) * (size_t)(argc + 1));
        int i;
        const char *body = cg_closure_fn(u);
        env = ge_addr(fp);
        av[0] = env;
        for (i = 0; i < argc; i++) av[i + 1] = argv[i];
        if (by_value && fp.root && fp.root->flag) E("%s = 0;\n", fp.root->flag);
        return ge_emit_call(body, av, argc + 1, gm(u->ret));
    }
    if (u->kind == TY_FN_PTR) {
        const char *fv = ge_snap(ty_int(IK_USIZE), ge_load(fp));
        return ge_emit_call(S("((%s)(%s))", ge_fnptr_type(gm(u->ret), 0, u->params, u->nparams), fv), argv, argc, gm(u->ret));
    }
    {   /* a fn item */
        Type **ta = 0, *sig = 0;
        int nt = 0;
        Sym *fn = cg_resolve_fn_type(u, &ta, &nt, &sig);
        const char *name;
        if (fn->flags & SF_INTRINSIC) return ge_intrinsic(at, fn->name->s, ta, nt, sig, argv, argc);
        name = cg_fn_instance(fn, ta, nt, 0);
        return ge_emit_call(name, argv, argc, gm(sig->ret));
    }
}

/* the dynamic call of trait method fn through the dyn place dp; argv[0] is not used (the receiver) */
static const char *ge_call_dyn(Type *dyn_ty, Sym *fn, Place dp, Type *ret, Type **params, int np,
                               const char **argv, int argc) {
    const char *d = dp.e, *fp, **av = (const char **)xalloc(sizeof(char *) * (size_t)(argc + 1));
    int slot = cg_vtable_slot(dyn_ty, fn), i;
    if (!ge_is_name(d)) d = ge_snap(ty_ref(dyn_ty, 0), d);
    fp = S("((%s)(((long *)%s.vt)[%d]))", ge_fnptr_type(ret, "long", params, np), d, slot);
    av[0] = S("%s.data", d);
    for (i = 0; i < argc; i++) av[i + 1] = argv[i];
    return ge_emit_call(fp, av, argc + 1, ret);
}

/* a call through a callee record.  argv are the evaluated arguments of the trait method / fn
   (for a method the receiver first) */
static const char *ge_call_callee(Node *at, Callee *c, const char **argv, int argc) {
    Type **targs = 0, *sig = 0, *self = c->self ? gm(c->self) : 0, *ret;
    int nt = 0;
    Sym *fn;
    Sym *tr = (c->fn && c->fn->parent && c->fn->parent->kind == SYM_TRAIT) ? c->fn->parent : 0;
    const char *name;
    if (tr && self && ge_is_callable_ty(self) && ge_is_fn_trait(tr)) {
        /* Fn::call(&f, (args)): the tuple is spread */
        Type *tt = c->fnty && c->fnty->nparams > 1 ? gm(c->fnty->params[1]) : 0;
        const char *fv, **av;
        int i, n = (tt && tt->kind == TY_TUPLE) ? tt->nargs : 0;
        Place fp;
        (void)i;
        av = (const char **)xalloc(sizeof(char *) * (size_t)(n + 1));
        for (i = 0; i < n; i++) av[i] = S("%s.f%d", argv[1], i);
        fv = argv[0];
        if (tr == L_fn_once) fp = ge_rvalue_place(0, self, fv);
        else fp = ge_deref_ref(fv, self, 0);
        return ge_call_callable(at, self, fp, tr == L_fn_once, av, n);
    }
    if (tr == L_clone && self && ge_is_callable_ty(self)) {
        Place p = ge_deref_ref(argv[0], self, 0);
        return ge_load(p);
    }
    fn = cg_resolve_callee(c, &targs, &nt, &sig);
    ret = sig ? gm(sig->ret) : ty_unit();
    if (fn->flags & SF_INTRINSIC) return ge_intrinsic(at, fn->name->s, targs, nt, sig, argv, argc);
    if (tr && self && self->kind == TY_DYN) {
        Place dp = ge_deref_ref(argv[0], self, 0);   /* the receiver is the fat reference itself */
        dp.kind = PK_FAT;
        dp.e = argv[0];
        return ge_call_dyn(self, c->fn, dp, ret, sig->params + 1, sig->nparams - 1, argv + 1, argc - 1);
    }
    name = cg_fn_instance(fn, targs, nt, (fn->parent && (fn->parent->kind == SYM_TRAIT || fn->parent->kind == SYM_IMPL)) ? self : 0);
    if (!name && tr == L_clone && self) {
        /* the built-in Clone of a Copy type: a bitwise copy of *self */
        Place cp = ge_deref_ref(argv[0], self, 0);
        return ge_snap(self, ge_load(cp));
    }
    if (!name)
        error_at(at ? at->file : -1, at ? at->line : 0, "code generator: no instance for `%s` (self %s)",
                 fn->name ? fn->name->s : "?", self ? ty_str(self) : "-");
    return ge_emit_call(name, argv, argc, ret);
}

/* ================================================================== literals and constants */

static const char *ge_int_lit(long v) {
    if (v == (-9223372036854775807L - 1)) return "(-9223372036854775807-1)";
    if (v < 0) return S("(%ld)", v);
    return S("%ld", v);
}

/* the spelling of a float literal as a C double constant */
static const char *ge_float_lit(Node *e) {
    char buf[128];
    int i = 0, has = 0;
    const char *sp = e->str ? e->str : "0";
    for (; *sp && i < 100; sp++) {
        if (*sp == '_') continue;
        if (*sp == '.' || *sp == 'e' || *sp == 'E') has = 1;
        buf[i++] = *sp;
    }
    if (i > 0 && buf[i - 1] == '.') buf[i++] = '0';
    else if (!has) { buf[i++] = '.'; buf[i++] = '0'; }
    buf[i] = 0;
    return S("%s", buf);
}

static const char *ge_str_value(const char *bytes, long len) {
    const char *l = cg_str_literal(bytes, len);
    return ge_mk_slice(S("((long)%s * 8)", l), S("%ld", len));
}

static const char *ge_const_expr(Sym *c, Subst *sub, Type *want);

/* the self type of an associated-const path (T::C, Self::C, <T as Tr>::C) */
static Type *ge_path_self(Node *path) {
    Sym *s;
    if (path->flags & F_QUALIFIED) return gm(ty_from_ast(path->a));
    if (path->list.n < 2) return 0;
    s = (Sym *)path->list.v[0]->sym;
    if (!s) return 0;
    if (s->kind == SYM_GENERIC) return gm(ty_param(s));
    if (s->kind == SYM_SELF_TY) {
        if (cg_subst && cg_subst->self) return gm(cg_subst->self);
        if (s->parent && s->parent->kind == SYM_IMPL) return gm(s->parent->impl_self);
        return 0;
    }
    return gm(ty_from_path_prefix(path, (int)path->list.n - 1));
}

static const char *ge_assoc_const(Node *e, Sym *item) {
    Sym *par = item->parent;
    Type *want = gnt(e);
    if (par && par->kind == SYM_TRAIT) {
        Type *self = ge_path_self(e->a);
        Subst sub;
        Sym *impl;
        if (self) {
            memset(&sub, 0, sizeof sub);
            impl = find_impl(par, self, 0, 0, &sub);
            if (impl) {
                Sym *ci = assoc_item(impl, NS_VALUE, item->name);
                if (ci && ci->kind == SYM_CONST && ci->node && ci->node->b) return ge_const_expr(ci, &sub, want);
            }
            /* the trait's default */
            memset(&sub, 0, sizeof sub);
            sub.self = self;
            if (item->node && item->node->b) return ge_const_expr(item, &sub, want);
        }
        fatal("code generator: cannot resolve an associated const");
    }
    return ge_const_expr(item, par && par->kind == SYM_IMPL ? cg_subst : 0, want);
}

static int ge_const_depth = 0;

/* lowers the initializer of a const at the use site */
static const char *ge_const_expr(Sym *c, Subst *sub, Type *want) {
    Subst *saved = cg_subst;
    const char *v;
    Node *init = c->node ? c->node->b : 0;
    if (!init) fatal("code generator: const without a value");
    if (ge_const_depth > 64) fatal("code generator: const recursion");
    ge_const_depth++;
    cg_subst = sub;
    v = ex_to(init, want);
    cg_subst = saved;
    ge_const_depth--;
    return v;
}

/* ================================================================== aggregates */

static const char *ge_new_adt(Type *t, Sym *vs, Sym **fs, const char **vals, int n) {
    const char *tmp;
    int i;
    int is_enum = vs && vs->kind == SYM_VARIANT;
    if (cg_is_zst(t)) return "0";
    tmp = ge_tmp_ty(t);
    if (is_enum) E("%s.tag = %d;\n", tmp, cg_variant_index(vs));
    for (i = 0; i < n; i++) {
        Type *ft;
        if (vals[i] == GE_NEVER) continue;
        ft = 0;
        (void)ft;
        if (is_enum) E("%s.%s.%s = %s;\n", tmp, cg_variant_member(vs), ge_field_member(fs[i]), vals[i]);
        else E("%s.%s = %s;\n", tmp, ge_field_member(fs[i]), vals[i]);
    }
    return tmp;
}

/* a unit struct / unit variant value */
static const char *ge_unit_value(Sym *vs, Type *t) {
    return ge_new_adt(t, vs, 0, 0, 0);
}

static int ge_simple(Node *e) {
    if (!e) return 1;
    switch (e->kind) {
    case N_E_LIT_INT: case N_E_LIT_FLOAT: case N_E_LIT_STR: case N_E_LIT_BYTESTR: case N_E_LIT_CHAR:
    case N_E_LIT_BYTE: case N_E_LIT_BOOL: case N_E_PATH:
        return 1;
    case N_E_PAREN: case N_E_FIELD: case N_E_TUPLE_FIELD: case N_E_DEREF: case N_E_REF: case N_E_CAST:
        return ge_simple(e->a);
    case N_E_UNARY:
        return ge_simple(e->a);
    case N_E_BINARY:
        return ge_simple(e->a) && ge_simple(e->b);
    case N_E_TUPLE:
        {
            long i;
            for (i = 0; i < e->list.n; i++)
                if (!ge_simple(e->list.v[i])) return 0;
        }
        return 1;
    }
    return 0;
}

/* evaluates the nodes in order into values of the given types; earlier values are snapshotted
   when a later node could change them */
static void ge_eval_list(Node **nodes, int n, Type **types, const char **out) {
    int i, j;
    for (i = 0; i < n; i++) {
        Type *w = types ? types[i] : 0;
        const char *v = ex_to(nodes[i], w);
        int later = 0;
        for (j = i + 1; j < n; j++)
            if (!ge_simple(nodes[j])) later = 1;
        if (later && v != GE_NEVER) v = ge_snap(w ? w : gnt(nodes[i]), v);
        out[i] = v;
    }
}

/* ================================================================== unsizing and coercions */

static Type *ge_pointee(Type *t) {
    if (!t) return 0;
    if (t->kind == TY_REF || t->kind == TY_PTR) return t->elem;
    if (cg_is_box(t)) return cg_box_inner(t);
    return 0;
}

static const char *ge_unsize(Node *at, const char *v, Type *from, Type *to) {
    Type *A = ge_pointee(from), *B = to ? ge_pointee(to) : 0;
    (void)at;
    if (!A) return v;
    if (A->kind == TY_ARRAY && (!B || B->kind == TY_SLICE)) {
        const char *base = cg_is_u8(A->elem) ? S("(%s * 8)", v) : v;
        return ge_mk_slice(base, S("%ld", ge_array_len(A)));
    }
    if (B && B->kind == TY_DYN && A->kind != TY_DYN) return ge_mk_dyn(v, S("((long)%s)", cg_vtable(A, B)));
    return v;
}

/* shims: a C function without env for a capture-less closure / constructor used as a fn pointer */
static const char *ge_closure_shim(Type *ct) {
    StrBuf sb;
    const char *body = cg_closure_fn(ct), *name;
    int i;
    Type *ret = gm(ct->ret);
    name = S("Sh%d", cg_new_id());
    sb_init(&sb);
    sb_puts(&sb, ge_ct(ret));
    sb_putc(&sb, ' ');
    sb_puts(&sb, name);
    sb_putc(&sb, '(');
    for (i = 0; i < ct->nparams; i++) {
        if (i) sb_puts(&sb, ", ");
        sb_puts(&sb, ge_ct(gm(ct->params[i])));
        sb_puts(&sb, S(" a%d", i));
    }
    if (!ct->nparams) sb_puts(&sb, "void");
    sb_putc(&sb, ')');
    cg_printf(CG_PROTOS, "static %s;\n", sb.s);
    cg_printf(CG_FUNCS, "static %s {\n    long env = 0;\n    return %s(&env", sb.s, body);
    for (i = 0; i < ct->nparams; i++) cg_printf(CG_FUNCS, ", a%d", i);
    cg_printf(CG_FUNCS, ");\n}\n");
    return name;
}

static const char *ge_to_fnptr(Node *at, Type *cur, const char *v) {
    (void)v;
    (void)at;
    if (cur->kind == TY_FN_DEF) {
        Type **ta = 0, *sig = 0;
        int nt = 0;
        Sym *fn = cg_resolve_fn_type(cur, &ta, &nt, &sig);
        return S("((long)%s)", cg_fn_instance(fn, ta, nt, 0));
    }
    if (cur->kind == TY_CLOSURE) return S("((long)%s)", ge_closure_shim(cur));
    return v;
}

/* ================================================================== value of an expression with its adjustments */

static const char *ex_to(Node *e, Type *want) {
    int adj = e->adj;
    Type *cur = gnt(e);
    const char *v;
    if (adj & (0xFF | ADJ_AUTOREF | ADJ_AUTOREF_MUT)) {
        int mut = (adj & ADJ_AUTOREF_MUT) != 0;
        Place p = ge_place_adj(e, mut);
        if (adj & (ADJ_AUTOREF | ADJ_AUTOREF_MUT)) {
            v = ge_addr(p);
            cur = ty_ref(p.ty, mut);
        } else {
            v = ge_take(p);
            cur = p.ty;
        }
    } else {
        v = ex0(e, want);
    }
    if (v == GE_NEVER || ge_is_never(cur)) return GE_NEVER;
    if (adj & ADJ_UNSIZE) {
        v = ge_unsize(e, v, cur, want);
        if (want) cur = want;
    }
    if (want && ge_is_callable_ty(cur) && want->kind == TY_FN_PTR) v = ge_to_fnptr(e, cur, v);
    return v;
}

static const char *ex(Node *e) {
    return ex_to(e, 0);
}

/* ================================================================== operators */

static int ge_scalar(Type *t) {
    return t && (t->kind == TY_INT || t->kind == TY_FLOAT || t->kind == TY_BOOL || t->kind == TY_CHAR);
}

static int ge_is_unit_ty(Type *t) {
    return t && t->kind == TY_TUPLE && t->nargs == 0;
}

static const char *ge_binary(Node *e, Type *want) {
    int op = e->op;
    Type *lt = gnt(e->a), *rt = gnt(e->b);
    (void)want;
    if (op == OP_LAND || op == OP_LOR) {
        const char *r = ge_tmp("long"), *va, *vb;
        va = ex(e->a);
        E("%s = %s;\n", r, va);
        E("if (%s%s) {\n", op == OP_LAND ? "" : "!", r);
        vb = ex(e->b);
        if (vb != GE_NEVER) E("%s = %s;\n", r, vb);
        E("}\n");
        return r;
    }
    if (ge_scalar(lt) && ge_scalar(rt)) {
        const char *va, *vb;
        if (ge_simple(e->b)) {
            va = ex(e->a);
            vb = ex(e->b);
        } else {
            va = ge_snap(lt, ex(e->a));
            vb = ex(e->b);
        }
        va = ge_fix(va, lt);
        vb = ge_fix(vb, rt);
        if (op >= OP_EQ && op <= OP_GE) return ge_cmp(op, lt, va, vb);
        return ge_binop(e, op, lt, ge_snap(lt, va), ge_snap(rt, vb), rt);
    }
    if (ge_is_unit_ty(lt) && op >= OP_EQ && op <= OP_GE) {
        ex(e->a);
        ex(e->b);
        return (op == OP_EQ || op == OP_LE || op == OP_GE) ? "1" : "0";
    }
    {
        Callee *c = check_callee(e, 0);
        const char *a[2];
        if (!c) fatal("code generator: operator without a callee");
        if (op >= OP_EQ && op <= OP_GE) {
            Place pa = ge_place_adj(e->a, 0), pb;
            const char *aa = ge_snap(ty_ref(pa.ty, 0), ge_addr(pa));
            pb = ge_place_adj(e->b, 0);
            a[0] = aa;
            a[1] = ge_addr(pb);
        } else {
            a[0] = ge_snap(lt, ex(e->a));
            a[1] = ex_to(e->b, rt);
        }
        return ge_call_callee(e, c, a, 2);
    }
}

static const char *ge_unary(Node *e) {
    Type *t = gnt(e->a);
    if (e->op == OP_NEG && e->a->kind == N_E_LIT_INT) {
        if (ge_is_float(t)) return S("(-%s)", ge_float_lit(e->a));
        return ge_int_lit(-(long)e->a->ival);
    }
    if (e->op == OP_NEG && e->a->kind == N_E_LIT_FLOAT) return S("(-%s)", ge_float_lit(e->a));
    if (ge_scalar(t)) {
        const char *v = ge_fix(ex(e->a), t);
        return e->op == OP_NEG ? ge_neg(e, t, ge_snap(t, v)) : ge_not(t, v);
    }
    {
        Callee *c = check_callee(e, 0);
        const char *a[1];
        if (!c) fatal("code generator: unary operator without a callee");
        a[0] = ex(e->a);
        return ge_call_callee(e, c, a, 1);
    }
}

/* assignment of v to the place p: drops the old value first */
static void ge_assign_place(Place p, const char *v) {
    Type *t = p.ty;
    if (cg_needs_drop(t) && p.kind == PK_LV) {
        const char *glue = cg_drop_glue(t);
        if (glue) {
            if (p.root && p.root->flag) E("if (%s) %s((long)&%s);\n", p.root->flag, glue, p.e);
            else if (!p.root) E("%s((long)&%s);\n", glue, p.e);
        }
    }
    ge_store(p, v);
    if (p.root && p.root->flag && p.root->kind == LK_VAR && p.e == p.root->name) E("%s = 1;\n", p.root->flag);
}

static const char *ge_assign(Node *e) {
    Node *l = e->a;
    Type *lt = gnt(l);
    const char *v;
    while (l->kind == N_E_PAREN) l = l->a;
    if (l->kind == N_E_UNDERSCORE) {
        v = ex(e->b);
        return "0";
    }
    if (l->kind == N_E_TUPLE) {
        /* destructuring assignment of a tuple */
        Type *tt = gnt(e->b);
        long i;
        const char *tv = ge_snap(tt, ex(e->b));
        for (i = 0; i < l->list.n; i++) {
            Node *ln = l->list.v[i];
            Place p;
            if (ln->kind == N_E_UNDERSCORE) continue;
            p = ge_place_adj(ln, 1);
            ge_assign_place(p, S("%s.f%ld", tv, i));
        }
        return "0";
    }
    v = ex_to(e->b, lt);
    v = ge_fix(v, lt);
    if (!ge_stable(v) && !ge_simple(l)) v = ge_snap(lt, v);
    {
        Place p = ge_place_adj(l, 1);
        ge_assign_place(p, v);
    }
    return "0";
}

static const char *ge_assign_op(Node *e) {
    Type *lt = gnt(e->a), *rt = gnt(e->b);
    int op = e->op;
    if (ge_scalar(lt) && ge_scalar(rt)) {
        const char *vb = ge_snap(rt, ge_fix(ex(e->b), rt)), *cur, *r;
        Place p = ge_place_adj(e->a, 1);
        cur = ge_snap(lt, ge_load(p));
        r = ge_binop(e, op, lt, cur, vb, rt);
        ge_store(p, r);
        return "0";
    }
    {
        Callee *c = check_callee(e, 0);
        const char *a[2];
        Place p;
        if (!c) fatal("code generator: compound assignment without a callee");
        p = ge_place_adj(e->a, 1);
        a[0] = ge_snap(ty_int(IK_USIZE), ge_addr(p));
        a[1] = ex_to(e->b, rt);
        ge_call_callee(e, c, a, 2);
        return "0";
    }
}

static const char *ge_cast_expr(Node *e) {
    Type *to = gnt(e), *from = gnt(e->a);
    const char *v;
    if (e->a->adj) from = to;
    v = ex_to(e->a, to);
    v = ge_fix(v, from);
    if (ge_is_callable_ty(from)) {
        if (to->kind == TY_FN_PTR) return ge_to_fnptr(e, from, v);
        if (to->kind == TY_INT) return ge_norm(to, ge_to_fnptr(e, from, v));
    }
    if (from->kind == TY_FN_PTR && to->kind == TY_INT) return ge_norm(to, v);
    return ge_cast(e, from, to, v);
}

/* ================================================================== calls: expressions */

static const char *ge_ctor_call(Node *e, Sym *vs, Type *adt, Type **ptypes, int np) {
    int n = (int)e->list.n, i;
    const char **vals = (const char **)xalloc(sizeof(char *) * (size_t)(n + 1));
    Sym **fs = (Sym **)xalloc(sizeof(Sym *) * (size_t)(n + 1));
    (void)np;
    ge_eval_list(e->list.v, n, ptypes, vals);
    for (i = 0; i < n; i++) {
        fs[i] = find_field(vs, 0, i);
        vals[i] = ge_fix(vals[i], ptypes ? ptypes[i] : 0);
    }
    return ge_new_adt(adt, vs, fs, vals, n);
}

static Node *ge_strip_paren(Node *f) {
    while (f->kind == N_E_PAREN && !f->adj) f = f->a;
    return f;
}

static const char *ge_call_expr(Node *e) {
    Node *f = ge_strip_paren(e->a);
    Type *ft = gnt(f);
    int n = (int)e->list.n;
    const char **argv = (const char **)xalloc(sizeof(char *) * (size_t)(n + 2));
    if (f->kind == N_E_PATH && f->sym && (((Sym *)f->sym)->kind == SYM_VARIANT || ((Sym *)f->sym)->kind == SYM_STRUCT) &&
        ft->kind == TY_FN_DEF) {
        return ge_ctor_call(e, (Sym *)f->sym, gnt(e), ft->params, ft->nparams);
    }
    if (ft->kind == TY_FN_DEF && !f->adj) {
        Callee *c = f->kind == N_E_PATH ? check_callee(f, 0) : 0, tmp;
        Type **pt;
        if (!c) {
            memset(&tmp, 0, sizeof tmp);
            tmp.node = f;
            tmp.fn = ft->def;
            tmp.fnty = ft;
            c = &tmp;
        }
        pt = gm(c->fnty)->params;
        if (f->kind != N_E_PATH) ex(f);
        ge_eval_list(e->list.v, n, pt, argv);
        return ge_call_callee(e, c, argv, n);
    }
    if (ft->kind == TY_FN_PTR && !f->adj) {
        const char *fv = ge_snap(ty_int(IK_USIZE), ex(f));
        ge_eval_list(e->list.v, n, ft->params, argv);
        return ge_emit_call(S("((%s)(%s))", ge_fnptr_type(gm(ft->ret), 0, ft->params, ft->nparams), fv), argv, n, gm(ft->ret));
    }
    {
        Callee *c = check_callee(e, 0);
        Place fp;
        Type *u, *tup, **pt, *ret;
        int by_val;
        if (!c) fatal("code generator: call without a callee");
        u = gm(c->self);
        by_val = c->fn && c->fn->parent == L_fn_once;
        fp = ge_place_adj(f, 0);
        u = fp.ty;
        tup = c->fnty && c->fnty->nparams > 1 ? gm(c->fnty->params[1]) : 0;
        pt = tup && tup->kind == TY_TUPLE ? tup->args : 0;
        ret = gm(c->fnty->ret);
        ge_eval_list(e->list.v, n, pt, argv);
        if (u->kind == TY_DYN) {
            Place dp = fp;
            if (by_val && fp.root && fp.root->flag) E("%s = 0;\n", fp.root->flag);
            return ge_call_dyn(u, c->fn, dp, ret, pt, n, argv, n);
        }
        if (u->kind == TY_FN_DEF && u->def->kind == SYM_FN) {
            /* a fn item held in a variable */
            return ge_call_callable(e, u, fp, 0, argv, n);
        }
        return ge_call_callable(e, u, fp, by_val, argv, n);
    }
}

static const char *ge_method_call(Node *e) {
    Callee *c = check_callee(e, 0);
    int n = (int)e->list.n, i;
    const char **argv = (const char **)xalloc(sizeof(char *) * (size_t)(n + 2));
    Type **pt = (Type **)xalloc(sizeof(Type *) * (size_t)(n + 2)), *ft;
    Node **nodes = (Node **)xalloc(sizeof(Node *) * (size_t)(n + 2));
    if (!c) fatal("code generator: method call without a callee");
    ft = gm(c->fnty);
    nodes[0] = e->a;
    pt[0] = ft->nparams > 0 ? ft->params[0] : 0;
    for (i = 0; i < n; i++) {
        nodes[i + 1] = e->list.v[i];
        pt[i + 1] = i + 1 < ft->nparams ? ft->params[i + 1] : 0;
    }
    ge_eval_list(nodes, n + 1, pt, argv);
    return ge_call_callee(e, c, argv, n + 1);
}

/* ================================================================== patterns */

typedef struct {
    const char *set_flag;      /* a binding's drop flag to set */
    const char *clear_flag;    /* a scrutinee root's drop flag to clear */
} Commit;

static Commit *ge_commits;
static int ge_ncommits, ge_ccommits;

static void ge_commit_add(const char *set_flag, const char *clear_flag) {
    int i;
    if (clear_flag)
        for (i = 0; i < ge_ncommits; i++)
            if (ge_commits[i].clear_flag == clear_flag) return;
    if (ge_ncommits >= ge_ccommits) {
        ge_ccommits = ge_ccommits ? ge_ccommits * 2 : 16;
        ge_commits = (Commit *)xrealloc(ge_commits, sizeof(Commit) * (size_t)ge_ccommits);
    }
    ge_commits[ge_ncommits].set_flag = set_flag;
    ge_commits[ge_ncommits].clear_flag = clear_flag;
    ge_ncommits++;
}

static void ge_commit(void) {
    int i;
    for (i = 0; i < ge_ncommits; i++) {
        if (ge_commits[i].clear_flag) E("%s = 0;\n", ge_commits[i].clear_flag);
        if (ge_commits[i].set_flag) E("%s = 1;\n", ge_commits[i].set_flag);
    }
    ge_ncommits = 0;
}

static Place ge_peel(Node *p, Place pl) {
    int n = p->adj & 0xFF;
    while (n-- > 0) pl = ge_deref_place(p, pl, 0);
    return pl;
}

static const char *ge_and(const char *a, const char *b) {
    if (!a) return b;
    if (!b) return a;
    return S("(%s && %s)", a, b);
}

static int ge_pat_binds(Node *p) {
    long i;
    if (!p) return 0;
    switch (p->kind) {
    case N_P_IDENT: {
        Sym *s = (Sym *)p->sym;
        if (s && s->kind == SYM_LOCAL) return 1;
        return 0;
    }
    case N_P_FIELD: case N_P_REF:
        return ge_pat_binds(p->a);
    case N_P_TUPLE: case N_P_SLICE: case N_P_TUPLE_STRUCT: case N_P_STRUCT: case N_P_OR:
        for (i = 0; i < p->list.n; i++)
            if (ge_pat_binds(p->list.v[i])) return 1;
        return 0;
    }
    return 0;
}

/* the variant / struct a pattern names (0 for none) */
static Sym *ge_pat_ctor(Node *p) {
    Sym *s = (Sym *)p->sym, *vs = 0;
    Type *adt = 0;
    if (p->kind == N_P_TUPLE_STRUCT || p->kind == N_P_STRUCT) return s;
    if (s && (s->kind == SYM_VARIANT || s->kind == SYM_STRUCT)) return s;
    if (p->kind == N_P_PATH && resolve_ctor(p->a, &vs, &adt)) return vs;
    return 0;
}

static const char *ge_member(Sym *vs, Place pl, Sym *f) {
    if (vs->kind == SYM_VARIANT) return S("%s.%s.%s", pl.e, cg_variant_member(vs), ge_field_member(f));
    return S("%s.%s", pl.e, ge_field_member(f));
}

static Place ge_sub_place(Sym *vs, Place pl, Sym *f) {
    Type *ft = gm(field_ty(f, pl.ty));
    return ge_pl_make(PK_LV, ge_member(vs, pl, f), ft, pl.root);
}

static const char *ge_pat_lit_value(Node *p, Type *t) {
    Node *lit = p->a;
    int neg = (p->flags & F_NEG) != 0;
    switch (lit->kind) {
    case N_E_LIT_INT:
        if (ge_is_float(t)) return S("%s%ld.0", neg ? "-" : "", (long)lit->ival);
        return ge_int_lit(neg ? -(long)lit->ival : (long)lit->ival);
    case N_E_LIT_FLOAT: return S("%s%s", neg ? "-" : "", ge_float_lit(lit));
    case N_E_LIT_CHAR: case N_E_LIT_BYTE: case N_E_LIT_BOOL: return S("%ld", (long)lit->ival);
    }
    return "0";
}

static const char *ge_range_bound(Node *b, Type *t) {
    if (b->kind == N_P_LIT) return ge_pat_lit_value(b, t);
    {
        Sym *s = (Sym *)b->sym;
        if (s && s->kind == SYM_CONST) return ge_snap(t, ge_const_expr(s, s->parent && s->parent->kind == SYM_IMPL ? cg_subst : 0, t));
    }
    return "0";
}

static const char *ge_str_eq(const char *sv, const char *bytes, long len) {
    return S("(%s.len == %ld && rs_cmp_bytes(%s.base, (long)%s * 8, %ld) == 0)", sv, len, sv, cg_str_literal(bytes, len), len);
}

static const char *ge_slice_parts(Place pl, const char **base, const char **len) {
    Type *t = pl.ty;
    if (t->kind == TY_ARRAY) {
        *base = ge_arr_base(pl, t);
        *len = S("%ld", ge_array_len(t));
    } else {
        *base = S("%s.base", pl.e);
        *len = S("%s.len", pl.e);
    }
    return 0;
}

static int ge_rest_pos(Node *p) {
    long i;
    for (i = 0; i < p->list.n; i++) {
        Node *sp = p->list.v[i];
        if (sp->kind == N_P_REST || (sp->kind == N_P_IDENT && sp->a && sp->a->kind == N_P_REST)) return (int)i;
    }
    return -1;
}

/* the place of element number i (front index) or (len - back) (back index) of an array / slice place */
static Place ge_slice_elem(Place pl, int front, int back) {
    Type *t = pl.ty, *el = t->elem;
    const char *idx;
    if (t->kind == TY_ARRAY) {
        long n = ge_array_len(t);
        idx = S("%ld", front >= 0 ? (long)front : n - back);
        if (cg_is_u8(el)) return ge_pl_make(PK_BYTE, S("((long)&(%s).w[0] * 8 + %s)", pl.e, idx), el, 0);
        return ge_pl_make(PK_LV, S("(%s).e[%s]", pl.e, idx), el, pl.root);
    }
    idx = front >= 0 ? S("%d", front) : S("(%s.len - %d)", pl.e, back);
    if (cg_is_u8(el)) return ge_pl_make(PK_BYTE, S("(%s.base + %s)", pl.e, idx), el, 0);
    return ge_pl_make(PK_LV, S("((%s *)(%s.base))[%s]", ge_ct(el), pl.e, idx), el, 0);
}

static const char *ge_pat_cond(Node *p, Place pl) {
    const char *c = 0;
    long i;
    pl = ge_peel(p, pl);
    switch (p->kind) {
    case N_P_WILD: case N_P_REST:
        return 0;
    case N_P_IDENT: {
        Sym *s = (Sym *)p->sym;
        if (s && s->kind == SYM_LOCAL) return (p->a && p->a->kind != N_P_REST) ? ge_pat_cond(p->a, pl) : 0;
        break;
    }
    case N_P_LIT: {
        Type *t = pl.ty;
        const char *v = ge_load(pl);
        if (p->a->kind == N_E_LIT_STR) {
            v = ge_snap(t, v);
            return ge_str_eq(v, p->a->str, p->a->slen);
        }
        return S("(%s == %s)", v, ge_pat_lit_value(p, t));
    }
    case N_P_RANGE: {
        Type *t = pl.ty;
        const char *v = ge_snap(t, ge_load(pl)), *lo = 0, *hi = 0;
        if (p->a) lo = ge_range_bound(p->a, t);
        if (p->b) hi = ge_range_bound(p->b, t);
        if (lo) c = ge_cmp(OP_GE, t, v, lo);
        if (hi) c = ge_and(c, ge_cmp((p->flags & F_INCLUSIVE) ? OP_LE : OP_LT, t, v, hi));
        return c;
    }
    case N_P_REF: {
        Type *t = pl.ty;
        Place sp = ge_deref_ref(ge_load(pl), t->elem, 0);
        return ge_pat_cond(p->a, sp);
    }
    case N_P_TUPLE: {
        Type *t = pl.ty;
        int nsub = (int)p->list.n, rest = -1, k;
        for (k = 0; k < nsub; k++)
            if (p->list.v[k]->kind == N_P_REST) rest = k;
        for (k = 0; k < nsub; k++) {
            int fi;
            if (k == rest) continue;
            fi = (rest >= 0 && k > rest) ? t->nargs - (nsub - k) : k;
            c = ge_and(c, ge_pat_cond(p->list.v[k], ge_pl_make(PK_LV, S("%s.f%d", pl.e, fi), t->args[fi], pl.root)));
        }
        return c;
    }
    case N_P_SLICE: {
        Type *t = pl.ty;
        int nsub = (int)p->list.n, rest = ge_rest_pos(p), k, fixed = rest >= 0 ? nsub - 1 : nsub;
        const char *base, *len;
        ge_slice_parts(pl, &base, &len);
        if (t->kind == TY_SLICE) c = S("(%s %s %d)", len, rest >= 0 ? ">=" : "==", fixed);
        for (k = 0; k < nsub; k++) {
            Place ep;
            if (k == rest) continue;
            ep = (rest >= 0 && k > rest) ? ge_slice_elem(pl, -1, nsub - k) : ge_slice_elem(pl, k, 0);
            c = ge_and(c, ge_pat_cond(p->list.v[k], ep));
        }
        return c;
    }
    case N_P_OR: {
        const char *r = 0;
        int irrefutable = 0;
        for (i = 0; i < p->list.n; i++) {
            const char *ci = ge_pat_cond(p->list.v[i], pl);
            if (!ci) irrefutable = 1;
            else r = r ? S("(%s || %s)", r, ci) : ci;
        }
        return irrefutable ? 0 : r;
    }
    }
    /* constructors, constants */
    {
        Sym *s = (Sym *)p->sym, *vs = ge_pat_ctor(p);
        if (!vs && p->kind != N_P_TUPLE_STRUCT && p->kind != N_P_STRUCT && s && (s->kind == SYM_CONST || s->kind == SYM_STATIC)) {
            Type *t = pl.ty;
            const char *cv = ge_snap(t, s->kind == SYM_STATIC ? cg_static(s) : ge_const_expr(s, s->parent && s->parent->kind == SYM_IMPL ? cg_subst : 0, t)), *pv;
            if (t->kind == TY_REF && t->elem->kind == TY_STR) {
                pv = ge_snap(t, ge_load(pl));
                return S("(%s.len == %s.len && rs_cmp_bytes(%s.base, %s.base, %s.len) == 0)", pv, cv, pv, cv, pv);
            }
            return S("(%s == %s)", ge_load(pl), cv);
        }
        if (!vs && p->kind == N_P_PATH && s && s->kind == SYM_CONST) return 0;
        if (!vs) return 0;
        if (vs->kind == SYM_VARIANT) {
            if (count_variants(vs->parent) > 1) c = S("(%s.tag == %d)", pl.e, cg_variant_index(vs));
        }
        if (p->kind == N_P_TUPLE_STRUCT) {
            int nsub = (int)p->list.n, nf = count_fields(vs), rest = -1, k;
            for (k = 0; k < nsub; k++)
                if (p->list.v[k]->kind == N_P_REST) rest = k;
            for (k = 0; k < nsub; k++) {
                int fi;
                if (k == rest) continue;
                fi = (rest >= 0 && k > rest) ? nf - (nsub - k) : k;
                c = ge_and(c, ge_pat_cond(p->list.v[k], ge_sub_place(vs, pl, find_field(vs, 0, fi))));
            }
        } else if (p->kind == N_P_STRUCT) {
            for (i = 0; i < p->list.n; i++) {
                Node *fp = p->list.v[i];
                c = ge_and(c, ge_pat_cond(fp->a, ge_sub_place(vs, pl, (Sym *)fp->sym)));
            }
        }
        return c;
    }
}

/* declares (or finds) the local of a binding */
static Local *ge_binding(Sym *s, Type *t) {
    Local *l = ge_local_find(s);
    if (l) return l;
    return ge_declare(s, t);
}

static void ge_pat_bind(Node *p, Place pl) {
    long i;
    if (!ge_pat_binds(p)) return;
    pl = ge_peel(p, pl);
    switch (p->kind) {
    case N_P_IDENT: {
        Sym *s = (Sym *)p->sym;
        Type *bt = gm((Type *)s->ty);
        Local *l = ge_binding(s, bt);
        const char *v;
        int moved = 0;
        if (p->adj & (ADJ_AUTOREF | ADJ_AUTOREF_MUT)) {
            v = ge_addr(pl);
        } else {
            v = ge_load(pl);
            moved = cg_needs_drop(pl.ty);
        }
        if (!cg_is_zst(bt) && v != GE_NEVER) E("%s = %s;\n", l->name, v);
        if (l->flag || (moved && pl.root && pl.root->flag))
            ge_commit_add(l->flag, moved && pl.root ? pl.root->flag : 0);
        if (p->a && p->a->kind != N_P_REST) ge_pat_bind(p->a, pl);
        return;
    }
    case N_P_REF: {
        Place sp = ge_deref_ref(ge_load(pl), pl.ty->elem, 0);
        ge_pat_bind(p->a, sp);
        return;
    }
    case N_P_TUPLE: {
        Type *t = pl.ty;
        int nsub = (int)p->list.n, rest = -1, k;
        for (k = 0; k < nsub; k++)
            if (p->list.v[k]->kind == N_P_REST) rest = k;
        for (k = 0; k < nsub; k++) {
            int fi;
            if (k == rest) continue;
            fi = (rest >= 0 && k > rest) ? t->nargs - (nsub - k) : k;
            ge_pat_bind(p->list.v[k], ge_pl_make(PK_LV, S("%s.f%d", pl.e, fi), t->args[fi], pl.root));
        }
        return;
    }
    case N_P_SLICE: {
        Type *t = pl.ty;
        int nsub = (int)p->list.n, rest = ge_rest_pos(p), k;
        for (k = 0; k < nsub; k++) {
            Node *sp = p->list.v[k];
            if (k == rest) {
                if (sp->kind == N_P_IDENT) {
                    /* name @ ..: a sub-slice (by reference) */
                    Sym *s = (Sym *)sp->sym;
                    Type *bt = gm((Type *)s->ty);
                    Local *l = ge_binding(s, bt);
                    const char *base, *len, *sl;
                    ge_slice_parts(pl, &base, &len);
                    if (cg_is_u8(t->elem))
                        sl = ge_mk_slice(S("(%s + %d)", base, k), S("(%s - %d)", len, nsub - 1));
                    else
                        sl = ge_mk_slice(S("((long)((%s *)(%s) + %d))", ge_ct(t->elem), base, k), S("(%s - %d)", len, nsub - 1));
                    E("%s = %s;\n", l->name, sl);
                    if (l->flag) ge_commit_add(l->flag, 0);
                }
                continue;
            }
            ge_pat_bind(sp, (rest >= 0 && k > rest) ? ge_slice_elem(pl, -1, nsub - k) : ge_slice_elem(pl, k, 0));
        }
        return;
    }
    case N_P_OR: {
        int first = 1;
        for (i = 0; i < p->list.n; i++) {
            Node *alt = p->list.v[i];
            const char *ci = ge_pat_cond(alt, pl);
            if (i == p->list.n - 1 && !first) E("else {\n");
            else E("%sif (%s) {\n", first ? "" : "else ", ci ? ci : "1");
            first = 0;
            ge_pat_bind(alt, pl);
            E("}\n");
        }
        return;
    }
    }
    {
        Sym *vs = ge_pat_ctor(p);
        if (!vs) return;
        if (p->kind == N_P_TUPLE_STRUCT) {
            int nsub = (int)p->list.n, nf = count_fields(vs), rest = -1, k;
            for (k = 0; k < nsub; k++)
                if (p->list.v[k]->kind == N_P_REST) rest = k;
            for (k = 0; k < nsub; k++) {
                int fi;
                if (k == rest) continue;
                fi = (rest >= 0 && k > rest) ? nf - (nsub - k) : k;
                ge_pat_bind(p->list.v[k], ge_sub_place(vs, pl, find_field(vs, 0, fi)));
            }
        } else if (p->kind == N_P_STRUCT) {
            for (i = 0; i < p->list.n; i++) {
                Node *fp = p->list.v[i];
                ge_pat_bind(fp->a, ge_sub_place(vs, pl, (Sym *)fp->sym));
            }
        }
    }
}

/* declares every binding of a pattern without initializing it (let x;) */
static void ge_pat_declare(Node *p) {
    long i;
    if (!p) return;
    if (p->kind == N_P_IDENT && p->sym && ((Sym *)p->sym)->kind == SYM_LOCAL) {
        Sym *s = (Sym *)p->sym;
        ge_binding(s, gm((Type *)s->ty));
    }
    ge_pat_declare(p->a);
    for (i = 0; i < p->list.n; i++) ge_pat_declare(p->list.v[i]);
}

/* ================================================================== control flow */

static void ge_line(Node *n) {
    StrBuf saved = cg_sect[CG_FUNCS];
    cg_sect[CG_FUNCS] = *ge_out;
    cg_line(n);
    *ge_out = cg_sect[CG_FUNCS];
    cg_sect[CG_FUNCS] = saved;
}

static int ge_has_let(Node *c) {
    while (c && c->kind == N_E_PAREN) c = c->a;
    if (!c) return 0;
    if (c->kind == N_E_LET) return 1;
    if (c->kind == N_E_BINARY && c->op == OP_LAND) return ge_has_let(c->a) || ge_has_let(c->b);
    return 0;
}

/* evaluates a condition; when it fails, drops the scopes down to depth and jumps to lbl */
static void ge_cond_false(Node *c, const char *lbl, int depth) {
    while (c->kind == N_E_PAREN) c = c->a;
    if (c->kind == N_E_BINARY && c->op == OP_LAND && ge_has_let(c)) {
        ge_cond_false(c->a, lbl, depth);
        ge_cond_false(c->b, lbl, depth);
        return;
    }
    if (c->kind == N_E_LET) {
        Place sp = ge_place_adj(c->b, 0);
        const char *cond = ge_pat_cond(c->a, sp);
        if (cond) {
            E("if (!%s) {\n", cond);
            ge_drops_to(depth);
            E("goto %s;\n}\n", lbl);
        }
        ge_pat_bind(c->a, sp);
        ge_commit();
        return;
    }
    {
        const char *v = ge_fix(ex(c), ty_prim(TY_BOOL));
        E("if (!%s) {\n", v);
        ge_drops_to(depth);
        E("goto %s;\n}\n", lbl);
    }
}

static GLoop *ge_loop_push(Name *label, int is_loop, int is_while, const char *res, Type *rty) {
    GLoop *l;
    if (ge_nloops >= ge_cloops) {
        ge_cloops = ge_cloops ? ge_cloops * 2 : 16;
        ge_loops = (GLoop *)xrealloc(ge_loops, sizeof(GLoop) * (size_t)ge_cloops);
    }
    l = &ge_loops[ge_nloops++];
    l->label = label;
    l->id = cg_new_id();
    l->sdepth = ge_nsc;
    l->res = res;
    l->rty = rty;
    l->is_loop = is_loop;
    l->is_while = is_while;
    return l;
}

static GLoop *ge_loop_find(Name *label, int for_continue) {
    int i;
    for (i = ge_nloops - 1; i >= 0; i--) {
        if (label) {
            if (ge_loops[i].label == label) return &ge_loops[i];
        } else if (ge_loops[i].is_loop || ge_loops[i].is_while) {
            return &ge_loops[i];
        }
    }
    (void)for_continue;
    fatal("code generator: break/continue outside a loop");
    return 0;
}

static void ge_assign_result(const char *dst, Type *t, const char *v) {
    if (!dst || !t || cg_is_zst(t)) return;
    if (v == GE_NEVER) return;
    E("%s = %s;\n", dst, v);
}

static int ge_value_type(Type *t) {
    return t && !cg_is_zst(t) && !ge_is_never(t);
}

static const char *ge_if(Node *e, Type *want) {
    Type *t = gnt(e);
    const char *r = 0;
    int id, depth;
    (void)want;
    if (e->c && ge_value_type(t)) r = ge_tmp_ty(t);
    id = cg_new_id();
    if (!ge_has_let(e->a)) {
        const char *cv;
        ge_push_scope(SC_STMT);
        cv = ge_snap(ty_prim(TY_BOOL), ge_fix(ex(e->a), ty_prim(TY_BOOL)));
        ge_pop_scope(1);
        E("if (%s) {\n", cv);
        ge_assign_result(r, t, ge_block_value(e->b, t));
        if (e->c) {
            E("} else {\n");
            ge_assign_result(r, t, ex_to(e->c, t));
        }
        E("}\n");
    } else {
        ge_push_scope(SC_STMT);
        depth = ge_nsc;
        ge_push_scope(SC_BLOCK);
        ge_cond_false(e->a, S("Le%d", id), depth);
        ge_assign_result(r, t, ge_block_value(e->b, t));
        ge_pop_scope(1);
        E("goto Lf%d;\nLe%d:;\n", id, id);
        if (e->c) ge_assign_result(r, t, ex_to(e->c, t));
        E("Lf%d:;\n", id);
        ge_pop_scope(1);
    }
    if (ge_is_never(t)) return GE_NEVER;
    return r ? r : "0";
}

static const char *ge_match(Node *e, Type *want) {
    Type *t = gnt(e);
    const char *r = ge_value_type(t) ? ge_tmp_ty(t) : 0;
    int id = cg_new_id(), i, depth;
    Place sp;
    (void)want;
    ge_push_scope(SC_STMT);
    depth = ge_nsc;
    sp = ge_place_adj(e->a, 0);
    for (i = 0; i < (int)e->list.n; i++) {
        Node *arm = e->list.v[i];
        const char *cond;
        ge_push_scope(SC_BLOCK);
        cond = ge_pat_cond(arm->a, sp);
        E("if (%s) {\n", cond ? cond : "1");
        ge_ncommits = 0;
        ge_pat_bind(arm->a, sp);
        if (arm->b) {
            ge_cond_false(arm->b, S("Ln%d_%d", id, i), depth);
        } else {
            ge_commit();
        }
        if (arm->b && ge_ncommits) ge_commit();
        ge_assign_result(r, t, ex_to(arm->c, t));
        ge_pop_scope(1);
        E("goto Lm%d;\n}\n", id);
        if (arm->b) E("Ln%d_%d:;\n", id, i);
    }
    E("Lm%d:;\n", id);
    ge_pop_scope(1);
    if (ge_is_never(t)) return GE_NEVER;
    return r ? r : "0";
}

static const char *ge_while(Node *e) {
    GLoop *lp;
    int id, depth;
    ge_loop_push(e->name, 0, 1, 0, 0);
    lp = &ge_loops[ge_nloops - 1];
    id = lp->id;
    depth = lp->sdepth;
    E("for (;;) {\n");
    ge_push_scope(SC_STMT);
    ge_push_scope(SC_BLOCK);
    ge_cond_false(e->a, S("brk%d", id), depth);
    ge_block_value(e->b, ty_unit());
    ge_pop_scope(1);
    ge_pop_scope(1);
    E("cnt%d:;\n}\nbrk%d:;\n", id, id);
    ge_nloops--;
    return "0";
}

static const char *ge_loop(Node *e) {
    Type *t = gnt(e);
    const char *r = ge_value_type(t) ? ge_tmp_ty(t) : 0;
    GLoop *lp = ge_loop_push(e->name, 1, 0, r, t);
    int id = lp->id;
    E("for (;;) {\n");
    ge_block_value(e->a, ty_unit());
    E("cnt%d:;\n}\nbrk%d:;\n", id, id);
    ge_nloops--;
    if (ge_is_never(t)) return GE_NEVER;
    return r ? r : "0";
}

static const char *ge_for(Node *e) {
    Callee *c0 = check_callee(e, 0), *c1 = check_callee(e, 1);
    Type *iter_ty, *opt_ty, *item_ty;
    const char *iv, *it, *o, *a[1];
    GLoop *lp;
    int id, depth;
    Local *itl;
    Sym *some;
    if (!c0 || !c1) fatal("code generator: for loop without callees");
    iter_ty = gm(c1->self);
    opt_ty = gm(c1->fnty->ret);
    item_ty = opt_ty->args[0];
    ge_push_scope(SC_BLOCK);
    a[0] = ex_to(e->b, gm(c0->fnty->params[0]));
    iv = ge_call_callee(e, c0, a, 1);
    it = ge_tmp_ty(iter_ty);
    if (!cg_is_zst(iter_ty)) E("%s = %s;\n", it, ge_fix(iv, iter_ty));
    itl = ge_temp_local(it, iter_ty, ge_nsc - 1);
    ge_set_init(itl);
    lp = ge_loop_push(e->name, 0, 1, 0, 0);
    id = lp->id;
    depth = lp->sdepth;
    E("for (;;) {\n");
    ge_push_scope(SC_BLOCK);
    a[0] = S("((long)&%s)", it);
    o = ge_call_callee(e, c1, a, 1);
    o = ge_snap(opt_ty, ge_fix(o, opt_ty));
    some = variant_at(L_option, 1);
    E("if (%s.tag == 0) {\n", o);
    ge_drops_to(depth);
    E("goto brk%d;\n}\n", id);
    ge_ncommits = 0;
    ge_pat_bind(e->a, ge_pl_make(PK_LV, S("%s.%s.%s", o, cg_variant_member(some), ge_field_member(find_field(some, 0, 0))), item_ty, 0));
    ge_commit();
    ge_block_value(e->c, ty_unit());
    ge_pop_scope(1);
    E("cnt%d:;\n}\nbrk%d:;\n", id, id);
    ge_nloops--;
    ge_pop_scope(1);
    return "0";
}

static const char *ge_break(Node *e) {
    GLoop *lp = ge_loop_find(e->name, 0);
    if (e->a) {
        const char *v = ex_to(e->a, lp->rty);
        ge_assign_result(lp->res, lp->rty, v);
    }
    ge_drops_to(lp->sdepth);
    E("goto brk%d;\n", lp->id);
    return GE_NEVER;
}

static const char *ge_continue(Node *e) {
    GLoop *lp = ge_loop_find(e->name, 1);
    ge_drops_to(lp->sdepth);
    E("goto cnt%d;\n", lp->id);
    return GE_NEVER;
}

static void ge_return_value(const char *v) {
    Type *rt = ge_ret;
    if (cg_is_zst(rt) || ge_is_never(rt) || v == GE_NEVER) {
        if (v == GE_NEVER && !cg_is_zst(rt) && !ge_is_never(rt)) v = ge_fix(v, rt);
        else v = "0";
        if (!cg_is_zst(rt) && !ge_is_never(rt)) {
            ge_drops_to(0);
            E("return %s;\n", v);
            return;
        }
        ge_drops_to(0);
        E("return 0;\n");
        return;
    }
    v = ge_snap(rt, v);
    ge_drops_to(0);
    E("return %s;\n", v);
}

static const char *ge_return(Node *e) {
    const char *v = "0";
    if (e->a) v = ex_to(e->a, ge_ret);
    ge_return_value(v);
    return GE_NEVER;
}

static const char *ge_try(Node *e) {
    Type *ot = gnt(e->a), *rt = ge_ret;
    const char *v = ge_snap(ot, ge_fix(ex(e->a), ot)), *rv;
    if (ot->def == L_result) {
        Callee *c = check_callee(e, 0);
        Sym *err = variant_at(L_result, 1), *ok = variant_at(L_result, 0);
        Type *et = ot->args[1];
        const char *ev = S("%s.%s.%s", v, cg_variant_member(err), ge_field_member(find_field(err, 0, 0))), *cv = ev;
        Sym *rerr = variant_at(rt->def, 1);
        E("if (%s.tag == 1) {\n", v);
        if (c) {
            const char *a[1];
            a[0] = ge_snap(et, ev);
            cv = ge_call_callee(e, c, a, 1);
        }
        rv = ge_tmp_ty(rt);
        E("%s.tag = 1;\n", rv);
        if (!cg_is_zst(gm(field_ty(find_field(rerr, 0, 0), rt)))) E("%s.%s.%s = %s;\n", rv, cg_variant_member(rerr), ge_field_member(find_field(rerr, 0, 0)), cv);
        ge_return_value(rv);
        E("}\n");
        return S("%s.%s.%s", v, cg_variant_member(ok), ge_field_member(find_field(ok, 0, 0)));
    }
    {
        Sym *some = variant_at(L_option, 1);
        E("if (%s.tag == 0) {\n", v);
        rv = ge_tmp_ty(rt);
        E("%s.tag = 0;\n", rv);
        ge_return_value(rv);
        E("}\n");
        return S("%s.%s.%s", v, cg_variant_member(some), ge_field_member(find_field(some, 0, 0)));
    }
}

/* ================================================================== blocks and statements */

static int ge_scope_has_drops(int si) {
    return ge_sc[si].n > 0;
}

static void ge_let(Node *s) {
    Node *pat = s->a, *init = s->c;
    Type *want = 0;
    if (!init) {
        ge_pat_declare(pat);
        return;
    }
    if (s->b) want = gm(ty_from_ast(s->b));
    {
        Node *ie = init;
        while (ie->kind == N_E_PAREN) ie = ie->a;
        if (ie->kind == N_E_REF) {
            Node *x = ie->a;
            while (x->kind == N_E_PAREN) x = x->a;
            ge_extend = x;
        }
    }
    if (pat->kind == N_P_IDENT && !pat->a && pat->sym && ((Sym *)pat->sym)->kind == SYM_LOCAL &&
        !(pat->adj & (ADJ_AUTOREF | ADJ_AUTOREF_MUT)) && !s->d) {
        Sym *sy = (Sym *)pat->sym;
        Type *bt = gm((Type *)sy->ty);
        const char *v = ex_to(init, bt);
        Local *l = ge_binding(sy, bt);
        ge_extend = 0;
        if (v != GE_NEVER && !cg_is_zst(bt)) E("%s = %s;\n", l->name, v);
        ge_set_init(l);
        return;
    }
    {
        Type *it = want ? want : gnt(init);
        Place sp;
        if (init->adj || want) {
            const char *v = ex_to(init, want);
            sp = ge_rvalue_place(0, it, v);
        } else {
            sp = ge_place_adj(init, 0);
        }
        ge_extend = 0;
        if (s->d) {
            const char *cond = ge_pat_cond(pat, sp);
            if (cond) {
                E("if (!%s) {\n", cond);
                ge_block_value(s->d, 0);
                E("}\n");
            }
        }
        ge_ncommits = 0;
        ge_pat_bind(pat, sp);
        ge_commit();
    }
}

static void ge_stmt(Node *s) {
    switch (s->kind) {
    case N_STMT_LET:
        ge_line(s);
        ge_push_scope(SC_STMT);
        ge_let(s);
        ge_pop_scope(1);
        break;
    case N_STMT_EXPR: {
        Type *t;
        const char *v;
        ge_line(s);
        ge_push_scope(SC_STMT);
        t = gnt(s->a);
        v = ex(s->a);
        if (v != GE_NEVER && cg_needs_drop(t)) ge_rvalue_place(0, t, v);
        ge_pop_scope(1);
        break;
    }
    default:
        break;
    }
}

static const char *ge_block_value(Node *b, Type *want) {
    Type *bt = want ? want : gnt(b);
    Type *own = gnt(b);
    const char *v = "0", *res = 0;
    long i;
    GLoop *lp = 0;
    int id = 0, top;
    if (!own) own = bt;
    if (b->name) {
        res = ge_value_type(own) ? ge_tmp_ty(own) : 0;
        lp = ge_loop_push(b->name, 0, 0, res, own);
        id = lp->id;
    }
    ge_push_scope(SC_BLOCK);
    top = ge_nsc - 1;
    for (i = 0; i < b->list.n; i++) ge_stmt(b->list.v[i]);
    if (b->a) {
        Type *tt = gnt(b->a);
        ge_line(b->a);
        ge_push_scope(SC_STMT);
        v = ex_to(b->a, bt);
        if (v != GE_NEVER && !cg_is_zst(tt) && !ge_stable(v) && (ge_scope_has_drops(ge_nsc - 1) || ge_scope_has_drops(top)))
            v = ge_snap(ge_is_never(bt) ? tt : (b->a->adj ? bt : tt), v);
        ge_pop_scope(1);
    }
    if (ge_scope_has_drops(top) && v != GE_NEVER && !ge_stable(v) && ge_value_type(bt)) v = ge_snap(bt, v);
    ge_pop_scope(1);
    if (lp) {
        ge_assign_result(res, own, v);
        E("brk%d:;\n", id);
        ge_nloops--;
        if (ge_is_never(own)) return GE_NEVER;
        return res ? res : "0";
    }
    return v;
}

/* ================================================================== closures */

static const char *ge_closure_value(Node *e) {
    Type *ct = gnt(e);
    Capture *caps = 0;
    int n = closure_captures(e, &caps), i;
    const char *t;
    cg_closure_instance(e);
    if (n == 0) return "0";
    t = ge_tmp_ty(ct);
    for (i = 0; i < n; i++) {
        Local *l = ge_local_find(caps[i].local);
        Place pl;
        const char *v;
        if (!l) fatal("code generator: captured variable not found");
        pl = ge_local_place(l);
        if (caps[i].mode == CAP_MOVE) v = ge_take(pl);
        else v = ge_addr(pl);
        E("%s.c%d = %s;\n", t, i, v);
    }
    return t;
}

/* ================================================================== the expression dispatcher */

static const char *ge_path_value(Node *e) {
    Sym *s = (Sym *)e->sym;
    Type *t = gnt(e);
    if (!s) fatal("code generator: unresolved path");
    switch (s->kind) {
    case SYM_LOCAL: {
        Local *l = ge_local_find(s);
        if (!l) fatal("code generator: unknown local variable");
        return ge_take(ge_local_place(l));
    }
    case SYM_STATIC:
        return ge_take(ge_pl_make(PK_LV, cg_static(s), t, 0));
    case SYM_CONST:
        if (s->parent && (s->parent->kind == SYM_IMPL || s->parent->kind == SYM_TRAIT)) return ge_assoc_const(e, s);
        return ge_const_expr(s, 0, t);
    case SYM_CONST_GENERIC:
        return S("%ld", ge_const_generic(s));
    case SYM_FN:
        return "0";
    case SYM_VARIANT: case SYM_STRUCT:
        if (t->kind == TY_FN_DEF) return "0";
        return ge_unit_value(s, t);
    }
    fatal("code generator: unsupported path");
    return "0";
}

static const char *ge_tuple(Node *e) {
    Type *t = gnt(e);
    int n = (int)e->list.n, i;
    const char **vals;
    const char *tmp;
    if (n == 0) return "0";
    vals = (const char **)xalloc(sizeof(char *) * (size_t)n);
    ge_eval_list(e->list.v, n, t->args, vals);
    if (cg_is_zst(t)) return "0";
    tmp = ge_tmp_ty(t);
    for (i = 0; i < n; i++)
        if (!cg_is_zst(t->args[i])) E("%s.f%d = %s;\n", tmp, i, ge_fix(vals[i], t->args[i]));
    return tmp;
}

static void ge_array_store(Type *el, const char *arr, const char *idx, const char *v) {
    if (cg_is_zst(el)) return;
    if (cg_is_u8(el)) E("__stb((long)&%s.w[0] * 8 + %s, %s);\n", arr, idx, v);
    else E("%s.e[%s] = %s;\n", arr, idx, v);
}

static const char *ge_array(Node *e) {
    Type *t = gnt(e), *el = t->elem;
    int n = (int)e->list.n, i;
    const char **vals = (const char **)xalloc(sizeof(char *) * (size_t)(n + 1));
    Type **ts = (Type **)xalloc(sizeof(Type *) * (size_t)(n + 1));
    const char *tmp;
    for (i = 0; i < n; i++) ts[i] = el;
    ge_eval_list(e->list.v, n, ts, vals);
    tmp = ge_tmp_ty(t);
    for (i = 0; i < n; i++) ge_array_store(el, tmp, S("%d", i), ge_fix(vals[i], el));
    return tmp;
}

static const char *ge_repeat(Node *e) {
    Type *t = gnt(e), *el = t->elem;
    long n = ge_array_len(t);
    const char *v = ge_snap(el, ge_fix(ex_to(e->a, el), el)), *tmp = ge_tmp_ty(t), *i;
    if (n <= 0 || cg_is_zst(el)) return tmp;
    i = ge_tmp("long");
    if (cg_is_u8(el)) {
        E("for (%s = 0; %s < %ld; %s++) __stb((long)&%s.w[0] * 8 + %s, %s);\n", i, i, n, i, tmp, i, v);
    } else {
        E("for (%s = 0; %s < %ld; %s++) %s.e[%s] = %s;\n", i, i, n, i, tmp, i, v);
    }
    return tmp;
}

static const char *ge_struct_lit(Node *e) {
    Type *t = gnt(e);
    Sym *vs = (Sym *)e->sym;
    int n = (int)e->list.n, i;
    const char **vals = (const char **)xalloc(sizeof(char *) * (size_t)(n + 1));
    Sym **fs = (Sym **)xalloc(sizeof(Sym *) * (size_t)(n + 1));
    Type **ts = (Type **)xalloc(sizeof(Type *) * (size_t)(n + 1));
    Node **nodes = (Node **)xalloc(sizeof(Node *) * (size_t)(n + 1));
    const char *res;
    for (i = 0; i < n; i++) {
        Node *fi = e->list.v[i];
        fs[i] = (Sym *)fi->sym;
        nodes[i] = fi->a;
        ts[i] = gm(field_ty(fs[i], t));
    }
    ge_eval_list(nodes, n, ts, vals);
    for (i = 0; i < n; i++) vals[i] = ge_fix(vals[i], ts[i]);
    if (e->b) {
        /* struct update: copy the base, then overwrite the given fields */
        const char *bv = ge_snap(t, ge_fix(ex_to(e->b, t), t));
        res = ge_tmp_ty(t);
        E("%s = %s;\n", res, bv);
        for (i = 0; i < n; i++) E("%s.%s = %s;\n", res, ge_field_member(fs[i]), vals[i]);
        return res;
    }
    return ge_new_adt(t, vs, fs, vals, n);
}

static const char *ge_range(Node *e) {
    Type *t = gnt(e);
    Sym *def = t->def;
    Type *it = t->nargs > 0 ? t->args[0] : 0;
    if (e->a && e->b && (e->flags & F_INCLUSIVE)) {
        Callee *c = check_callee(e, 0);
        const char *a[2];
        Node *nodes[2];
        Type *ts[2];
        nodes[0] = e->a;
        nodes[1] = e->b;
        ts[0] = ts[1] = it;
        ge_eval_list(nodes, 2, ts, a);
        return ge_call_callee(e, c, a, 2);
    }
    if (!e->a && !e->b) return "0";
    {
        Sym *fs[2];
        const char *vals[2];
        Node *nodes[2];
        Type *ts[2];
        int n = 0;
        if (e->a) { nodes[n] = e->a; ts[n] = it; fs[n] = ge_find_named(def, "start"); n++; }
        if (e->b) { nodes[n] = e->b; ts[n] = it; fs[n] = ge_find_named(def, "end"); n++; }
        ge_eval_list(nodes, n, ts, vals);
        return ge_new_adt(t, def, fs, vals, n);
    }
}

static const char *ge_ref_expr(Node *e) {
    int mut = (e->flags & F_MUT) != 0;
    Place p;
    Node *o = e->a;
    if (!mut) {
        /* constant promotion: a reference to a literal aggregate lives for the whole program */
        Node *q = o;
        while (q->kind == N_E_PAREN) q = q->a;
        if (q->kind == N_E_ARRAY || q->kind == N_E_LIT_INT || q->kind == N_E_LIT_FLOAT || q->kind == N_E_LIT_BOOL ||
            q->kind == N_E_LIT_CHAR || q->kind == N_E_LIT_STR) {
            int ok = 1;
            long i;
            if (q->kind == N_E_ARRAY)
                for (i = 0; i < q->list.n; i++) {
                    int k = q->list.v[i]->kind;
                    if (k != N_E_LIT_INT && k != N_E_LIT_FLOAT && k != N_E_LIT_BOOL && k != N_E_LIT_CHAR) ok = 0;
                }
            if (ok && !o->adj) {
                Type *t = gnt(o);
                const char *v = ex0(o, t), *tmp = S("t%d", cg_new_id());
                if (v == GE_NEVER) return "0";
                ge_decl_line(S("static %s", ge_ct(t)), tmp, 0);
                if (!cg_is_zst(t)) E("%s = %s;\n", tmp, v);
                return ge_addr(ge_pl_make(PK_LV, tmp, t, 0));
            }
        }
    }
    p = ge_place_adj(o, mut);
    return ge_addr(p);
}

static const char *ex0(Node *e, Type *want) {
    Type *t = gnt(e);
    switch (e->kind) {
    case N_E_LIT_INT:
        if (ge_is_float(t)) return S("%ld.0", (long)e->ival);
        return ge_int_lit((long)e->ival);
    case N_E_LIT_FLOAT:
        return S("(%s)", ge_float_lit(e));
    case N_E_LIT_STR:
        return ge_str_value(e->str, e->slen);
    case N_E_LIT_BYTESTR:
        return S("((long)%s)", cg_str_literal(e->str, e->slen));
    case N_E_LIT_CHAR: case N_E_LIT_BYTE: case N_E_LIT_BOOL:
        return S("%ld", (long)e->ival);
    case N_E_PATH:
        return ge_path_value(e);
    case N_E_PAREN:
        return ex_to(e->a, want);
    case N_E_UNARY:
        return ge_unary(e);
    case N_E_DEREF: case N_E_FIELD: case N_E_TUPLE_FIELD: case N_E_INDEX:
        return ge_take(ge_place0(e, 0));
    case N_E_REF:
        return ge_ref_expr(e);
    case N_E_TRY:
        return ge_try(e);
    case N_E_BINARY:
        return ge_binary(e, want);
    case N_E_ASSIGN:
        return ge_assign(e);
    case N_E_ASSIGN_OP:
        return ge_assign_op(e);
    case N_E_CAST:
        return ge_cast_expr(e);
    case N_E_CALL:
        return ge_call_expr(e);
    case N_E_METHOD_CALL:
        return ge_method_call(e);
    case N_E_RANGE:
        return ge_range(e);
    case N_BLOCK:
        return ge_block_value(e, t);
    case N_E_IF:
        return ge_if(e, want);
    case N_E_MATCH:
        return ge_match(e, want);
    case N_E_WHILE:
        return ge_while(e);
    case N_E_LOOP:
        return ge_loop(e);
    case N_E_FOR:
        return ge_for(e);
    case N_E_BREAK:
        return ge_break(e);
    case N_E_CONTINUE:
        return ge_continue(e);
    case N_E_RETURN:
        return ge_return(e);
    case N_E_CLOSURE:
        return ge_closure_value(e);
    case N_E_STRUCT:
        return ge_struct_lit(e);
    case N_E_TUPLE:
        return ge_tuple(e);
    case N_E_ARRAY:
        return ge_array(e);
    case N_E_REPEAT:
        return ge_repeat(e);
    case N_E_UNDERSCORE:
        return "0";
    }
    fatal(S("code generator: unsupported expression %s", node_kind_name(e->kind)));
    return "0";
}

/* ================================================================== function bodies */

typedef struct GSave {
    StrBuf *out;
    StrBuf body, decl;
    GScope *sc;
    int nsc, csc;
    GLoop *loops;
    int nloops, cloops;
    Node *closure;
    Type *ret;
    const char *envtype;
    Node *extend;
    Local *locals[GE_LHASH];
    int ncommits;
    struct GSave *prev;
} GSave;

static GSave *ge_saved;

static void ge_begin(Type *ret, Node *closure) {
    GSave *sv = (GSave *)xalloc(sizeof(GSave));
    int i;
    sv->out = ge_out;
    sv->body = ge_body;
    sv->decl = ge_decl;
    sv->sc = ge_sc;
    sv->nsc = ge_nsc;
    sv->csc = ge_csc;
    sv->loops = ge_loops;
    sv->nloops = ge_nloops;
    sv->cloops = ge_cloops;
    sv->closure = ge_closure;
    sv->ret = ge_ret;
    sv->envtype = ge_envtype;
    sv->extend = ge_extend;
    sv->ncommits = ge_ncommits;
    for (i = 0; i < GE_LHASH; i++) sv->locals[i] = ge_locals[i];
    sv->prev = ge_saved;
    ge_saved = sv;
    ge_locals_reset();
    sb_init(&ge_body);
    sb_init(&ge_decl);
    ge_out = &ge_body;
    ge_sc = 0;
    ge_nsc = ge_csc = 0;
    ge_loops = 0;
    ge_nloops = ge_cloops = 0;
    ge_closure = closure;
    ge_ret = ret;
    ge_envtype = 0;
    ge_extend = 0;
    ge_ncommits = 0;
    ge_gen++;
    ge_push_scope(SC_FN);
}

static void ge_end(void) {
    GSave *sv = ge_saved;
    int i;
    ge_out = sv->out;
    ge_body = sv->body;
    ge_decl = sv->decl;
    ge_sc = sv->sc;
    ge_nsc = sv->nsc;
    ge_csc = sv->csc;
    ge_loops = sv->loops;
    ge_nloops = sv->nloops;
    ge_cloops = sv->cloops;
    ge_closure = sv->closure;
    ge_ret = sv->ret;
    ge_envtype = sv->envtype;
    ge_extend = sv->extend;
    ge_ncommits = sv->ncommits;
    for (i = 0; i < GE_LHASH; i++) ge_locals[i] = sv->locals[i];
    ge_saved = sv->prev;
}

/* a parameter: the C parameter `cname` holds the value */
static void ge_param(Node *pn, Type *pt, const char *cname) {
    Node *pat = pn->kind == N_PARAM ? pn->a : 0;
    if (pn->kind == N_SELF_PARAM) {
        Sym *s = (Sym *)pn->sym;
        Local *l;
        if (!s) return;
        l = ge_local_new(s, pt, LK_VAR);
        l->name = cname;
        if (cg_needs_drop(pt)) {
            l->flag = ge_new_flag();
            ge_scope_add(0, cname, pt, l->flag);
            E("%s = 1;\n", l->flag);
        }
        return;
    }
    if (!pat) return;
    if (pat->kind == N_P_IDENT && !pat->a && pat->sym && ((Sym *)pat->sym)->kind == SYM_LOCAL &&
        !(pat->adj & (ADJ_AUTOREF | ADJ_AUTOREF_MUT))) {
        Local *l = ge_local_new((Sym *)pat->sym, pt, LK_VAR);
        l->name = cname;
        if (cg_needs_drop(pt)) {
            l->flag = ge_new_flag();
            ge_scope_add(0, cname, pt, l->flag);
            E("%s = 1;\n", l->flag);
        }
        return;
    }
    {
        Local *tl = ge_temp_local(cname, pt, 0);
        Place sp = ge_pl_make(PK_LV, cname, pt, tl);
        ge_set_init(tl);
        ge_ncommits = 0;
        ge_pat_bind(pat, sp);
        ge_commit();
    }
}

static const char *ge_finish_value(const char *v, Type *rt) {
    if (cg_is_zst(rt) || ge_is_never(rt)) return "0";
    v = ge_fix(v, rt);
    if (ge_sc[0].n > 0 && !ge_stable(v)) v = ge_snap(rt, v);
    return v;
}

static void ge_flush(const char *header, StrBuf *dst) {
    sb_puts(dst, header);
    sb_puts(dst, ge_decl.s);
    sb_puts(dst, ge_body.s);
    sb_puts(dst, "}\n\n");
}

void cg_fn_body(Sym *fn, const char *cname) {
    Node *n = fn->node;
    Type *sig = gm(fn->ty), *ret;
    StrBuf hdr, path;
    int i, pi = 0;
    const char *v;
    if (!n || !n->c) fatal("code generator: function without a body");
    ret = gm(sig->ret);
    ge_begin(ret, 0);
    sb_init(&hdr);
    sb_init(&path);
    sym_path(fn, &path);
    sb_puts(&hdr, "/* ");
    sb_puts(&hdr, path.s);
    sb_puts(&hdr, " */\nstatic ");
    sb_puts(&hdr, ge_ct(ret));
    sb_putc(&hdr, ' ');
    sb_puts(&hdr, cname);
    sb_putc(&hdr, '(');
    for (i = 0; i < n->list.n; i++) {
        Node *pn = n->list.v[i];
        Type *pt;
        const char *pname;
        if (pi >= sig->nparams) break;
        pt = gm(sig->params[pi]);
        pname = S("a%d", pi);
        if (pi) sb_puts(&hdr, ", ");
        sb_puts(&hdr, ge_ct(pt));
        sb_putc(&hdr, ' ');
        sb_puts(&hdr, pname);
        ge_param(pn, pt, pname);
        pi++;
    }
    if (!pi) sb_puts(&hdr, "void");
    sb_puts(&hdr, ") {\n");
    ge_line(n->c);
    v = ge_block_value(n->c, ret);
    v = ge_finish_value(v, ret);
    ge_pop_scope(1);
    E("return %s;\n", v);
    ge_flush(hdr.s, &cg_sect[CG_FUNCS]);
    ge_end();
}

void cg_closure_body(Node *closure, const char *cname) {
    Type *ct = gm((Type *)closure->ty), *ret;
    Capture *caps = 0;
    int ncaps = closure_captures(closure, &caps), i;
    StrBuf hdr;
    const char *v;
    ret = gm(ct->ret);
    ge_begin(ret, closure);
    ge_envtype = ge_ct(ct);
    sb_init(&hdr);
    sb_puts(&hdr, "static ");
    sb_puts(&hdr, ge_ct(ret));
    sb_putc(&hdr, ' ');
    sb_puts(&hdr, cname);
    sb_puts(&hdr, "(long env");
    for (i = 0; i < ncaps; i++) {
        Local *l = ge_local_new(caps[i].local, gm((Type *)caps[i].local->ty), caps[i].mode == CAP_MOVE ? LK_CAPVAL : LK_CAPREF);
        l->field = S("c%d", i);
    }
    for (i = 0; i < closure->list.n; i++) {
        Node *pn = closure->list.v[i];
        Type *pt = gm(ct->params[i]);
        const char *pname = S("a%d", i);
        sb_puts(&hdr, ", ");
        sb_puts(&hdr, ge_ct(pt));
        sb_putc(&hdr, ' ');
        sb_puts(&hdr, pname);
        ge_param(pn, pt, pname);
    }
    sb_puts(&hdr, ") {\n");
    ge_line(closure->b);
    v = ex_to(closure->b, ret);
    v = ge_finish_value(v, ret);
    ge_pop_scope(1);
    E("return %s;\n", v);
    ge_flush(hdr.s, &cg_sect[CG_FUNCS]);
    ge_end();
}

void cg_static_init(Sym *s, const char *gname) {
    Type *t = gm((Type *)s->ty);
    Node *init = s->node ? s->node->b : 0;
    const char *v;
    if (!init) return;
    ge_begin(ty_unit(), 0);
    ge_line(init);
    v = ex_to(init, t);
    if (v != GE_NEVER && !cg_is_zst(t)) E("%s = %s;\n", gname, v);
    ge_pop_scope(1);
    sb_puts(&cg_sect[CG_MAIN], ge_decl.s);
    sb_puts(&cg_sect[CG_MAIN], ge_body.s);
    ge_end();
}

/*
 * Runtime functions called from the generated code (all in lib/rust_rt.c):
 *   rs_panic, rs_panic_bounds, rs_check_range, rs_check_str_range, rs_wmul,
 *   rs_mul_ovf_i64, rs_mul_ovf_u64, rs_fmod, rs_f64_to_int, rs_u64_to_f64,
 *   rs_f64_to_bits, rs_f64_from_bits, rs_sqrt, rs_floor, rs_ceil, rs_alloc,
 *   rs_realloc, rs_free, rs_alloc_bytes, rs_realloc_bytes, rs_free_bytes,
 *   rs_cmp_bytes, rs_fmt_f64, rs_parse_f64, rs_write_bytes, rs_read_byte,
 *   rs_arg_char (and the variables rs_argc, rs_argv), rs_exit, rs_abort
 */
