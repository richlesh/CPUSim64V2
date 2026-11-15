/*
 * exc.c - exceptions and type descriptors.
 *
 * Exceptions use setjmp/longjmp and a stack of cleanup records.  Every local
 * object (and every constructed base or member of a constructor in progress)
 * that has a destructor pushes a record (object, destructor) when it is
 * constructed and pops it when it is destroyed.  A throw pops the records
 * down to the innermost handler, calling the destructors, and longjmps to it.
 * A try statement saves the handler state in a struct, a catch clause tests
 * the type of the exception against a type descriptor.
 *
 * The runtime is part of the generated C and exists only when the program
 * has a try or throw; otherwise __CPPC_PUSH and friends expand to nothing.
 *
 * A type descriptor (struct __cppc_type) is generated for each type that is
 * thrown or caught, and for each polymorphic class (vtable slot 0, for RTTI).
 */

typedef struct TDesc TDesc;
struct TDesc {
    Type *t;
    int id;
    int tio_declared;
    TDesc *next;
};
static TDesc *tdescs = 0;
static int ntdesc = 0;
static StrBuf out_tdesc_decl, out_tdesc_def;
static int array_dtor_count = 0;
static int uses_typeid = 0;
static Ent *typeinfo_cls = 0;

/* the type that a descriptor stands for: no references, no top-level cv, arrays and functions as pointers */
static Type *desc_type(Type *t) {
    t = ty_unqual(ty_strip_ref(t));
    if (t->kind == TY_ARRAY) t = ty_ptr(t->base);
    else if (t->kind == TY_FUNC) t = ty_ptr(t);
    if (t->kind == TY_PTR) {
        Type *b = ty_unqual(t->base);
        t = b == t->base ? t : ty_ptr(b);          /* pointers to const and non-const are one type */
    }
    return t;
}

static const char *type_desc_sym(Type *t) {
    TDesc *d;
    int id;
    t = desc_type(t);
    for (d = tdescs; d; d = d->next)
        if (ty_equal(d->t, t)) return fmt_str("__cppc_ti%d", d->id);
    id = ++ntdesc;
    d = anew(sizeof(TDesc));
    d->t = t;
    d->id = id;
    d->next = tdescs;
    tdescs = d;
    sb_printf(&out_tdesc_decl, "static struct __cppc_type __cppc_ti%d;\n", id);
    {
        StrBuf *saved = out;
        const char *dtor = "0";
        const char *name = ty_str(t);
        int kind = 0;
        const char *pointee = "0";
        StrBuf bases, ups;
        int nb = 0, i;
        sb_init(&bases);
        sb_init(&ups);
        out = &out_tdesc_def;
        if (t->kind == TY_CLASS) {
            ClassInfo *ci = cinfo(t);
            kind = 1;
            if (ci->needs_dtor && ci->dtor) dtor = fmt_str("(void (*)(void *))%s", ci->dtor->cname);
            for (i = 0; i < ci->nbases; i++) {
                const char *bs = type_desc_sym(ci->bases[i].cls->type);
                sb_printf(&bases, "%s&%s", i ? ", " : "", bs);
                sb_printf(&ups, "case %d: return (void *)&((%s *)o)->%s;\n", i, class_tag(t->ent), ci->bases[i].fname);
                nb++;
            }
        } else if (t->kind == TY_PTR) {
            Type *b = ty_unqual(t->base);
            kind = 2;
            if (b->kind == TY_CLASS) pointee = fmt_str("&%s", type_desc_sym(b));
        }
        if (nb > 0) {
            emitf("static const struct __cppc_type *const __cppc_tb%d[] = { %s };\n", id, bases.s);
            emitf("static void *__cppc_up%d(void *o, int i) {\nswitch (i) {\n%sdefault: break;\n}\nreturn 0;\n}\n", id, ups.s);
        }
        emitf("static struct __cppc_type __cppc_ti%d = { \"%s\", %d, %d, %s, %s, %s, %s, 0 };\n", id, name, kind, nb,
              nb > 0 ? fmt_str("__cppc_tb%d", id) : "0", nb > 0 ? fmt_str("__cppc_up%d", id) : "0", dtor, pointee);
        out = saved;
    }
    return fmt_str("__cppc_ti%d", id);
}

/* the std::type_info class (the program has to include <typeinfo>) */
static Ent *find_typeinfo_class(Node *at) {
    Ent *std_ns, *e;
    if (typeinfo_cls) return typeinfo_cls;
    std_ns = lookup_in(scope2_global, intern_str("std"), 0);
    e = std_ns && std_ns->kind == EK_NS ? lookup_in(std_ns->members, intern_str("type_info"), 0) : 0;
    if (!e || e->kind != EK_CLASS || !e->ci || !e->ci->complete) serr(at, "you have to #include <typeinfo> before using typeid");
    typeinfo_cls = e;
    return e;
}

/* the C expression for the address of the descriptor of t */
static const char *type_desc(Type *t) {
    return fmt_str("(&%s)", type_desc_sym(t));
}

/* a cleanup record for the object (or array) at the pointer text */
static void emit_push_object(const char *ptr, Type *t) {
    Type *el = strip_arrays(t);
    const char *fn;
    if (el->kind != TY_CLASS) return;
    if (t->kind == TY_ARRAY) {
        i64 n = 1;
        Type *x = t;
        StrBuf *saved = out;
        int k = array_dtor_count++;
        while (x->kind == TY_ARRAY) {
            n *= x->len;
            x = x->base;
        }
        out = &pending_helpers;
        emitf("static void __cppc_ad%d(void *p) { long i; for (i = %lld - 1; i >= 0; i--) %s(&((%s)p)[i]); }\n", k, n, cinfo(el)->dtor->cname, c_decl(ty_ptr(el), ""));
        out = saved;
        fn = fmt_str("__cppc_ad%d", k);
    } else fn = cinfo(el)->dtor->cname;
    emitf("__CPPC_PUSH(%s, %s);\n", ptr, fn);
}

static void emit_ctor_return(void) {
    if (cur_ctor_pushed) emitf("__CPPC_POPN(%d);\n", cur_ctor_pushed);
    emit("return this;\n");
}

static Val *throw_expr(Node *n) {
    Val *v, *r;
    Type *t;
    const char *tmp, *text;
    uses_exc = 1;
    if (!n->a) return val_new(T_VOID, "__cppc_rethrow()");
    {
        int m0 = nftemps;
        unevaluated++;
        v = expr(n->a);                 /* only the type is wanted */
        unevaluated--;
        nftemps = m0;
        t = v->t;
        if (v->fn) serr(n, "cannot throw a function");
    }
    if (t->kind == TY_ARRAY) t = ty_ptr(t->base);
    t = ty_unqual(t);
    if (t->kind == TY_CLASS) {
        Val *cv;
        require_complete(t, n);
        require_concrete(t, n);
        tmp = new_temp(ty_ptr(t));
        cv = expr_hinted(n->a, tmp, t);
        if (dest_used) {
            text = fmt_str("(void)(%s)", cv->c);
            dest_used = 0;
        } else {
            const char *c = copy_into(t, tmp, cv, n);
            text = c ? c : "0";
        }
        r = val_new(T_VOID, fmt_str("(%s = (%s)malloc(sizeof(%s)), %s, __cppc_throw(%s, %s))", tmp, c_decl(ty_ptr(t), ""), c_decl(t, ""), text, tmp, type_desc(t)));
        return r;
    }
    {
        Val *sv = convert(rvalue(expr(n->a)), t->kind == TY_NULLPTR ? ty_ptr(T_VOID) : t, n);
        tmp = new_temp(ty_ptr(t->kind == TY_NULLPTR ? ty_ptr(T_VOID) : t));
        r = val_new(T_VOID, fmt_str("(%s = (%s)malloc(sizeof(%s)), *%s = %s, __cppc_throw(%s, %s))", tmp,
                                    c_decl(ty_ptr(t->kind == TY_NULLPTR ? ty_ptr(T_VOID) : t), ""), c_decl(t->kind == TY_NULLPTR ? ty_ptr(T_VOID) : t, ""), tmp, sv->c, tmp,
                                    type_desc(t->kind == TY_NULLPTR ? ty_ptr(T_VOID) : t)));
        return r;
    }
}

static void try_stmt(Node *n) {
    int k = label_count++, nc = 0;
    Node *c;
    uses_exc = 1;
    emit_line(n);
    emitf("{\nstruct __cppc_handler __h%d;\nvoid *__a%d;\n", k, k);
    emitf("__h%d.prev = __cppc_htop;\n__h%d.ctop = __cppc_ctop;\n__cppc_htop = &__h%d;\n", k, k, k);
    emitf("if (setjmp(__h%d.jb) == 0) {\n", k);
    push_scope();
    scope2_cur->try_prev = fmt_str("__h%d.prev", k);
    stmt(n->a);
    pop_scope();
    emit("} else {\n");
    for (c = n->b; c; c = c->next, nc++) {
        Type *ct = 0, *base = 0;
        Node *nid = 0, *dummy = 0;
        const char *cond;
        if (c->a) {
            int fl;
            base = decl_specs_type(c->a->a, &fl);
            ct = apply_declarator(base, c->a->b, &nid, &dummy);
            if (ct->kind == TY_ARRAY) ct = ty_ptr(ct->base);
            cond = fmt_str("__cppc_match(%s, &__a%d)", type_desc(ct), k);
        } else cond = "1";
        emitf("%sif (%s) {\n", nc ? "else " : "", cond);
        push_scope();
        scope2_cur->catch_end = 1;
        emit("__cppc_begin_catch();\n");
        if (c->a && nid && nid->a && nid->a->name) {
            Name *vn = nid->a->name;
            Ent *ve = ent_new(EK_VAR, vn, ct, scope2_cur);
            Type *vt = ty_is_ref(ct) ? ct->base : ct;
            ve->cname = vn->s;
            if (ty_is_ref(ct)) {
                emitf("%s = (%s)__a%d;\n", c_decl(ty_ptr(vt), vn->s), c_decl(ty_ptr(vt), ""), k);
                scope_add(scope2_cur, ve);
            } else if (ty_unqual(vt)->kind == TY_CLASS) {
                Val *src = val_new(vt, fmt_str("(*(%s)__a%d)", c_decl(ty_ptr(ty_unqual(vt)), ""), k));
                const char *text;
                src->lval = 1;
                emitf("%s;\n", c_decl(ty_unqual(vt), vn->s));
                scope_add(scope2_cur, ve);
                text = copy_into(ty_unqual(vt), fmt_str("(&%s)", vn->s), src, c);
                if (text) emitf("%s;\n", text);
                register_local_dtor(ve);
            } else {
                emitf("%s = *(%s)__a%d;\n", c_decl(ty_unqual(vt), vn->s), c_decl(ty_ptr(ty_unqual(vt)), ""), k);
                scope_add(scope2_cur, ve);
            }
        }
        stmt(c->b);
        pop_scope();
        emit("}\n");
    }
    emit("else __cppc_unwind();\n}\n}\n");
}

/* the runtime, written to the head of the generated C when the program uses exceptions */
static const char *exc_runtime_text =
    "#include <setjmp.h>\n#include <stdio.h>\n"
    "struct __cppc_exc { void *obj; const struct __cppc_type *type; int refs; struct __cppc_exc *next; };\n"
    "struct __cppc_handler { jmp_buf jb; long ctop; struct __cppc_handler *prev; };\n"
    "struct __cppc_rec { void *obj; void (*dtor)(void *); };\n"
    "__CPPC_SHARED struct __cppc_rec *__cppc_cs;\n__CPPC_SHARED long __cppc_ctop, __cppc_ccap;\n"
    "__CPPC_SHARED struct __cppc_handler *__cppc_htop;\n"
    "__CPPC_SHARED struct __cppc_exc *__cppc_prop, *__cppc_caught;\n"
    "static void *__cppc_ptrtmp;\n"
    "static void __cppc_grow(void) {\n"
    "    __cppc_ccap = __cppc_ccap ? __cppc_ccap * 2 : 256;\n"
    "    __cppc_cs = (struct __cppc_rec *)realloc(__cppc_cs, __cppc_ccap * sizeof(struct __cppc_rec));\n"
    "}\n"
    "#define __CPPC_PUSH(o, d) (__cppc_ctop == __cppc_ccap ? __cppc_grow() : (void)0, __cppc_cs[__cppc_ctop].obj = (void *)(o), __cppc_cs[__cppc_ctop].dtor = (void (*)(void *))(d), __cppc_ctop++)\n"
    "#define __CPPC_POP() (__cppc_ctop--)\n"
    "#define __CPPC_POPN(n) (__cppc_ctop -= (n))\n"
    "static void __cppc_terminate(void) {\n"
    "    if (__cppc_prop) fprintf(stderr, \"terminate called after throwing an instance of '%s'\\n\", __cppc_prop->type->name);\n"
    "    else fprintf(stderr, \"terminate called\\n\");\n"
    "    abort();\n"
    "}\n"
    "static void __cppc_unwind(void) {\n"
    "    struct __cppc_handler *h = __cppc_htop;\n"
    "    if (!h || !h->prev) __cppc_terminate();\n"      /* the handler of main: no handler for this exception, the stack is not unwound */
    "    __cppc_htop = h->prev;\n"
    "    while (__cppc_ctop > h->ctop) {\n"
    "        struct __cppc_rec r;\n"
    "        __cppc_ctop--;\n"
    "        r = __cppc_cs[__cppc_ctop];\n"
    "        r.dtor(r.obj);\n"
    "    }\n"
    "    longjmp(h->jb, 1);\n"
    "}\n"
    "static void __cppc_throw(void *obj, const struct __cppc_type *t) {\n"
    "    struct __cppc_exc *e = (struct __cppc_exc *)malloc(sizeof(struct __cppc_exc));\n"
    "    e->obj = obj;\n    e->type = t;\n    e->refs = 1;\n    e->next = 0;\n"
    "    __cppc_prop = e;\n"
    "    __cppc_unwind();\n"
    "}\n"
    "static void __cppc_rethrow(void) {\n"
    "    if (!__cppc_caught) __cppc_terminate();\n"
    "    __cppc_prop = __cppc_caught;\n"
    "    __cppc_caught->refs++;\n"
    "    __cppc_unwind();\n"
    "}\n"
    "static int __cppc_match(const struct __cppc_type *want, void **out) {\n"
    "    const struct __cppc_type *t = __cppc_prop->type;\n"
    "    void *obj = __cppc_prop->obj;\n"
    "    if (t == want || strcmp(t->name, want->name) == 0) { *out = obj; return 1; }\n"
    "    if (t->kind == 1) {\n"
    "        void *b = __cppc_find_base(obj, t, want);\n"
    "        if (b) { *out = b; return 1; }\n"
    "    }\n"
    "    if (t->kind == 2 && want->kind == 2 && t->pointee && want->pointee) {\n"
    "        void *b = __cppc_find_base(*(void **)obj, t->pointee, want->pointee);\n"
    "        if (b) { __cppc_ptrtmp = b; *out = &__cppc_ptrtmp; return 1; }\n"
    "    }\n"
    "    return 0;\n"
    "}\n"
    "static void __cppc_end_catch_rec(void *p) {\n"
    "    struct __cppc_exc *e = (struct __cppc_exc *)p;\n"
    "    __cppc_caught = e->next;\n"
    "    if (--e->refs == 0) {\n"
    "        if (e->type->dtor) e->type->dtor(e->obj);\n"
    "        free(e->obj);\n"
    "        free(e);\n"
    "    }\n"
    "}\n"
    "static void __cppc_begin_catch(void) {\n"
    "    struct __cppc_exc *e = __cppc_prop;\n"
    "    e->next = __cppc_caught;\n"
    "    __cppc_caught = e;\n"
    "    __CPPC_PUSH(e, __cppc_end_catch_rec);\n"
    "}\n"
    "#define __CPPC_END_CATCH() (__cppc_ctop--, __cppc_end_catch_rec(__cppc_cs[__cppc_ctop].obj))\n";

/* the type information of the program: the descriptors, the vtable slot 0 and what dynamic_cast and typeid use */
static const char *rtti_runtime_text =
    "struct __cppc_vtinfo { const struct __cppc_type *type; long off; };\n"
    "static void *__cppc_find_base(void *obj, const struct __cppc_type *t, const struct __cppc_type *want) {\n"
    "    int i;\n"
    "    if (t == want || strcmp(t->name, want->name) == 0) return obj;\n"
    "    for (i = 0; i < t->nbases; i++) {\n"
    "        void *r = __cppc_find_base(t->up(obj, i), t->bases[i], want);\n"
    "        if (r) return r;\n"
    "    }\n"
    "    return 0;\n"
    "}\n"
    "static void *__cppc_dyncast(void *p, const struct __cppc_type *want) {\n"
    "    struct __cppc_vtinfo *vi;\n"
    "    void *complete;\n"
    "    if (!p) return 0;\n"
    "    vi = (struct __cppc_vtinfo *)(*(void ***)p)[0];\n"
    "    complete = (char *)p - vi->off;\n"
    "    if (!want) return complete;\n"
    "    return __cppc_find_base(complete, vi->type, want);\n"
    "}\n"
    "static void *__cppc_dyntype(void *p) {\n"
    "    struct __cppc_vtinfo *vi = (struct __cppc_vtinfo *)(*(void ***)p)[0];\n"
    "    return vi->type->tio;\n"
    "}\n";

static const char *exc_noop_text =
    "#define __CPPC_PUSH(o, d) ((void)0)\n#define __CPPC_POP() ((void)0)\n#define __CPPC_POPN(n) ((void)0)\n";

/* typeid(type) and typeid(expression): a const std::type_info lvalue */
static Val *typeid_expr(Node *n) {
    Ent *tc = find_typeinfo_class(n);
    Type *t;
    Val *r;
    uses_typeid = 1;
    if (n->b) t = typeid_type(n->b);
    else {
        Val *v;
        int m0 = nftemps;
        v = expr(n->a);
        t = v->t;
        if (t->kind == TY_CLASS && cinfo(t)->poly && cinfo(t)->complete && (v->lval || v->xvalue)) {
            /* the dynamic type of a polymorphic object */
            nftemps = m0;
            r = val_new(ty_cv(tc->type, CV_CONST), fmt_str("(*(%s)__cppc_dyntype((void *)&%s))", c_decl(ty_ptr(tc->type), ""), v->c));
            r->lval = 1;
            return r;
        }
    }
    t = desc_type(t);
    if (t->kind == TY_CLASS) require_complete(t, n);
    {
        const char *sym = type_desc_sym(t);
        TDesc *d;
        for (d = tdescs; d; d = d->next)
            if (ty_equal(d->t, t)) break;
        if (d && !d->tio_declared) {
            StrBuf *saved = out;
            d->tio_declared = 1;
            out = &pending_helpers;
            emitf("static %s __cppc_tio%d;\n", class_tag(tc), d->id);
            out = saved;
        }
        r = val_new(ty_cv(tc->type, CV_CONST), fmt_str("__cppc_tio%s", sym + 9));
    }
    r->lval = 1;
    return r;
}

static Val *dynamic_cast_expr(Node *n) {
    Type *to = typeid_type(n->b), *tb, *src;
    Val *v = expr(n->a), *r;
    int is_ref = ty_is_ref(to);
    const char *obj, *res, *tmp;
    if (is_ref) tb = ty_unqual(to->base);
    else if (to->kind == TY_PTR) tb = ty_unqual(to->base);
    else {
        serr(n, "dynamic_cast needs a pointer or a reference type");
        return 0;
    }
    if (tb->kind != TY_CLASS && !(tb->kind == TY_VOID && !is_ref)) serr(n, "the target of the dynamic_cast is not a class");
    if (is_ref) {
        if (v->t->kind != TY_CLASS || !v->lval) serr(n, "dynamic_cast to a reference needs an object of a class");
        src = ty_unqual(v->t);
        obj = fmt_str("(&%s)", v->c);
    } else {
        Val *pv = rvalue(v);
        if (is_null_ptr_const(pv)) return val_new(to, "((void *)0)");
        if (pv->t->kind != TY_PTR || pv->t->base->kind != TY_CLASS) serr(n, "dynamic_cast needs a pointer to a class");
        src = ty_unqual(pv->t->base);
        obj = pv->c;
    }
    require_complete(src, n);
    if (tb->kind == TY_CLASS) require_complete(tb, n);
    if (tb->kind == TY_CLASS && (ty_same(src, tb) || derived_to_base(src, tb))) {
        /* a conversion that needs no run-time test */
        if (is_ref) return cast_value(v, to, KW_static_cast, n);
        return cast_value(v, to, KW_static_cast, n);
    }
    if (!cinfo(src)->poly) serr(n, "'%s' is not polymorphic: dynamic_cast needs a class with virtual functions", ty_str(src));
    if (tb->kind == TY_VOID) res = fmt_str("__cppc_dyncast((void *)%s, 0)", obj);
    else res = fmt_str("__cppc_dyncast((void *)%s, %s)", obj, type_desc(tb));
    if (!is_ref) return val_new(ty_cv(to, 0), fmt_str("((%s)%s)", c_decl(to, ""), res));
    {
        /* a reference: no match throws std::bad_cast */
        Ent *std_ns = lookup_in(scope2_global, intern_str("std"), 0), *thr = 0;
        if (std_ns && std_ns->kind == EK_NS) thr = lookup_in(std_ns->members, intern_str("__throw_bad_cast"), 0);
        if (!thr || thr->kind != EK_FUNC) serr(n, "you have to #include <typeinfo> before a dynamic_cast to a reference");
        use_function(thr);
        tmp = new_temp(ty_ptr(tb));
        r = val_new(tb, fmt_str("(*(%s = (%s)%s, %s ? %s : (%s(), %s)))", tmp, c_decl(ty_ptr(tb), ""), res, tmp, tmp, thr->cname, tmp));
        r->lval = to->kind == TY_REF;
        r->xvalue = to->kind == TY_RREF;
        return r;
    }
}

/* the type traits that the compiler provides to <type_traits> */
static int type_is_empty_class(Type *t) {
    ClassInfo *ci;
    int i;
    if (t->kind != TY_CLASS) return 0;
    ci = cinfo(t);
    if (ci->fields || ci->poly) return 0;
    for (i = 0; i < ci->nbases; i++)
        if (!type_is_empty_class(ci->bases[i].cls->type)) return 0;
    return 1;
}

static Val *typetrait_expr(Node *n) {
    const char *nm = n->name->s;
    Type *a = typeid_type(n->a), *b = n->b ? typeid_type(n->b) : 0;
    Type *ua = ty_unqual(ty_strip_ref(a));
    int r = 0;
    (void)ua;
    if (strcmp(nm, "__is_enum") == 0) r = ty_unqual(a)->kind == TY_ENUM;
    else if (strcmp(nm, "__is_class") == 0) r = ty_unqual(a)->kind == TY_CLASS && !(ty_unqual(a)->ent->flags & EF_UNION);
    else if (strcmp(nm, "__is_union") == 0) r = ty_unqual(a)->kind == TY_CLASS && (ty_unqual(a)->ent->flags & EF_UNION) != 0;
    else if (strcmp(nm, "__is_signed") == 0) r = ty_is_arith(ty_unqual(a)) && (ty_is_float(ty_unqual(a)) || !ty_is_unsigned(ty_unqual(a)));
    else if (strcmp(nm, "__is_unsigned") == 0) r = ty_is_integral(ty_unqual(a)) && ty_is_unsigned(ty_unqual(a));
    else if (strcmp(nm, "__is_polymorphic") == 0) r = ty_unqual(a)->kind == TY_CLASS && cinfo(ty_unqual(a))->poly;
    else if (strcmp(nm, "__is_abstract") == 0) r = ty_unqual(a)->kind == TY_CLASS && cinfo(ty_unqual(a))->abstract;
    else if (strcmp(nm, "__is_final") == 0) r = ty_unqual(a)->kind == TY_CLASS && cinfo(ty_unqual(a))->is_final;
    else if (strcmp(nm, "__is_empty") == 0) r = type_is_empty_class(ty_unqual(a));
    else if (strcmp(nm, "__is_trivially_copyable") == 0 || strcmp(nm, "__is_trivial") == 0 || strcmp(nm, "__is_pod") == 0) {
        Type *e = strip_arrays(ty_unqual(a));
        r = e->kind != TY_CLASS || (!type_nontrivial_copy(e) && !type_needs_dtor(e) && (strcmp(nm, "__is_trivially_copyable") == 0 || !type_needs_ctor(e)));
    } else if (strcmp(nm, "__is_trivially_destructible") == 0 || strcmp(nm, "__has_trivial_destructor") == 0) {
        Type *e = strip_arrays(ty_unqual(a));
        r = e->kind != TY_CLASS || !type_needs_dtor(e);
    } else if (strcmp(nm, "__has_virtual_destructor") == 0) r = ty_unqual(a)->kind == TY_CLASS && cinfo(ty_unqual(a))->has_virtual_dtor;
    else if (strcmp(nm, "__is_base_of") == 0 && b) {
        Type *ub = ty_unqual(b);
        Type *ba = ty_unqual(a);
        r = ba->kind == TY_CLASS && ub->kind == TY_CLASS && (ba->ent == ub->ent || derived_to_base(ub, ba));
    } else if (strcmp(nm, "__is_convertible_to") == 0 && b) {
        Val *dv = val_new(ty_unqual(a), "0");
        dv->lval = ty_is_ref(a) || 0;
        r = arg_conversion(dv, b) != CONV_NONE;
    } else serr(n, "unknown type trait '%s'", nm);
    return const_val(T_BOOL, r);
}
