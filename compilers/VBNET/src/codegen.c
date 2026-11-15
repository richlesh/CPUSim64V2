/*
 * codegen.c - code generation: expressions (stack machine).
 *
 * A value is computed into r0 (integers, Boolean, Char, references) or f0
 * (Single and Double); temporaries are pushed on the stack.  Boolean True
 * is -1.  Integral values are kept in the range of their type: arithmetic
 * is done in 64 bits and checked (System.OverflowException).
 *
 * Frames: CALL pushes the return address and the old SF, and sets SF to
 * SP.  The caller pushes the arguments from left to right, so parameter i
 * of n is at sf[3 + n - 1 - i]; a ByRef parameter holds the address of the
 * variable.  Locals are at sf[0], sf[-1], ... (Sym.index), followed by the
 * temporaries of the code generator.  The run-time library (rt/vb_rt.c) is
 * called with the C convention: the arguments are pushed in reverse order.
 */

static FILE *cg_out;
static StrBuf cg_code;      /* the code of the method being generated */
static StrBuf cg_data;      /* constants and string literals */
static StrBuf cg_globals;   /* #global data */
static int cg_label = 0;
static int cg_nconst = 0;
static int cg_nstr = 0;
static Sym *cg_method;
static int cg_ntemps, cg_maxtemps;
static int cg_epilogue;     /* the label of the method's return */
static const char *cg_asm_dir;  /* the directory of the .asm file */

/* the .source line for the next instruction (written only before an
   instruction, so that every .source line has code) */
static const char *cg_pend_src = 0;
static int cg_pend_line = 0;

static void emit(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    if (cg_pend_src) {
        sb_printf(&cg_code, ".source \"%s\", %d\n", cg_pend_src, cg_pend_line);
        cg_pend_src = 0;
    }
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    sb_puts(&cg_code, "    ");
    sb_puts(&cg_code, buf);
    sb_putc(&cg_code, '\n');
}

static void emit_label(int l) {
    sb_printf(&cg_code, "__vbL%d:\n", l);
}

static int new_label(void) { return ++cg_label; }

static void cg_fail(Node *at, const char *what) {
    serr(at, "%s not supported yet by the code generator.", what);
}

/* ------------------------------------------------------------ constants */

#define IMM_MAX 0x1FFFFFFFFFFLL     /* move immediates have 42 signed bits */

static void load_int(int reg, i64 v) {
    if (v >= -IMM_MAX && v <= IMM_MAX) emit("move    r%d, %lld", reg, (long long)v);
    else {
        int k = ++cg_nconst;
        sb_printf(&cg_data, "__vbK%d: .dci 0x%llx\n", k, (u64)v);
        emit("load    r%d, __vbK%d", reg, k);
    }
}

static void load_double(int freg, double d) {
    int k = ++cg_nconst;
    u64 bits;
    memcpy(&bits, &d, sizeof bits);
    sb_printf(&cg_data, "__vbK%d: .dci 0x%llx\n", k, bits);
    emit("load    f%d, __vbK%d", freg, k);
}

/* a string literal: two header words (tag, GC), then the length and the
   codepoints (.dcw) */
static int string_literal(const char *s) {
    int k = ++cg_nstr;
    StrBuf b;
    sb_init(&b);
#ifdef __CPUSIM64__
    for (; *s; s++) {
        int c = *s;
#else
    while (*s) {
        int c = (unsigned char)*s++, extra = 0;
        if (c >= 0xF0) { c &= 0x07; extra = 3; }
        else if (c >= 0xE0) { c &= 0x0F; extra = 2; }
        else if (c >= 0xC0) { c &= 0x1F; extra = 1; }
        while (extra-- > 0 && *s) c = (c << 6) | ((unsigned char)*s++ & 0x3F);
#endif
        if (c >= 32 && c < 127 && c != '"' && c != '\\') sb_putc(&b, c);
        else sb_printf(&b, "\\u{%x}", c);
    }
    sb_printf(&cg_data, "    .dci %d\n    .dci 0\n", TY_STRING);
    if (b.len == 0) sb_printf(&cg_data, "__vbS%d: .dci 0\n", k);
    else sb_printf(&cg_data, "__vbS%d: .dcw \"%s\"\n", k, b.s);
    free(b.s);
    return k;
}

/* ------------------------------------------------------------ helpers */

static int is_f(Type *t) { return t && ty_float(t); }

static void push_val(Type *t) { emit(is_f(t) ? "push    f0" : "push    r0"); }

/* reverses the n words on top of the stack (for the C argument order) */
static void reverse_args(int n) {
    int i;
    for (i = 0; i < n / 2; i++) {
        emit("load    r1, sp[%d]", 1 + i);
        emit("load    r2, sp[%d]", n - i);
        emit("store   r2, sp[%d]", 1 + i);
        emit("store   r1, sp[%d]", n - i);
    }
}

/* calls a run-time function with the n words pushed (left to right) */
static void rt_call(const char *fn, int n) {
    reverse_args(n);
    emit("call    __c_vbrt_%s", fn);
    if (n) emit("add     sp, %d", n);
}

static void rt_throw_jump(const char *cond, const char *what) {
    emit("jump    %s, __vb_%s", cond, what);
}

static int alloc_temp(void) {
    int t = cg_method->locals.n + cg_ntemps++;
    if (cg_ntemps > cg_maxtemps) cg_maxtemps = cg_ntemps;
    return t;
}

static void free_temp(void) { cg_ntemps--; }

/* the frame offset of a parameter */
static int param_off(Sym *p) {
    return 3 + p->owner->params.n - 1 - p->index;
}

/* Me: after the parameters */
static void load_me(void) {
    emit("load    r0, sf[%d]", 3 + cg_method->params.n);
}

static int is_global_var(Sym *s) {
    return s->kind == S_FIELD || (s->kind == S_LOCAL && (s->mods & M_STATIC));
}

static void cg_expr(Node *e);

/* ------------------------------------------------------------ structure values */

/* a structure value that nothing else refers to (it need not be copied) */
static int is_fresh(Node *e) {
    return e->kind == N_NEW || e->kind == N_INVOKE || e->kind == N_NOTHING ||
           (e->kind == N_CONV && e->a->type->kind != TY_STRUCT);
}

static void struct_copy_if(Node *e) {
    if (e->type && e->type->kind == TY_STRUCT && !is_fresh(e)) {
        emit("push    r0");
        emit("call    __c_vbrt_struct_copy");
        emit("add     sp, 1");
    }
}

/* a value to be stored: a structure is copied */
static void cg_value(Node *e) {
    cg_expr(e);
    struct_copy_if(e);
}

static void struct_new(Type *t) {
    emit("move    r0, __vbD%d+1", t->sym->desc);
    emit("push    r0");
    emit("call    __c_vbrt_struct_new");
    emit("add     sp, 1");
}

/* ------------------------------------------------------------ environments */

/* The variables that lambdas capture are elements of environment objects
   (Object arrays: element 0 the enclosing environment, then the variables).
   A block, loop or method whose scope has captured variables makes its
   environment each time it starts; a lambda's Me is the innermost
   environment where it is created, and it reaches the outer ones through
   element 0. */

typedef struct {
    Scope *sc;
    int slot;
} EnvLv;

static EnvLv cg_envs[64];
static int cg_nenvs;

#define ENV_ELEM(k) (3 + (k))

/* the innermost environment into r<reg> (0 if none) */
static void env_current(int reg) {
    if (cg_nenvs) emit("load    r%d, sf[%d]", reg, -cg_envs[cg_nenvs - 1].slot);
    else if (cg_method && (cg_method->sflags & SF_LAMBDA)) emit("load    r%d, sf[%d]", reg, 3 + cg_method->params.n);
    else emit("move    r%d, 0", reg);
}

/* the environment of the captured variable s into r<reg> */
static void env_of(Sym *s, int reg) {
    Scope *sc = s->scope;
    int i, k;
    for (i = cg_nenvs - 1; i >= 0; i--)
        if (cg_envs[i].sc == sc) {
            emit("load    r%d, sf[%d]", reg, -cg_envs[i].slot);
            return;
        }
    if (!cg_method || !(cg_method->sflags & SF_LAMBDA)) serr(0, "internal error: no environment for '%s'", s->spelling);
    /* through the environments of the lambda's creation */
    emit("load    r%d, sf[%d]", reg, 3 + cg_method->params.n);
    for (k = cg_method->nenvchain - 1; k >= 0 && cg_method->envchain[k] != sc; k--)
        emit("load    r%d, r%d[%d]", reg, reg, ENV_ELEM(0));
    if (k < 0) serr(0, "internal error: environment of '%s' not found", s->spelling);
}

/* the scopes visible here, outermost first (the environment chain of a lambda) */
static Scope **env_snapshot(int *n) {
    int base = (cg_method && (cg_method->sflags & SF_LAMBDA)) ? cg_method->nenvchain : 0, i;
    Scope **v = xalloc(sizeof(Scope *) * (base + cg_nenvs + 1));
    for (i = 0; i < base; i++) v[i] = cg_method->envchain[i];
    for (i = 0; i < cg_nenvs; i++) v[base + i] = cg_envs[i].sc;
    *n = base + cg_nenvs;
    return v;
}

static int alloc_temp(void);
static void free_temp(void);
static void rt_call(const char *fn, int n);

/* makes the environment of a scope (its captured variables are in syms) */
static void env_enter(Scope *sc, SymVec *syms) {
    int slot = alloc_temp(), i;
    emit("move    r0, %d", -(256 * TY_OBJECT + 65536));
    emit("push    r0");
    emit("move    r0, 1");
    emit("push    r0");
    emit("move    r0, %d", sc->nenv);
    emit("push    r0");
    rt_call("array_new", 3);
    emit("store   r0, sf[%d]", -slot);
    env_current(1);
    emit("store   r1, r0[%d]", ENV_ELEM(0));
    for (i = 0; i < syms->n; i++) {
        Sym *s = syms->v[i];
        if (s->scope != sc) continue;
        if (s->kind == S_PARAM) {
            emit("load    r1, sf[%d]", 3 + s->owner->params.n - 1 - s->index);
            emit("load    r0, sf[%d]", -slot);
            emit("store   r1, r0[%d]", ENV_ELEM(s->envslot));
        } else if (s->name == intern("$me")) {
            emit("load    r0, sf[%d]", -slot);
            emit("load    r1, sf[%d]", 3 + cg_method->params.n);
            emit("store   r1, r0[%d]", ENV_ELEM(s->envslot));
        } else if (s->type && s->type->kind == TY_STRUCT) {
            emit("move    r0, __vbD%d+1", s->type->sym->desc);
            emit("push    r0");
            emit("call    __c_vbrt_struct_new");
            emit("add     sp, 1");
            emit("load    r1, sf[%d]", -slot);
            emit("store   r0, r1[%d]", ENV_ELEM(s->envslot));
        }
    }
    if (cg_nenvs == 64) serr(0, "Lambdas nested too deeply.");
    cg_envs[cg_nenvs].sc = sc;
    cg_envs[cg_nenvs].slot = slot;
    cg_nenvs++;
}

static void env_leave(void) {
    cg_nenvs--;
    free_temp();
}

/* the captured variables of the method being generated, by scope */
static SymVec cg_captured;

/* ------------------------------------------------------------ variables */

static void load_var(Sym *s) {
    const char *r = is_f(s->type) ? "f0" : "r0";
    if (s->sflags & SF_CAPTURED) {
        env_of(s, 1);
        emit("load    %s, r1[%d]", r, ENV_ELEM(s->envslot));
        return;
    }
    if (is_global_var(s)) emit("load    %s, __vbG%d+1", r, s->index);
    else if (s->kind == S_PARAM) {
        if (s->mods & M_BYREF) {
            emit("load    r1, sf[%d]", param_off(s));
            emit("load    %s, r1[0]", r);
        } else emit("load    %s, sf[%d]", r, param_off(s));
    } else emit("load    %s, sf[%d]", r, -s->index);
}

/* stores r0 / f0 */
static void store_var(Sym *s) {
    const char *r = is_f(s->type) ? "f0" : "r0";
    if (s->sflags & SF_CAPTURED) {
        env_of(s, 1);
        emit("store   %s, r1[%d]", r, ENV_ELEM(s->envslot));
        return;
    }
    if (is_global_var(s)) emit("store   %s, __vbG%d+1", r, s->index);
    else if (s->kind == S_PARAM) {
        if (s->mods & M_BYREF) {
            emit("load    r1, sf[%d]", param_off(s));
            emit("store   %s, r1[0]", r);
        } else emit("store   %s, sf[%d]", r, param_off(s));
    } else emit("store   %s, sf[%d]", r, -s->index);
}

/* the address of a variable into r0 */
static void addr_var(Sym *s) {
    if (s->sflags & SF_CAPTURED) {
        env_of(s, 0);
        emit("add     r0, %d", ENV_ELEM(s->envslot));
        return;
    }
    if (is_global_var(s)) emit("move    r0, __vbG%d+1", s->index);
    else if (s->kind == S_PARAM) {
        if (s->mods & M_BYREF) emit("load    r0, sf[%d]", param_off(s));
        else emit("add     r0, sf, %d", param_off(s));
    } else emit("add     r0, sf, %d", -s->index);
}

static int array_tag(Type *t) {
    /* the runtime tag: -(256 * element kind + 65536 * rank); enums count as
       their underlying type, classes as Object */
    Type *e = ty_under(t->elem);
    int ek = e->kind == TY_CLASS ? TY_OBJECT : e->kind == TY_STRUCT ? TY_OBJECT : e->kind;
    return -(256 * ek + 65536 * t->rank);
}

/* the address of an array element into r0 */
static void addr_index(Node *e) {
    Type *at = e->a->type;
    int i, rank = at->rank;
    cg_expr(e->a);
    if (rank == 1) {
        emit("push    r0");
        cg_expr(e->list->v[0]);
        emit("move    r1, r0");
        emit("pop     r0");
        emit("test    r0");
        rt_throw_jump("z", "null");
        emit("test    r1");
        rt_throw_jump("n", "index");
        emit("load    r2, r0[0]");
        emit("cmp     r1, r2");
        rt_throw_jump("ge", "index");
        emit("add     r0, r0, r1");
        emit("add     r0, 3");
        return;
    }
    emit("push    r0");
    emit("move    r0, %d", rank);
    emit("push    r0");
    for (i = 0; i < rank; i++) {
        cg_expr(e->list->v[i]);
        emit("push    r0");
    }
    rt_call("index", rank + 2);
}

/* the address of an lvalue (EC_VAR) into r0 */
/* the address of an instance field into r0 */
static void addr_field(Node *e) {
    cg_expr(e->a);
    if (e->a->type->kind == TY_CLASS) {
        emit("test    r0");
        rt_throw_jump("z", "null");
    }
    if (e->sym->index) emit("add     r0, %d", e->sym->index);
}

static void cg_addr(Node *e) {
    if (e->kind == N_NAME) addr_var(e->sym);
    else if (e->kind == N_INDEX) addr_index(e);
    else if (e->kind == N_MEMBER) addr_field(e);
    else cg_fail(e, "This kind of variable is");
}

/* ------------------------------------------------------------ the assignment target */

/* N_TARGET: the value of the assignment target (compound assignment), the
   temporary of a ByRef copy-back, or the element of a For Each loop */
enum { TG_NONE, TG_VAR, TG_ADDR, TG_SLOT };
static int tg_mode;
static Sym *tg_var;
static int tg_slot;

static void load_target(Type *t) {
    const char *r = is_f(t) ? "f0" : "r0";
    switch (tg_mode) {
    case TG_VAR: load_var(tg_var); return;
    case TG_ADDR:
        emit("load    r1, sf[%d]", -tg_slot);
        emit("load    %s, r1[0]", r);
        return;
    case TG_SLOT:
        emit("load    %s, sf[%d]", r, -tg_slot);
        return;
    }
    serr(0, "internal error: no assignment target");
}

/* Select Case: the slot of the selector value */
static int sel_slot = -1;

/* ?. : the slot of the receiver of the innermost null-conditional access */
static int q_slot = -1;

static void cg_qcond(Node *e) {
    int slot = alloc_temp(), sv = q_slot, l_null = new_label(), l_end = new_label();
    cg_expr(e->a);
    emit("store   r0, sf[%d]", -slot);
    if (e->a->type->kind == TY_STRUCT) {
        /* a nullable: its HasValue (field 0 of the box) */
        emit("load    r0, r0[0]");
    }
    emit("test    r0");
    emit("jump    z, __vbL%d", l_null);
    q_slot = slot;
    cg_expr(e->b);
    q_slot = sv;
    emit("jump    __vbL%d", l_end);
    emit_label(l_null);
    if (e->type && e->type->kind == TY_STRUCT) struct_new(e->type);     /* an empty T? */
    else emit("move    r0, 0");
    emit_label(l_end);
    free_temp();
}

/* ------------------------------------------------------------ range checks */

/* checks that r0 is in the range of the integral type t (after 64-bit
   arithmetic on values of that type) */
static void range_check(Type *t) {
    i64 lo, hi;
    switch (t->kind) {
    case TY_SBYTE: lo = -128; hi = 127; break;
    case TY_BYTE: lo = 0; hi = 255; break;
    case TY_SHORT: lo = -32768; hi = 32767; break;
    case TY_USHORT: lo = 0; hi = 65535; break;
    case TY_INTEGER: lo = -2147483648LL; hi = 2147483647; break;
    case TY_UINTEGER: lo = 0; hi = 4294967295LL; break;
    default: return;
    }
    emit("cmp     r0, %lld", (long long)hi);
    rt_throw_jump("gt", "ovf");
    emit("cmp     r0, %lld", (long long)lo);
    rt_throw_jump("lt", "ovf");
}

/* ------------------------------------------------------------ conversions */

static void to_single(void) {
    emit("push    f0");
    rt_call("to_single", 1);
}

/* converts the value in r0 / f0 from type f to type t */
static void cg_convert(Type *f, Type *t, Node *at) {
    int fk = f->kind, tk = t->kind;
    if (f == t) return;
    if (fk == TY_ENUM && tk == TY_OBJECT) {
        /* a boxed enum: tagged with its type descriptor */
        emit("push    r0");
        emit("move    r0, __vbD%d+1", f->sym->desc);
        emit("push    r0");
        emit("call    __c_vbrt_box");
        emit("add     sp, 2");
        return;
    }
    if (fk == TY_STRUCT && tk == TY_OBJECT) {
        /* boxing: a copy */
        emit("push    r0");
        emit("call    __c_vbrt_struct_copy");
        emit("add     sp, 1");
        return;
    }
    if (fk == TY_ENUM) {
        f = f->elem;
        fk = f->kind;
    }
    if (tk == TY_ENUM) {
        t = t->elem;
        tk = t->kind;
    }
    if (f == t) return;
    if (fk == TY_NOTHING) {
        if (is_f(t)) emit("move    r0, 0");
        if (is_f(t)) emit("move    f0, r0");
        return;
    }
    if (tk == TY_OBJECT) {
        if (ty_ref(f)) return;
        push_val(f);
        emit("move    r0, %d", fk);
        emit("push    r0");
        emit("call    __c_vbrt_box");     /* vbrt_box(kind, bits) */
        emit("add     sp, 2");
        return;
    }
    if (fk == TY_ARRAY || tk == TY_ARRAY) {
        if (fk == TY_ARRAY && tk == TY_ARRAY) return;   /* covariance */
        emit("push    r0");
        rt_call(fk == TY_ARRAY ? "chars_to_str" : "s_tochararray", 1);
        return;
    }
    if (tk == TY_INTERFACE) {
        if (fk == TY_STRUCT) {
            /* boxing a structure */
            emit("push    r0");
            emit("call    __c_vbrt_struct_copy");
            emit("add     sp, 1");
            return;
        }
        if (conv_class(f, t) == CV_WIDEN) return;
        emit("push    r0");
        emit("move    r0, __vbD%d+1", t->sym->desc);
        emit("push    r0");
        rt_call("cast", 2);
        return;
    }
    if (fk == TY_INTERFACE && tk == TY_OBJECT) return;
    if (tk == TY_STRUCT) {
        /* unboxing: checked, a copy */
        emit("push    r0");
        emit("move    r0, __vbD%d+1", t->sym->desc);
        emit("push    r0");
        rt_call("cast", 2);
        emit("test    r0");
        rt_throw_jump("z", "null");
        emit("push    r0");
        emit("call    __c_vbrt_struct_copy");
        emit("add     sp, 1");
        return;
    }
    if (tk == TY_CLASS && conv_class(f, t) == CV_WIDEN) return;     /* to a base class */
    if (tk == TY_CLASS) {
        /* Object -> class: checked */
        emit("push    r0");
        emit("move    r0, __vbD%d+1", t->sym->desc);
        emit("push    r0");
        rt_call("cast", 2);
        return;
    }
    if (fk == TY_OBJECT || fk == TY_STRING) {
        if (tk == TY_STRING) {
            emit("push    r0");
            rt_call("obj_str", 1);
            return;
        }
        emit("push    r0");
        emit("move    r0, %d", tk);
        emit("push    r0");
        rt_call("unbox", 2);
        if (is_f(t)) {
            emit("push    r0");
            emit("pop     f0");
        }
        return;
    }
    if (tk == TY_STRING) {
        if (fk == TY_CHAR) {
            emit("push    r0");
            rt_call("str_char", 1);
            return;
        }
        emit("move    r1, %d", fk);
        emit("push    r1");
        push_val(f);
        rt_call("to_str", 2);
        return;
    }
    if (fk == TY_CHAR || tk == TY_CHAR) cg_fail(at, "This conversion is");
    if (tk == TY_BOOLEAN) {
        if (is_f(f)) {
            emit("move    r1, 0");
            emit("move    f1, r1");
            emit("cmp     f0, f1");
        } else emit("test    r0");
        emit("move    nz, r0, -1, 0");
        return;
    }
    if (is_f(t)) {
        if (is_f(f)) {
            if (tk == TY_SINGLE) to_single();
            return;
        }
        if (fk == TY_ULONG) {
            emit("push    r0");
            rt_call("u2d", 1);
        } else emit("move    f0, r0");
        if (tk == TY_SINGLE && (fk == TY_INTEGER || fk == TY_UINTEGER || fk == TY_LONG || fk == TY_ULONG)) to_single();
        return;
    }
    /* to an integral type */
    if (is_f(f)) {
        emit("move    r0, %d", tk);
        emit("push    r0");
        emit("push    f0");
        emit("call    __c_vbrt_d2i");     /* vbrt_d2i(d, kind) */
        emit("add     sp, 2");
        return;
    }
    if (fk == TY_BOOLEAN) {
        /* True (-1) is all ones: the largest value of an unsigned type */
        switch (tk) {
        case TY_BYTE: emit("and     r0, 255"); break;
        case TY_USHORT: emit("and     r0, 65535"); break;
        case TY_UINTEGER: emit("move    r1, 4294967295"); emit("and     r0, r1"); break;
        }
        return;
    }
    if (fk == TY_ULONG || tk == TY_ULONG) {
        emit("test    r0");
        rt_throw_jump("n", "ovf");
        if (tk != TY_ULONG) range_check(t);
        return;
    }
    if (ty_bits(t) < ty_bits(f) || (ty_unsigned(t) && !ty_unsigned(f)) ||
        (ty_signed(t) && ty_unsigned(f) && ty_bits(t) <= ty_bits(f)))
        range_check(t);
}

/* ------------------------------------------------------------ operators */

/* r0 (left) op r1 (right) for integral types */
static void int_arith(int op, Type *t) {
    switch (op) {
    case P_PLUS: emit("add     r0, r1"); break;
    case P_MINUS: emit("sub     r0, r1"); break;
    case P_STAR:
        if (t->kind == TY_LONG) {
            emit("push    r0");
            emit("push    r1");
            rt_call("l_mul", 2);
            return;
        }
        emit("mult    r0, r1");
        break;
    case P_BACKSLASH: case K_Mod:
        emit("test    r1");
        rt_throw_jump("z", "divz");
        if (t->kind == TY_LONG) {
            /* Long.MinValue \ -1 overflows; x Mod -1 is 0 */
            int ok = new_label(), done = new_label();
            emit("cmp     r1, -1");
            emit("jump    ne, __vbL%d", ok);
            if (op == K_Mod) emit("move    r0, 0");
            else {
                emit("load    r2, __vb_lmin");
                emit("cmp     r0, r2");
                rt_throw_jump("eq", "ovf");
                emit("neg     r0");
            }
            emit("jump    __vbL%d", done);
            emit_label(ok);
            if (op == P_BACKSLASH) emit("div     r0, r1");
            else {
                emit("div     r2, r3, r0, r1");
                emit("move    r0, r3");
            }
            emit_label(done);
            return;
        }
        if (op == P_BACKSLASH) emit("div     r0, r1");
        else {
            emit("div     r2, r3, r0, r1");
            emit("move    r0, r3");
        }
        break;
    case K_And: case K_AndAlso: emit("and     r0, r1"); return;
    case K_Or: case K_OrElse: emit("or      r0, r1"); return;
    case K_Xor: emit("xor     r0, r1"); return;
    }
    if (t->kind == TY_LONG && (op == P_PLUS || op == P_MINUS)) rt_throw_jump("o", "ovf");
    else range_check(t);
}

/* r0 (left) op r1 (right) for ULong: vbrt_ul_op(op, a, b) */
static void ulong_arith(int op) {
    int c = op == P_PLUS ? '+' : op == P_MINUS ? '-' : op == P_STAR ? '*' : op == P_BACKSLASH ? '/' : '%';
    emit("push    r1");
    emit("push    r0");
    emit("move    r0, %d", c);
    emit("push    r0");
    emit("call    __c_vbrt_ul_op");
    emit("add     sp, 3");
}

static const char *cond_of(int op) {
    switch (op) {
    case P_EQ: return "eq";
    case P_NE: return "ne";
    case P_LT: return "lt";
    case P_GT: return "gt";
    case P_LE: return "le";
    }
    return "ge";
}

/* sets r0 to True (-1) or False from comparing r0 (or f0) with r1 (f1) */
static void compare(int op, Type *t) {
    if (is_f(t)) {
        emit("cmp     f0, f1");
        emit("move    %s, r0, -1, 0", cond_of(op));
        return;
    }
    if (t->kind == TY_STRING) {
        emit("push    r0");
        emit("push    r1");
        emit("move    r0, %d", opts->compare_text ? 1 : 0);
        emit("push    r0");
        rt_call("strcmp", 3);
        emit("cmp     r0, 0");
    } else if (t->kind == TY_ULONG) {
        emit("push    r0");
        emit("push    r1");
        rt_call("ul_cmp", 2);
        emit("cmp     r0, 0");
    } else if (t->kind == TY_LONG) {
        /* a 64-bit compare can overflow: then the sign is the wrong way */
        int ov = new_label(), done = new_label();
        emit("cmp     r0, r1");
        emit("jump    o, __vbL%d", ov);
        emit("move    %s, r0, -1, 0", cond_of(op));
        emit("jump    __vbL%d", done);
        emit_label(ov);
        /* overflow: r0 < r1 iff r0 is negative */
        if (op == P_EQ) emit("move    r0, 0");
        else if (op == P_NE) emit("move    r0, -1");
        else {
            emit("test    r0");
            emit("move    %s, r0, -1, 0", (op == P_LT || op == P_LE) ? "n" : "nn");
        }
        emit_label(done);
        return;
    } else emit("cmp     r0, r1");
    emit("move    %s, r0, -1, 0", cond_of(op));
}

/* wraps r0 to the width of type t (after a shift) */
static void wrap_width(Type *t) {
    int b = ty_bits(t);
    if (b == 64) return;
    if (ty_signed(t)) {
        emit("lshift  r0, %d", 64 - b);
        emit("arshift r0, %d", 64 - b);
    } else if (b == 32) {
        emit("move    r1, 4294967295");
        emit("and     r0, r1");
    } else emit("and     r0, %d", (1 << b) - 1);
}

static void cg_binary(Node *e) {
    int op = e->op;
    Type *t = e->a->type;
    if (op == K_AndAlso || op == K_OrElse) {
        int done = new_label();
        cg_expr(e->a);
        emit("test    r0");
        emit("jump    %s, __vbL%d", op == K_AndAlso ? "z" : "nz", done);
        cg_expr(e->b);
        emit_label(done);
        return;
    }
    if (op == K_Like) {
        cg_expr(e->a);
        emit("push    r0");
        cg_expr(e->b);
        emit("push    r0");
        emit("move    r0, %d", opts->compare_text ? 1 : 0);
        emit("push    r0");
        rt_call("like", 3);
        return;
    }
    cg_expr(e->a);
    push_val(t);
    cg_expr(e->b);
    if (is_f(e->b->type)) {
        emit("move    f1, f0");
        emit("pop     f0");
    } else {
        emit("move    r1, r0");
        if (is_f(t)) emit("pop     f0");
        else emit("pop     r0");
    }
    switch (op) {
    case P_EQ: case P_NE: case P_LT: case P_GT: case P_LE: case P_GE:
        compare(op, t);
        return;
    case K_Is: case K_IsNot:
        emit("cmp     r0, r1");
        emit("move    %s, r0, -1, 0", op == K_Is ? "eq" : "ne");
        return;
    case P_AMP:
        emit("push    r0");
        emit("push    r1");
        rt_call("concat", 2);
        return;
    case P_PLUS:
        if (t->kind == TY_STRING) {
            emit("push    r0");
            emit("push    r1");
            rt_call("concat", 2);
            return;
        }
        break;
    case P_SHL: case P_SHR:
        emit("and     r1, %d", ty_bits(t) - 1);
        if (op == P_SHL) emit("lshift  r0, r1");
        else emit(ty_unsigned(t) ? "rshift  r0, r1" : "arshift r0, r1");
        if (op == P_SHL) wrap_width(t);
        return;
    case P_CARET:
        emit("move    f2, f1");
        emit("move    f1, f0");
        emit("int     110");        /* iPOW */
        return;
    }
    if (is_f(t)) {
        switch (op) {
        case P_PLUS: emit("add     f0, f1"); break;
        case P_MINUS: emit("sub     f0, f1"); break;
        case P_STAR: emit("mult    f0, f1"); break;
        case P_SLASH: emit("div     f0, f1"); break;
        case K_Mod:
            emit("push    f1");
            emit("push    f0");
            emit("call    __c_vbrt_fmod");
            emit("add     sp, 2");
            break;
        default: cg_fail(e, "This operator is");
        }
        if (t->kind == TY_SINGLE) to_single();
        return;
    }
    if (t->kind == TY_ULONG && op != K_And && op != K_Or && op != K_Xor) {
        ulong_arith(op);
        return;
    }
    int_arith(op, t);
}

static void cg_unary(Node *e) {
    Type *t = e->type;
    cg_expr(e->a);
    switch (e->op) {
    case P_PLUS: return;
    case P_MINUS:
        if (is_f(t)) {
            emit("neg     f0");
            return;
        }
        if (t->kind == TY_LONG) {
            emit("load    r1, __vb_lmin");
            emit("cmp     r0, r1");
            rt_throw_jump("eq", "ovf");
        }
        emit("neg     r0");
        range_check(t);
        return;
    case K_Not:
        emit("compl   r0");
        if (ty_unsigned(t)) wrap_width(t);
        return;
    }
}

/* ------------------------------------------------------------ calls */

static void cg_new_array_of(Type *t, int n);
static void cg_invoke(Node *e);

/* Console.Write / WriteLine */
static void cg_console_write(Node *e, int nl) {
    Sym *m = e->sym;
    int np = m->params.n;
    if (np == 0) {
        emit("move    r0, 1");
        emit("push    r0");
        emit("move    r0, 0");
        emit("push    r0");
        emit("push    r0");
        emit("call    __c_vbrt_write");
        emit("add     sp, 3");
        return;
    }
    if (np == 1) {
        Type *pt = m->params.v[0]->type;
        int kind = pt->kind == TY_ARRAY ? TY_ARRAY : pt->kind;
        emit("move    r0, %d", nl);
        emit("push    r0");
        cg_expr(e->list->v[0]->a);
        push_val(pt);
        emit("move    r0, %d", kind);
        emit("push    r0");
        emit("call    __c_vbrt_write");
        emit("add     sp, 3");
        return;
    }
    /* (format, args...): the arguments as an Object() */
    {
        int fmt = alloc_temp(), i;
        cg_expr(e->list->v[0]->a);
        emit("store   r0, sf[%d]", -fmt);
        if (np == 2 && (m->params.v[1]->mods & M_PARAMARRAY)) cg_expr(e->list->v[1]->a);
        else {
            int arr = alloc_temp();
            cg_new_array_of(ty_array(ty[TY_OBJECT], 1), np - 1);
            emit("store   r0, sf[%d]", -arr);
            for (i = 1; i < np; i++) {
                cg_expr(e->list->v[i]->a);
                emit("load    r1, sf[%d]", -arr);
                emit("store   r0, r1[%d]", 3 + i - 1);
            }
            emit("load    r0, sf[%d]", -arr);
            free_temp();
        }
        emit("move    r1, %d", nl);
        emit("push    r1");
        emit("push    r0");
        emit("load    r0, sf[%d]", -fmt);
        emit("push    r0");
        emit("call    __c_vbrt_write_format");
        emit("add     sp, 3");
        free_temp();
    }
}

/* the builtins expanded here: "#N" floating point interrupts, "@id",
   "@tostr", "@tostrf" */
static void cg_special(Node *e) {
    const char *rt = e->sym->rt;
    NodeVec *args = e->list;
    if (rt[0] == '#') {
        cg_expr(args->v[0]->a);
        if (args->n == 2) {
            emit("push    f0");
            cg_expr(args->v[1]->a);
            emit("move    f2, f0");
            emit("pop     f1");
        } else emit("move    f1, f0");
        emit("int     %s", rt + 1);
        return;
    }
    if (strcmp(rt, "@invoke") == 0) {
        cg_invoke(e);
        return;
    }
    if (strcmp(rt, "@id") == 0) {
        cg_expr(args->v[0]->a);
        return;
    }
    if (strcmp(rt, "@tostr") == 0) {
        Type *t = e->a->type;
        cg_expr(e->a);
        if (t->kind == TY_STRING) return;
        if (t->kind == TY_ENUM) {
            emit("push    r0");
            emit("move    r0, __vbD%d+1", t->sym->desc);
            emit("push    r0");
            emit("call    __c_vbrt_enum_str");
            emit("add     sp, 2");
            return;
        }
        if (t->kind == TY_OBJECT || t->kind == TY_CLASS || t->kind == TY_ARRAY) {
            emit("push    r0");
            rt_call("obj_str", 1);
            return;
        }
        cg_convert(t, ty[TY_STRING], e);
        return;
    }
    /* @tostrf: vbrt_format_value(kind, bits, format) */
    emit("move    r0, %d", e->a->type->kind);
    emit("push    r0");
    cg_expr(e->a);
    push_val(e->a->type);
    cg_expr(args->v[0]->a);
    emit("push    r0");
    rt_call("format_value", 3);
}

static void cg_builtin(Node *e) {
    if (e->sym->rt) {
        cg_special(e);
        return;
    }
    switch (e->sym->builtin) {
    case B_CONSOLE_WRITE: cg_console_write(e, 0); return;
    case B_CONSOLE_WRITELINE: cg_console_write(e, 1); return;
    case B_CONSOLE_READLINE: emit("call    __c_vbrt_readline"); return;
    case B_STRING_LENGTH:
        cg_expr(e->a);
        emit("push    r0");
        rt_call("str_length", 1);
        return;
    case B_ARRAY_LENGTH:
        cg_expr(e->a);
        emit("push    r0");
        rt_call("array_length", 1);
        return;
    }
    cg_fail(e, "This library member is");
}

static int method_label(Sym *m);

/* a call of a method of the program */
static void cg_call(Node *e) {
    Sym *m = e->sym;
    int i, n = e->list->n, *temps = xalloc(sizeof(int) * (n + 1));
    int is_rt = m->rt && m->rt[0] != '#' && m->rt[0] != '@', npush = n;
    if (m->builtin || (m->rt && !is_rt)) {
        cg_builtin(e);
        return;
    }
    if (e->a) {
        /* the receiver of an instance method is the first argument */
        cg_expr(e->a);
        if (!is_rt && e->a->type->kind == TY_CLASS && e->a->kind != N_ME) {
            emit("test    r0");
            rt_throw_jump("z", "null");
        }
        push_val(e->a->type);
        npush++;
    }
    for (i = 0; i < n; i++) {
        Node *a = e->list->v[i];
        temps[i] = -1;
        if (a->op == K_ByRef) {
            if (a->a->cls == EC_VAR && !(a->flags & F_COPYBACK)) {
                cg_addr(a->a);
            } else {
                /* a temporary: a value, or a variable of another type (copy-back) */
                int t = alloc_temp();
                temps[i] = t;
                cg_expr(a->a);
                emit(is_f(a->a->type) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -t);
                emit("add     r0, sf, %d", -t);
            }
            emit("push    r0");
        } else {
            cg_value(a->a);
            push_val(a->a->type);
        }
    }
    if (is_rt) {
        /* "name" or "name:K" */
        const char *colon = strchr(m->rt, ':');
        char fn[64];
        int len = colon ? (int)(colon - m->rt) : (int)strlen(m->rt);
        memcpy(fn, m->rt, len);
        fn[len] = 0;
        if (colon) {
            emit("move    r0, %s", colon + 1);
            emit("push    r0");
            npush++;
        }
        reverse_args(npush);
        emit("call    __c_vbrt_%s", fn);
        if (npush) emit("add     sp, %d", npush);
    } else if (m->owner && m->owner->kind == S_TYPE && m->owner->type->kind == TY_INTERFACE) {
        /* an interface call: the itable of the interface in the object's class */
        emit("load    r0, sp[%d]", npush);
        emit("push    r0");
        emit("move    r0, __vbD%d+1", m->owner->desc);
        emit("push    r0");
        rt_call("itable", 2);
        emit("load    r1, r0[%d]", m->vslot);
        emit("call    r1");
        if (npush) emit("add     sp, %d", npush);
    } else if (m->vslot >= 0 && e->a && e->a->type->kind == TY_CLASS && !(e->a->flags & F_DIRECT)) {
        /* a virtual call: the slot of the object's vtable (descriptor word 7) */
        emit("load    r0, sp[%d]", npush);
        emit("load    r1, r0[-2]");
        emit("load    r1, r1[7]");
        emit("load    r1, r1[%d]", m->vslot);
        emit("call    r1");
        if (npush) emit("add     sp, %d", npush);
    } else {
        emit("call    __vbM%d", method_label(m));
        if (npush) emit("add     sp, %d", npush);
    }
    /* ByRef copy-back (keeping the result) */
    for (i = 0; i < n; i++) {
        Node *a = e->list->v[i];
        if (a->flags & F_COPYBACK) {
            int sv_mode = tg_mode, sv_slot = tg_slot, keep = alloc_temp();
            emit(is_f(m->type) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -keep);
            tg_mode = TG_SLOT;
            tg_slot = temps[i];
            if (a->c->kind == N_NAME) {
                cg_expr(a->b);
                store_var(a->c->sym);
            } else {
                int ad = alloc_temp();
                cg_addr(a->c);
                emit("store   r0, sf[%d]", -ad);
                cg_expr(a->b);
                emit("load    r1, sf[%d]", -ad);
                emit(is_f(a->c->type) ? "store   f0, r1[0]" : "store   r0, r1[0]");
                free_temp();
            }
            tg_mode = sv_mode;
            tg_slot = sv_slot;
            emit(is_f(m->type) ? "load    f0, sf[%d]" : "load    r0, sf[%d]", -keep);
            free_temp();
        }
    }
    for (i = n - 1; i >= 0; i--) if (temps[i] >= 0) free_temp();
    free(temps);
}

/* ------------------------------------------------------------ delegates */

/* a new delegate: fields 0 target, 1 method address, 2 next (System.Delegate) */
static void cg_dlgnew(Node *e) {
    Sym *m = e->sym;
    int obj = alloc_temp();
    emit("move    r0, __vbD%d+1", e->type->sym->desc);
    emit("push    r0");
    emit("move    r0, 3");
    emit("push    r0");
    emit("call    __c_vbrt_alloc");
    emit("add     sp, 2");
    emit("store   r0, sf[%d]", -obj);
    if (m->sflags & SF_LAMBDA) {
        /* the environment where the lambda is created */
        if (!m->envchain) m->envchain = env_snapshot(&m->nenvchain);
        env_current(1);
        emit("load    r0, sf[%d]", -obj);
        emit("store   r1, r0[0]");
        emit("move    r1, __vbM%d", method_label(m));
        emit("store   r1, r0[1]");
    } else if (!(m->sflags & SF_INSTANCE)) {
        emit("move    r1, __vbT%d", method_label(m));
        emit("store   r1, r0[1]");
    } else {
        Type *rt = e->a->type;
        cg_expr(e->a);
        if (rt->kind == TY_STRUCT) {
            emit("push    r0");
            emit("call    __c_vbrt_struct_copy");
            emit("add     sp, 1");
        }
        emit("test    r0");
        rt_throw_jump("z", "null");
        emit("load    r1, sf[%d]", -obj);
        emit("store   r0, r1[0]");
        if (m->owner->type->kind == TY_INTERFACE) {
            emit("push    r0");
            emit("move    r0, __vbD%d+1", m->owner->desc);
            emit("push    r0");
            rt_call("itable", 2);
            emit("load    r1, r0[%d]", m->vslot);
        } else if (m->vslot >= 0 && rt->kind == TY_CLASS && !(e->a->flags & F_DIRECT)) {
            emit("load    r1, r0[-2]");
            emit("load    r1, r1[7]");
            emit("load    r1, r1[%d]", m->vslot);
        } else emit("move    r1, __vbM%d", method_label(m));
        emit("load    r0, sf[%d]", -obj);
        emit("store   r1, r0[1]");
    }
    emit("load    r0, sf[%d]", -obj);
    free_temp();
}

/* d.Invoke(args): each delegate of the invocation list with the arguments
   (target first, as Me), the result of the last one */
static void cg_invoke(Node *e) {
    int n = e->list->n, d = alloc_temp(), i, top = new_label();
    int *slots = xalloc(sizeof(int) * (n + 1));
    cg_expr(e->a);
    emit("test    r0");
    rt_throw_jump("z", "null");
    emit("store   r0, sf[%d]", -d);
    for (i = 0; i < n; i++) {
        Node *a = e->list->v[i];
        slots[i] = alloc_temp();
        if (a->op == K_ByRef && a->a->cls == EC_VAR && !(a->flags & F_COPYBACK)) cg_addr(a->a);
        else if (a->op == K_ByRef) {
            int t = alloc_temp();
            cg_expr(a->a);
            emit(is_f(a->a->type) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -t);
            emit("add     r0, sf, %d", -t);
        } else {
            cg_value(a->a);
            if (is_f(a->a->type)) {
                emit("push    f0");
                emit("pop     r0");
            }
        }
        emit("store   r0, sf[%d]", -slots[i]);
    }
    emit_label(top);
    emit("load    r1, sf[%d]", -d);
    emit("load    r0, r1[0]");
    emit("push    r0");
    for (i = 0; i < n; i++) {
        emit("load    r0, sf[%d]", -slots[i]);
        emit("push    r0");
    }
    emit("load    r1, sf[%d]", -d);
    emit("load    r1, r1[1]");
    emit("call    r1");
    emit("add     sp, %d", n + 1);
    emit("load    r1, sf[%d]", -d);
    emit("load    r1, r1[2]");
    emit("store   r1, sf[%d]", -d);
    emit("test    r1");
    emit("jump    nz, __vbL%d", top);
    for (i = 0; i < n; i++) {
        Node *a = e->list->v[i];
        if (a->op == K_ByRef && !(a->a->cls == EC_VAR && !(a->flags & F_COPYBACK))) free_temp();
        free_temp();
    }
    free_temp();
    free(slots);
}

/* ------------------------------------------------------------ New */

static void cg_new(Node *e) {
    Type *t = e->type;
    Sym *ts = t->sym;
    int obj = alloc_temp(), i;
    if (t->kind == TY_STRUCT) struct_new(t);
    else {
        emit("move    r0, __vbD%d+1", ts->desc);
        emit("push    r0");
        emit("move    r0, %d", ts->size ? ts->size : 1);
        emit("push    r0");
        emit("call    __c_vbrt_alloc");       /* vbrt_alloc(words, tag) */
        emit("add     sp, 2");
    }
    emit("store   r0, sf[%d]", -obj);
    if (e->sym) {
        /* the constructor: Me, then the arguments */
        emit("push    r0");
        for (i = 0; i < e->list->n; i++) {
            Node *a = e->list->v[i];
            if (a->op == K_ByRef && a->a->cls == EC_VAR) cg_addr(a->a);
            else cg_value(a->a);
            push_val(a->a->type);
        }
        emit("call    __vbM%d", method_label(e->sym));
        emit("add     sp, %d", e->list->n + 1);
    }
    if (e->list3) {
        /* the Add calls of a collection initializer: the new object is the target */
        int sv_mode = tg_mode, sv_slot = tg_slot;
        tg_mode = TG_SLOT;
        tg_slot = obj;
        for (i = 0; i < e->list3->n; i++) cg_expr(e->list3->v[i]);
        tg_mode = sv_mode;
        tg_slot = sv_slot;
    }
    if (e->b) {
        for (i = 0; i < e->b->list->n; i++) {
            Node *fi = e->b->list->v[i];
            Sym *m = fi->sym;
            cg_value(fi->a);
            if (m->kind == S_FIELD) {
                emit("load    r1, sf[%d]", -obj);
                emit(is_f(m->type) ? "store   f0, r1[%d]" : "store   r0, r1[%d]", m->index);
            } else {
                /* a property setter: (Me, value) */
                push_val(m->type);
                emit("load    r0, sf[%d]", -obj);
                emit("push    r0");
                reverse_args(2);
                emit("call    __vbM%d", method_label(m->setter));
                emit("add     sp, 2");
            }
        }
    }
    emit("load    r0, sf[%d]", -obj);
    free_temp();
}

/* ------------------------------------------------------------ arrays */

/* a new 1-D array of n elements into r0 */
static void cg_new_array_of(Type *t, int n) {
    emit("move    r0, %d", array_tag(t));
    emit("push    r0");
    emit("move    r0, 1");
    emit("push    r0");
    emit("move    r0, %d", n - 1);
    emit("push    r0");
    rt_call("array_new", 3);
}

static void collect_dims(Node *init, int rank, int *dims) {
    int i;
    for (i = 0; i < rank; i++) {
        dims[i] = init->list->n;
        if (init->list->n == 0) {
            for (; i < rank; i++) dims[i] = 0;
            return;
        }
        init = init->list->v[0];
    }
}

static void store_elements(Node *init, int depth, int *lin, int arr_slot, int base) {
    int i;
    for (i = 0; i < init->list->n; i++) {
        Node *x = init->list->v[i];
        if (depth > 1) {
            store_elements(x, depth - 1, lin, arr_slot, base);
            continue;
        }
        cg_value(x);
        emit("load    r1, sf[%d]", -arr_slot);
        emit(is_f(x->type) ? "store   f0, r1[%d]" : "store   r0, r1[%d]", base + (*lin)++);
    }
}

/* an array from its bounds (upper bounds, or NULL) and initializer */
static void cg_array(Type *t, NodeVec *bounds, Node *init) {
    int rank = t->rank, i;
    emit("move    r0, %d", array_tag(t));
    emit("push    r0");
    emit("move    r0, %d", rank);
    emit("push    r0");
    if (bounds) {
        for (i = 0; i < rank; i++) {
            cg_expr(bounds->v[i]);
            emit("push    r0");
        }
    } else {
        int *dims = xalloc(sizeof(int) * rank);
        collect_dims(init, rank, dims);
        for (i = 0; i < rank; i++) emit("push    %d", dims[i] - 1);
        free(dims);
    }
    rt_call("array_new", rank + 2);
    if (t->elem->kind == TY_STRUCT) {
        /* the elements of a structure type */
        emit("push    r0");
        emit("move    r1, __vbD%d+1", t->elem->sym->desc);
        emit("push    r1");
        emit("load    r1, sp[2]");
        emit("push    r1");
        emit("call    __c_vbrt_array_fill");
        emit("add     sp, 2");
        emit("pop     r0");
    }
    if (init && init->list->n) {
        int slot = alloc_temp(), lin = 0;
        emit("store   r0, sf[%d]", -slot);
        store_elements(init, rank, &lin, slot, 2 + rank);
        emit("load    r0, sf[%d]", -slot);
        free_temp();
    }
}

/* ------------------------------------------------------------ interpolated strings */

static void cg_interp(Node *e) {
    int i, acc = alloc_temp(), first = 1;
    for (i = 0; i < e->list->n; i++) {
        Node *h = e->list->v[i];
        if (h->kind == N_STRING) {
            emit("move    r0, __vbS%d", string_literal(h->text));
        } else {
            Type *t = h->a->type;
            if (h->b || h->text) {
                /* alignment or format: through String.Format("{0,a:f}", value) */
                StrBuf f;
                int arr = alloc_temp();
                sb_init(&f);
                sb_puts(&f, "{0");
                if (h->b) sb_printf(&f, ",%lld", (long long)h->b->ival);
                if (h->text) {
                    sb_putc(&f, ':');
                    sb_puts(&f, h->text);
                }
                sb_putc(&f, '}');
                cg_new_array_of(ty_array(ty[TY_OBJECT], 1), 1);
                emit("store   r0, sf[%d]", -arr);
                cg_expr(h->a);
                cg_convert(t, ty[TY_OBJECT], h);
                emit("load    r1, sf[%d]", -arr);
                emit("store   r0, r1[3]");
                emit("push    r1");
                emit("move    r0, __vbS%d", string_literal(f.s));
                emit("push    r0");
                emit("call    __c_vbrt_format");
                emit("add     sp, 2");
                free_temp();
            } else {
                /* the value's ToString(), as for a boxed value (an enum gives its name) */
                cg_expr(h->a);
                if (t->kind != TY_STRING) {
                    cg_convert(t, ty[TY_OBJECT], h);
                    emit("push    r0");
                    rt_call("obj_str", 1);
                }
            }
        }
        if (first) {
            emit("store   r0, sf[%d]", -acc);
            first = 0;
        } else {
            emit("push    r0");
            emit("load    r0, sf[%d]", -acc);
            emit("push    r0");
            emit("call    __c_vbrt_concat");
            emit("add     sp, 2");
            emit("store   r0, sf[%d]", -acc);
        }
    }
    if (first) emit("move    r0, __vbS%d", string_literal(""));
    else emit("load    r0, sf[%d]", -acc);
    free_temp();
}

/* ------------------------------------------------------------ expressions */

static void cg_expr(Node *e) {
    switch (e->kind) {
    case N_INT:
        load_int(0, (i64)e->ival);
        return;
    case N_BOOL:
        emit("move    r0, %d", e->ival ? -1 : 0);
        return;
    case N_CHAR:
        emit("move    r0, %lld", (long long)e->ival);
        return;
    case N_FLOAT:
        load_double(0, e->dval);
        return;
    case N_STRING:
        emit("move    r0, __vbS%d", string_literal(e->text));
        return;
    case N_NOTHING:
        if (e->type->kind == TY_STRUCT) {
            struct_new(e->type);
            return;
        }
        emit("move    r0, 0");
        if (is_f(e->type)) emit("move    f0, r0");
        return;
    case N_NAME:
        load_var(e->sym);
        return;
    case N_ME:
        load_me();
        return;
    case N_MEMBER:
        addr_field(e);
        emit(is_f(e->type) ? "load    f0, r0[0]" : "load    r0, r0[0]");
        return;
    case N_NEW:
        cg_new(e);
        return;
    case N_DLGNEW:
        cg_dlgnew(e);
        return;
    case N_PAREN:
        cg_expr(e->a);
        return;
    case N_CONV:
        cg_expr(e->a);
        if (e->flags & F_TRYCAST) {
            emit("push    r0");
            emit("move    r0, __vbD%d+1", e->type->sym->desc);
            emit("push    r0");
            rt_call("trycast", 2);
            return;
        }
        cg_convert(e->a->type, e->type, e);
        return;
    case N_TYPEOF: {
        Type *t = e->b->type;
        cg_expr(e->a);
        emit("push    r0");
        if (ty_user(t)) emit("move    r0, __vbD%d+1", t->sym->desc);
        else if (t->kind == TY_ARRAY) cg_fail(e, "'TypeOf' with an array type is");
        else emit("move    r0, %d", t->kind);
        emit("push    r0");
        rt_call("isinst", 2);
        if (e->flags & F_ISNOT) emit("compl   r0");
        return;
    }
    case N_BINARY: cg_binary(e); return;
    case N_UNARY: cg_unary(e); return;
    case N_INVOKE: cg_call(e); return;
    case N_INDEX:
        if (e->a->type->kind == TY_STRING) {
            cg_expr(e->a);
            emit("push    r0");
            cg_expr(e->list->v[0]);
            emit("push    r0");
            rt_call("char_at", 2);
            return;
        }
        addr_index(e);
        emit(is_f(e->type) ? "load    f0, r0[0]" : "load    r0, r0[0]");
        return;
    case N_TARGET:
        load_target(e->type);
        return;
    case N_QREF:
        emit("load    r0, sf[%d]", -q_slot);
        return;
    case N_QCOND:
        cg_qcond(e);
        return;
    case N_SELREF:
        emit(is_f(e->type) ? "load    f0, sf[%d]" : "load    r0, sf[%d]", -sel_slot);
        return;
    case N_IIF: {
        int l_else = new_label(), l_end = new_label();
        if (e->c) {
            cg_expr(e->a);
            emit("test    r0");
            emit("jump    z, __vbL%d", l_else);
            cg_expr(e->b);
            emit("jump    __vbL%d", l_end);
            emit_label(l_else);
            cg_expr(e->c);
        } else {
            cg_expr(e->a);
            emit("test    r0");
            emit("jump    nz, __vbL%d", l_end);
            cg_expr(e->b);
        }
        emit_label(l_end);
        return;
    }
    case N_COLLINIT:
        cg_array(e->type, 0, e);
        return;
    case N_NEWARRAY:
        cg_array(e->type, e->list, e->b);
        return;
    case N_INTERP: cg_interp(e); return;
    }
    cg_fail(e, node_names[e->kind]);
}
