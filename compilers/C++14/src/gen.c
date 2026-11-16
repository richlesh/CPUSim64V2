/*
 * gen.c - semantic analysis and C code generation.
 *
 * translate() walks the AST of the translation unit in order and writes the C
 * program.  Expressions are analysed and written in one step: expr() returns a
 * Val with the C++ type, the value category and the C text of the expression.
 * References are pointers in C: a reference variable's C text already has the
 * dereference, and an argument for a reference parameter is passed as its
 * address.
 */

#define MAXARGS 64

static StrBuf out_head, out_body, out_init;
static StrBuf *out;                 /* where the declarations and definitions are written */
static int cur_line_file = -1, cur_line_no = -1;
static Ent *cur_fn = 0;             /* the function being defined */
static Type *cur_ret = 0;
static StrBuf *temps;               /* where new_temp() declares temporaries (a function's or the initialization function's) */
static int temp_count = 0;
static int label_count = 0;
static int dyn_init_count = 0;
static const char *lib_dir_path = 0;
static int loop_depth = 0;

static void serr(Node *n, const char *fmt, ...) {
    va_list ap;
    char buf[500];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    error_at(n ? n->file : -1, n ? n->line : 0, "%s", buf);
}

static void emit(const char *s) {
    sb_puts(out, s);
}

static void emitf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    sb_vprintf(out, fmt, ap);
    va_end(ap);
}

static const char *fmt_str(const char *fmt, ...);
static const char *fmt_str_qn(const char *a, const char *b) {
    return fmt_str("%s%s", a, b);
}

/* ---- classes (the code is further down) */
static Name *nm_ctor, *nm_dtor;
static Ent *cur_class = 0;          /* the class of the member function being defined or the class being defined */
static int cur_is_ctor = 0, cur_is_dtor = 0, cur_method_const = 0;
static const char *dest_hint = 0;   /* the C pointer text of the object that the next class prvalue of node dest_node constructs */
static Node *dest_node = 0;
static Type *dest_type = 0;
static int dest_used = 0;
static int nftemps = 0;
static int cond_depth = 0;
static int in_cond_decl = 0;
static Name *nrvo_name = 0;         /* the local variable that every return statement returns (named return value optimization) */
static StrBuf pending_helpers;      /* helper functions written before the next function */

/* templates (tmpl.c) */
static StrBuf late_defs;            /* the function definitions of template instances, written at the end */
static int inst_active_count(void);
static int late_len(void) { return late_defs.len; }
static void late_truncate(int n) {
    if (late_defs.len > n) {
        late_defs.len = n;
        late_defs.s[n] = 0;
    }
}
/* separate translation units (translate.c) */
static int multi_unit = 0;           /* the program has several translation units */
static int unit_secondary = 0;       /* this is not the unit with main() */
static int in_class_def = 0;          /* the function being declared is defined in its class */
static Scope2 *body_parent = 0;     /* the parent scope of the next function body (the bindings of a template instance) */
static int tmpl_access = 0;
static Ent *apply_part_args(Ent *e, Node *p);
static void use_function(Ent *f);
static Ent *inst_template(Ent *e);
static Node *use_at = 0;                  /* where the function that use_function is asked for is used */
static void process_template(Node *t, Ent *in_cls);
static void process_alias_template(Node *t, Node *d);
static int expr_args(Node *list, Val **args, int nargs, int max);
static int pack_count(Node *pat);
static Scope2 *pack_enter(Node *pat, int i);
static Node *expand_func_params(Node *d, Type ***ptypes, int *pn);
static int declarator_has_pack(Node *d);
static void pack_append(Ent *pack, Ent *elem);
static void lazy_set(Ent *e, Node *fd, Node *fnode, Type *ft, Scope2 *parent);
static void run_eager_list(void);
static Ent *deduce_function(Ent *tf, Val **args, int nargs, Type *objt);
static Ent *deduce_function_for_type(Ent *set, Type *ft);

/* braced lists (braced.c) */
static int brace_init = 0;          /* the next ctor_call is a list-initialization: prefer a std::initializer_list constructor */
static Val *braced_val(Node *n);
static Type *braced_il_type(Val *bv, Node *at);
static int is_init_list_type(Type *t);
static Val *lambda_expr(Node *n);
/* exceptions (exc.c) */
static int uses_exc = 0;
static int cur_ctor_pushed = 0;     /* the cleanup records that the constructor being written has pushed */
static Val *throw_expr(Node *n);
static const char *type_desc(Type *t);
static Val *typeid_expr(Node *n);
static Val *typetrait_expr(Node *n);
static Val *placement_new(Node *n, Type *t);
static Val *dynamic_cast_expr(Node *n);
static void try_stmt(Node *n);
static void emit_push_object(const char *ptr_text, Type *cls);
static void emit_ctor_return(void);
static int unevaluated = 0;         /* inside sizeof, decltype or noexcept: no object is needed for a member name */
static int const_call(Ent *f, Val **args, int nargs, i64 *result);
static Ent *lambda_invoker(Type *from, Type *to);
static Ent *lambda_outer_next = 0;      /* the class around the lambda that define_class is about to define */
static int lambda_depth = 0;            /* lambda bodies being analysed */
static Val *braced_materialize(Val *bv, Type *to, Node *at);
static const char *list_init_text(Type *cls, const char *dest, Val *bv, Node *at);
static Ent *find_il_ctor(Type *cls, Type **ilt);
static int il_viable(Type *ilt, Val **vals, int n);
static const char *il_dest_text(Type *ilt, Val **vals, int n, const char *dest, Node *at);

static ClassInfo *cinfo(Type *t);
static void emit_set_vptrs(Ent *cls);
static Type *vtable_fn_type(Ent *f, Type *this_class);
static int force_nonvirtual = 0;
static void require_concrete(Type *t, Node *at);
static const char *upcast_pointer(const char *p, Type *from, Type *to, Node *at);
static Val *upcast_value(Val *v, Type *to, Node *at);
static const char *base_path(Type *from, Ent *to, int *count);
static const char *base_subobject(const char *obj, Type *from, Type *to, Node *at);
static int specs_empty(Node *ts);
static const char *dtor_expr_from(int mark);
static const char *op_word(int op);
static int type_needs_ctor(Type *t);
static void require_complete(Type *t, Node *at);
static Ent *class_ctors(Type *cls);
static const char *prvalue_dest(Type *cls, Node *n);
static void aggregate_init_stmts(Type *cls, const char *dest, Node *list, Node *at);
static Val *string_literal(Node *n);
static int is_char_kind(Type *t);
static void flush_temps_stmt(int mark);
static const char *new_temp(Type *t);
static Val *call_result(Type *ret, const char *call);
static int is_nonstatic_member(Ent *f);
static Type *c_return_type(Ent *f, Type *ft);
static const char *c_param_type_text(Type *pt, const char *name);
static Type *strip_arrays(Type *t);
static int is_class_type(Type *t);
static void check_access(Ent *member, int access, Ent *owner, Node *at);
static int access_allowed(Ent *owner, int access);
static Val *member_expr(Node *n);
static const char *class_assign_text_fwd(Type *t, const char *dst, Val *src, Node *at);
static Ent *class_member(Type *cls, Name *name);
static Val *class_to_builtin(Val *v, Node *at, int op);
static int is_class_operand(Val *v);
static int type_needs_dtor(Type *t);
static int type_nontrivial_copy(Type *t);
static Val *class_prvalue(Val *a, Type *cls, Node *at);
static Val *convert_class_any(Val *v, Type *to, Node *at);
static int find_conversion(Type *from, Type *to, Ent **best);
static Val *member_value(Ent *e, Node *at);
static Val *call_function(Ent *f, const char *this_c, Val **args, int nargs, Node *at, Node *hint);
static Val *try_operator(Node *at, int op, int postfix, Val **ops, int nops);
static const char *dtor_text_from(int mark);
static const char *new_class_temp(Type *cls);
static const char *ctor_call(Type *cls, const char *dest, Val **args, int nargs, Node *at);
static const char *copy_into(Type *cls, const char *dest, Val *src, Node *at);
static const char *default_construct_text(Type *cls, const char *dest, Node *at);

/* the name of a source file as the generated C names it in #line: relative to the output directory */
static const char *line_file_name(int file);

static const char *class_tag(Ent *e) {
    return fmt_str("%s %s", (e->flags & EF_UNION) ? "union" : "struct", e->cname ? e->cname : "anon");
}

/* #line before a statement or declaration of the C++ source */
static void emit_line(Node *n) {
    if (!n || n->file < 0 || n->line <= 0) return;
    if (n->file == cur_line_file && n->line == cur_line_no) return;
    cur_line_file = n->file;
    cur_line_no = n->line;
    emitf("\n#line %d \"%s\"\n", n->line, line_file_name(n->file));
}


static const char *fmt_str(const char *fmt, ...) {
    va_list ap;
    StrBuf sb;
    sb_init(&sb);
    va_start(ap, fmt);
    sb_vprintf(&sb, fmt, ap);
    va_end(ap);
    return sb.s;
}

/* ------------------------------------------------------------ literals */

/* the codepoints of a string or character literal's body (between the quotes) with the escapes decoded;
   returns the count; *prefix = the encoding prefix characters (L, u, U, u8) */
static int decode_literal(const char *text, int **cps, int *raw_prefix, char *prefix) {
    int *r;
    int n = 0, i = 0, len, np = 0, k;
    int cn;
    int *src = native_to_cps(text, &cn);
    int raw = 0;
    len = cn;
    while (i < len && src[i] != '"' && src[i] != '\'') {
        if (src[i] == 'R') raw = 1;
        else if (np < 3) prefix[np++] = (char)src[i];
        i++;
    }
    prefix[np] = 0;
    *raw_prefix = raw;
    r = xalloc(sizeof(int) * (len + 2));
    if (raw) {
        /* R"delim( ... )delim" */
        int ds = i + 1, de = ds;
        while (src[de] != '(') de++;
        k = de + 1;
        while (k < len - 1 - (de - ds) - 1) r[n++] = src[k++];
        *cps = r;
        return n;
    }
    i++;                                    /* the opening quote */
    while (i < len - 1) {
        int c = src[i++];
        if (c != '\\') {
            r[n++] = c;
            continue;
        }
        c = src[i++];
        switch (c) {
        case 'n': r[n++] = 10; break;
        case 't': r[n++] = 9; break;
        case 'r': r[n++] = 13; break;
        case 'a': r[n++] = 7; break;
        case 'b': r[n++] = 8; break;
        case 'f': r[n++] = 12; break;
        case 'v': r[n++] = 11; break;
        case 'x':
            {
                int v = 0;
                while ((src[i] >= '0' && src[i] <= '9') || (src[i] >= 'a' && src[i] <= 'f') || (src[i] >= 'A' && src[i] <= 'F')) {
                    int d = src[i] <= '9' ? src[i] - '0' : (src[i] | 0x20) - 'a' + 10;
                    v = v * 16 + d;
                    i++;
                }
                r[n++] = v;
            }
            break;
        case 'u': case 'U':
            {
                int v = 0, cnt = c == 'u' ? 4 : 8, j;
                for (j = 0; j < cnt && i < len; j++, i++) {
                    int d = src[i] <= '9' ? src[i] - '0' : (src[i] | 0x20) - 'a' + 10;
                    v = v * 16 + d;
                }
                r[n++] = v;
            }
            break;
        default:
            if (c >= '0' && c <= '7') {
                int v = c - '0', cnt = 1;
                while (cnt < 3 && src[i] >= '0' && src[i] <= '7') {
                    v = v * 8 + (src[i++] - '0');
                    cnt++;
                }
                r[n++] = v;
            } else r[n++] = c;      /* \\ \' \" \? */
        }
    }
    *cps = r;
    return n;
}

/* a C string literal for the decoded codepoints */
static const char *c_string_literal(int *cps, int n, const char *prefix) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    if (prefix[0] == 'L') sb_putc(&sb, 'L');
    sb_putc(&sb, '"');
    for (i = 0; i < n; i++) {
        int c = cps[i];
        if (c == '"' || c == '\\') {
            sb_putc(&sb, '\\');
            sb_putc(&sb, c);
        } else if (c >= 32 && c < 127) sb_putc(&sb, c);
        else if (c >= 127) sb_put_cp(&sb, c);
        else sb_printf(&sb, "\\%03o", c);
    }
    sb_putc(&sb, '"');
    return sb.s;
}

/* the decimal C text of an integral constant of type t */
static const char *c_int_text(i64 v, Type *t) {
    StrBuf sb;
    sb_init(&sb);
    if (ty_is_unsigned(t) && t->kind != TY_BOOL) sb_printf(&sb, "%llu", (u64)v);
    else if (v < 0) sb_printf(&sb, "(%lld)", v);
    else sb_printf(&sb, "%lld", v);
    if (ty_is_unsigned(t) && t->kind != TY_BOOL) sb_puts(&sb, "u");
    return sb.s;
}

static Val *const_val(Type *t, i64 v) {
    Val *r = val_new(t, c_int_text(v, t));
    r->is_const = 1;
    r->cv = v;
    r->static_init = 1;
    return r;
}

/* ------------------------------------------------------------ errors about types */

static Val *expr(Node *n);
static Val *rvalue(Val *v);
static Val *convert(Val *v, Type *to, Node *at);
static Type *decl_specs_type(Node *ts, int *flags);
static void declare_params(Node *fn, Type *ft);

/* ------------------------------------------------------------ name resolution */

/* the entity that an id-expression or type name refers to (the head of an overload set), or 0 */
static Ent *resolve_id(Node *id, int report) {
    Node *p = id->a;
    Scope2 *sc = 0;
    Ent *e = 0;
    int first = 1;
    if (id->flags & ID_GLOBAL) {
        sc = scope2_global;
        first = 0;
    }
    for (; p; p = p->next) {
        if ((p->flags & 7) != PART_NAME)
            serr(p, "operator and destructor names are not supported yet");
        if (first) e = lookup_unqual(scope2_cur, p->name);
        else e = lookup_in(sc, p->name, 0);
        first = 0;
        if (!e) {
            if (report) {
                if (p == id->a && !(id->flags & ID_GLOBAL)) serr(p, "'%s' was not declared in this scope", p->name->s);
                serr(p, "'%s' is not a member of the namespace", p->name->s);
            }
            return 0;
        }
        if (p->flags & PART_ARGS) e = apply_part_args(e, p);
        if (p->next) {
            if (e->kind == EK_NS) sc = e->members;
            else if (e->kind == EK_CLASS && e->ci) sc = e->ci->scope;
            else if (e->kind == EK_TYPEDEF && e->type->kind == TY_CLASS && e->type->ent->ci) sc = e->type->ent->ci->scope;
            else if (e->kind == EK_ENUM && (e->flags & EF_SCOPED)) sc = e->members;
            else if (e->kind == EK_TYPEDEF && e->type->kind == TY_ENUM && e->type->ent->members) sc = e->type->ent->members;
            else if (e->kind == EK_ENUM && e->members) sc = e->members;
            else {
                if (report) serr(p, "'%s' is not a namespace or a scoped enumeration", p->name->s);
                return 0;
            }
        }
    }
    return e;
}

static Ent *declare_enum_node(Node *en);
static Ent *declare_class_node(Node *cn);

/* ------------------------------------------------------------ types from declarations */

static Type *auto_type(void) {
    return ty_builtin(TY_AUTO);
}

/* the type that the decl-specifier-seq names; *flags receives DS_ flags */
static Type *decl_specs_type(Node *ts, int *flags) {
    i64 v = ts->val;
    Type *t = 0;
    if (flags) *flags = ts->flags;
    if (ts->a) {
        Node *a = ts->a;
        if (a->kind == N_ID) {
            Ent *e = resolve_id(a, 1);
            if (e->kind == EK_TYPEDEF) t = e->type;
            else if (e->kind == EK_ENUM || e->kind == EK_CLASS) t = e->type;
            else if (e->kind == EK_TEMPLATE) serr(a, "'%s' is a template and needs template arguments", e->name->s);
            else if (e->kind == EK_PACK) serr(a, "the parameter pack '%s' must be expanded", e->name->s);
            else serr(a, "'%s' is not a type", e->name->s);
        } else if (a->kind == N_ENUM) {
            Ent *e;
            if (a->c) {
                /* handled by declare_enum, which returns the entity */
                e = declare_enum_node(a);
            } else {
                e = a->a ? resolve_id(a->a, 0) : 0;
                if (!e) serr(a, "unknown enumeration");
            }
            t = e->type;
        } else if (a->kind == N_CLASS) {
            Ent *e = declare_class_node(a);
            t = e->type;
        } else if (a->kind == N_TYPE_EXPR) {
            Val *dv;
            int m0 = nftemps;
            unevaluated++;
            dv = expr(a->a);
            unevaluated--;
            nftemps = m0;                  /* nothing is constructed in decltype */
            t = dv->t;
            if (dv->lval && !(a->a->kind == N_ID)) t = ty_ref(t);
        } else serr(a, "unsupported type specifier");
    } else if (v & BT_AUTO) {
        t = auto_type();
    } else if (v & BT_VOID) t = T_VOID;
    else if (v & BT_BOOL) t = T_BOOL;
    else if (v & BT_CHAR) t = (v & BT_UNSIGNED) ? ty_builtin(TY_UCHAR) : (v & BT_SIGNED) ? ty_builtin(TY_SCHAR) : T_CHAR;
    else if (v & BT_WCHAR) t = ty_builtin(TY_WCHAR);
    else if (v & BT_CHAR16) t = ty_builtin(TY_CHAR16);
    else if (v & BT_CHAR32) t = ty_builtin(TY_CHAR32);
    else if (v & BT_FLOAT) t = ty_builtin(TY_FLOAT);
    else if (v & BT_DOUBLE) t = (v & (BT_LONG | BT_LONG2)) ? ty_builtin(TY_LDOUBLE) : T_DOUBLE;
    else if (v & BT_SHORT) t = (v & BT_UNSIGNED) ? ty_builtin(TY_USHORT) : ty_builtin(TY_SHORT);
    else if (v & BT_LONG2) t = (v & BT_UNSIGNED) ? ty_builtin(TY_ULLONG) : T_LLONG;
    else if (v & BT_LONG) t = (v & BT_UNSIGNED) ? T_ULONG : T_LONG;
    else if (v & (BT_INT | BT_SIGNED | BT_UNSIGNED)) t = (v & BT_UNSIGNED) ? T_UINT : T_INT;
    else {
        if (ts->flags & (DS_CONST | DS_VOLATILE | DS_STATIC | DS_EXTERN | DS_INLINE | DS_CONSTEXPR | DS_VIRTUAL | DS_EXPLICIT)) {
            /* "const x = 1;" has no implicit int in C++ */
            serr(ts, "ISO C++ forbids declaration with no type");
        }
        serr(ts, "expected a type");
    }
    if (ty_is_ref(t)) return t;            /* const T with a T that is a reference: the qualifier is ignored */
    if (ts->flags & DS_CONST) t = ty_cv(t, CV_CONST);
    if (ts->flags & DS_VOLATILE) t = ty_cv(t, CV_VOLATILE);
    return t;
}

/* an integral constant expression (array bounds, enumerators, case labels, static_assert) */
static i64 const_expr_value(Node *n) {
    Val *v = expr(n);
    if (!v->is_const) serr(n, "the expression is not an integral constant expression");
    return v->cv;
}

/* applies a declarator to the base type; returns the declared type.  *name = the declared name's id node;
   *fn = the innermost function declarator whose parameters belong to the name */
static Type *apply_declarator(Type *base, Node *d, Node **name_id, Node **fn) {
    for (;;) {
        if (!d) return base;
        switch (d->kind) {
        case N_D_NAME:
            if (name_id) *name_id = d->a;
            return base;
        case N_D_PTR:
            base = ty_ptr(base);
            if (d->flags & DS_CONST) base = ty_cv(base, CV_CONST);
            if (d->flags & DS_VOLATILE) base = ty_cv(base, CV_VOLATILE);
            break;
        case N_D_REF:
            base = ty_ref(base);
            break;
        case N_D_RREF:
            base = ty_rref(base);
            break;
        case N_D_PACK:
            break;
        case N_D_MEMPTR:
            serr(d, "pointers to members are not supported yet");
            break;
        case N_D_ARRAY:
            {
                i64 len = -1;
                if (d->b) {
                    len = const_expr_value(d->b);
                    if (len < 0) serr(d, "the size of the array is negative");
                }
                if (base->kind == TY_FUNC) serr(d, "an array of functions is not allowed");
                if (ty_is_ref(base)) serr(d, "an array of references is not allowed");
                base = ty_array(base, len);
            }
            break;
        case N_D_FUNC:
            {
                Type **params = 0;
                int n = 0, k = 0, has_pack = 0;
                Node *p, *dd = d;
                for (p = d->b; p; p = p->next)
                    if (declarator_has_pack(p->b)) has_pack = 1;
                if (has_pack) {
                    dd = expand_func_params(d, &params, &n);
                    if (!params) params = anew(sizeof(Type *) * 2);
                } else {
                    for (p = d->b; p; p = p->next) n++;
                    params = anew(sizeof(Type *) * (n + 1));
                    for (p = d->b; p; p = p->next) {
                        int fl;
                        Type *pt = decl_specs_type(p->a, &fl);
                        pt = apply_declarator(pt, p->b, 0, 0);
                        params[k++] = ty_adjust_param(pt);
                    }
                }
                /* f(void) has no parameters */
                if (n == 1 && params[0]->kind == TY_VOID && !dd->b->b) n = 0;
                if (ty_is_ref(base) && 0) serr(d, "a function cannot return a reference here");
                if (base->kind == TY_ARRAY) serr(d, "a function cannot return an array");
                if (base->kind == TY_FUNC) serr(d, "a function cannot return a function");
                if (fn && !*fn) *fn = dd;
                base = ty_func(base, params, n, (d->flags & FQ_VARIADIC) != 0);
                if (d->c) {
                    /* a trailing return type: auto f() -> T; the parameters are visible in it (decltype(a + b)) */
                    Scope2 *saved = scope2_cur;
                    Type *rt;
                    scope2_cur = scope2_new(SS_BLOCK, saved);
                    declare_params(dd, base);
                    rt = apply_declarator(decl_specs_type(d->c->a, 0), d->c->b, 0, 0);
                    scope2_cur = saved;
                    base = ty_func(rt, base->params, base->nparams, base->variadic);
                }
            }
            break;
        default:
            serr(d, "unsupported declarator");
        }
        d = d->a;
    }
}

/* ------------------------------------------------------------ expressions */

/* does the C++ type have a size that is known here (every scalar is one word in C11)? */
static int type_size_words(Type *t, i64 *out) {
    i64 n;
    switch (t->kind) {
    case TY_VOID: case TY_FUNC: case TY_AUTO: case TY_CLASS:
        return 0;
    case TY_ARRAY:
        if (t->len < 0 || !type_size_words(t->base, &n)) return 0;
        *out = n * t->len;
        return 1;
    case TY_REF: case TY_RREF:
        return type_size_words(t->base, out);
    }
    *out = 1;
    return 1;
}

/* the C text with the type's cast */
static const char *cast_text(Type *to, const char *c) {
    return fmt_str("((%s)(%s))", c_decl(to, ""), c);
}

/* lvalue-to-rvalue, array-to-pointer and function-to-pointer conversions */
static Val *rvalue(Val *v) {
    Val *r;
    if (v->t->kind == TY_ARRAY) {
        r = val_new(ty_ptr(v->t->base), v->c);
        /* the address of a string literal or of a static array is an address constant */
        r->static_init = v->static_init;
        return r;
    }
    if (v->fn) {
        Ent *f = v->fn;
        if (f->overload || f->kind == EK_FTEMPLATE) serr(0, "the address of an overloaded function needs a target type");
        use_function(f);
        r = val_new(ty_ptr(f->type), f->cname);
        r->static_init = 1;
        return r;
    }
    if (v->t->kind == TY_FUNC) {
        r = val_new(ty_ptr(v->t), v->c);
        return r;
    }
    if (!v->lval && !v->t->cv) return v;
    r = anew(sizeof(Val));
    *r = *v;
    r->lval = 0;
    r->t = ty_unqual(v->t);
    r->var = 0;
    if (r->is_const) {
        r->static_init = 1;
        r->c = c_int_text(v->cv, r->t);
    }
    return r;
}

static const char *type_error_text(Val *v, Type *to) {
    return fmt_str("cannot convert '%s' to '%s'", ty_str(v->t), ty_str(to));
}

/* the implicit conversion of v to the (non-reference) type 'to' */
static Val *convert(Val *v, Type *to, Node *at) {
    Val *r;
    if (v->braced) v = braced_materialize(v, to, at);
    int rank;
    Type *tu = ty_unqual(to);
    if (v->fn) {
        /* a function name converted to a pointer to function: pick the overload with that type */
        Ent *f;
        if (tu->kind != TY_PTR || tu->base->kind != TY_FUNC) serr(at, "%s", type_error_text(v, to));
        f = find_function(v->fn, tu->base);
        if (!f) f = deduce_function_for_type(v->fn, tu->base);
        if (!f) serr(at, "no overload of '%s' matches '%s'", v->fn->name->s, ty_str(to));
        use_function(f);
        r = val_new(tu, f->cname);
        r->static_init = 1;
        return r;
    }
    if (tu->kind == TY_CLASS || v->t->kind == TY_CLASS) {
        if (tu->kind == TY_CLASS && v->t->kind == TY_CLASS && ty_same(v->t, tu) && v->is_temp && !v->lval) return v;
        if (value_conversion(v, tu) == CONV_NONE) serr(at, "%s", type_error_text(v, to));
        return convert_class_any(v, tu, at);
    }
    rank = value_conversion(v, tu);
    if (rank == CONV_NONE) serr(at, "%s", type_error_text(v, to));
    v = rvalue(v);
    if (ty_same(v->t, tu)) {
        if (v->t == tu) return v;
        r = anew(sizeof(Val));
        *r = *v;
        r->t = tu;
        return r;
    }
    if (v->is_const && ty_is_integral(tu) ) {
        i64 c = v->cv;
        if (tu->kind == TY_BOOL) c = c != 0;
        r = const_val(tu, c);
        return r;
    }
    if (tu->kind == TY_BOOL) {
        r = val_new(tu, fmt_str("((%s) != 0)", v->c));
        return r;
    }
    if (tu->kind == TY_PTR && is_null_ptr_const(v)) {
        r = val_new(tu, "((void *)0)");
        r->static_init = 1;
        return r;
    }
    if (tu->kind == TY_PTR && v->t->kind == TY_PTR && v->t->base->kind == TY_CLASS && tu->base->kind == TY_CLASS
        && !ty_same(v->t->base, tu->base) && derived_to_base(v->t->base, tu->base)) {
        return val_new(tu, upcast_pointer(v->c, ty_unqual(v->t->base), ty_unqual(tu->base), at));
    }
    r = val_new(tu, cast_text(tu, v->c));
    r->static_init = v->static_init;
    if (v->is_const && ty_is_float(tu)) r->static_init = 1;
    return r;
}

/* is v a modifiable lvalue? */
static void require_lvalue(Val *v, Node *at, const char *what) {
    if (!v->lval || v->fn) serr(at, "%s needs an lvalue", what);
}

static void require_modifiable(Val *v, Node *at) {
    require_lvalue(v, at, "the operation");
    if (v->t->cv & CV_CONST) serr(at, "the object has type '%s' and cannot be modified", ty_str(v->t));
    if (v->t->kind == TY_ARRAY) serr(at, "an array cannot be assigned to");
}

/* ------------------------------------------------------------ literals */

static Val *number_literal(Node *n) {
    const char *p = n->text;
    int base = 10, any = 0, is_float = 0, k;
    u64 v = 0;
    char buf[200];
    int bl = 0, usfx = 0, lsfx = 0;
    Type *t;
    for (k = 0; p[k]; k++) {
        if (p[k] == '.') is_float = 1;
        if (base == 10 && (p[k] == 'e' || p[k] == 'E') && !(p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))) is_float = 1;
        if ((p[k] == 'p' || p[k] == 'P') && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) is_float = 1;
    }
    if (is_float) {
        int fsfx = 0;
        for (k = 0; p[k] && bl < 190; k++) {
            if (p[k] == '\'') continue;
            buf[bl++] = p[k];
        }
        buf[bl] = 0;
        if (bl > 0 && (buf[bl - 1] == 'f' || buf[bl - 1] == 'F') && !(buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X'))) {
            fsfx = 1;
            buf[--bl] = 0;
        } else if (bl > 0 && (buf[bl - 1] == 'l' || buf[bl - 1] == 'L')) buf[--bl] = 0;
        t = fsfx ? ty_builtin(TY_FLOAT) : T_DOUBLE;
        {
            Val *r = val_new(t, fmt_str("%s", buf));
            if (strchr(buf, '.') == 0 && strchr(buf, 'e') == 0 && strchr(buf, 'E') == 0) r->c = fmt_str("%s.0", buf);
            r->static_init = 1;
            return r;
        }
    }
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { base = 16; p += 2; }
    else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) { base = 2; p += 2; }
    else if (p[0] == '0' && p[1]) base = 8;
    for (; *p; p++) {
        int d;
        if (*p == '\'') continue;
        if (*p >= '0' && *p <= '9') d = *p - '0';
        else if (*p >= 'a' && *p <= 'f') d = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F') d = *p - 'A' + 10;
        else break;
        if (d >= base) break;
        v = v * base + d;
        any = 1;
    }
    if (!any && base != 8 && base != 10) serr(n, "invalid integer constant '%s'", n->text);
    for (; *p; p++) {
        if (*p == 'u' || *p == 'U') usfx++;
        else if (*p == 'l' || *p == 'L') lsfx++;
        else serr(n, "invalid suffix '%s' on integer constant", p);
    }
    /* the type: int, long, long long, and the unsigned forms (decimal constants without 'u' stay signed) */
    if (lsfx == 0) {
        if (!usfx && v <= 0x7FFFFFFFULL) t = T_INT;
        else if (usfx && v <= 0xFFFFFFFFULL) t = T_UINT;
        else if (!usfx && base != 10 && v <= 0xFFFFFFFFULL) t = T_UINT;
        else if (!usfx && v <= 0x7FFFFFFFFFFFFFFFULL) t = T_LONG;
        else t = T_ULONG;
    } else if (lsfx == 1) t = usfx ? T_ULONG : (v <= 0x7FFFFFFFFFFFFFFFULL ? T_LONG : T_ULONG);
    else t = usfx ? ty_builtin(TY_ULLONG) : (v <= 0x7FFFFFFFFFFFFFFFULL ? T_LLONG : ty_builtin(TY_ULLONG));
    {
        Val *r = const_val(t, (i64)v);
        if (v == 0 && !usfx && !lsfx) r->is_null_lit = 1;
        return r;
    }
}

static Val *char_literal(Node *n) {
    int *cps, nc, raw;
    char prefix[8];
    Type *t = T_CHAR;
    Val *r;
    nc = decode_literal(n->text, &cps, &raw, prefix);
    if (nc != 1) serr(n, "unsupported character literal");
    if (strcmp(prefix, "L") == 0) t = ty_builtin(TY_WCHAR);
    else if (strcmp(prefix, "u") == 0) t = ty_builtin(TY_CHAR16);
    else if (strcmp(prefix, "U") == 0) t = ty_builtin(TY_CHAR32);
    r = const_val(t, cps[0]);
    return r;
}

static Val *string_literal(Node *n) {
    /* adjacent literals were collected by the parser in n->a, n->a->a ... */
    StrBuf all;
    Node *l;
    int total = 0, i;
    int *buf;
    char prefix[8];
    Type *ct = T_CHAR;
    Val *r;
    sb_init(&all);
    buf = xalloc(sizeof(int) * 4);
    prefix[0] = 0;
    {
        int cap = 64, cnt = 0;
        buf = xalloc(sizeof(int) * cap);
        for (l = n; l; l = l->a) {
            int *cps, nc, raw;
            char pf[8];
            nc = decode_literal(l->text, &cps, &raw, pf);
            if (pf[0] && !prefix[0]) strcpy(prefix, pf);
            for (i = 0; i < nc; i++) {
                if (cnt == cap) {
                    cap *= 2;
                    buf = xrealloc(buf, sizeof(int) * cap);
                }
                buf[cnt++] = cps[i];
            }
        }
        total = cnt;
    }
    if (strcmp(prefix, "L") == 0) ct = ty_builtin(TY_WCHAR);
    else if (strcmp(prefix, "u") == 0) ct = ty_builtin(TY_CHAR16);
    else if (strcmp(prefix, "U") == 0) ct = ty_builtin(TY_CHAR32);
    r = val_new(ty_array(ty_cv(ct, CV_CONST), total + 1), c_string_literal(buf, total, prefix));
    r->lval = 1;
    r->static_init = 1;
    return r;
}

/* ------------------------------------------------------------ identifiers */

/* the type of *this in the member function being written */
/* the class whose object 'this' means: inside a lambda that captured this, the enclosing class */
static int in_lambda_class(void) {
    return cur_class && cur_class->ci && cur_class->ci->is_lambda;
}

static Ent *cur_this_class(void) {
    if (in_lambda_class()) return cur_class->ci->lambda_outer;
    return cur_class;
}

static const char *cur_this_text(void) {
    if (in_lambda_class()) return "(this->__this)";
    return "this";
}

static Type *this_class_type(void) {
    if (in_lambda_class()) {
        Ent *tf = cur_class->ci->lambda_outer ? lookup_in(cur_class->ci->scope, intern_str("__this"), 0) : 0;
        if (tf && tf->type->kind == TY_PTR) return tf->type->base;
        return cur_class->ci->lambda_outer->type;
    }
    {
        Type *t = cur_class->type;
        if (cur_method_const) t = ty_cv(t, CV_CONST);
        return t;
    }
}
static int cur_this_const(void) {
    return in_lambda_class() ? (this_class_type()->cv & CV_CONST) != 0 : cur_method_const;
}
static int have_this(void) {
    if (in_lambda_class()) return 1;           /* operator() of the closure: its fields, and the enclosing object if this was captured */
    return cur_fn && cur_class && is_nonstatic_member(cur_fn);
}

/* a non-static member named without an object: the object is *this */
static Val *member_value(Ent *e, Node *at) {
    Val *v;
    /* a member of the closure itself uses its own this; any other member belongs to the enclosing object of a lambda that captured this */
    int own = !in_lambda_class() || e->owner == cur_class;
    Ent *tcls;
    const char *ttext;
    int tconst;
    Type *tctype;
    if (!own && cur_class && !cur_class->ci->lambda_outer) serr(at, "'%s' cannot be used in the lambda: 'this' was not captured", e->name->s);
    tcls = own ? cur_class : cur_this_class();
    ttext = own ? "this" : cur_this_text();
    tconst = own ? cur_method_const : cur_this_const();
    tctype = own ? (cur_class ? (cur_method_const ? ty_cv(cur_class->type, CV_CONST) : cur_class->type) : 0) : this_class_type();
    if (e->kind == EK_FIELD && unevaluated && !have_this()) {
        Val *uv = val_new(ty_is_ref(e->type) ? e->type->base : e->type, "0");
        uv->lval = 1;
        return uv;
    }
    if (e->kind == EK_FIELD) {
        Type *t = e->type;
        if (!have_this()) serr(at, "invalid use of the non-static data member '%s'", e->name->s);
        check_access(e, e->access, e->owner, at);
        {
            const char *obj = fmt_str("(*%s)", ttext);
            if (e->owner != tcls) obj = base_subobject(obj, tcls->type, e->owner->type, at);
            if (ty_is_ref(t)) {
                v = val_new(t->base, fmt_str("(*%s.%s)", obj, e->name->s));
            } else {
                if (tconst && t->kind != TY_ARRAY) t = ty_cv(t, CV_CONST);
                else if (tconst) t = ty_array(ty_cv(strip_arrays(t), CV_CONST), t->len);
                v = val_new(t, fmt_str("(%s.%s)", obj, e->name->s));
            }
        }
        v->lval = 1;
        return v;
    }
    /* a member function */
    v = val_new(e->type, e->cname);
    v->fn = e;
    v->lval = 1;
    if (!(e->flags & EF_STATIC) && have_this()) {
        if (e->owner != tcls) {
            if (!derived_to_base(tcls->type, e->owner->type)) return v;
            v->this_c = upcast_pointer(ttext, tcls->type, e->owner->type, at);
            v->this_t = ty_cv(e->owner->type, tconst ? CV_CONST : 0);
        } else {
            v->this_c = ttext;
            v->this_t = tctype;
        }
        v->implicit_this = 1;
    }
    return v;
}

static Val *ent_value(Ent *e, Node *at) {
    Val *v;
    switch (e->kind) {
    case EK_FIELD:
        return member_value(e, at);
    case EK_VAR:
        if (e->has_val && ty_is_integral(ty_unqual(ty_strip_ref(e->type))) && !ty_is_ref(e->type)) {
            /* a const integral variable with a constant initializer: an lvalue that reads as its value */
            v = val_new(e->type, e->cname);
            v->lval = 1;
            v->is_const = 1;
            v->cv = e->cv;
            v->var = e;
            return v;
        }
        if (ty_is_ref(e->type)) {
            v = val_new(e->type->base, fmt_str("(*%s)", e->cname));
            v->lval = 1;
            v->is_ref_var = 1;
        } else if (e->flags & EF_INDIRECT) {
            v = val_new(e->type, fmt_str("(*%s)", e->cname));
            v->lval = 1;
            v->is_ref_var = 1;
        } else {
            v = val_new(e->type, e->cname);
            v->lval = 1;
        }
        v->var = e;
        return v;
    case EK_ENUMERATOR:
        v = const_val(e->type, e->cv);
        return v;
    case EK_FUNC: case EK_FTEMPLATE:
        if (e->flags & EF_MEMBER) return member_value(e, at);
        v = val_new(e->type, e->cname);
        v->fn = e;
        v->lval = 1;
        return v;
    }
    serr(at, "'%s' is not a variable or function", e->name->s);
    return 0;
}

static Val *id_expr(Node *n) {
    Ent *e = resolve_id(n, 1);
    Val *v;
    if (e->flags & EF_MEMBER) check_access(e, e->access, e->owner, n);
    v = ent_value(e, n);
    if (n->a->next && v->fn) v->nonvirtual = 1;
    return v;
}

/* ------------------------------------------------------------ operators */

static const char *op_text(int op) {
    return punct_spelling(op);
}

/* the value of an integral operation on constants, as the type t computes it */
static int fold_binary(Node *at, int op, Type *t, i64 a, i64 b, i64 *out) {
    int uns = ty_is_unsigned(t);
    switch (op) {
    case P_PLUS: *out = (i64)((u64)a + (u64)b); return 1;
    case P_MINUS: *out = (i64)((u64)a - (u64)b); return 1;
    case P_STAR: *out = (i64)((u64)a * (u64)b); return 1;
    case P_SLASH:
        if (b == 0) serr(at, "division by zero in a constant expression");
        *out = uns ? (i64)((u64)a / (u64)b) : a / b;
        return 1;
    case P_PERCENT:
        if (b == 0) serr(at, "division by zero in a constant expression");
        *out = uns ? (i64)((u64)a % (u64)b) : a % b;
        return 1;
    case P_AMP: *out = a & b; return 1;
    case P_PIPE: *out = a | b; return 1;
    case P_CARET: *out = a ^ b; return 1;
    case P_SHL: *out = (b < 0 || b > 63) ? 0 : (i64)((u64)a << b); return 1;
    case P_SHR: *out = (b < 0 || b > 63) ? 0 : (uns ? (i64)((u64)a >> b) : a >> b); return 1;
    }
    return 0;
}

static int fold_compare(int op, Type *t, i64 a, i64 b) {
    int uns = ty_is_unsigned(t);
    switch (op) {
    case P_EQ: return a == b;
    case P_NE: return a != b;
    case P_LT: return uns ? (u64)a < (u64)b : a < b;
    case P_GT: return uns ? (u64)a > (u64)b : a > b;
    case P_LE: return uns ? (u64)a <= (u64)b : a <= b;
    case P_GE: return uns ? (u64)a >= (u64)b : a >= b;
    }
    return 0;
}

/* an operand of an arithmetic operator: scalar rvalue */
static Val *operand(Val *v) {
    return rvalue(v);
}

static int is_pointer_like(Type *t) {
    return t->kind == TY_PTR;
}

static Val *binary_expr(Node *n) {
    int op = n->op;
    Val *l, *r, *res;
    Type *lt, *rt, *ct;
    if (op == P_ANDAND || op == P_OROR) {
        Val *l0 = expr(n->a), *r0;
        cond_depth++;
        r0 = expr(n->b);
        cond_depth--;
        if (is_class_operand(l0) || is_class_operand(r0)) {
            Val *ops[2];
            ops[0] = l0;
            ops[1] = r0;
            res = try_operator(n, op, 0, ops, 2);
            if (res) return res;
            l0 = class_to_builtin(l0, n, op);
            r0 = class_to_builtin(r0, n, op);
        }
        l = operand(l0);
        r = operand(r0);
        if (!ty_is_scalar(l->t) || !ty_is_scalar(r->t)) serr(n, "invalid operands to '%s'", op_text(op));
        if (l->is_const && r->is_const) {
            i64 lv = l->cv != 0, rv = r->cv != 0;
            return const_val(T_BOOL, op == P_ANDAND ? (lv && rv) : (lv || rv));
        }
        res = val_new(T_BOOL, fmt_str("(%s %s %s)", l->c, op_text(op), r->c));
        return res;
    }
    {
        Val *l0 = expr(n->a), *r0 = expr(n->b);
        if (is_class_operand(l0) || is_class_operand(r0)) {
            Val *ops[2];
            ops[0] = l0;
            ops[1] = r0;
            res = try_operator(n, op, 0, ops, 2);
            if (res) return res;
            l0 = class_to_builtin(l0, n, op);
            r0 = class_to_builtin(r0, n, op);
        }
        l = operand(l0);
        r = operand(r0);
    }
    lt = l->t;
    rt = r->t;
    /* pointer arithmetic */
    if (op == P_PLUS || op == P_MINUS) {
        if (is_pointer_like(lt) && (ty_is_integral(rt) || (rt->kind == TY_ENUM && !(rt->ent->flags & EF_SCOPED)))) {
            if (lt->base->kind == TY_VOID || lt->base->kind == TY_FUNC) serr(n, "arithmetic on a pointer to void or function");
            return val_new(ty_unqual(lt), fmt_str("(%s %s %s)", l->c, op_text(op), r->c));
        }
        if (op == P_PLUS && is_pointer_like(rt) && ty_is_integral(lt)) {
            if (rt->base->kind == TY_VOID || rt->base->kind == TY_FUNC) serr(n, "arithmetic on a pointer to void or function");
            return val_new(ty_unqual(rt), fmt_str("(%s + %s)", l->c, r->c));
        }
        if (op == P_MINUS && is_pointer_like(lt) && is_pointer_like(rt)) {
            if (!ty_same(lt->base, rt->base)) serr(n, "subtraction of pointers to different types");
            return val_new(T_LONG, fmt_str("(%s - %s)", l->c, r->c));
        }
    }
    /* comparisons */
    if (op == P_EQ || op == P_NE || op == P_LT || op == P_GT || op == P_LE || op == P_GE) {
        int eq = op == P_EQ || op == P_NE;
        if (is_pointer_like(lt) || is_pointer_like(rt) || lt->kind == TY_NULLPTR || rt->kind == TY_NULLPTR) {
            Type *pt = is_pointer_like(lt) ? lt : rt;
            if (!is_pointer_like(pt) && !(lt->kind == TY_NULLPTR && rt->kind == TY_NULLPTR))
                serr(n, "invalid operands to '%s'", op_text(op));
            if (is_pointer_like(lt) && is_pointer_like(rt)) {
                if (!ty_same(lt->base, rt->base) && lt->base->kind != TY_VOID && rt->base->kind != TY_VOID)
                    serr(n, "comparison of distinct pointer types ('%s' and '%s')", ty_str(lt), ty_str(rt));
            } else if (!(is_null_ptr_const(l) || is_null_ptr_const(r)) && !(lt->kind == TY_NULLPTR && rt->kind == TY_NULLPTR)) {
                serr(n, "comparison between a pointer and an integer");
            }
            if (!eq && (lt->kind == TY_NULLPTR || rt->kind == TY_NULLPTR)) serr(n, "invalid operands to '%s'", op_text(op));
            return val_new(T_BOOL, fmt_str("(%s %s %s)", l->c, op_text(op), r->c));
        }
        if (lt->kind == TY_ENUM && rt->kind == TY_ENUM && lt->ent == rt->ent) {
            if (l->is_const && r->is_const) return const_val(T_BOOL, fold_compare(op, T_INT, l->cv, r->cv));
            return val_new(T_BOOL, fmt_str("(%s %s %s)", l->c, op_text(op), r->c));
        }
        if ((lt->kind == TY_ENUM && (lt->ent->flags & EF_SCOPED)) || (rt->kind == TY_ENUM && (rt->ent->flags & EF_SCOPED)))
            serr(n, "invalid operands to '%s' ('%s' and '%s')", op_text(op), ty_str(lt), ty_str(rt));
        if (!(ty_is_arith(lt) || lt->kind == TY_ENUM) || !(ty_is_arith(rt) || rt->kind == TY_ENUM))
            serr(n, "invalid operands to '%s' ('%s' and '%s')", op_text(op), ty_str(lt), ty_str(rt));
        ct = ty_common(lt, rt);
        if (l->is_const && r->is_const && ty_is_integral(ct)) return const_val(T_BOOL, fold_compare(op, ct, l->cv, r->cv));
        return val_new(T_BOOL, fmt_str("(%s %s %s)", l->c, op_text(op), r->c));
    }
    /* arithmetic and bitwise */
    if ((lt->kind == TY_ENUM && (lt->ent->flags & EF_SCOPED)) || (rt->kind == TY_ENUM && (rt->ent->flags & EF_SCOPED)))
        serr(n, "invalid operands to '%s' ('%s' and '%s')", op_text(op), ty_str(lt), ty_str(rt));
    if (!(ty_is_arith(lt) || lt->kind == TY_ENUM) || !(ty_is_arith(rt) || rt->kind == TY_ENUM))
        serr(n, "invalid operands to '%s' ('%s' and '%s')", op_text(op), ty_str(lt), ty_str(rt));
    if (op == P_PERCENT || op == P_AMP || op == P_PIPE || op == P_CARET || op == P_SHL || op == P_SHR) {
        if (ty_is_float(lt) || ty_is_float(rt)) serr(n, "invalid operands to '%s' ('%s' and '%s')", op_text(op), ty_str(lt), ty_str(rt));
    }
    if (op == P_SHL || op == P_SHR) ct = ty_promote(lt);
    else ct = ty_common(lt, rt);
    if (l->is_const && r->is_const && ty_is_integral(ct)) {
        i64 v;
        if (fold_binary(n, op, ct, l->cv, r->cv, &v)) {
            return const_val(ct, v);
        }
    }
    return val_new(ct, fmt_str("(%s %s %s)", l->c, op_text(op), r->c));
}

static Val *unary_expr(Node *n) {
    int op = n->op;
    Val *v, *r, *a = expr(n->a);
    if (op == P_AMP) {
        if (a->fn) {
            if (a->fn->overload) serr(n, "the address of an overloaded function needs a target type");
            if (a->fn->flags & EF_MEMBER) serr(n, "pointers to members are not supported yet");
            r = val_new(ty_ptr(a->fn->type), a->fn->cname);
            r->static_init = 1;
            return r;
        }
        if (a->t->kind == TY_FUNC) return val_new(ty_ptr(a->t), a->c);
        if (!a->lval) serr(n, "the operand of '&' must be an lvalue");
        r = val_new(ty_ptr(a->t), fmt_str("(&%s)", a->c));
        if (a->var && a->var->scope && a->var->scope->kind != SS_BLOCK && !a->is_ref_var) r->static_init = 1;
        return r;
    }
    if (is_class_operand(a)) {
        Val *ops[1];
        ops[0] = a;
        r = try_operator(n, op, 0, ops, 1);
        if (r) return r;
        if (op == P_STAR || op == P_INC || op == P_DEC) serr(n, "no match for 'operator%s' (operand type is '%s')", op_word(op), ty_str(a->t));
        a = class_to_builtin(a, n, op);
    }
    if (op == P_STAR) {
        v = rvalue(a);
        if (v->t->kind != TY_PTR) serr(n, "the operand of '*' is not a pointer ('%s')", ty_str(v->t));
        if (v->t->base->kind == TY_VOID) serr(n, "dereferencing a pointer to void");
        r = val_new(v->t->base, fmt_str("(*%s)", v->c));
        r->lval = 1;
        return r;
    }
    if (op == P_INC || op == P_DEC) {
        require_modifiable(a, n);
        if (!(ty_is_arith(a->t) || a->t->kind == TY_PTR) || a->t->kind == TY_BOOL)
            serr(n, "invalid operand of type '%s' to '%s'", ty_str(a->t), op_text(op));
        r = val_new(a->t, fmt_str("(%s%s)", op_text(op), a->c));
        r->lval = 1;
        return r;
    }
    v = rvalue(a);
    switch (op) {
    case P_NOT:
        if (!ty_is_scalar(v->t)) serr(n, "invalid operand of type '%s' to '!'", ty_str(v->t));
        if (v->is_const) return const_val(T_BOOL, v->cv == 0);
        return val_new(T_BOOL, fmt_str("(!%s)", v->c));
    case P_TILDE:
        if (!ty_is_integral(v->t) && !(v->t->kind == TY_ENUM && !(v->t->ent->flags & EF_SCOPED)))
            serr(n, "invalid operand of type '%s' to '~'", ty_str(v->t));
        {
            Type *pt = ty_promote(v->t);
            if (v->is_const) return const_val(pt, ~v->cv);
            return val_new(pt, fmt_str("(~%s)", v->c));
        }
    case P_PLUS: case P_MINUS:
        if (!ty_is_arith(v->t) && !(v->t->kind == TY_ENUM && !(v->t->ent->flags & EF_SCOPED)))
            serr(n, "invalid operand of type '%s' to unary '%s'", ty_str(v->t), op_text(op));
        {
            Type *pt = ty_promote(v->t);
            if (v->is_const && ty_is_integral(pt)) return const_val(pt, op == P_MINUS ? (i64)(0 - (u64)v->cv) : v->cv);
            return val_new(pt, fmt_str("(%s%s)", op_text(op), v->c));
        }
    }
    serr(n, "unsupported unary operator");
    return 0;
}

static Val *postfix_expr(Node *n) {
    Val *a = expr(n->a), *r;
    if (is_class_operand(a)) {
        Val *ops[1];
        ops[0] = a;
        r = try_operator(n, n->op, 1, ops, 1);
        if (r) return r;
        serr(n, "no match for 'operator%s' (operand type is '%s')", op_word(n->op), ty_str(a->t));
    }
    require_modifiable(a, n);
    if (!(ty_is_arith(a->t) || a->t->kind == TY_PTR) || a->t->kind == TY_BOOL)
        serr(n, "invalid operand of type '%s' to '%s'", ty_str(a->t), op_text(n->op));
    r = val_new(ty_unqual(a->t), fmt_str("(%s%s)", a->c, op_text(n->op)));
    return r;
}

static Val *assign_expr(Node *n) {
    Val *l = expr(n->a), *r, *res;
    Type *lt;
    if (is_class_operand(l)) {
        Val *ops[2];
        Val *rv;
        rv = expr(n->b);
        ops[0] = l;
        ops[1] = rv;
        res = try_operator(n, n->op, 0, ops, 2);
        if (res) return res;
        if (n->op != P_ASSIGN) serr(n, "no match for 'operator%s' (operand types are '%s' and '%s')", op_word(n->op), ty_str(l->t), ty_str(rv->t));
        require_modifiable(l, n);
        {
            Val *src = rv;
            if (rv->braced) src = braced_materialize(rv, ty_unqual(l->t), n);
            else if (!(is_class_operand(rv) && ty_same(rv->t, l->t))) src = class_prvalue(rv, ty_unqual(l->t), n);
            res = val_new(ty_unqual(l->t), class_assign_text_fwd(l->t, l->c, src, n));
            res->lval = 1;
            return res;
        }
    }
    require_modifiable(l, n);
    lt = l->t;
    if (n->op == P_ASSIGN) {
        if (n->b->kind == N_INITLIST) {
            /* x = {v}: a scalar from a braced list of one element */
            Node *e = n->b->a;
            if (!e || e->next) serr(n, "unsupported braced assignment");
            r = convert(expr(e), lt, n);
        } else r = convert(expr(n->b), lt, n);
        res = val_new(ty_unqual(lt), fmt_str("(%s = %s)", l->c, r->c));
        res->lval = 1;
        return res;
    }
    /* compound assignment: the operator applies to the arithmetic types (or pointer += integer) */
    {
        int op = n->op;
        Val *rv0 = expr(n->b), *rv;
        if (is_class_operand(rv0)) rv0 = class_to_builtin(rv0, n, op);
        rv = operand(rv0);
        if (lt->kind == TY_PTR) {
            if ((op != P_ADDEQ && op != P_SUBEQ) || !ty_is_integral(rv->t)) serr(n, "invalid operands to '%s'", op_text(op));
        } else {
            if (!ty_is_arith(lt) || !(ty_is_arith(rv->t) || (rv->t->kind == TY_ENUM && !(rv->t->ent->flags & EF_SCOPED))))
                serr(n, "invalid operands to '%s' ('%s' and '%s')", op_text(op), ty_str(lt), ty_str(rv->t));
            if ((op == P_MODEQ || op == P_ANDEQ || op == P_OREQ || op == P_XOREQ || op == P_SHLEQ || op == P_SHREQ)
                && (ty_is_float(lt) || ty_is_float(rv->t)))
                serr(n, "invalid operands to '%s' ('%s' and '%s')", op_text(op), ty_str(lt), ty_str(rv->t));
        }
        if (lt->kind == TY_BOOL) res = val_new(ty_unqual(lt), fmt_str("(%s = ((%s %c %s) != 0))", l->c, l->c, op_text(op)[0], rv->c));
        else res = val_new(ty_unqual(lt), fmt_str("(%s %s %s)", l->c, op_text(op), rv->c));
        res->lval = 1;
        return res;
    }
}

static Val *cond_expr(Node *n) {
    Val *c0 = expr(n->a), *c, *a, *b, *r;
    Type *t = 0;
    if (is_class_operand(c0)) c0 = class_to_builtin(c0, n, P_QUESTION);
    c = operand(c0);
    cond_depth++;
    a = expr(n->b);
    b = expr(n->c);
    cond_depth--;
    if (!ty_is_scalar(c->t)) serr(n, "the condition of '?:' is not a scalar");
    if (a->t->kind == TY_VOID || b->t->kind == TY_VOID) {
        if (a->t->kind != TY_VOID || b->t->kind != TY_VOID) serr(n, "the operands of '?:' have different types");
        return val_new(T_VOID, fmt_str("(%s ? %s : %s)", c->c, a->c, b->c));
    }
    if (a->lval && b->lval && ty_same(a->t, b->t) && !a->fn && !b->fn && a->t->kind != TY_ARRAY) {
        /* both lvalues of one type: the result is an lvalue (written with pointers in C) */
        r = val_new(a->t, fmt_str("(*(%s ? &%s : &%s))", c->c, a->c, b->c));
        r->lval = 1;
        return r;
    }
    a = rvalue(a);
    b = rvalue(b);
    if (ty_same(a->t, b->t)) t = ty_unqual(a->t);
    else if ((ty_is_arith(a->t) || a->t->kind == TY_ENUM) && (ty_is_arith(b->t) || b->t->kind == TY_ENUM)) t = ty_common(a->t, b->t);
    else if (a->t->kind == TY_PTR && is_null_ptr_const(b)) t = a->t;
    else if (b->t->kind == TY_PTR && is_null_ptr_const(a)) t = b->t;
    else if (a->t->kind == TY_PTR && b->t->kind == TY_PTR && ptr_conversion(a->t, b->t) != CONV_NONE) t = b->t;
    else if (a->t->kind == TY_PTR && b->t->kind == TY_PTR && ptr_conversion(b->t, a->t) != CONV_NONE) t = a->t;
    else serr(n, "the operands of '?:' have incompatible types '%s' and '%s'", ty_str(a->t), ty_str(b->t));
    a = convert(a, t, n);
    b = convert(b, t, n);
    if (c->is_const && a->is_const && b->is_const) return c->cv ? a : b;
    return val_new(t, fmt_str("(%s ? %s : %s)", c->c, a->c, b->c));
}

/* ------------------------------------------------------------ calls */

/* the C text of an argument for a parameter of type p (a reference binds by address) */
static const char *bind_argument(Val *a, Type *p, Node *at) {
    if (a->braced) a = braced_materialize(a, p, at);
    if (p->kind == TY_CLASS) {
        Val *cv = class_prvalue(a, p, at);
        return fmt_str("(&%s)", cv->c);
    }
    if (p->kind == TY_REF || p->kind == TY_RREF) {
        Type *tb = p->base;
        if (tb->kind == TY_CLASS) {
            if (a->t->kind == TY_CLASS && !ty_same(a->t, tb) && derived_to_base(a->t, tb)) {
                Val *bv = upcast_value(a, tb, at);
                return fmt_str("(&%s)", bv->c);
            }
            if ((a->lval || a->xvalue) && !a->fn && a->t->kind == TY_CLASS && ty_same(a->t, tb)) return fmt_str("(&%s)", a->c);
            if (a->t->kind == TY_CLASS && ty_same(a->t, tb) && a->is_temp) return fmt_str("(&%s)", a->c);
            {
                Val *cv = class_prvalue(a, ty_unqual(tb), at);
                return fmt_str("(&%s)", cv->c);
            }
        }
        if (a->lval && !a->fn && (ty_same(a->t, tb) || (a->t->kind == TY_ARRAY && tb->kind == TY_ARRAY))) {
            return fmt_str("(&%s)", a->c);
        }
        /* a temporary: const T& from a value or a converted value */
        {
            Val *cv = convert(a, ty_unqual(tb), at);
            return fmt_str("(&(%s){%s})", c_decl(ty_unqual(tb), ""), cv->c);
        }
    }
    return convert(a, p, at)->c;
}

static const char *arg_list_text(Type *ft, Ent *fent, Val **args, int nargs, Node *at) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    for (i = 0; i < ft->nparams; i++) {
        const char *t;
        if (i) sb_puts(&sb, ", ");
        if (i < nargs) t = bind_argument(args[i], ft->params[i], at);
        else {
            Node *def = fent && fent->defaults ? fent->defaults[i] : 0;
            Val *dv;
            if (!def) serr(at, "too few arguments to function");
            {
                Scope2 *saved = scope2_cur;
                scope2_cur = fent->scope;
                dv = expr(def);
                scope2_cur = saved;
            }
            t = bind_argument(dv, ft->params[i], at);
        }
        sb_puts(&sb, t);
    }
    for (i = ft->nparams; i < nargs; i++) {
        Val *a = rvalue(args[i]);
        if (i) sb_puts(&sb, ", ");
        if (a->t->kind == TY_FLOAT) a = convert(a, T_DOUBLE, at);
        else if (ty_is_integral(a->t) && ty_int_rank(a->t) < 4) a = convert(a, T_INT, at);
        sb_puts(&sb, a->c);
    }
    return sb.s;
}

static Val *call_result(Type *ret, const char *call) {
    Val *r;
    if (ty_is_ref(ret)) {
        r = val_new(ret->base, fmt_str("(*%s)", call));
        if (ret->kind == TY_RREF) r->xvalue = 1;      /* an rvalue reference result is an xvalue */
        else r->lval = 1;
        return r;
    }
    return val_new(ret, call);
}

static Val *call_expr(Node *n) {
    Val *callee = expr(n->a);
    Val *args[MAXARGS];
    int nargs = 0;
    Node *a;
    Ent *f = 0;
    Type *ft = 0;
    const char *argtext;
    (void)a;
    nargs = expr_args(n->b, args, 0, MAXARGS);
    if (callee->noop_call) return val_new(T_VOID, "((void)0)");
    if (callee->fn) {
        int amb;
        f = resolve_overload_obj(callee->fn, args, nargs, callee->this_t, &amb);
        if (!f) {
            if (amb) serr(n, "call of overloaded '%s' is ambiguous", callee->fn->name->s);
            if (callee->fn->overload || callee->fn->kind == EK_FTEMPLATE) serr(n, "no matching function for call to '%s'", callee->fn->name->s);
            if (nargs < func_min_args(callee->fn)) serr(n, "too few arguments to function '%s'", callee->fn->name->s);
            if (nargs > callee->fn->type->nparams && !callee->fn->type->variadic)
                serr(n, "too many arguments to function '%s'", callee->fn->name->s);
            {
                int i;
                for (i = 0; i < nargs && i < callee->fn->type->nparams; i++)
                    if (arg_conversion(args[i], callee->fn->type->params[i]) == CONV_NONE)
                        serr(n, "cannot convert argument %d from '%s' to '%s'", i + 1, ty_str(args[i]->t), ty_str(callee->fn->type->params[i]));
            }
            if (callee->this_t && (callee->this_t->cv & CV_CONST))
                serr(n, "passing a const object to the non-const member function '%s'", callee->fn->name->s);
            serr(n, "no matching function for call to '%s'", callee->fn->name->s);
        }
        if (f->flags & EF_MEMBER) check_access(f, f->access, f->owner, n);
        if (is_nonstatic_member(f) && !callee->this_c) serr(n, "cannot call the member function '%s' without an object", ent_qualified_name(f));
        {
            i64 cres;
            if ((f->flags & EF_CONSTEXPR) && const_call(f, args, nargs, &cres)) return const_val(ty_unqual(f->type->base), cres);
        }
        {
            Val *res;
            force_nonvirtual = callee->nonvirtual;
            res = call_function(f, callee->this_c, args, nargs, n, n);
            force_nonvirtual = 0;
            return res;
        }
    }
    if (is_class_type(callee->t)) {
        /* a function object: operator() */
        Val *ops[MAXARGS + 1];
        int i;
        Val *r;
        ops[0] = callee;
        for (i = 0; i < nargs; i++) ops[i + 1] = args[i];
        r = try_operator(n, P_LPAREN, 0, ops, nargs + 1);
        if (!r) serr(n, "'%s' cannot be used as a function", ty_str(callee->t));
        return r;
    }
    callee = rvalue(callee);
    if (callee->t->kind == TY_PTR && callee->t->base->kind == TY_FUNC) ft = callee->t->base;
    else serr(n, "the called object is not a function ('%s')", ty_str(callee->t));
    if (nargs < ft->nparams) serr(n, "too few arguments in the call");
    if (nargs > ft->nparams && !ft->variadic) serr(n, "too many arguments in the call");
    {
        int i;
        for (i = 0; i < ft->nparams; i++)
            if (arg_conversion(args[i], ft->params[i]) == CONV_NONE)
                serr(n, "cannot convert argument %d from '%s' to '%s'", i + 1, ty_str(args[i]->t), ty_str(ft->params[i]));
    }
    argtext = arg_list_text(ft, 0, args, nargs, n);
    return call_result(ft->base, fmt_str("%s(%s)", callee->c, argtext));
}

static Val *index_expr(Node *n) {
    Val *a = expr(n->a), *i0 = expr(n->b), *i, *r;
    Type *et;
    if (is_class_operand(a)) {
        Val *ops[2];
        ops[0] = a;
        ops[1] = i0;
        r = try_operator(n, P_LBRACK, 0, ops, 2);
        if (!r) serr(n, "no match for 'operator[]' (operand types are '%s' and '%s')", ty_str(a->t), ty_str(i0->t));
        return r;
    }
    i = rvalue(i0);
    if (a->t->kind == TY_ARRAY || a->t->kind == TY_PTR) {
        et = a->t->base;
        if (!ty_is_integral(i->t) && !(i->t->kind == TY_ENUM && !(i->t->ent->flags & EF_SCOPED)))
            serr(n, "the array index is not an integer ('%s')", ty_str(i->t));
        r = val_new(et, fmt_str("%s[%s]", a->c, i->c));
        r->lval = 1;
        return r;
    }
    {
        Val *ar = rvalue(i);
        if (ar->t->kind == TY_PTR && ty_is_integral(rvalue(a)->t)) {
            r = val_new(ar->t->base, fmt_str("%s[%s]", ar->c, rvalue(a)->c));
            r->lval = 1;
            return r;
        }
    }
    serr(n, "invalid types '%s[%s]' for array subscript", ty_str(a->t), ty_str(i->t));
    return 0;
}

/* ------------------------------------------------------------ casts */

static Type *typeid_type(Node *ti) {
    Type *t = decl_specs_type(ti->a, 0);
    return apply_declarator(t, ti->b, 0, 0);
}

/* the pointer text p (to class 'base') converted to the derived class d that contains it */
static const char *downcast_pointer(const char *p, Type *base, Type *d, Node *at) {
    int n;
    const char *path = base_path(d, base->ent, &n);
    const char *tmp;
    if (!path) serr(at, "'%s' is not a base of '%s'", ty_str(base), ty_str(d));
    if (n > 1) serr(at, "'%s' is an ambiguous base of '%s'", ty_str(base), ty_str(d));
    if (path[0] == 0) return fmt_str("((%s)%s)", c_decl(ty_ptr(d), ""), p);
    tmp = new_temp(ty_ptr(base));
    return fmt_str("(%s = %s, %s ? (%s)((char *)%s - ((char *)&((%s *)0)->%s - (char *)0)) : 0)", tmp, p, tmp, c_decl(ty_ptr(d), ""), tmp, class_tag(d->ent), path + 1);
}

/* a cast of v to the type t: kind = 0 C-style, KW_static_cast, KW_reinterpret_cast, KW_const_cast */
static Val *cast_value(Val *v, Type *to, int kind, Node *at) {
    Val *r;
    Type *tu = ty_unqual(to);
    if (to->kind == TY_RREF && (v->lval || v->xvalue || v->is_temp) && ty_same(v->t, to->base) && kind != KW_reinterpret_cast) {
        /* static_cast<T&&>(x): an xvalue */
        r = val_new(to->base, v->c);
        r->xvalue = 1;
        return r;
    }
    if (ty_is_ref(to)) {
        Type *tb = to->base;
        if (!v->lval) {
            if (kind == KW_const_cast || kind == KW_reinterpret_cast || !(tb->cv & CV_CONST)) serr(at, "cannot cast an rvalue to '%s'", ty_str(to));
            return convert(v, ty_unqual(tb), at);
        }
        if (ty_same(v->t, tb)) {
            r = val_new(tb, v->c);
            r->lval = 1;
            return r;
        }
        if (v->t->kind == TY_CLASS && tb->kind == TY_CLASS && kind != KW_reinterpret_cast && kind != KW_const_cast) {
            if (derived_to_base(v->t, tb)) {
                r = upcast_value(v, tb, at);
                return r;
            }
            if (derived_to_base(tb, v->t)) {
                r = val_new(tb, fmt_str("(*%s)", downcast_pointer(fmt_str("(&%s)", v->c), ty_unqual(v->t), ty_unqual(tb), at)));
                r->lval = 1;
                return r;
            }
        }
        if (kind == KW_static_cast) serr(at, "invalid static_cast from '%s' to '%s'", ty_str(v->t), ty_str(to));
        r = val_new(tb, fmt_str("(*(%s)&%s)", c_decl(ty_ptr(tb), ""), v->c));
        r->lval = 1;
        return r;
    }
    if (tu->kind == TY_VOID) return val_new(T_VOID, fmt_str("((void)(%s))", v->c));
    v = rvalue(v);
    if (v->t->kind == TY_CLASS && tu->kind != TY_CLASS) {
        Ent *cf = 0;
        if (find_conversion(v->t, tu, &cf) != CONV_NONE && cf) {
            Val *cv = call_function(cf, fmt_str("(&%s)", v->c), 0, 0, at, 0);
            return cast_value(cv, to, kind, at);
        }
    }
    if (kind == KW_const_cast) {
        if (!(ty_same(ty_unqual(v->t), tu) || (v->t->kind == TY_PTR && tu->kind == TY_PTR && ty_same(v->t->base, tu->base))))
            serr(at, "invalid const_cast from '%s' to '%s'", ty_str(v->t), ty_str(to));
        r = val_new(tu, v->c);
        return r;
    }
    if (ty_same(v->t, tu)) return v;
    /* arithmetic, enum, bool */
    if ((ty_is_arith(tu) || tu->kind == TY_ENUM) && (ty_is_arith(v->t) || v->t->kind == TY_ENUM)) {
        if (kind == KW_reinterpret_cast) serr(at, "invalid reinterpret_cast from '%s' to '%s'", ty_str(v->t), ty_str(to));
        if (v->is_const && ty_is_integral(tu)) return const_val(tu, tu->kind == TY_BOOL ? v->cv != 0 : v->cv);
        if (v->is_const && tu->kind == TY_ENUM) {
            r = const_val(tu, v->cv);
            return r;
        }
        if (tu->kind == TY_BOOL) return val_new(tu, fmt_str("((%s) != 0)", v->c));
        r = val_new(tu, cast_text(tu, v->c));
        r->static_init = v->static_init;
        return r;
    }
    /* pointers to classes: up and down the hierarchy */
    if (tu->kind == TY_PTR && v->t->kind == TY_PTR && tu->base->kind == TY_CLASS && v->t->base->kind == TY_CLASS
        && !ty_same(tu->base, v->t->base) && kind != KW_reinterpret_cast && kind != KW_const_cast) {
        if (derived_to_base(v->t->base, tu->base)) return val_new(tu, upcast_pointer(v->c, ty_unqual(v->t->base), ty_unqual(tu->base), at));
        if (derived_to_base(tu->base, v->t->base)) return val_new(tu, downcast_pointer(v->c, ty_unqual(v->t->base), ty_unqual(tu->base), at));
    }
    /* pointers */
    if (tu->kind == TY_PTR && (v->t->kind == TY_PTR || is_null_ptr_const(v))) {
        if (kind == KW_static_cast && v->t->kind == TY_PTR && v->t->base->kind != TY_VOID && tu->base->kind != TY_VOID
            && !ty_same(v->t->base, tu->base))
            serr(at, "invalid static_cast from '%s' to '%s'", ty_str(v->t), ty_str(to));
        r = val_new(tu, cast_text(tu, v->c));
        r->static_init = v->static_init;
        return r;
    }
    if (tu->kind == TY_PTR && ty_is_integral(v->t) && kind != KW_static_cast) {
        r = val_new(tu, cast_text(tu, v->c));
        r->static_init = v->static_init;
        return r;
    }
    if (ty_is_integral(tu) && v->t->kind == TY_PTR && kind != KW_static_cast) {
        r = val_new(tu, cast_text(tu, v->c));
        return r;
    }
    if (tu->kind == TY_BOOL && v->t->kind == TY_PTR) return val_new(tu, fmt_str("((%s) != 0)", v->c));
    serr(at, "invalid %scast from '%s' to '%s'", kind == KW_static_cast ? "static_" : kind == KW_reinterpret_cast ? "reinterpret_" : "", ty_str(v->t), ty_str(to));
    return 0;
}

static Val *sizeof_expr(Node *n) {
    Type *t;
    i64 sz;
    if (n->flags & 1) {
        Node *id = n->a ? n->a : (n->b && n->b->a && n->b->a->a ? n->b->a->a : 0);
        Ent *pk = id && id->kind == N_ID && id->a && !id->a->next ? lookup_unqual(scope2_cur, id->a->name) : 0;
        if (!pk || pk->kind != EK_PACK) serr(n, "sizeof... needs a parameter pack");
        return const_val(T_ULONG, pk->nelems);
    }
    if (n->b) t = typeid_type(n->b);
    else {
        Val *v;
        unevaluated++;
        v = expr(n->a);
        unevaluated--;
        t = v->t;
    }
    if (t->kind == TY_FUNC || t->kind == TY_VOID) serr(n, "invalid application of 'sizeof' to '%s'", ty_str(t));
    if (type_size_words(t, &sz)) {
        Val *r = const_val(T_ULONG, sz);
        return r;
    }
    {
        Val *r = val_new(T_ULONG, fmt_str("sizeof(%s)", c_decl(t, "")));
        r->static_init = 1;
        return r;
    }
}

/* a temporary variable of the function being written */
static const char *new_temp(Type *t) {
    const char *name = fmt_str("__t%d", temp_count++);
    sb_puts(temps, c_decl(t, name));
    sb_puts(temps, ";\n");
    return name;
}

/* the helper functions for arrays of objects: allocate and construct, destroy and free */
static void ensure_array_helpers(Type *cls) {
    ClassInfo *ci = cinfo(cls);
    StrBuf *saved = out;
    const char *tag = class_tag(cls->ent), *cn = cls->ent->cname;
    const char *ctor;
    if (ci->has_array_helpers) return;
    ci->has_array_helpers = 1;
    out = &pending_helpers;
    ctor = type_needs_ctor(cls) ? default_construct_text(cls, "(&__a[__i])", 0) : 0;
    emitf("static %s *%s__newarr(long __n) {\n    long *__base = (long *)malloc((__n * sizeof(%s)) + 1);\n    %s *__a;\n    long __i;\n", tag, cn, tag, tag);
    emitf("    __base[0] = __n;\n    __a = (%s *)(__base + 1);\n", tag);
    if (ctor) emitf("    for (__i = 0; __i < __n; __i++) %s;\n", ctor);
    emit("    return __a;\n}\n");
    emitf("static void %s__delarr(%s *__a) {\n    long *__base = ((long *)__a) - 1;\n    long __i;\n", cn, tag);
    if (ci->needs_dtor) emitf("    for (__i = __base[0] - 1; __i >= 0; __i--) %s(&__a[__i]);\n", ci->dtor->cname);
    emit("    free(__base);\n}\n");
    out = saved;
}

static Val *new_expr(Node *n) {
    Type *t = typeid_type(n->b);
    Val *r;
    const char *tmp;
    if (n->a) return placement_new(n, t);
    if (t->kind == TY_VOID || t->kind == TY_FUNC) serr(n, "cannot allocate an object of type '%s'", ty_str(t));
    if (n->flags & 4) {
        /* new T[n] */
        Val *cnt = n->d->b ? rvalue(expr(n->d->b)) : 0;
        Type *at = t;
        Node *dim;
        const char *count = cnt->c;
        for (dim = n->d->next; dim; dim = dim->next) {
            Val *dv = expr(dim->b);
            if (!dv->is_const) serr(dim, "the array bound must be constant");
            at = ty_array(at, dv->cv);
            count = fmt_str("(%s * %lld)", count, dv->cv);
        }
        if (!cnt || !ty_is_integral(cnt->t)) serr(n, "the array size is not an integer");
        if (n->c) serr(n, "array initializers in new-expressions are not supported yet");
        if (at->kind == TY_CLASS && (type_needs_ctor(at) || type_needs_dtor(at))) {
            ensure_array_helpers(at);
            return val_new(ty_ptr(at), fmt_str("%s__newarr(%s)", at->ent->cname, count));
        }
        r = val_new(ty_ptr(at), fmt_str("((%s)malloc((%s) * sizeof(%s)))", c_decl(ty_ptr(at), ""), count, c_decl(at, "")));
        return r;
    }
    if (t->kind == TY_CLASS) {
        Val *args[MAXARGS];
        int na = 0;
        Node *a, *list = n->c ? (n->c->kind == N_INITLIST ? n->c->a : n->c) : 0;
        const char *text;
        require_complete(t, n);
        require_concrete(t, n);
        (void)a;
        na = expr_args(list, args, 0, MAXARGS);
        tmp = new_temp(ty_ptr(t));
        if ((n->flags & 2) && cinfo(t)->aggregate && !class_ctors(t) && na > 0) {
            /* the allocation and the field initializations are written before the statement */
            emitf("%s = (%s)malloc(sizeof(%s));\n", tmp, c_decl(ty_ptr(t), ""), c_decl(t, ""));
            aggregate_init_stmts(ty_unqual(t), tmp, list, n);
            return val_new(ty_ptr(t), tmp);
        }
        text = ctor_call(ty_unqual(t), tmp, args, na, n);
        if (!text) return val_new(ty_ptr(t), fmt_str("(%s = (%s)malloc(sizeof(%s)))", tmp, c_decl(ty_ptr(t), ""), c_decl(t, "")));
        return val_new(ty_ptr(t), fmt_str("(%s = (%s)malloc(sizeof(%s)), %s, %s)", tmp, c_decl(ty_ptr(t), ""), c_decl(t, ""), text, tmp));
    }
    if (n->c) {
        Node *e = n->c->kind == N_INITLIST ? n->c->a : n->c;
        Val *iv, *sv[2];
        int sn;
        if (!e) {
            r = val_new(ty_ptr(t), fmt_str("((%s)calloc(1, sizeof(%s)))", c_decl(ty_ptr(t), ""), c_decl(t, "")));
            return r;
        }
        sn = expr_args(e, sv, 0, 2);
        if (sn > 1) serr(n, "unsupported initializer in new-expression");
        if (sn == 0) {
            r = val_new(ty_ptr(t), fmt_str("((%s)calloc(1, sizeof(%s)))", c_decl(ty_ptr(t), ""), c_decl(t, "")));
            return r;
        }
        iv = convert(sv[0], t, n);
        tmp = new_temp(ty_ptr(t));
        r = val_new(ty_ptr(t), fmt_str("(%s = (%s)malloc(sizeof(%s)), *%s = %s, %s)", tmp, c_decl(ty_ptr(t), ""), c_decl(t, ""), tmp, iv->c, tmp));
        return r;
    }
    return val_new(ty_ptr(t), fmt_str("((%s)malloc(sizeof(%s)))", c_decl(ty_ptr(t), ""), c_decl(t, "")));
}

static Val *delete_expr(Node *n) {
    Val *v = rvalue(expr(n->a));
    Type *t;
    if (v->t->kind != TY_PTR) serr(n, "the operand of delete is not a pointer ('%s')", ty_str(v->t));
    t = ty_unqual(v->t->base);
    if (t->kind == TY_CLASS) {
        ClassInfo *ci = cinfo(t);
        if (n->flags & 2) {
            if (type_needs_ctor(t) || type_needs_dtor(t)) {
                ensure_array_helpers(t);
                return val_new(T_VOID, fmt_str("%s__delarr(%s)", t->ent->cname, v->c));
            }
        } else if (ci->has_virtual_dtor) {
            /* the virtual destructor slot holds the deleting destructor of the dynamic type */
            Ent *d = ci->dtor;
            const char *tmp = new_temp(ty_ptr(t));
            const char *q = upcast_pointer(tmp, t, d->vowner->type, n);
            Type *fty = ty_func(T_VOID, 0, 0, 0);
            Type **ps = anew(sizeof(Type *) * 2);
            ps[0] = ty_ptr(d->vowner->type);
            fty = ty_func(T_VOID, ps, 1, 0);
            return val_new(T_VOID, fmt_str("(%s = %s, %s ? ((%s)(*(void ***)%s)[%d])(%s) : (void)0)", tmp, v->c, tmp,
                                           c_decl(ty_ptr(fty), ""), q, d->vslot + 1, q));
        } else if (ci->needs_dtor) {
            const char *tmp = new_temp(ty_ptr(t));
            return val_new(T_VOID, fmt_str("(%s = %s, %s ? (%s(%s), free(%s)) : (void)0)", tmp, v->c, tmp, ci->dtor->cname, tmp, tmp));
        }
    }
    return val_new(T_VOID, fmt_str("free(%s)", v->c));
}

static Val *funccast_expr(Node *n) {
    Type *t = typeid_type(n->b);
    Node *e = n->a;
    if (n->flags & 1) e = n->a ? n->a->a : 0;
    if (ty_unqual(t)->kind == TY_CLASS) {
        /* T(args) / T{args}: a temporary object */
        Type *cls = ty_unqual(t);
        Val *args[MAXARGS];
        int na = 0;
        const char *dest, *text;
        Val *r;
        Node *a;
        require_complete(cls, n);
        require_concrete(cls, n);
        if ((n->flags & 1) && cinfo(cls)->aggregate && !class_ctors(cls)) {
            const char *tmp;
            Node *l = anew(sizeof(Node));
            l->kind = N_INITLIST;
            l->file = n->file;
            l->line = n->line;
            l->a = e;
            dest = prvalue_dest(cls, n);
            aggregate_init_stmts(cls, dest, e, n);
            r = val_new(cls, fmt_str("(*%s)", dest));
            r->is_temp = 1;
            (void)tmp;
            (void)l;
            return r;
        }
        (void)a;
        na = expr_args(e, args, 0, MAXARGS);
        dest = prvalue_dest(cls, n);
        text = ctor_call(cls, dest, args, na, n);
        r = val_new(cls, fmt_str("(*%s)", text ? text : dest));
        r->is_temp = 1;
        return r;
    }
    if (!e) {
        /* T(): the value-initialized scalar */
        if (!ty_is_scalar(t)) serr(n, "unsupported value-initialization of '%s'", ty_str(t));
        if (t->kind == TY_PTR) return val_new(ty_unqual(t), "((void *)0)");
        return ty_is_float(t) ? val_new(ty_unqual(t), "0.0") : const_val(ty_unqual(t), 0);
    }
    if (e->next) serr(n, "a functional cast takes one argument");
    return cast_value(expr(e), t, KW_static_cast, n);
}

/* ------------------------------------------------------------ overloaded operators */

static int is_class_operand(Val *v) {
    return v->t->kind == TY_CLASS;
}

static Scope2 *namespace_of_ent(Ent *e) {
    Scope2 *sc = e->scope;
    while (sc && sc->kind != SS_NS && sc->kind != SS_GLOBAL) sc = sc->parent;
    return sc;
}

/* appends copies of the free functions of an overload set to the chain (head, tail) */
static void add_free_candidates(Ent **head, Ent **tail, Ent *set, Val **args, int nargs) {
    Ent *f;
    for (f = set; f; f = f->overload) {
        Ent *c;
        Ent *x;
        Ent *src = f;
        int dup = 0;
        if (f->kind == EK_FTEMPLATE) {
            if (f->flags & EF_MEMBER) continue;
            src = deduce_function(f, args, nargs, 0);
            if (!src) continue;
        } else if (f->kind != EK_FUNC || (f->flags & EF_MEMBER)) continue;
        for (x = *head; x; x = x->overload) if (x->cname == src->cname) dup = 1;
        if (dup) continue;
        c = anew(sizeof(Ent));
        *c = *src;
        c->overload = 0;
        if (*tail) (*tail)->overload = c;
        else *head = c;
        *tail = c;
    }
}

/* the operator function for the operands: a member of the first operand's class, or a free function found by
   unqualified lookup and by argument-dependent lookup; 0 if there is none */
static Val *try_operator(Node *at, int op, int postfix, Val **ops, int nops) {
    Name *name = intern_str(fmt_str("operator%s", op_word(op)));
    Ent *mem = 0, *chain = 0, *tail = 0, *f;
    Val *extra[MAXARGS + 1];
    Val *zero;
    int amb = 0, i, any = 0;
    for (i = 0; i < nops; i++) if (is_class_operand(ops[i])) any = 1;
    if (!any) return 0;
    for (i = 0; i < nops; i++) extra[i] = ops[i];
    if (postfix) {
        zero = const_val(T_INT, 0);
        extra[nops++] = zero;
    }
    if (is_class_operand(ops[0]) && ops[0]->t->ent->ci && ops[0]->t->ent->ci->complete) mem = class_member(ops[0]->t, name);
    /* free functions: ordinary lookup (ignoring members) and the namespaces of the class operands */
    {
        Ent *e = lookup_unqual(scope2_cur, name);
        Scope2 *sc;
        if (e && funcish_kind(e->kind)) add_free_candidates(&chain, &tail, e, extra, nops);
        for (i = 0; i < nops; i++) {
            Type *t = ty_unqual(ty_strip_ref(extra[i]->t));
            if (t->kind == TY_CLASS || t->kind == TY_ENUM) {
                sc = namespace_of_ent(t->ent);
                if (sc) {
                    Ent *a = sfind(sc, name);
                    if (a && funcish_kind(a->kind)) add_free_candidates(&chain, &tail, a, extra, nops);
                }
            }
        }
    }
    {
        /* member and free candidates compete on the same arguments (the object is the first one for a member) */
        Ent *cands[2 * MAX_CANDS];
        int *rk[2 * MAX_CANDS];
        int ismem[2 * MAX_CANDS];
        Ent *mlist[MAX_CANDS];
        int nm = 0, mi;
        int nc = 0, j, best = -1;
        Ent *set;
        {
            Ent *mf;
            for (mf = mem; mf && nm < MAX_CANDS; mf = mf->overload) {
                if (mf->kind == EK_FTEMPLATE) {
                    Ent *inst = deduce_function(mf, extra + 1, nops - 1, ops[0]->t);
                    if (inst) mlist[nm++] = inst;
                } else mlist[nm++] = mf;
            }
        }
        for (j = 0; j < 2; j++) {
            mi = 0;
            for (set = j == 0 ? (nm ? mlist[0] : 0) : chain; set && nc < 2 * MAX_CANDS; set = (j == 0 ? (++mi < nm ? mlist[mi] : 0) : set->overload)) {
                int nparams, k, ok = 1, off = j == 0 ? 1 : 0;
                int *r;
                if (set->kind != EK_FUNC) continue;
                nparams = set->type->nparams + off;
                if (nops < nparams - set->type->nparams + func_min_args(set) || (nops > nparams && !set->type->variadic)) continue;
                if (j == 0 && (ops[0]->t->cv & CV_CONST) && !(set->flags & EF_CONST_METHOD)) continue;
                r = anew(sizeof(int) * (nops + 2));
                for (k = 0; k < nops; k++) {
                    int c;
                    int tb = 0;
                    if (j == 0 && k == 0) c = (set->flags & EF_CONST_METHOD) && !(ops[0]->t->cv & CV_CONST) ? 1 : 0;
                    else {
                        int pi = k - off;
                        c = pi < set->type->nparams ? arg_conversion(extra[k], set->type->params[pi]) : CONV_ELLIPSIS;
                        if (pi < set->type->nparams) tb = arg_tiebreak(extra[k], set->type->params[pi]);
                    }
                    if (c == CONV_NONE) { ok = 0; break; }
                    r[k] = c * 2 + tb;
                }
                if (!ok) continue;
                cands[nc] = set;
                rk[nc] = r;
                ismem[nc] = j == 0;
                nc++;
            }
        }
        for (j = 0; j < nc && best < 0; j++) {
            int k, all = 1;
            for (k = 0; k < nc; k++)
                if (k != j && compare_candidates(cands[j], cands[k], rk[j], rk[k], nops - 1) >= 0) all = 0;
            if (all) best = j;
        }
        if (best >= 0) {
            f = cands[best];
            if (ismem[best]) {
                check_access(f, f->access, f->owner, at);
                return call_function(f, fmt_str("(&%s)", ops[0]->c), extra + 1, nops - 1, at, at);
            }
            return call_function(f, 0, extra, nops, at, at);
        }
        if (nc > 0) serr(at, "ambiguous overload for 'operator%s'", op_word(op));
        (void)amb;
    }
    return 0;
}

/* a class operand of a built-in operator: converted with its conversion function */
static Val *class_to_builtin(Val *v, Node *at, int op) {
    ClassInfo *ci;
    Ent *set, *f;
    int n = 0;
    if (!is_class_operand(v)) return v;
    ci = v->t->ent->ci;
    if (op == P_NOT || op == P_ANDAND || op == P_OROR || op == P_QUESTION) return rvalue(convert(v, T_BOOL, at));
    set = ci ? sfind(ci->scope, intern_str("operator conv")) : 0;
    for (f = set; f; f = f->overload) n++;
    if (n == 1) return rvalue(call_function(set, fmt_str("(&%s)", v->c), 0, 0, at, 0));
    if (n > 1) serr(at, "ambiguous conversion of '%s' for 'operator%s'", ty_str(v->t), op_word(op));
    serr(at, "no match for 'operator%s' (operand type is '%s')", op_word(op), ty_str(v->t));
    return 0;
}

/* ------------------------------------------------------------ member access */

/* a name in the class (not its enclosing scopes) */
static Ent *class_member(Type *cls, Name *name) {
    ClassInfo *ci;
    if (!cls->ent->ci) return 0;
    ci = cls->ent->ci;
    return lookup_in(ci->scope, name, 0);
}

static Val *member_expr(Node *n) {
    Val *obj = expr(n->a), *v;
    Type *ct;
    const char *objc, *this_c;
    Node *id = n->b;
    Node *part;
    Ent *m;
    Type *objt;
    int qualified = 0;
    if ((id->a->flags & 7) == PART_DTOR) {
        /* a pseudo-destructor call on an object that is not of a class type (p->~T() with T = int) does nothing */
        Type *ot = obj->t;
        if (n->op == P_ARROW) ot = rvalue(obj)->t->kind == TY_PTR ? rvalue(obj)->t->base : ot;
        if (ot->kind != TY_CLASS) {
            Val *dv = val_new(T_VOID, "((void)0)");
            dv->noop_call = 1;
            return dv;
        }
    }
    if (n->op == P_ARROW) {
        Val *p = rvalue(obj);
        /* a class with operator-> yields the pointer */
        while (p->t->kind == TY_CLASS) {
            Val *ops[1];
            Val *r;
            ops[0] = p;
            r = try_operator(n, P_ARROW, 0, ops, 1);
            if (!r) serr(n, "the type '%s' does not have a member operator->", ty_str(p->t));
            p = rvalue(r);
        }
        if (p->t->kind != TY_PTR || p->t->base->kind != TY_CLASS) serr(n, "the left operand of '->' is not a pointer to a class ('%s')", ty_str(p->t));
        objt = p->t->base;
        ct = ty_unqual(objt);
        objc = fmt_str("(*%s)", p->c);
        this_c = p->c;
    } else {
        if (obj->t->kind != TY_CLASS) serr(n, "the left operand of '.' is not a class object ('%s')", ty_str(obj->t));
        objt = obj->t;
        ct = ty_unqual(objt);
        objc = obj->c;
        this_c = fmt_str("(&%s)", obj->c);
    }
    if (!ct->ent->ci || !ct->ent->ci->complete) serr(n, "invalid use of incomplete type '%s'", ty_str(ct));
    part = id->a;
    if ((part->flags & 7) == PART_DTOR) {
        ClassInfo *dci = cinfo(ct);
        Val *dv;
        if (!dci->needs_dtor || !dci->dtor) {
            dv = val_new(T_VOID, "((void)0)");
            dv->noop_call = 1;
            return dv;
        }
        dv = val_new(dci->dtor->type, dci->dtor->cname);
        dv->fn = dci->dtor;
        dv->lval = 1;
        dv->this_c = this_c;
        dv->this_t = objt;
        dv->nonvirtual = 1;
        return dv;
    }
    if ((part->flags & 7) == PART_OPERATOR) {
        Name *on = intern_str(fmt_str("operator%s", op_word(part->op)));
        m = class_member(ct, on);
        if (!m) serr(n, "'%s' has no member named '%s'", ty_str(ct), on->s);
    } else {
        if (part->next) {
            /* obj.Base::name */
            Ent *qe = lookup_unqual(scope2_cur, part->name);
            if (qe && qe->kind == EK_TYPEDEF && qe->type->kind == TY_CLASS) qe = qe->type->ent;
            if (!qe || qe->kind != EK_CLASS || !qe->ci || part->next->next) serr(n, "unsupported qualified member name");
            if (!derived_to_base(ct, qe->type) && qe != ct->ent) serr(n, "'%s' is not a base of '%s'", ty_str(qe->type), ty_str(ct));
            m = lookup_in(qe->ci->scope, part->next->name, 0);
            if (!m) serr(n, "'%s' has no member named '%s'", ty_str(qe->type), part->next->name->s);
            qualified = 1;
        } else {
            m = class_member(ct, part->name);
            if (!m) serr(n, "'%s' has no member named '%s'", ty_str(ct), part->name->s);
        }
        if (part->flags & PART_ARGS) m = apply_part_args(m, part);
    }
    check_access(m, m->access, m->owner, n);
    if ((m->kind == EK_FIELD || (funcish_kind(m->kind) && !(m->flags & EF_STATIC))) && m->owner && m->owner != ct->ent) {
        /* a member of a base class: go through the base subobject */
        const char *bp = base_subobject(objc, ct, m->owner->type, n);
        objc = bp;
        this_c = fmt_str("(&%s)", bp);
        objt = ty_cv(m->owner->type, objt->cv);
    }
    switch (m->kind) {
    case EK_FIELD:
        {
            Type *t = m->type;
            if (ty_is_ref(t)) {
                v = val_new(t->base, fmt_str("(*%s.%s)", objc, m->name->s));
                v->lval = 1;
                return v;
            }
            if (objt->cv & CV_CONST) t = t->kind == TY_ARRAY ? ty_array(ty_cv(strip_arrays(t), CV_CONST), t->len) : ty_cv(t, CV_CONST);
            v = val_new(t, fmt_str("(%s.%s)", objc, m->name->s));
            v->lval = n->op == P_ARROW || obj->lval;
            if (!v->lval && obj->is_temp) v->lval = 0;
            return v;
        }
    case EK_FUNC: case EK_FTEMPLATE:
        v = val_new(m->type, m->cname);
        v->fn = m;
        v->lval = 1;
        v->nonvirtual = qualified;
        if (!(m->flags & EF_STATIC)) {
            v->this_c = this_c;
            v->this_t = objt;
        }
        return v;
    case EK_VAR: case EK_ENUMERATOR:
        return ent_value(m, n);
    }
    serr(n, "'%s' is not a data member or member function", m->name->s);
    return 0;
}

/* ------------------------------------------------------------ the dispatcher */

static Val *expr(Node *n) {
    Val *v, *r;
    switch (n->kind) {
    case N_NUM: return number_literal(n);
    case N_CHARLIT: return char_literal(n);
    case N_STRLIT: return string_literal(n);
    case N_BOOLLIT: return const_val(T_BOOL, n->val);
    case N_NULLPTR:
        r = val_new(T_NULLPTR, "((void *)0)");
        r->static_init = 1;
        return r;
    case N_THIS:
        if (!have_this()) serr(n, "'this' is only available in a non-static member function");
        r = val_new(ty_ptr(this_class_type()), cur_this_text());
        return r;
    case N_PAREN:
        v = expr(n->a);
        if (v->lval && !v->fn) {
            r = anew(sizeof(Val));
            *r = *v;
            r->c = fmt_str("(%s)", v->c);
            return r;
        }
        return v;
    case N_ID: return id_expr(n);
    case N_UNARY: return unary_expr(n);
    case N_POSTFIX: return postfix_expr(n);
    case N_BINARY: return binary_expr(n);
    case N_ASSIGN: return assign_expr(n);
    case N_COND: return cond_expr(n);
    case N_COMMA:
        v = expr(n->a);
        r = expr(n->b);
        {
            Val *res = anew(sizeof(Val));
            *res = *r;
            res->c = fmt_str("(%s, %s)", v->c, r->c);
            res->is_const = 0;
            res->static_init = 0;
            return res;
        }
    case N_CALL: return call_expr(n);
    case N_INDEX: return index_expr(n);
    case N_CAST: return cast_value(expr(n->a), typeid_type(n->b), 0, n);
    case N_NAMEDCAST:
        if (n->op == OPKW + KW_dynamic_cast) return dynamic_cast_expr(n);
        return cast_value(expr(n->a), typeid_type(n->b), n->op - OPKW, n);
    case N_FUNCCAST: return funccast_expr(n);
    case N_SIZEOF: return sizeof_expr(n);
    case N_ALIGNOF:
        return const_val(T_ULONG, 1);
    case N_NEW: return new_expr(n);
    case N_DELETE: return delete_expr(n);
    case N_NOEXCEPT_EXPR:
        expr(n->a);
        return const_val(T_BOOL, 0);
    case N_MEMBER: return member_expr(n);
    case N_PTRMEM: serr(n, "pointers to members are not supported yet");
    case N_THROW: return throw_expr(n);
    case N_LAMBDA: return lambda_expr(n);
    case N_PACKEXP:
        {
            /* a pack expansion where one value is wanted (T x(args...) with a pack of one element) */
            Val *pv[2];
            Node *nx = n->next;
            int cnt;
            n->next = 0;
            cnt = expr_args(n, pv, 0, 2);
            n->next = nx;
            if (cnt != 1) serr(n, "the pack expansion has %d elements where one value is needed", cnt);
            return pv[0];
        }
    case N_INITLIST: return braced_val(n);
    case N_TYPEID_EXPR: return typeid_expr(n);
    case N_TYPETRAIT: return typetrait_expr(n);
    }
    serr(n, "unsupported expression (%s)", node_kind_name(n->kind));
    return 0;
}

/* ------------------------------------------------------------ entities and scopes */

static int linkage_c = 0;           /* inside extern "C" */
static int anon_ns_count = 0;

static int node_is_lib(Node *n) {
    return n && n->file >= 0 && n->file < nfiles && files[n->file].is_lib;
}

/* adds an entity to a scope: functions of one name form an overload set */
static void scope_add(Scope2 *sc, Ent *e) {
    Ent *head = sfind(sc, e->name);
    if (!head) {
        sadd_head(sc, e);
        return;
    }
    if (funcish_kind(head->kind) && funcish_kind(e->kind)) {
        Ent *t = head;
        while (t->overload) t = t->overload;
        t->overload = e;
        return;
    }
    sreplace_head(sc, e->name, e);
}

/* the last component of an id */
static Node *id_last(Node *id) {
    Node *p = id->a;
    while (p && p->next) p = p->next;
    return p;
}

/* the scope in which a (possibly qualified) declarator-id declares its name */
static Scope2 *declaration_scope(Node *id) {
    Node *p;
    Scope2 *sc = (id->flags & ID_GLOBAL) ? scope2_global : 0;
    Ent *e;
    if (!id->a || !id->a->next) return (id->flags & ID_GLOBAL) ? scope2_global : scope2_cur;
    for (p = id->a; p->next; p = p->next) {
        e = sc ? lookup_in(sc, p->name, 0) : lookup_unqual(scope2_cur, p->name);
        if (e && (p->flags & PART_ARGS)) e = apply_part_args(e, p);
        if (e && e->kind == EK_TYPEDEF && e->type->kind == TY_CLASS) e = e->type->ent;
        if (e && e->kind == EK_CLASS && e->ci) {
            sc = e->ci->scope;
            continue;
        }
        if (!e || e->kind != EK_NS) serr(p, "'%s' is not a namespace or class", p->name->s);
        sc = e->members;
    }
    return sc;
}

static const char *mangle_function_name(Scope2 *sc, Name *name, Type *ft) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    sb_puts(&sb, sc->prefix);
    sb_puts(&sb, name->s);
    sb_puts(&sb, "__");
    if (ft->nparams == 0) sb_puts(&sb, "v");
    for (i = 0; i < ft->nparams; i++) mangle_type(&sb, ft->params[i]);
    if (ft->variadic) sb_puts(&sb, "z");
    return sb.s;
}

/* ------------------------------------------------------------ enums */

static Ent *declare_enum_node(Node *en) {
    Node *lp = en->a ? id_last(en->a) : 0;
    Name *name = lp ? lp->name : 0;
    Ent *e = 0;
    Type *t;
    Node *c;
    i64 next = 0;
    Scope2 *target = scope2_cur;
    if (name) {
        e = sfind(target, name);
        if (e && e->kind != EK_ENUM) e = 0;
    }
    if (!e) {
        e = ent_new(EK_ENUM, name, 0, target);
        t = anew(sizeof(Type));
        t->kind = TY_ENUM;
        t->ent = e;
        t->len = -1;
        e->type = t;
        e->cname = name ? fmt_str("%s%s", target->prefix, name->s) : "__anon_enum";
        e->members = scope2_new(SS_NS, target);
        e->members->prefix = fmt_str("%s%s_", target->prefix, name ? name->s : "anon");
        if (en->flags & EN_CLASS) e->flags |= EF_SCOPED;
        if (name) scope_add(target, e);
    }
    for (c = en->c; c; c = c->next) {
        Ent *k = ent_new(EK_ENUMERATOR, c->name, e->type, target);
        if (c->a) next = const_expr_value(c->a);
        k->cv = next++;
        k->has_val = 1;
        k->cname = fmt_str("%s%s", e->members->prefix, c->name->s);
        scope_add(e->members, k);
        if (!(en->flags & EN_CLASS)) {
            Ent *k2 = anew(sizeof(Ent));
            *k2 = *k;
            scope_add(target, k2);
        }
    }
    return e;
}

static void define_class(Ent *cls, Node *cn);
static int anon_class_count = 0;
static void emit_line_none(void) { cur_line_file = -1; }

static Ent *declare_class_node(Node *cn) {
    Node *lp = cn->a ? id_last(cn->a) : 0;
    Name *name = lp ? lp->name : 0;
    Ent *e = 0;
    Type *t;
    Scope2 *target = scope2_cur;
    if (cn->a && cn->a->a->next) {
        e = resolve_id(cn->a, 1);
        if (e->kind == EK_TYPEDEF && e->type->kind == TY_CLASS) e = e->type->ent;
        if (e->kind != EK_CLASS) serr(cn, "'%s' is not a class", name->s);
    } else if (name) {
        e = sfind(target, name);
        if (!e && !(cn->flags & CL_DEFINITION)) e = lookup_unqual(scope2_cur, name);
        if (e && e->kind == EK_TYPEDEF && e->type->kind == TY_CLASS) e = e->type->ent;
        if (e && e->kind != EK_CLASS) e = 0;
    }
    if (!e) {
        if (!name) {
            name = intern_str(fmt_str("__anon_class%d", anon_class_count++));
        }
        e = ent_new(EK_CLASS, name, 0, target);
        t = anew(sizeof(Type));
        t->kind = TY_CLASS;
        t->ent = e;
        t->len = -1;
        e->type = t;
        e->cname = fmt_str("%s%s", target->prefix, name->s);
        if (node_is_lib(cn) && linkage_c > 0) e->flags |= EF_LIB;
        if (cn->op == OPKW + KW_union) e->flags |= EF_UNION;
        scope_add(target, e);
        if (!(e->flags & EF_LIB)) emitf("%s;\n", class_tag(e));
    }
    if (cn->flags & CL_DEFINITION) define_class(e, cn);
    return e;
}

/* ------------------------------------------------------------ initializers */

static Type *subst_auto(Type *t, Type *rep) {
    switch (t->kind) {
    case TY_AUTO: return ty_cv(rep, t->cv);
    case TY_PTR: return ty_cv(ty_ptr(subst_auto(t->base, rep)), t->cv);
    case TY_REF: return ty_ref(subst_auto(t->base, rep));
    case TY_RREF: return ty_rref(subst_auto(t->base, rep));
    case TY_ARRAY: return ty_array(subst_auto(t->base, rep), t->len);
    }
    return t;
}

static int contains_auto(Type *t) {
    switch (t->kind) {
    case TY_AUTO: return 1;
    case TY_PTR: case TY_REF: case TY_RREF: case TY_ARRAY: return contains_auto(t->base);
    }
    return 0;
}

/* the type that 'auto' (inside the declared type) stands for, from the initializer value v */
static Type *deduce_auto(Type *declared, Val *v, Node *at) {
    Type *src = v->t, *rep;
    Type *probe = declared;
    int via_ref = 0;
    if (v->fn) {
        if (v->fn->overload) serr(at, "cannot deduce the type from an overloaded function");
        src = ty_ptr(v->fn->type);
    }
    if (ty_is_ref(probe)) {
        via_ref = 1;
        probe = probe->base;
    }
    if (via_ref) {
        if (probe->kind == TY_AUTO) {
            rep = src;
            if (probe->cv & CV_CONST) rep = ty_unqual(src);
            if (!v->lval && !(probe->cv & CV_CONST) && declared->kind != TY_RREF)
                serr(at, "cannot bind a non-const reference to an rvalue");
            return subst_auto(declared, rep);
        }
        rep = ty_unqual(src->kind == TY_ARRAY ? src : src);
        if (probe->kind == TY_PTR) return subst_auto(declared, src->kind == TY_PTR ? src->base : src);
        return subst_auto(declared, rep);
    }
    /* by value: references, top-level cv, arrays and functions decay */
    if (src->kind == TY_ARRAY) src = ty_ptr(src->base);
    else if (src->kind == TY_FUNC) src = ty_ptr(src);
    src = ty_unqual(src);
    if (probe->kind == TY_PTR) {
        if (src->kind != TY_PTR) serr(at, "cannot deduce 'auto *' from '%s'", ty_str(v->t));
        return subst_auto(probe, src->base);
    }
    return subst_auto(probe, src);
}

/* the C text of a constant-expression-or-not initializer for an object of type t.
   *static_ok = 0 if the text is not valid in a static initializer. */
static const char *init_text(Type *t, Node *init, int form, Node *at, int *static_ok);

static const char *init_array_text(Type *t, Node *list, Node *at, int *static_ok) {
    StrBuf sb;
    Node *e;
    i64 count = 0;
    sb_init(&sb);
    sb_puts(&sb, "{");
    for (e = list; e; e = e->next) {
        const char *s;
        if (t->len >= 0 && count >= t->len) serr(e, "too many initializers for '%s'", ty_str(t));
        if (count) sb_puts(&sb, ", ");
        s = init_text(t->base, e, e->kind == N_INITLIST ? 3 : 1, e, static_ok);
        sb_puts(&sb, s);
        count++;
    }
    sb_puts(&sb, "}");
    if (t->len < 0) t->len = count;
    return sb.s;
}

static const char *init_text(Type *t, Node *init, int form, Node *at, int *static_ok) {
    Val *v;
    Node *e = init;
    if (init && init->kind == N_INITLIST && form != 3) form = 3;
    if (t->kind == TY_ARRAY) {
        if (!init) return 0;
        if (e->kind == N_STRLIT || (e->kind == N_INITLIST && e->a && !e->a->next && e->a->kind == N_STRLIT && 0)) {
            Val *sv = string_literal(e);
            Type *ct = ty_unqual(t->base);
            if (!is_char_kind(ct))
                serr(at, "an array of '%s' cannot be initialized with a string literal", ty_str(t->base));
            if (t->len < 0) t->len = sv->t->len;
            else if (sv->t->len - 1 > t->len) serr(at, "the initializer-string for the array is too long");
            return sv->c;
        }
        if (e->kind != N_INITLIST) serr(at, "an array must be initialized with a brace-enclosed list");
        return init_array_text(t, e->a, at, static_ok);
    }
    if (form == 3 && e->kind == N_INITLIST) {
        /* T x{v} / T x = {v} for a scalar */
        if (!e->a) {
            if (ty_is_float(t)) return "0.0";
            if (t->kind == TY_PTR) return "((void *)0)";
            return "0";
        }
        if (e->a->next) serr(at, "too many initializers for '%s'", ty_str(t));
        e = e->a;
        if (e->kind == N_INITLIST) serr(e, "too many braces around the scalar initializer");
    }
    if (ty_is_ref(t)) {
        Val *rv = expr(e);
        if (!rv->lval && !((t->base->cv & CV_CONST) || t->kind == TY_RREF)) serr(at, "cannot bind a non-const reference to an rvalue");
        {
            const char *s = bind_argument(rv, t, at);
            *static_ok = 0;
            if (rv->var && rv->var->scope && rv->var->scope->kind != SS_BLOCK && !rv->is_ref_var && (rv->lval)) *static_ok = *static_ok || 1;
            return s;
        }
    }
    v = expr(e);
    v = convert(v, t, at);
    if (!v->static_init) *static_ok = 0;
    return v->c;
}

/* ------------------------------------------------------------ declarations */


/* the parameter names of a function declarator: declares them in the current scope */
static void declare_params(Node *fn, Type *ft) {
    Node *p;
    int i = 0;
    for (p = fn ? fn->b : 0; p; p = p->next, i++) {
        Node *pid = 0;
        Type *pt;
        Ent *e;
        if (i >= ft->nparams) break;
        pt = ft->params[i];
        if (p->b) apply_declarator(decl_specs_type(p->a, 0), p->b, &pid, 0);
        if (pid && pid->a) {
            e = ent_new(EK_VAR, pid->a->name, pt, scope2_cur);
            e->cname = pid->a->name->s;
            e->flags |= EF_PARAM;
            if (pt->kind == TY_CLASS) e->flags |= EF_INDIRECT;
            scope_add(scope2_cur, e);
            if (p->val == 1 && p->name) {
                /* an element of a function parameter pack */
                Ent *pk = lookup_unqual(scope2_cur, p->name);
                if (pk && pk->kind == EK_PACK) pack_append(pk, e);
            }
        }
    }
}

/* the C parameter list of a function: the result object of a class result, this, the parameters */
static const char *c_params(Ent *e, Node *fn, Type *ft, int named) {
    StrBuf sb;
    Node *p;
    int i = 0, first = 1;
    sb_init(&sb);
    if (ft->base->kind == TY_CLASS && !(e->flags & EF_CTOR)) {
        sb_puts(&sb, c_decl(ty_ptr(ty_unqual(ft->base)), named ? "__ret" : ""));
        first = 0;
    }
    if (is_nonstatic_member(e)) {
        if (!first) sb_puts(&sb, ", ");
        sb_puts(&sb, c_decl(ty_ptr(ty_unqual(e->owner->type)), named ? "this" : ""));
        first = 0;
    }
    p = fn ? fn->b : 0;
    for (i = 0; i < ft->nparams; i++) {
        const char *name = "";
        if (named) {
            Node *pid = 0;
            if (p && p->b) apply_declarator(decl_specs_type(p->a, 0), p->b, &pid, 0);
            name = pid && pid->a ? pid->a->name->s : fmt_str("__p%d", i);
        }
        if (!first) sb_puts(&sb, ", ");
        first = 0;
        sb_puts(&sb, c_param_type_text(ft->params[i], name));
        if (p) p = p->next;
    }
    if (ft->variadic) {
        if (!first) sb_puts(&sb, ", ");
        sb_puts(&sb, "...");
        first = 0;
    }
    if (first) sb_puts(&sb, "void");
    return sb.s;
}

/* functions that every translation unit that uses them defines (inline functions, members defined in
   their class, template instances, generated members) have internal linkage in C */
static int fn_is_local(Ent *e) {
    if (e->flags & EF_CLINK) return 0;
    if ((e->flags & EF_STATIC) && !(e->flags & EF_MEMBER)) return 1;
    if (e->flags & (EF_TINST | EF_IMPLICIT | EF_INLINE | EF_CONSTEXPR | EF_INCLASS | EF_DEFAULTED)) return 1;
    if (e->lazy) return 1;
    if (e->owner && (e->owner->inst || (e->owner->ci && e->owner->ci->is_lambda))) return 1;
    if (e->scope && e->scope->kind == SS_TMPL) return 1;
    return 0;
}

static const char *c_function_decl(Ent *e, Node *fn, Type *ft) {
    if (fn_is_local(e)) return fmt_str("static %s", c_decl(c_return_type(e, ft), fmt_str("%s(%s)", e->cname, c_params(e, fn, ft, 1))));
    return c_decl(c_return_type(e, ft), fmt_str("%s(%s)", e->cname, c_params(e, fn, ft, 1)));
}

static const char *op_word(int op);

/* the entity name of a function declarator-id; sets EF_ flags for special member functions */
static Name *function_entity_name(Node *at, Node *id, Ent *cls, int *eflags, Type **conv_type) {
    Node *lp = id_last(id);
    *conv_type = 0;
    if ((lp->flags & 7) == PART_DTOR) {
        *eflags |= EF_DTOR;
        return nm_dtor;
    }
    if ((lp->flags & 7) == PART_CONV) {
        *eflags |= EF_CONV;
        *conv_type = apply_declarator(decl_specs_type(lp->a->a, 0), lp->a->b, 0, 0);
        return intern_str("operator conv");
    }
    if ((lp->flags & 7) == PART_OPERATOR) return intern_str(fmt_str("operator%s", op_word(lp->op)));
    if ((lp->flags & 7) == PART_LITERAL) serr(at, "user-defined literals are not supported yet");
    if (cls && lp->name == cls->name) {
        *eflags |= EF_CTOR;
        return nm_ctor;
    }
    return lp->name;
}

static const char *entity_c_word(Name *name);

static const char *sig_text(Type *ft) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    if (ft->nparams == 0) sb_puts(&sb, "v");
    for (i = 0; i < ft->nparams; i++) mangle_type(&sb, ft->params[i]);
    if (ft->variadic) sb_puts(&sb, "z");
    return sb.s;
}

static const char *function_cname(Scope2 *sc, Ent *cls, Name *name, int eflags, Type *ft, int is_const) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    sb_puts(&sb, sc->prefix);
    if (eflags & EF_DTOR) {
        sb_puts(&sb, "dtor");
        return sb.s;
    }
    if (eflags & EF_CTOR) sb_puts(&sb, "ctor");
    else if (eflags & EF_CONV) {
        sb_puts(&sb, "conv_");
        mangle_type(&sb, ft->base);
        return sb.s;
    } else if (strncmp(name->s, "operator", 8) == 0) sb_printf(&sb, "op_%s", entity_c_word(name));
    else sb_puts(&sb, name->s);
    sb_puts(&sb, "__");
    if (ft->nparams == 0) sb_puts(&sb, "v");
    for (i = 0; i < ft->nparams; i++) mangle_type(&sb, ft->params[i]);
    if (ft->variadic) sb_puts(&sb, "z");
    if (is_const) sb_puts(&sb, "K");
    (void)cls;
    return sb.s;
}

static const char *op_word_names[] = {
    "plus", "minus", "mul", "div", "mod", "xor", "and", "or", "compl", "not", "assign", "lt", "gt",
    "addeq", "subeq", "muleq", "diveq", "modeq", "xoreq", "andeq", "oreq", "shl", "shr", "shleq", "shreq",
    "eq", "ne", "le", "ge", "andand", "oror", "inc", "dec", "comma", "arrowstar", "arrow", "call", "index", 0
};
static const int op_word_ids[] = {
    P_PLUS, P_MINUS, P_STAR, P_SLASH, P_PERCENT, P_CARET, P_AMP, P_PIPE, P_TILDE, P_NOT, P_ASSIGN, P_LT, P_GT,
    P_ADDEQ, P_SUBEQ, P_MULEQ, P_DIVEQ, P_MODEQ, P_XOREQ, P_ANDEQ, P_OREQ, P_SHL, P_SHR, P_SHLEQ, P_SHREQ,
    P_EQ, P_NE, P_LE, P_GE, P_ANDAND, P_OROR, P_INC, P_DEC, P_COMMA, P_ARROWSTAR, P_ARROW, P_LPAREN, P_LBRACK
};

/* the word of an operator for C names and entity names: "operator+" is declared as "operator" + spelling;
   the spelling of the operator is kept in the entity name, the word is used in C names */
static const char *op_word(int op) {
    if (op >= OPKW) return op - OPKW == KW_new ? " new" : " delete";
    if (op == P_LPAREN) return "()";
    if (op == P_LBRACK) return "[]";
    return punct_spelling(op);
}

static const char *op_cword(const char *spelling) {
    int i;
    for (i = 0; op_word_names[i]; i++)
        if (strcmp(punct_spelling(op_word_ids[i]), spelling) == 0 || (op_word_ids[i] == P_LPAREN && strcmp(spelling, "()") == 0)
            || (op_word_ids[i] == P_LBRACK && strcmp(spelling, "[]") == 0))
            return op_word_names[i];
    return "op";
}

/* the conversion of an entity name "operator+" to the word used in a C name */
static const char *entity_c_word(Name *name) {
    if (strncmp(name->s, "operator", 8) == 0) return op_cword(name->s + 8);
    return name->s;
}

static Ent *find_member_overload(Ent *head, Type *ft, int is_const, int eflags) {
    Ent *f;
    for (f = head; f; f = f->overload) {
        if (f->kind != EK_FUNC) continue;
        if ((eflags & EF_CONV) && !(f->flags & EF_CONV)) continue;
        if ((eflags & EF_CONV)) {
            if (ty_equal(f->type->base, ft->base) && (!!(f->flags & EF_CONST_METHOD)) == is_const) return f;
            continue;
        }
        if ((!!(f->flags & EF_CONST_METHOD)) != is_const && (f->flags & EF_MEMBER)) continue;
        if (find_function(f, ft) == f) return f;
    }
    return 0;
}

static Ent *declare_function(Node *at, Node *id, Type *ft, int dsflags, Node *fnode) {
    Node *lp = id_last(id);
    Scope2 *sc = declaration_scope(id);
    Ent *e, *head, *cls = sc->kind == SS_CLASS ? sc->classent : 0;
    int i, eflags = 0, is_c = linkage_c > 0 && !cls;
    int is_const = fnode && (fnode->flags & FQ_CONST);
    Type *conv = 0;
    Name *ename = function_entity_name(at, id, cls, &eflags, &conv);
    if (conv) ft = ty_func(conv, ft->params, ft->nparams, ft->variadic);
    if ((eflags & (EF_CTOR | EF_DTOR | EF_CONV)) && !cls) serr(at, "'%s' is not a member function", lp->name ? lp->name->s : "operator");
    if (!cls && (lp->flags & 7) != PART_OPERATOR && (lp->flags & 7) != PART_NAME) serr(at, "unsupported function name");
    head = sfind(sc, ename);
    if (head && !funcish_kind(head->kind)) serr(at, "'%s' redeclared as a different kind of symbol", ename->s);
    e = cls ? find_member_overload(head, ft, is_const, eflags) : (head ? find_function(head, ft) : 0);
    if (e) {
        if (!(eflags & (EF_CTOR | EF_DTOR)) && !ty_equal(e->type->base, ft->base)) serr(at, "conflicting return type for '%s'", ename->s);
    } else {
        e = ent_new(EK_FUNC, ename, ft, sc);
        e->flags |= eflags;
        if (cls) {
            e->flags |= EF_MEMBER;
            e->owner = cls;
            if (is_const) e->flags |= EF_CONST_METHOD;
            if (dsflags & DS_EXPLICIT) e->flags |= EF_EXPLICIT;
            if (dsflags & DS_VIRTUAL) e->flags |= EF_VIRTUAL;
            if (fnode && (fnode->flags & FQ_DELETE)) e->flags |= EF_DELETED;
            if (fnode && (fnode->flags & FQ_DEFAULT)) e->flags |= EF_DEFAULTED;
            e->cname = function_cname(sc, cls, ename, eflags, ft, is_const);
            if (eflags & (EF_CTOR | EF_CONV | EF_DTOR)) { /* names come from function_cname */ }
        } else if (strncmp(ename->s, "operator", 8) == 0) {
            e->cname = fmt_str("%sop_%s__%s", sc->prefix, entity_c_word(ename), sig_text(ft));
        } else if (is_c) e->cname = lp->name->s;
        else e->cname = mangle_function_name(sc, ename, ft);
        if (!cls && sc == scope2_global && strcmp(ename->s, "main") == 0) e->cname = "__cppc_main";
        if (is_c) e->flags |= EF_CLINK;
        if (is_c && node_is_lib(at)) e->flags |= EF_LIB;
        if (dsflags & DS_STATIC) e->flags |= EF_STATIC;
        if (dsflags & DS_CONSTEXPR) e->flags |= EF_CONSTEXPR;
        if (dsflags & DS_INLINE) e->flags |= EF_INLINE;
        if (in_class_def) e->flags |= EF_INCLASS;
        e->defaults = anew(sizeof(Node *) * (ft->nparams + 1));
        e->file = at->file;
        e->line = at->line;
        scope_add(sc, e);
        if (cls && cls->ci) {
            ClassInfo *ci = cls->ci;
            ci->methods = xrealloc(ci->methods, sizeof(Ent *) * (ci->nmethods + 1));
            ci->methods[ci->nmethods++] = e;
        }
        if (fnode && (fnode->flags & FQ_PURE)) e->flags |= EF_PURE;
        if (fnode && (fnode->flags & FQ_OVERRIDE)) e->flags |= EF_OVERRIDE;
        if (fnode && (fnode->flags & FQ_FINAL)) e->flags |= EF_FINAL;
    }
    /* default arguments */
    if (fnode) {
        Node *p;
        for (p = fnode->b, i = 0; p && i < ft->nparams; p = p->next, i++) {
            if (p->c) {
                if (e->defaults[i]) serr(p, "default argument given for parameter %d of '%s' after a previous declaration", i + 1, ename->s);
                e->defaults[i] = p->c;
            }
        }
        for (i = 0; i < ft->nparams; i++) {
            int j;
            if (!e->defaults[i]) continue;
            for (j = i + 1; j < ft->nparams; j++)
                if (!e->defaults[j]) serr(at, "default argument missing for parameter %d of '%s'", j + 1, ename->s);
        }
    }
    return e;
}

static void emit_prototype(Ent *e, Node *fnode, int dsflags) {
    if (contains_auto(e->type->base)) return;           /* a deduced return type: written with the definition */
    if (e->flags & EF_LIB) return;
    if (e->flags & EF_DELETED) return;
    emitf("%s;\n", c_function_decl(e, fnode, e->type));
}

static void stmt(Node *n);
static void range_for_class(Node *n, Val *rng);
static void binding_decl(Node *b);

/* the N_ID of the name that a declarator declares */
static Node *declarator_name_id_of(Node *d) {
    while (d) {
        if (d->kind == N_D_NAME) return d->a;
        d = d->a;
    }
    return 0;
}

/* ------------------------------------------------------------ construction of objects */

static Val *expr_hinted(Node *e, const char *dest, Type *t);
static void emit_scope_dtors_text(Scope2 *sc);

/* statements that initialize the object of type t at the pointer text dest:
   form 0 = copy-init from expression init, 1 = braced list, 2 = parenthesized list, 3 = default */
static void object_init_stmts(Type *t, const char *dest, Node *init, int form, Node *at);

static void flush_temps_stmt(int mark) {
    const char *d = dtor_text_from(mark);
    if (d[0]) emit(d);
}

/* a character type: an array of it may be initialized with a string literal */
static int is_char_kind(Type *t) {
    Type *ct = ty_unqual(t);
    return ct->kind == TY_CHAR || ct->kind == TY_SCHAR || ct->kind == TY_UCHAR || ct->kind == TY_WCHAR
        || ct->kind == TY_CHAR16 || ct->kind == TY_CHAR32;
}

/* aggregate initialization of the class object at dest from a braced list */
static void aggregate_init_stmts(Type *cls, const char *dest, Node *list, Node *at) {
    ClassInfo *ci = cinfo(cls);
    Field *f;
    Node *e = list;
    for (f = ci->fields; f; f = f->next) {
        const char *fd = fmt_str("(&(*%s).%s)", dest, f->name->s);
        if (e) {
            if (f->type->kind == TY_ARRAY) {
                Node *el;
                i64 i = 0;
                if (e->kind == N_INITLIST) {
                    for (el = e->a; el; el = el->next, i++) {
                        if (f->type->len >= 0 && i >= f->type->len) serr(el, "too many initializers for '%s'", ty_str(f->type));
                        object_init_stmts(f->type->base, fmt_str("(&(*%s).%s[%lld])", dest, f->name->s, i), el, el->kind == N_INITLIST ? 1 : 0, at);
                    }
                    for (; f->type->len >= 0 && i < f->type->len; i++)
                        if (!is_class_type(f->type->base)) emitf("(*%s).%s[%lld] = 0;\n", dest, f->name->s, i);
                } else if (e->kind == N_STRLIT && is_char_kind(f->type->base)) {
                    /* a char array member from a string literal; for other element types (const char *)
                       the literal is the first element of an elided brace list */
                    Val *sv = string_literal(e);
                    emitf("memcpy((*%s).%s, %s, %lld * sizeof(char));\n", dest, f->name->s, sv->c, sv->t->len);
                } else {
                    /* brace elision: the elements of the array are the next initializers of the list */
                    i64 k = 0;
                    for (; e && (f->type->len < 0 || k < f->type->len); k++, e = e->next)
                        object_init_stmts(f->type->base, fmt_str("(&(*%s).%s[%lld])", dest, f->name->s, k), e, e->kind == N_INITLIST ? 1 : 0, at);
                    for (; f->type->len >= 0 && k < f->type->len; k++)
                        if (!is_class_type(f->type->base)) emitf("(*%s).%s[%lld] = 0;\n", dest, f->name->s, k);
                    continue;
                }
            } else object_init_stmts(f->type, fd, e, e->kind == N_INITLIST ? 1 : 0, at);
            e = e->next;
        } else if (f->init) {
            object_init_stmts(f->type, fd, f->init, f->init_form == 1 ? 1 : (f->init_form == 2 ? 2 : 0), at);
        } else if (f->type->kind == TY_CLASS) {
            object_init_stmts(f->type, fd, 0, 3, at);
        } else if (f->type->kind == TY_ARRAY) {
            if (!is_class_type(strip_arrays(f->type)) && f->type->len >= 0) {
                i64 i;
                for (i = 0; i < f->type->len; i++) emitf("(*%s).%s[%lld] = 0;\n", dest, f->name->s, i);
            }
        } else if (ty_is_ref(f->type)) {
            serr(at, "the reference member '%s' must be initialized", f->name->s);
        } else {
            emitf("(*%s).%s = %s;\n", dest, f->name->s, ty_is_float(f->type) ? "0.0" : (f->type->kind == TY_PTR ? "((void *)0)" : "0"));
        }
    }
    if (e) serr(e, "too many initializers for '%s'", ty_str(cls));
}

static void object_init_stmts(Type *t, const char *dest, Node *init, int form, Node *at) {
    int mark = nftemps;
    Type *tu = ty_unqual(t);
    if (tu->kind == TY_ARRAY) serr(at, "arrays of objects are initialized element by element (not supported here)");
    if (tu->kind != TY_CLASS) {
        /* a scalar member or element: *dest = value */
        Val *v;
        Node *e = init;
        if (form == 3 || !init) {
            emitf("*%s = %s;\n", dest, ty_is_float(tu) ? "0.0" : (tu->kind == TY_PTR ? "((void *)0)" : "0"));
            return;
        }
        if (form == 1) {
            if (!init->a) {
                emitf("*%s = %s;\n", dest, ty_is_float(tu) ? "0.0" : (tu->kind == TY_PTR ? "((void *)0)" : "0"));
                return;
            }
            if (init->a->next) serr(at, "too many initializers for '%s'", ty_str(tu));
            e = init->a;
        } else if (form == 2) {
            if (!init || init->next) serr(at, "unsupported initializer for '%s'", ty_str(tu));
        }
        if (ty_is_ref(t)) {
            v = expr(e);
            emitf("*%s = %s;\n", dest, bind_argument(v, t, at));
        } else {
            v = convert(expr(e), tu, at);
            emitf("*%s = %s;\n", dest, v->c);
        }
        flush_temps_stmt(mark);
        return;
    }
    require_complete(tu, at);
    if (form == 3) {
        const char *text = default_construct_text(tu, dest, at);
        if (text) emitf("%s;\n", text);
        return;
    }
    if (form == 1 && is_init_list_type(tu)) {
        emitf("%s;\n", list_init_text(tu, dest, braced_val(init), at));
        flush_temps_stmt(mark);
        return;
    }
    if (form == 1 && cinfo(tu)->aggregate && !class_ctors(tu)) {
        aggregate_init_stmts(tu, dest, init->a, at);
        flush_temps_stmt(mark);
        return;
    }
    if (form == 0) {
        Val *v = expr_hinted(init, dest, tu);
        if (dest_used) {
            emitf("%s;\n", v->c);
        } else if (v->t->kind == TY_CLASS && ty_same(v->t, tu)) {
            const char *text = copy_into(tu, dest, v, at);
            if (text) emitf("%s;\n", text);
        } else if (v->t->kind == TY_CLASS && derived_to_base(v->t, tu)) {
            /* initialization from a derived object: the base part is copied */
            Val *bv = upcast_value(v, tu, at);
            const char *text;
            bv->is_temp = 0;
            bv->lval = 1;
            text = copy_into(tu, dest, bv, at);
            if (text) emitf("%s;\n", text);
        } else if (v->t->kind != TY_CLASS) {
            /* a converting constructor builds the object in place */
            Val *args[1];
            Ent *set = class_ctors(tu), *f;
            const char *text;
            args[0] = v;
            for (f = set; f; f = f->overload)
                (void)f;
            if (class_conversion(v, tu) == CONV_NONE)
                serr(at, "cannot convert '%s' to '%s' in copy-initialization", ty_str(v->t), ty_str(tu));
            text = ctor_call(tu, dest, args, 1, at);
            if (text) emitf("%s;\n", text);
        } else {
            Val *cv = class_prvalue(v, tu, at);
            const char *text = copy_into(tu, dest, cv, at);
            if (text) emitf("%s;\n", text);
        }
        dest_used = 0;
        flush_temps_stmt(mark);
        return;
    }
    {
        /* direct-initialization (parentheses) or list-initialization of a class with constructors */
        Val *args[MAXARGS];
        int n = 0;
        Node *a = form == 1 ? init->a : init, *first = a;
        const char *text;
        for (; a; a = a->next) {
            if (n >= MAXARGS) serr(at, "too many arguments");
            if (a->kind == N_PACKEXP) {
                n = expr_args(a, args, n, MAXARGS);        /* the rest of the list */
                break;
            }
            {
                Val *av = (n == 0 && !first->next) ? expr_hinted(a, dest, tu) : expr(a);
                args[n++] = av;
            }
        }
        if (n == 1 && dest_used) {
            emitf("%s;\n", args[0]->c);
            dest_used = 0;
            flush_temps_stmt(mark);
            return;
        }
        brace_init = form == 1;
        text = ctor_call(tu, dest, args, n, at);
        brace_init = 0;
        if (text) emitf("%s;\n", text);
        flush_temps_stmt(mark);
    }
}

/* the expression e analysed with the hint that a class prvalue of type t may be built at dest */
static Val *expr_hinted(Node *e, const char *dest, Type *t) {
    Val *v;
    Node *target = e;
    while (target->kind == N_PAREN) target = target->a;
    dest_hint = dest;
    dest_node = target;
    dest_type = t;
    dest_used = 0;
    v = expr(e);
    dest_hint = 0;
    return v;
}

/* the member f of *this: initialization from a mem-initializer or the default member initializer */
static void init_member(Field *f, Node *mi, Node *at) {
    const char *dest = fmt_str("(&this->%s)", f->name->s);
    int mark = nftemps;
    Type *t = f->type;
    if (ty_is_ref(t)) {
        Val *v;
        if (!mi || !mi->b) {
            if (f->init) {
                v = expr(f->init);
                emitf("this->%s = %s;\n", f->name->s, bind_argument(v, t, at));
                return;
            }
            serr(at, "the reference member '%s' is not initialized", f->name->s);
        }
        v = expr(mi->b);
        emitf("this->%s = %s;\n", f->name->s, bind_argument(v, t, at));
        flush_temps_stmt(mark);
        return;
    }
    if (t->kind == TY_ARRAY) {
        Type *el = strip_arrays(t);
        if (mi && mi->b) {
            /* items{a, b, c}: the elements of a one-dimensional array */
            Node *item = (mi->flags & 1) && mi->b->kind == N_INITLIST ? mi->b->a : mi->b;
            i64 i = 0;
            if (t->base->kind == TY_ARRAY) serr(mi, "initializing multi-dimensional array members from a mem-initializer is not supported yet");
            for (; item; item = item->next, i++) {
                if (t->len >= 0 && i >= t->len) serr(item, "too many initializers for '%s'", ty_str(t));
                object_init_stmts(el, fmt_str("(&((%s)this->%s)[%lld])", c_decl(ty_ptr(el), ""), f->name->s, i), item, item->kind == N_INITLIST ? 1 : 0, mi);
            }
            if (t->len >= 0 && i < t->len && type_needs_ctor(el)) {
                const char *text = default_construct_text(el, fmt_str("(&((%s)this->%s)[__i])", c_decl(ty_ptr(el), ""), f->name->s), at);
                emitf("{ long __i; for (__i = %lld; __i < %lld; __i++) %s; }\n", i, t->len, text ? text : "(void)0");
            }
            return;
        }
        if (el->kind == TY_CLASS && type_needs_ctor(el)) {
            i64 n = 1;
            Type *x = t;
            const char *text;
            while (x->kind == TY_ARRAY) { n *= x->len; x = x->base; }
            emitf("{ long __i; for (__i = 0; __i < %lld; __i++) ", n);
            text = default_construct_text(el, fmt_str("(&((%s)this->%s)[__i])", c_decl(ty_ptr(el), ""), f->name->s), at);
            emitf("%s; }\n", text ? text : "(void)0");
        }
        return;
    }
    if (mi) {
        int form = (mi->flags & 1) ? 1 : 2;
        Node *list = mi->b;
        if (form == 1) {
            Node *l;
            if (list && list->kind == N_INITLIST && !list->next) l = list;
            else {
                l = anew(sizeof(Node));
                l->kind = N_INITLIST;
                l->file = mi->file;
                l->line = mi->line;
                l->a = list;
            }
            object_init_stmts(t, dest, l, 1, mi);
        } else if (t->kind != TY_CLASS && !list) object_init_stmts(t, dest, 0, 3, mi);
        else if (t->kind != TY_CLASS && list && !list->next) object_init_stmts(t, dest, list, 0, mi);
        else object_init_stmts(t, dest, list, list ? 2 : 3, mi);
        return;
    }
    if (f->init) {
        Node *e = f->init;
        if (f->init_form == 1) {
            Node *l = e;
            object_init_stmts(t, dest, l, 1, at);
        } else if (f->init_form == 2) object_init_stmts(t, dest, e, 2, at);
        else object_init_stmts(t, dest, e, 0, at);
        return;
    }
    if (t->kind == TY_CLASS) object_init_stmts(t, dest, 0, 3, at);
}

/* the mem-initializer of the member named n in a constructor's initializer list */
static Node *find_mem_init(Node *list, Name *n) {
    for (; list; list = list->next) {
        Node *lp = list->a ? id_last(list->a) : 0;
        if (lp && lp->name == n && !list->a->a->next) return list;
    }
    return 0;
}

/* the mem-initializer of a constructor that names the base class b */
static Node *find_base_init(Node *list, Ent *b) {
    for (; list; list = list->next) {
        Ent *e;
        if (!list->a) continue;
        e = resolve_id(list->a, 0);
        if (e && e->kind == EK_TYPEDEF && e->type->kind == TY_CLASS) e = e->type->ent;
        if (e == b) return list;
    }
    return 0;
}

static void emit_ctor_prologue(Ent *e, Node *fd) {
    ClassInfo *ci = cinfo(e->owner->type);
    Field *f;
    Node *mi;
    int i, pushed = 0;
    cur_ctor_pushed = 0;
    if (fd && fd->c && !fd->c->next) {
        /* a delegating constructor: the only mem-initializer names the class itself */
        Ent *te = resolve_id(fd->c->a, 0);
        if (te && te->kind == EK_TYPEDEF && te->type->kind == TY_CLASS) te = te->type->ent;
        if (te == e->owner) {
            Val *dargs[MAXARGS];
            int dn;
            const char *text;
            Node *list = fd->c->b;
            if (list && list->kind == N_INITLIST) list = list->a;
            dn = expr_args(list, dargs, 0, MAXARGS);
            text = ctor_call(e->owner->type, "this", dargs, dn, fd->c);
            if (text) emitf("%s;\n", text);
            return;
        }
    }
    for (i = 0; i < ci->nbases; i++) {
        BaseSpec *b = &ci->bases[i];
        const char *dest = fmt_str("(&this->%s)", b->fname);
        mi = find_base_init(fd ? fd->c : 0, b->cls);
        if (mi) {
            if (mi->flags & 1) {
                Node *l;
                if (mi->b && mi->b->kind == N_INITLIST && !mi->b->next) l = mi->b;
                else {
                    l = anew(sizeof(Node));
                    l->kind = N_INITLIST;
                    l->file = mi->file;
                    l->line = mi->line;
                    l->a = mi->b;
                }
                object_init_stmts(b->cls->type, dest, l, 1, mi);
            } else object_init_stmts(b->cls->type, dest, mi->b, mi->b ? 2 : 3, mi);
        } else object_init_stmts(b->cls->type, dest, 0, 3, fd);
        if (type_needs_dtor(b->cls->type)) {
            emit_push_object(dest, b->cls->type);
            pushed++;
        }
    }
    if (ci->poly) emit_set_vptrs(e->owner);
    if (ci->key != KW_union) {
        for (f = ci->fields; f; f = f->next) {
            init_member(f, find_mem_init(fd ? fd->c : 0, f->name), fd);
            if (!ty_is_ref(f->type) && type_needs_dtor(f->type)) {
                if (f->type->kind == TY_ARRAY) emit_push_object(fmt_str("this->%s", f->name->s), f->type);
                else emit_push_object(fmt_str("(&this->%s)", f->name->s), f->type);
                pushed++;
            }
        }
    }
    cur_ctor_pushed = pushed;
    if (fd) {
        for (mi = fd->c; mi; mi = mi->next) {
            Node *lp = id_last(mi->a);
            int found = 0;
            for (f = ci->fields; f; f = f->next) if (f->name == lp->name) found = 1;
            if (!found) {
                Ent *be = resolve_id(mi->a, 0);
                for (i = 0; i < ci->nbases; i++) if (be == ci->bases[i].cls) found = 1;
            }
            if (!found) serr(mi, "'%s' is not a nonstatic data member or base class of '%s'", lp->name->s, ent_qualified_name(e->owner));
        }
    }
}

/* the destructor calls of the data members, in reverse order */
static void emit_member_dtors(Ent *e) {
    ClassInfo *ci = cinfo(e->owner->type);
    Field *list[256];
    int n = 0, i;
    Field *f;
    for (f = ci->fields; f && n < 256; f = f->next) list[n++] = f;
    if (ci->key == KW_union) return;
    for (i = n - 1; i >= 0; i--) {
        Type *t = list[i]->type;
        Type *el = strip_arrays(t);
        if (!type_needs_dtor(el)) continue;
        if (t->kind == TY_ARRAY) {
            i64 cnt = 1;
            Type *x = t;
            while (x->kind == TY_ARRAY) { cnt *= x->len; x = x->base; }
            emitf("{ long __i; for (__i = %lld - 1; __i >= 0; __i--) %s(&((%s)this->%s)[__i]); }\n", cnt, cinfo(el)->dtor->cname, c_decl(ty_ptr(el), ""), list[i]->name->s);
        } else emitf("%s(&this->%s);\n", cinfo(el)->dtor->cname, list[i]->name->s);
    }
}

/* the destructors of the base subobjects, in reverse order */
static void emit_base_dtors(Ent *e) {
    ClassInfo *ci = cinfo(e->owner->type);
    int i;
    for (i = ci->nbases - 1; i >= 0; i--)
        if (ci->bases[i].cls->ci->needs_dtor) emitf("%s(&this->%s);\n", ci->bases[i].cls->ci->dtor->cname, ci->bases[i].fname);
}

/* ------------------------------------------------------------ scopes and destructors of locals */

static void emit_scope_dtors_text(Scope2 *sc) {
    int i;
    for (i = sc->ndtors - 1; i >= 0; i--) {
        Ent *v = sc->dtors[i];
        Type *el = strip_arrays(v->type);
        emit("__CPPC_POP();\n");
        if (v->type->kind == TY_ARRAY) {
            i64 cnt = 1;
            Type *x = v->type;
            while (x->kind == TY_ARRAY) { cnt *= x->len; x = x->base; }
            emitf("{ long __i; for (__i = %lld - 1; __i >= 0; __i--) %s(&((%s)%s)[__i]); }\n", cnt, cinfo(el)->dtor->cname, c_decl(ty_ptr(el), ""), v->cname);
        } else emitf("%s(&%s);\n", cinfo(el)->dtor->cname, v->cname);
    }
    if (sc->try_prev) emitf("__cppc_htop = %s;\n", sc->try_prev);
    if (sc->catch_end) emit("__CPPC_END_CATCH();\n");
}

static void register_local_dtor(Ent *v) {
    Scope2 *sc = scope2_cur;
    if (!type_needs_dtor(v->type)) return;
    sc->dtors = xrealloc(sc->dtors, sizeof(Ent *) * (sc->ndtors + 1));
    sc->dtors[sc->ndtors++] = v;
    if (v->type->kind == TY_ARRAY) emit_push_object(v->cname, v->type);
    else emit_push_object(fmt_str("&%s", v->cname), v->type);
}

/* the destructors of all the scopes up to the function (for return) */
static void emit_dtors_to_function(void) {
    Scope2 *sc;
    for (sc = scope2_cur; sc && sc->kind == SS_BLOCK; sc = sc->parent) {
        emit_scope_dtors_text(sc);
        if (sc->is_func) break;
    }
}

/* the destructors of the scopes that break (flag 0) or continue (flag 1) leaves */
static void emit_dtors_to_loop(int cont) {
    Scope2 *sc;
    for (sc = scope2_cur; sc && sc->kind == SS_BLOCK; sc = sc->parent) {
        emit_scope_dtors_text(sc);
        if (cont ? sc->cont : sc->brk) break;
        if (sc->is_func) break;
    }
}

/* the variable that all return statements of the body return, or 0 (0 also if some return returns something else) */
static int scan_returns(Node *n, Name **name, int *bad) {
    for (; n; n = n->next) {
        switch (n->kind) {
        case N_RETURN:
            if (n->a) {
                Node *e = n->a;
                if (e->kind == N_ID && !e->a->next && !(e->flags & ID_GLOBAL) && !(e->a->flags & PART_ARGS)) {
                    if (!*name) *name = e->a->name;
                    else if (*name != e->a->name) *bad = 1;
                } else *bad = 1;
            }
            break;
        case N_BLOCK: scan_returns(n->a, name, bad); break;
        case N_IF: scan_returns(n->b, name, bad); scan_returns(n->c, name, bad); break;
        case N_WHILE: case N_SWITCH: scan_returns(n->b, name, bad); break;
        case N_DO: scan_returns(n->a, name, bad); break;
        case N_FOR: case N_RANGEFOR: scan_returns(n->d, name, bad); break;
        case N_CASE: scan_returns(n->b, name, bad); break;
        case N_DEFAULT: case N_LABEL: scan_returns(n->a, name, bad); break;
        case N_TRY:
            {
                Node *cc;
                scan_returns(n->a, name, bad);
                for (cc = n->b; cc; cc = cc->next) scan_returns(cc->b, name, bad);
            }
            break;
        }
    }
    return 0;
}

/* the body of a function definition */
static void define_function(Node *fd, Ent *e, Node *fnode, Type *ft) {
    StrBuf body, tmps;
    StrBuf *saved_out = out, *saved_temps = temps;
    Ent *saved_fn = cur_fn, *saved_class = cur_class;
    Name *saved_nrvo = nrvo_name;
    Type *saved_ret = cur_ret;
    Scope2 *saved_scope = scope2_cur;
    int saved_tc = temp_count, saved_ctor = cur_is_ctor, saved_dtor = cur_is_dtor, saved_const = cur_method_const;
    int saved_nf = nftemps;
    int is_main = e->scope == scope2_global && strcmp(e->name->s, "main") == 0;
    int deduce_ret = 0;
    int saved_pushed = cur_ctor_pushed;
    Scope2 *bparent = body_parent ? body_parent : e->scope;
    body_parent = 0;
    if (e->flags & EF_DEFINED) serr(fd, "redefinition of '%s'", ent_qualified_name(e));
    if (e->flags & EF_DELETED) serr(fd, "the deleted function '%s' cannot be defined", ent_qualified_name(e));
    e->flags |= EF_DEFINED;
    if (e->flags & EF_CONSTEXPR) {
        e->cdef = fd;
        e->cfn = fnode;
    }
    deduce_ret = contains_auto(ft->base);
    sb_init(&body);
    sb_init(&tmps);
    out = &body;
    temps = &tmps;
    cur_fn = e;
    cur_ret = ft->base;
    cur_class = (e->flags & EF_MEMBER) ? e->owner : 0;
    cur_is_ctor = (e->flags & EF_CTOR) != 0;
    cur_is_dtor = (e->flags & EF_DTOR) != 0;
    cur_method_const = (e->flags & EF_CONST_METHOD) != 0;
    temp_count = 0;
    nftemps = 0;
    scope2_cur = scope2_new(SS_BLOCK, bparent);
    scope2_cur->is_func = 1;
    declare_params(fnode, ft);
    nrvo_name = 0;
    if (ft->base->kind == TY_CLASS && !cur_is_ctor) {
        Name *nm = 0;
        int bad = 0;
        scan_returns(fd->d->a, &nm, &bad);
        if (!bad) nrvo_name = nm;
    }
    if (cur_is_ctor) emit_ctor_prologue(e, fd);
    if (cur_is_dtor && cinfo(e->owner->type)->poly) emit_set_vptrs(e->owner);
    {
        Node *b = fd->d;
        Node *s;
        if (b->kind != N_BLOCK) serr(fd, "function-try-blocks are not supported yet");
        for (s = b->a; s; s = s->next) stmt(s);
    }
    emit_scope_dtors_text(scope2_cur);
    if (cur_is_dtor) {
        emit("__dtor_end:;\n");
        emit_member_dtors(e);
        emit_base_dtors(e);
    }
    if (cur_is_ctor) emit_ctor_return();
    else if (is_main) emit("return 0;\n");
    if (deduce_ret) {
        /* the return type came from the return statements */
        Type *rt = contains_auto(cur_ret) ? subst_auto(cur_ret, T_VOID) : cur_ret;
        ft = ty_func(rt, ft->params, ft->nparams, ft->variadic);
        e->type = ft;
    }
    out = saved_out;
    temps = saved_temps;
    if (pending_helpers.len) {
        emit(pending_helpers.s);
        sb_init(&pending_helpers);
    }
    {
        /* the code of templates (and lambdas) goes to the end of the C file: the structs that it uses are all
           defined there, and the prototypes are written already */
        int late = inst_active_count() > 0 || (e->flags & EF_TINST) || e->lazy || (e->owner && e->owner->inst);
        StrBuf *fo = out;
        const char *st = "";
        if (late) {
            if (deduce_ret) emitf("%s%s;\n", st, c_function_decl(e, fnode, ft));
            out = &late_defs;
            cur_line_file = -1;
        }
        emit_line(fd);
        emitf("%s%s {\n", st, c_function_decl(e, fnode, ft));
        emit(tmps.s);
        emit(body.s);
        emit("}\n");
        out = fo;
    }
    cur_line_file = -1;
    cur_ctor_pushed = saved_pushed;
    cur_fn = saved_fn;
    cur_class = saved_class;
    nrvo_name = saved_nrvo;
    cur_ret = saved_ret;
    cur_is_ctor = saved_ctor;
    cur_is_dtor = saved_dtor;
    cur_method_const = saved_const;
    scope2_cur = saved_scope;
    temp_count = saved_tc;
    nftemps = saved_nf;
}

static const char *var_cname(Scope2 *sc, Name *name, int is_global) {
    if (is_global && linkage_c > 0) return name->s;
    return fmt_str("%s%s", sc->prefix, name->s);
}

static int try_const_init(Type *t, Val *iv) {
    return ty_is_integral(ty_unqual(t)) && (t->cv & CV_CONST) && iv && iv->is_const;
}

/* a variable declaration: local, global, or static local */
static void declare_object_tail(Node *at, Ent *e, Type *t, Node *init, int form, int is_global, int is_static_local, int is_extern, Ent *old);

static void declare_variable(Node *at, Node *initdecl, Node *id, Type *t, int dsflags) {
    Node *lp = id_last(id);
    Scope2 *sc = declaration_scope(id);
    int is_global = sc->kind != SS_BLOCK;
    int is_static_local = !is_global && (dsflags & DS_STATIC);
    Ent *e, *old;
    Node *init = initdecl->b;
    int form = initdecl->flags & 3;          /* 1 = braces, 2 = parentheses; 0 with init = '=' */
    int static_ok = 1;
    const char *itext = 0;
    Val *iv = 0;
    const char *cname;
    int is_extern = (dsflags & DS_EXTERN) && !init;
    int is_lib = node_is_lib(at) && linkage_c > 0;
    int is_cls, mark = nftemps;
    if ((lp->flags & 7) != PART_NAME) serr(at, "invalid variable name");
    if (t->kind == TY_FUNC) serr(at, "'%s' is declared as a function", lp->name->s);
    if (t->kind == TY_VOID) serr(at, "variable '%s' has type void", lp->name->s);
    if ((dsflags & DS_CONSTEXPR) && !ty_is_ref(t) && t->kind != TY_FUNC) t = ty_cv(t, CV_CONST);
    old = sfind(sc, lp->name);
    if (old && old->kind != EK_VAR) serr(at, "'%s' redeclared as a different kind of symbol", lp->name->s);
    if (old && is_global) {
        /* extern int x; int x = 1; */
        if (contains_auto(t) == 0 && !ty_equal(ty_unqual(old->type), ty_unqual(t)) && !(old->type->kind == TY_ARRAY && t->kind == TY_ARRAY))
            serr(at, "conflicting declaration of '%s'", lp->name->s);
    } else if (old && !is_global && old->scope == sc) serr(at, "redeclaration of '%s'", lp->name->s);
    /* deduce auto */
    if (contains_auto(t)) {
        if (!init) serr(at, "declaration of '%s' has no initializer to deduce 'auto'", lp->name->s);
        {
            Node *ie = init->kind == N_INITLIST ? init->a : init;
            if (init->kind == N_INITLIST && form == 0 && ie && t->kind == TY_AUTO) {
                /* auto x = {a, b}: a std::initializer_list */
                int m = nftemps;
                Val *bv = braced_val(init);
                nftemps = m;
                t = braced_il_type(bv, at);
                ie = 0;
            }
            if (ie && ie->next) serr(at, "cannot deduce 'auto' from a braced list");
            if (!ie) goto auto_done;
            {
                int m = nftemps;
                iv = expr(ie);
                nftemps = m;           /* the probe constructed nothing */
            }
            t = deduce_auto(t, iv, at);
        }
    }
auto_done:
    is_cls = strip_arrays(t)->kind == TY_CLASS;
    cname = is_global ? var_cname(sc, lp->name, 1) : (is_static_local ? fmt_str("%s__s%d", lp->name->s, label_count++) : lp->name->s);
    if (is_global && (is_lib || (linkage_c > 0))) cname = lp->name->s;
    /* the initializer: evaluated before the name is visible only for auto; here the name is declared first */
    e = old;
    if (!e) {
        e = ent_new(EK_VAR, lp->name, t, sc);
        e->cname = cname;
        if (dsflags & DS_STATIC) e->flags |= EF_STATIC;
        if (is_lib) e->flags |= EF_LIB;
        if (dsflags & DS_CONSTEXPR) e->flags |= EF_CONSTEXPR;
    } else if (old->type->kind == TY_ARRAY && t->kind == TY_ARRAY && old->type->len < 0) {
        old->type = t;
    }
    if (init && is_cls && t->kind == TY_ARRAY && t->len < 0 && init->kind == N_INITLIST) {
        /* T a[] = { ... } of a class type: the length is the number of initializers */
        Node *it;
        i64 cnt = 0;
        for (it = init->a; it; it = it->next) cnt++;
        t = ty_array(t->base, cnt);
        e->type = t;
    }
    if (init && !is_cls) {
        int st = 1;
        if (t->kind == TY_ARRAY && t->len < 0) t = ty_array(t->base, -1);
        itext = init_text(t, init, form ? form : 1, at, &st);
        static_ok = st;
        /* the value again, for a constant initializer: only its text is discarded, so the
           temporaries this probe creates are not constructed and must not be destroyed */
        if (!ty_is_ref(t) && init->kind != N_INITLIST && t->kind != TY_ARRAY) {
            int m = nftemps;
            iv = expr(init);
            nftemps = m;
        } else if (!ty_is_ref(t) && init->kind == N_INITLIST && init->a && !init->a->next && t->kind != TY_ARRAY) {
            int m = nftemps;
            iv = expr(init->a);
            nftemps = m;
        }
        e->type = t;
    } else if (t->kind == TY_ARRAY && t->len < 0 && !is_extern) {
        serr(at, "the array '%s' has an unknown size", lp->name->s);
    }
    if (ty_is_ref(t) && !init && !is_extern && !(dsflags & DS_EXTERN) && !(e->flags & EF_PARAM))
        serr(at, "'%s' is declared as a reference but is not initialized", lp->name->s);
    if (!init && (t->cv & CV_CONST) && !is_extern && !ty_is_ref(t) && !is_cls)
        serr(at, "uninitialized const '%s'", lp->name->s);
    if (init && iv && try_const_init(t, iv)) {
        e->has_val = 1;
        e->cv = iv->cv;
    }
    if (!old) scope_add(sc, e);
    e->type = t;
    if (is_global && !is_extern && ((dsflags & DS_INLINE) || inst_active_count() > 0 || (old && old->owner && old->owner->inst)))
        e->flags |= EF_STATIC;               /* an inline variable or a static member of a template instance: every unit has its own */
    if (is_lib) return;
    if (is_cls) {
        declare_object_tail(at, e, t, init, form, is_global, is_static_local, is_extern, old);
        return;
    }
    /* the C declaration */
    if (is_global) {
        const char *decl = c_decl(ty_is_ref(t) ? ty_ptr(t->base) : t, e->cname);
        if (is_extern) {
            if (!old) emitf("extern %s;\n", decl);
            return;
        }
        emit_line(at);
        if (!init) {
            emitf("%s%s;\n", (e->flags & EF_STATIC) ? "static " : "", decl);
        } else if (static_ok) {
            emitf("%s%s = %s;\n", (e->flags & EF_STATIC) ? "static " : "", decl, itext);
        } else {
            if (t->kind == TY_ARRAY) serr(at, "dynamic initialization of arrays is not supported yet");
            emitf("%s%s;\n", (e->flags & EF_STATIC) ? "static " : "", decl);
            sb_printf(&out_init, "%s = %s;\n", e->cname, itext);
            sb_puts(&out_init, dtor_text_from(mark));
            dyn_init_count++;
        }
        return;
    }
    {
        const char *decl = c_decl(ty_is_ref(t) ? ty_ptr(t->base) : t, e->cname);
        emit_line(at);
        if (is_static_local) {
            if (!init) emitf("static %s;\n", decl);
            else if (static_ok) emitf("static %s = %s;\n", decl, itext);
            else {
                /* a dynamic initializer runs the first time the declaration is reached */
                emitf("static %s;\nstatic int %s__done;\nif (!%s__done) { %s__done = 1; %s = %s; }\n", decl, e->cname, e->cname, e->cname, e->cname, itext);
            }
        } else if (init) emitf("%s = %s;\n", decl, itext);
        else emitf("%s;\n", decl);
        flush_temps_stmt(mark);
    }
}

/* the C declaration and the construction of an object (or array of objects) of a class type */
static int dtor_wrapper_count = 0;

static void declare_object_tail(Node *at, Ent *e, Type *t, Node *init, int form, int is_global, int is_static_local, int is_extern, Ent *old) {
    Type *cls = strip_arrays(t);
    const char *decl = c_decl(t, e->cname);
    StrBuf *saved_out = out;
    int fm = init ? (init->kind == N_INITLIST && form != 2 ? 1 : (form == 2 ? 2 : (form == 1 ? 1 : 0))) : 3;
    Node *ie = init;
    require_complete(cls, at);
    if (in_cond_decl) serr(at, "declaring a class object in a condition is not supported yet");
    require_concrete(cls, at);
    if (init && form == 2 && init->kind == N_INITLIST && !init->a) fm = 3;
    if (is_extern) {
        if (!old) emitf("extern %s;\n", decl);
        return;
    }
    if (ty_is_ref(t)) return;
    if (!is_global && !is_static_local && nrvo_name && e->name == nrvo_name && cur_ret && cur_ret->kind == TY_CLASS
        && t->kind == TY_CLASS && ty_same(t, cur_ret) && cur_fn && !(cur_fn->flags & EF_CTOR)) {
        /* the object is built in the function's result */
        e->flags |= EF_NRVO | EF_INDIRECT;
        e->cname = "__ret";
        decl = 0;
    }
    if (decl == 0) {
        /* nothing to declare */
    } else if (is_global) {
        emit_line(at);
        emitf("%s%s;\n", (e->flags & EF_STATIC) ? "static " : "", decl);
    } else if (is_static_local) {
        /* the storage is at file scope (before the function) so that an exit handler can destroy it */
        StrBuf *so = out;
        out = &pending_helpers;
        emitf("static %s;\nstatic int %s__done;\n", decl, e->cname);
        if (type_needs_dtor(cls)) {
            Type *x = t;
            i64 cnt = 1;
            while (x->kind == TY_ARRAY) { cnt *= x->len; x = x->base; }
            if (t->kind == TY_ARRAY)
                emitf("static void %s__dtor_exit(void) { long __i; for (__i = %lld - 1; __i >= 0; __i--) %s(&((%s)%s)[__i]); }\n", e->cname, cnt, cinfo(cls)->dtor->cname, c_decl(ty_ptr(cls), ""), e->cname);
            else emitf("static void %s__dtor_exit(void) { %s(&%s); }\n", e->cname, cinfo(cls)->dtor->cname, e->cname);
        }
        out = so;
    } else {
        emit_line(at);
        emitf("%s;\n", decl);
    }
    /* the initialization statements */
    {
        StrBuf init_code;
        int needs = fm != 3 || type_needs_ctor(cls);
        sb_init(&init_code);
        if (!needs && !is_global && !is_static_local) {
            if (type_needs_dtor(cls) && !(e->flags & EF_NRVO)) register_local_dtor(e);
            else if (type_needs_dtor(cls)) emit_push_object("__ret", cls);
            return;
        }
        if (is_global) out = &out_init;
        else out = &init_code;
        if (t->kind == TY_ARRAY) {
            /* an array of objects: each element is default constructed or initialized from the list */
            Type *el = strip_arrays(t);
            i64 n = 1, i;
            Type *x = t;
            Node *item = fm == 1 ? init->a : 0;
            if (fm == 0 || fm == 2) serr(at, "unsupported initializer for an array of objects");
            while (x->kind == TY_ARRAY) { n *= x->len; x = x->base; }
            if (t->base->kind == TY_ARRAY && item) serr(at, "initializing multi-dimensional arrays of objects from a list is not supported yet");
            for (i = 0; i < n && item; i++, item = item->next)
                object_init_stmts(el, fmt_str("(&((%s)%s)[%lld])", c_decl(ty_ptr(el), ""), e->cname, i), item, item->kind == N_INITLIST ? 1 : 0, at);
            if (item) serr(item, "too many initializers for '%s'", ty_str(t));
            if (i < n && type_needs_ctor(el)) {
                const char *text = default_construct_text(el, fmt_str("(&((%s)%s)[__i])", c_decl(ty_ptr(el), ""), e->cname), at);
                emitf("{ long __i; for (__i = %lld; __i < %lld; __i++) %s; }\n", i, n, text ? text : "(void)0");
            }
        } else {
            object_init_stmts(t, (e->flags & EF_NRVO) ? "__ret" : fmt_str("(&%s)", e->cname), ie, fm, at);
        }
        out = saved_out;
        if (is_global) {
            dyn_init_count++;
            if (type_needs_dtor(cls)) {
                int w = dtor_wrapper_count++;
                Type *el2 = cls;
                if (t->kind == TY_ARRAY) {
                    i64 n = 1;
                    Type *x = t;
                    while (x->kind == TY_ARRAY) { n *= x->len; x = x->base; }
                    emitf("static void __cppc_dtor%d(void) { long __i; for (__i = %lld - 1; __i >= 0; __i--) %s(&((%s)%s)[__i]); }\n", w, n,
                          cinfo(el2)->dtor->cname, c_decl(ty_ptr(el2), ""), e->cname);
                } else emitf("static void __cppc_dtor%d(void) { %s(&%s); }\n", w, cinfo(el2)->dtor->cname, e->cname);
                sb_printf(&out_init, "atexit(__cppc_dtor%d);\n", w);
            }
        } else if (is_static_local) {
            emitf("if (!%s__done) {\n%s__done = 1;\n%s%s}\n", e->cname, e->cname, init_code.s,
                  type_needs_dtor(cls) ? fmt_str("atexit(%s__dtor_exit);\n", e->cname) : "");
        } else {
            emit(init_code.s);
            if (type_needs_dtor(cls) && !(e->flags & EF_NRVO)) register_local_dtor(e);
            else if (type_needs_dtor(cls)) emit_push_object("__ret", cls);
        }
    }
}

static void process_decl(Node *n);
static void process_toplevel_list(Node *n);

/* a typedef, function or variable declaration of one init-declarator */
static void declare_one(Node *d, Node *specs, Type *base, int dsflags, Node *initdecl) {
    Node *id = 0, *fn = 0;
    Type *t;
    Node *lp;
    t = apply_declarator(base, initdecl->a, &id, &fn);
    if (!id) {
        if (dsflags & DS_TYPEDEF) serr(d, "a typedef needs a name");
        return;
    }
    lp = id_last(id);
    if (dsflags & DS_TYPEDEF) {
        Ent *e;
        if ((lp->flags & 7) != PART_NAME) serr(d, "invalid typedef name");
        e = sfind(scope2_cur, lp->name);
        if (e && !(e->kind == EK_TYPEDEF && ty_equal(e->type, t))) serr(d, "'%s' redeclared as a different kind of symbol", lp->name->s);
        if (!e) {
            e = ent_new(EK_TYPEDEF, lp->name, t, scope2_cur);
            scope_add(scope2_cur, e);
        }
        return;
    }
    if (t->kind == TY_FUNC) {
        Ent *e = declare_function(d, id, t, dsflags, fn);
        emit_prototype(e, fn, dsflags);
        return;
    }
    declare_variable(d, initdecl, id, t, dsflags);
}

static void process_decl(Node *d) {
    if (d->kind == N_DECL) {
        Type *base;
        int dsflags = 0;
        Node *id;
        if (specs_empty(d->a) && d->b) {
            base = T_VOID;
            dsflags = d->a->flags;
        } else base = decl_specs_type(d->a, &dsflags);
        for (id = d->b; id; id = id->next) declare_one(d, d->a, base, dsflags, id);
        return;
    }
    if (d->kind == N_FUNCDEF) {
        int dsflags = 0;
        Type *base, *t;
        Node *id = 0, *fn = 0;
        Ent *e;
        if (specs_empty(d->a)) {
            base = T_VOID;
            dsflags = d->a->flags;
        } else base = decl_specs_type(d->a, &dsflags);
        t = apply_declarator(base, d->b, &id, &fn);
        if (!id || t->kind != TY_FUNC || !fn) serr(d, "unsupported function definition");
        {
            /* a definition outside its namespace (void ns::f() {}) names a scope */
            Scope2 *sc = declaration_scope(id), *saved = scope2_cur;
            scope2_cur = sc;
            e = declare_function(d, id, t, dsflags, fn);
            if (!(e->flags & EF_LIB)) emit_prototype(e, fn, dsflags);
            if (!(e->flags & EF_LIB) || !node_is_lib(d)) define_function(d, e, fn, t);
            scope2_cur = saved;
        }
        return;
    }
    switch (d->kind) {
    case N_EMPTY_DECL: case N_ATTR:
        return;
    case N_BINDING:
        binding_decl(d);
        return;
    case N_NAMESPACE:
        {
            Scope2 *saved = scope2_cur, *ns;
            Ent *e = 0;
            if (d->name) {
                e = sfind(scope2_cur, d->name);
                if (e && e->kind != EK_NS) serr(d, "'%s' redeclared as a different kind of symbol", d->name->s);
                if (!e) {
                    e = ent_new(EK_NS, d->name, 0, scope2_cur);
                    e->members = scope2_new(SS_NS, scope2_cur);
                    e->members->nsent = e;
                    e->members->prefix = fmt_str("%s%s_", scope2_cur->prefix, d->name->s);
                    scope_add(scope2_cur, e);
                }
                ns = e->members;
                if (d->flags & NS_INLINE) scope_use(scope2_cur, ns);
            } else {
                ns = scope2_new(SS_NS, scope2_cur);
                ns->prefix = fmt_str("%s__anon%d_", scope2_cur->prefix, anon_ns_count++);
                scope_use(scope2_cur, ns);
            }
            scope2_cur = ns;
            process_toplevel_list(d->a);
            scope2_cur = saved;
            return;
        }
    case N_NSALIAS:
        {
            Ent *t = resolve_id(d->a, 1), *e;
            if (t->kind != EK_NS) serr(d, "'%s' is not a namespace", t->name->s);
            e = ent_new(EK_NS, d->name, 0, scope2_cur);
            e->members = t->members;
            scope_add(scope2_cur, e);
            return;
        }
    case N_USING_DIR:
        {
            Ent *t = resolve_id(d->a, 1);
            if (t->kind != EK_NS) serr(d, "'%s' is not a namespace", t->name->s);
            scope_use(scope2_cur, t->members);
            return;
        }
    case N_USING_DECL:
        {
            Ent *t = resolve_id(d->a, 1);
            for (; t; t = t->overload) {
                Ent *c = anew(sizeof(Ent));
                *c = *t;
                c->overload = 0;
                c->flags |= EF_ALIAS;
                scope_add(scope2_cur, c);
                if (t->kind != EK_FUNC) break;
            }
            return;
        }
    case N_ALIAS:
        {
            Type *t;
            Ent *e;
            t = apply_declarator(decl_specs_type(d->a->a, 0), d->a->b, 0, 0);
            e = ent_new(EK_TYPEDEF, d->name, t, scope2_cur);
            scope_add(scope2_cur, e);
            return;
        }
    case N_STATIC_ASSERT:
        {
            Val *c = expr(d->a);
            if (!c->is_const) serr(d, "the condition of static_assert is not a constant expression");
            if (!c->cv) {
                if (d->b) {
                    int *cps, nc, raw;
                    char prefix[8];
                    StrBuf sb;
                    int i;
                    nc = decode_literal(d->b->text, &cps, &raw, prefix);
                    sb_init(&sb);
                    for (i = 0; i < nc; i++) sb_put_cp(&sb, cps[i]);
                    serr(d, "static assertion failed: %s", sb.s);
                }
                serr(d, "static assertion failed");
            }
            return;
        }
    case N_LINKAGE:
        {
            int c = strcmp(d->text, "\"C\"") == 0;
            if (!c && strcmp(d->text, "\"C++\"") != 0) serr(d, "unknown linkage %s", d->text);
            if (c) linkage_c++;
            process_toplevel_list(d->a);
            if (c) linkage_c--;
            return;
        }
    case N_TEMPLATE: case N_INSTANTIATION:
        process_template(d, 0);
        return;
    case N_ASM:
        serr(d, "asm is not supported");
        return;
    }
    serr(d, "unsupported declaration (%s)", node_kind_name(d->kind));
}

/* ------------------------------------------------------------ class definitions */

typedef struct DeferredBody DeferredBody;
struct DeferredBody {
    Ent *e;
    Node *fd, *fnode;
    Type *ft;
    DeferredBody *next;
};
static DeferredBody *def_head = 0, *def_tail = 0;
static int class_nest = 0;
static int inherit_constructors(Ent *cls, Node *m, int access);

static int specs_empty(Node *ts) {
    return !ts->a && !ts->val && !(ts->flags & (DS_CONST | DS_VOLATILE));
}

static int access_of(int op) {
    return op >= OPKW ? op - OPKW : op;
}

/* may the code being written access a member of owner with this access? */
static int access_allowed(Ent *owner, int access) {
    ClassInfo *ci;
    int i;
    Scope2 *sc;
    if (access == 0 || access == KW_public) return 1;
    if (cur_class == owner) return 1;
    if (access == KW_protected && cur_class && derived_to_base(cur_class->type, owner->type)) return 1;
    /* a class nested in the owner has the owner's access */
    for (sc = scope2_cur; sc; sc = sc->parent)
        if (sc->kind == SS_CLASS && sc->classent == owner) return 1;
    ci = owner->ci;
    if (!ci) return 0;
    for (i = 0; i < ci->nfriend_classes; i++)
        if (ci->friend_classes[i] == cur_class) return 1;
    for (i = 0; cur_class && cur_class->inst && i < ci->nfriend_tmpls; i++)
        if (ci->friend_tmpls[i] == inst_template(cur_class)) return 1;
    for (i = 0; cur_fn && cur_fn->inst && i < ci->nfriend_tmpls; i++)
        if (ci->friend_tmpls[i] == inst_template(cur_fn)) return 1;
    for (i = 0; i < ci->nfriend_funcs; i++)
        if (ci->friend_funcs[i] == cur_fn) return 1;
    return 0;
}

static void check_access(Ent *member, int access, Ent *owner, Node *at) {
    if (access_allowed(owner, access)) return;
    serr(at, "'%s' is %s within this context", member->name->s, access == KW_private ? "private" : "protected");
}

static void gen_implicit_move(Ent *cls, Ent *f);
static void gen_implicit_massign(Ent *cls, Ent *f);
static void emit_implicit_members(Ent *cls);
static void emit_struct(Ent *cls);
static void analyze_virtuals(Ent *cls);
static void emit_vtables(Ent *cls);

static Type *field_type_check(Node *at, Type *t, Name *name) {
    if (t->kind == TY_VOID) serr(at, "field '%s' has type void", name->s);
    if (t->kind == TY_FUNC) serr(at, "field '%s' is declared as a function", name->s);
    if (contains_auto(t)) serr(at, "the field '%s' cannot have a deduced type", name->s);
    require_complete(t, at);
    return t;
}

static void add_friend_class(ClassInfo *ci, Ent *c) {
    ci->friend_classes = xrealloc(ci->friend_classes, sizeof(Ent *) * (ci->nfriend_classes + 1));
    ci->friend_classes[ci->nfriend_classes++] = c;
}

static void add_friend_func(ClassInfo *ci, Ent *f) {
    ci->friend_funcs = xrealloc(ci->friend_funcs, sizeof(Ent *) * (ci->nfriend_funcs + 1));
    ci->friend_funcs[ci->nfriend_funcs++] = f;
}

static void add_field(Ent *cls, Name *name, Type *t, Node *init, int form, int access, Node *at) {
    ClassInfo *ci = cls->ci;
    Field *f;
    Ent *e;
    if (sfind(ci->scope, name)) serr(at, "'%s' redeclared in the class", name->s);
    f = anew(sizeof(Field));
    f->name = name;
    f->type = t;
    f->init = init;
    f->init_form = form;
    f->access = access;
    if (ci->last_field) ci->last_field->next = f;
    else ci->fields = f;
    ci->last_field = f;
    e = ent_new(EK_FIELD, name, t, ci->scope);
    e->flags |= EF_MEMBER;
    e->owner = cls;
    e->access = access;
    e->field = f;
    f->ent = e;
    scope_add(ci->scope, e);
}

/* a member function (or static member data) of the class that is being defined */
static void process_class_member(Ent *cls, Node *m, int *access) {
    ClassInfo *ci = cls->ci;
    int dsflags = 0;
    Type *base;
    Node *id;
    switch (m->kind) {
    case N_ACCESS:
        *access = access_of(m->op);
        return;
    case N_DECL:
    case N_FUNCDEF:
        break;
    case N_TEMPLATE: case N_INSTANTIATION:
        tmpl_access = *access;
        process_template(m, cls);
        return;
    case N_USING_DECL:
        if (inherit_constructors(cls, m, *access)) return;
        process_decl(m);
        return;
    default:
        process_decl(m);
        return;
    }
    if (m->kind == N_FUNCDEF) {
        Node *nid = 0, *fn = 0;
        Type *t;
        Ent *e;
        base = specs_empty(m->a) ? T_VOID : decl_specs_type(m->a, &dsflags);
        if (specs_empty(m->a)) dsflags = m->a->flags;
        t = apply_declarator(base, m->b, &nid, &fn);
        if (!nid || t->kind != TY_FUNC || !fn) serr(m, "unsupported member function definition");
        if (dsflags & DS_FRIEND) {
            Scope2 *saved = scope2_cur;
            scope2_cur = ci->scope->parent;
            in_class_def = 1;
            e = declare_function(m, nid, t, dsflags, fn);
            in_class_def = 0;
            emit_prototype(e, fn, dsflags);
            scope2_cur = saved;
            add_friend_func(ci, e);
            if (!(e->flags & EF_DEFINED) && m->d) {
                /* the body of a friend defined in the class is parsed with the complete class */
                DeferredBody *d = anew(sizeof(DeferredBody));
                d->e = e;
                d->fd = m;
                d->fnode = fn;
                d->ft = e->type;
                if (def_tail) def_tail->next = d;
                else def_head = d;
                def_tail = d;
            }
            return;
        }
        in_class_def = m->d != 0;
        e = declare_function(m, nid, t, dsflags, fn);
        in_class_def = 0;
        e->access = *access;
        emit_prototype(e, fn, dsflags);
        if (!(m->d)) return;
        if (cls->inst && !(e->flags & EF_DEFINED)) {
            /* a member of a class template instance: the body is generated when the function is first used */
            lazy_set(e, m, fn, e->type, ci->scope);
            return;
        }
        {
            DeferredBody *d = anew(sizeof(DeferredBody));
            d->e = e;
            d->fd = m;
            d->fnode = fn;
            d->ft = e->type;
            if (def_tail) def_tail->next = d;
            else def_head = d;
            def_tail = d;
        }
        return;
    }
    /* N_DECL */
    if (specs_empty(m->a)) {
        base = T_VOID;
        dsflags = m->a->flags;
    } else if (m->a->flags & DS_FRIEND) {
        /* a friend class is looked up (and declared) in the enclosing namespace */
        Scope2 *saved = scope2_cur;
        scope2_cur = ci->scope->parent;
        base = decl_specs_type(m->a, &dsflags);
        scope2_cur = saved;
    } else base = decl_specs_type(m->a, &dsflags);
    if (dsflags & DS_FRIEND) {
        if (!m->b) {
            /* friend class X; */
            if (m->a->a && m->a->a->kind == N_CLASS) {
                Ent *c = declare_class_node(m->a->a);
                add_friend_class(ci, c);
            } else if (base->kind == TY_CLASS) add_friend_class(ci, base->ent);
            return;
        }
        for (id = m->b; id; id = id->next) {
            Node *nid = 0, *fn = 0;
            Type *t = apply_declarator(base, id->a, &nid, &fn);
            Scope2 *saved = scope2_cur;
            Ent *e;
            if (!nid || t->kind != TY_FUNC) serr(m, "unsupported friend declaration");
            scope2_cur = ci->scope->parent;
            e = declare_function(m, nid, t, dsflags, fn);
            emit_prototype(e, fn, dsflags);
            scope2_cur = saved;
            add_friend_func(ci, e);
        }
        return;
    }
    if (!m->b) return;                       /* a class or enum declaration only */
    for (id = m->b; id; id = id->next) {
        Node *nid = 0, *fn = 0;
        Type *t;
        Node *lp;
        t = apply_declarator(base, id->a, &nid, &fn);
        if (!nid) continue;
        lp = id_last(nid);
        if (dsflags & DS_TYPEDEF) {
            declare_one(m, m->a, base, dsflags, id);
            continue;
        }
        if (t->kind == TY_FUNC) {
            Ent *e = declare_function(m, nid, t, dsflags, fn);
            e->access = *access;
            emit_prototype(e, fn, dsflags);
            continue;
        }
        if ((lp->flags & 7) != PART_NAME) serr(m, "invalid member name");
        if (id->c) serr(m, "bit-fields are not supported yet");
        if (dsflags & DS_STATIC) {
            Ent *e;
            const char *cname;
            Node *init = id->b;
            Val *iv = 0;
            if (ci->scope == 0) break;
            if (sfind(ci->scope, lp->name)) serr(m, "'%s' redeclared in the class", lp->name->s);
            if ((dsflags & DS_CONSTEXPR) && !ty_is_ref(t)) t = ty_cv(t, CV_CONST);
            cname = fmt_str("%s%s", ci->scope->prefix, lp->name->s);
            e = ent_new(EK_VAR, lp->name, t, ci->scope);
            e->cname = cname;
            e->flags |= EF_MEMBER | EF_STATIC;
            e->owner = cls;
            e->access = *access;
            scope_add(ci->scope, e);
            if (init) {
                int st = 1;
                const char *it;
                Node *ie = init->kind == N_INITLIST && init->a && !init->a->next ? init->a : init;
                if (t->kind == TY_CLASS) serr(m, "a static class member needs an out-of-class definition");
                it = init_text(t, init, 1, m, &st);
                if (ie->kind != N_INITLIST && t->kind != TY_ARRAY) iv = expr(ie);
                if (iv && try_const_init(t, iv)) {
                    e->has_val = 1;
                    e->cv = iv->cv;
                }
                if (!st) serr(m, "the in-class initializer of '%s' is not a constant expression", lp->name->s);
                e->flags |= EF_DEFINED;
                emit_line(m);
                emitf("static %s = %s;\n", c_decl(t, cname), it);      /* defined in every unit that includes the class */
            } else if (cls->inst) {
                emitf("static %s;\n", c_decl(ty_is_ref(t) ? ty_ptr(t->base) : t, cname));
            } else {
                emitf("extern %s;\n", c_decl(ty_is_ref(t) ? ty_ptr(t->base) : t, cname));
            }
            continue;
        }
        {
            Node *init = id->b;
            int form = id->flags & 3;
            t = field_type_check(m, t, lp->name);
            if (init && init->kind == N_INITLIST && form == 0) form = 1;
            add_field(cls, lp->name, t, init, form, *access, m);
        }
    }
}

/* ------------------------------------------------------------ traits and implicit members */

static int is_copy_param(Ent *cls, Type *p) {
    if (p->kind != TY_REF) return 0;
    return p->base->kind == TY_CLASS && p->base->ent == cls;
}

static int is_move_param(Ent *cls, Type *p) {
    if (p->kind != TY_RREF) return 0;
    return p->base->kind == TY_CLASS && p->base->ent == cls;
}

/* is the copy constructor or copy assignment of the class deleted (explicitly or because a member's is)? */
static int class_copy_deleted(Ent *cls, int assign) {
    ClassInfo *ci = cls->ci;
    Ent *f, *set;
    if (!ci) return 0;
    if (assign ? ci->assign_deleted : ci->copy_deleted) return 1;
    set = sfind(ci->scope, assign ? intern_str("operator=") : nm_ctor);
    for (f = set; f; f = f->overload)
        if ((f->flags & EF_DELETED) && f->type->nparams == 1 && is_copy_param(cls, f->type->params[0])) return 1;
    return 0;
}

static void compute_traits(Ent *cls) {
    ClassInfo *ci = cls->ci;
    Ent *f, *set;
    Field *fl;
    int any_dtor = 0, any_copy = 0, any_ctor = 0, any_nsdmi = 0, public_only = 1;
    ci->user_ctor = 0;
    set = sfind(ci->scope, nm_ctor);
    for (f = set; f; f = f->overload) {
        ci->user_ctor = 1;
        if (f->type->nparams >= 1 && is_copy_param(cls, f->type->params[0]) && func_min_args(f) <= 1) ci->decl_copy_ctor = 1;
        if (f->type->nparams >= 1 && is_copy_param(cls, f->type->params[0]) && func_min_args(f) <= 1 && !(f->flags & EF_DEFAULTED))
            ci->user_copy_ctor = 1;
        if (f->type->nparams >= 1 && is_move_param(cls, f->type->params[0]) && func_min_args(f) <= 1 && !(f->flags & EF_DEFAULTED))
            ci->user_move_ctor = 1;
    }
    set = sfind(ci->scope, intern_str("operator="));
    for (f = set; f; f = f->overload) {
        if (f->type->nparams == 1 && (is_copy_param(cls, f->type->params[0]) || (f->type->params[0]->kind == TY_CLASS && f->type->params[0]->ent == cls))
            && !(f->flags & EF_DEFAULTED))
            ci->user_copy_assign = 1;
        if (f->type->nparams == 1 && (is_copy_param(cls, f->type->params[0]) || (f->type->params[0]->kind == TY_CLASS && f->type->params[0]->ent == cls))) ci->decl_copy_assign = 1;
        if (f->type->nparams == 1 && is_move_param(cls, f->type->params[0]) && !(f->flags & EF_DEFAULTED)) ci->user_move_assign = 1;
    }
    ci->dtor = sfind(ci->scope, nm_dtor);
    if (ci->dtor && !(ci->dtor->flags & EF_DEFAULTED)) ci->user_dtor = 1;
    for (fl = ci->fields; fl; fl = fl->next) {
        Type *el = strip_arrays(fl->type);
        if (type_needs_dtor(el)) any_dtor = 1;
        if (type_nontrivial_copy(el)) any_copy = 1;
        if (type_needs_ctor(el)) any_ctor = 1;
        if (fl->init) any_nsdmi = 1;
        if (fl->access != KW_public && fl->access != 0) public_only = 0;
    }
    {
        int i;
        for (i = 0; i < ci->nbases; i++) {
            ClassInfo *bc = ci->bases[i].cls->ci;
            if (bc->needs_dtor) any_dtor = 1;
            if (bc->nontrivial_copy) any_copy = 1;
            if (bc->needs_ctor) any_ctor = 1;
        }
    }
    if (ci->poly) any_ctor = any_copy = 1;
    /* a user move constructor or assignment deletes the implicit copy; a member or base that cannot be copied does too */
    if ((ci->user_move_ctor || ci->user_move_assign) && !ci->decl_copy_ctor) ci->copy_deleted = 1;
    if ((ci->user_move_ctor || ci->user_move_assign) && !ci->decl_copy_assign) ci->assign_deleted = 1;
    for (fl = ci->fields; fl; fl = fl->next) {
        Type *el = strip_arrays(fl->type);
        if (el->kind == TY_CLASS && class_copy_deleted(el->ent, 0) && !ci->user_copy_ctor) ci->copy_deleted = 1;
        if (el->kind == TY_CLASS && class_copy_deleted(el->ent, 1) && !ci->user_copy_assign) ci->assign_deleted = 1;
    }
    {
        int i;
        for (i = 0; i < ci->nbases; i++) {
            if (class_copy_deleted(ci->bases[i].cls, 0) && !ci->user_copy_ctor) ci->copy_deleted = 1;
            if (class_copy_deleted(ci->bases[i].cls, 1) && !ci->user_copy_assign) ci->assign_deleted = 1;
        }
    }
    for (fl = ci->fields; fl; fl = fl->next)
        if ((ty_is_ref(fl->type) || (strip_arrays(fl->type)->cv & CV_CONST)) && !ci->user_copy_assign) ci->assign_deleted = 1;
    ci->needs_dtor = ci->user_dtor || ci->dtor != 0 || any_dtor;
    ci->nontrivial_copy = ci->user_copy_ctor || any_copy || ci->user_move_ctor || ci->user_move_assign || ci->copy_deleted;
    ci->needs_ctor = ci->user_ctor || any_ctor || any_nsdmi;
    ci->aggregate = !ci->user_ctor && public_only && ci->nbases == 0 && !ci->poly;
}

/* an entity for a special member function that the translator writes */
static Ent *implicit_ent(Ent *cls, Name *name, int eflags, const char *cname, Type *ft) {
    Ent *e = ent_new(EK_FUNC, name, ft, cls->ci->scope);
    e->flags |= EF_MEMBER | EF_IMPLICIT | eflags | EF_DEFINED;
    e->owner = cls;
    e->cname = cname;
    e->access = KW_public;
    e->defaults = anew(sizeof(Node *) * (ft->nparams + 1));
    return e;
}

/* generates the implicit (or defaulted) special members that the class needs */
static void make_implicit_members(Ent *cls) {
    ClassInfo *ci = cls->ci;
    Type *ct = cls->type;
    Type *cref = ty_ref(ty_cv(ct, CV_CONST));
    Type **p1;
    Field *fl;
    int any_assign = 0;
    Ent *d;
    /* defaulted declarations use the generated bodies */
    d = sfind(ci->scope, nm_ctor);
    for (; d; d = d->overload) {
        if (!(d->flags & EF_DEFAULTED)) continue;
        if (d->type->nparams == 0) ci->implicit_ctor = d;
        else if (d->type->nparams == 1 && is_copy_param(cls, d->type->params[0])) ci->implicit_copy = d;
        else if (d->type->nparams == 1 && is_move_param(cls, d->type->params[0])) ci->implicit_move = d;
    }
    if (!ci->user_ctor && ci->needs_ctor && !ci->is_lambda) {
        ci->implicit_ctor = implicit_ent(cls, nm_ctor, EF_CTOR, fmt_str("%s__ctor__v", cls->cname), ty_func(T_VOID, 0, 0, 0));
    }
    if (ci->nontrivial_copy && !ci->user_copy_ctor && !ci->implicit_copy && !ci->copy_deleted) {
        p1 = anew(sizeof(Type *) * 2);
        p1[0] = cref;
        ci->implicit_copy = implicit_ent(cls, nm_ctor, EF_CTOR, fmt_str("%s__copy", cls->cname), ty_func(T_VOID, p1, 1, 0));
    }
    {
        /* the implicit move constructor and move assignment: only if a member or base can be moved */
        int any_mc = 0, any_ma = 0, i;
        int block = ci->user_copy_ctor || ci->user_copy_assign || ci->user_move_assign || ci->user_move_ctor || ci->user_dtor;
        for (fl = ci->fields; fl; fl = fl->next) {
            Type *el = strip_arrays(fl->type);
            if (el->kind == TY_CLASS && el->ent->ci) {
                if (el->ent->ci->user_move_ctor || el->ent->ci->implicit_move) any_mc = 1;
                if (el->ent->ci->user_move_assign || el->ent->ci->implicit_massign) any_ma = 1;
            }
        }
        for (i = 0; i < ci->nbases; i++) {
            ClassInfo *bc = ci->bases[i].cls->ci;
            if (bc->user_move_ctor || bc->implicit_move) any_mc = 1;
            if (bc->user_move_assign || bc->implicit_massign) any_ma = 1;
        }
        if (any_mc && !block && !ci->implicit_move) {
            p1 = anew(sizeof(Type *) * 2);
            p1[0] = ty_rref(ct);
            ci->implicit_move = implicit_ent(cls, nm_ctor, EF_CTOR, fmt_str("%s__move", cls->cname), ty_func(T_VOID, p1, 1, 0));
        }
        if (any_ma && !block && !ci->implicit_massign && !ci->assign_deleted) {
            p1 = anew(sizeof(Type *) * 2);
            p1[0] = ty_rref(ct);
            ci->implicit_massign = implicit_ent(cls, intern_str("operator="), 0, fmt_str("%s__massign", cls->cname), ty_func(ty_ref(ct), p1, 1, 0));
        }
    }
    for (fl = ci->fields; fl; fl = fl->next) {
        Type *el = strip_arrays(fl->type);
        if (el->kind == TY_CLASS && el->ent->ci && (el->ent->ci->user_copy_assign || el->ent->ci->implicit_assign)) any_assign = 1;
    }
    {
        int i;
        for (i = 0; i < ci->nbases; i++) {
            ClassInfo *bc = ci->bases[i].cls->ci;
            if (bc->user_copy_assign || bc->implicit_assign) any_assign = 1;
        }
        if (ci->poly) any_assign = 1;
    }
    if (!ci->user_copy_assign && any_assign && !ci->assign_deleted) {
        p1 = anew(sizeof(Type *) * 2);
        p1[0] = cref;
        ci->implicit_assign = implicit_ent(cls, intern_str("operator="), 0, fmt_str("%s__assign", cls->cname), ty_func(ty_ref(ct), p1, 1, 0));
    }
    if (ci->needs_dtor && !ci->dtor) {
        ci->dtor = implicit_ent(cls, nm_dtor, EF_DTOR, fmt_str("%s__dtor", cls->cname), ty_func(T_VOID, 0, 0, 0));
    } else if (ci->dtor && (ci->dtor->flags & EF_DEFAULTED)) {
        ci->dtor->flags |= EF_IMPLICIT;
    }
}

static void define_class(Ent *cls, Node *cn) {
    ClassInfo *ci;
    Node *m;
    int access;
    Scope2 *saved_scope = scope2_cur;
    Ent *saved_class = cur_class;
    if (cls->ci && cls->ci->complete) serr(cn, "redefinition of '%s'", ent_qualified_name(cls));
    ci = anew(sizeof(ClassInfo));
    cls->ci = ci;
    ci->key = cn->op - OPKW;
    if (cls->name && strncmp(cls->name->s, "__lambda", 8) == 0) {
        ci->is_lambda = 1;
        ci->lambda_outer = lambda_outer_next;
        lambda_outer_next = 0;
    }
    ci->self = cls;
    ci->scope = scope2_new(SS_CLASS, scope2_cur);
    ci->scope->classent = cls;
    ci->scope->prefix = fmt_str("%s__", cls->cname);
    access = ci->key == KW_class ? KW_private : KW_public;
    ci->is_final = (cn->flags & CL_FINAL) != 0;
    {
        Node *b;
        int bi = 0, i;
        for (b = cn->b; b; b = b->next, bi++) {
            Ent *be = resolve_id(b->a, 1);
            BaseSpec *bs;
            if (be->kind == EK_TYPEDEF && be->type->kind == TY_CLASS) be = be->type->ent;
            if (be->kind != EK_CLASS) serr(b, "'%s' is not a class", ent_qualified_name(be));
            if (!be->ci || !be->ci->complete) serr(b, "invalid use of incomplete type '%s' as a base class", ent_qualified_name(be));
            if (b->flags & 1) serr(b, "virtual base classes are not supported yet");
            if (b->flags & 2) serr(b, "pack expansions in base lists are not supported yet");
            if (be->ci->is_final) serr(b, "cannot derive from the final class '%s'", ent_qualified_name(be));
            if (ci->key == KW_union) serr(b, "a union cannot have base classes");
            for (i = 0; i < ci->nbases; i++)
                if (ci->bases[i].cls == be) serr(b, "duplicate base class '%s'", ent_qualified_name(be));
            ci->bases = xrealloc(ci->bases, sizeof(BaseSpec) * (ci->nbases + 1));
            bs = &ci->bases[ci->nbases++];
            bs->cls = be;
            bs->access = b->op ? access_of(b->op) : (ci->key == KW_class ? KW_private : KW_public);
            bs->fname = fmt_str("__b%d", bi);
            ci->scope->bases = xrealloc(ci->scope->bases, sizeof(Scope2 *) * (ci->scope->nbases + 1));
            ci->scope->bases[ci->scope->nbases++] = be->ci->scope;
        }
    }
    cls->type->len = -1;
    scope2_cur = ci->scope;
    cur_class = cls;
    class_nest++;
    for (m = cn->c; m; m = m->next) process_class_member(cls, m, &access);
    ci->complete = 1;
    {
        int i;
        for (i = 0; i < ci->nbases; i++) if (ci->bases[i].cls->ci->poly) ci->poly = 1;
        for (i = 0; i < ci->nmethods; i++) if (ci->methods[i]->flags & EF_VIRTUAL) ci->poly = 1;
    }
    compute_traits(cls);
    make_implicit_members(cls);
    analyze_virtuals(cls);
    scope2_cur = saved_scope;
    emit_struct(cls);
    scope2_cur = ci->scope;
    {
        /* prototypes of the generated functions: the vtables and thunks name them */
        Ent *fs[6];
        int k;
        fs[0] = ci->implicit_ctor;
        fs[1] = ci->implicit_copy;
        fs[2] = ci->implicit_assign;
        fs[3] = (ci->dtor && (ci->dtor->flags & EF_IMPLICIT)) ? ci->dtor : 0;
        fs[4] = ci->implicit_move;
        fs[5] = ci->implicit_massign;
        for (k = 0; k < 6; k++)
            if (fs[k] && !(fs[k]->flags & EF_DELETED) && !(fs[k]->flags & EF_LIB)) emitf("%s;\n", c_function_decl(fs[k], 0, fs[k]->type));
    }
    emit_vtables(cls);
    emit_implicit_members(cls);
    scope2_cur = saved_scope;
    class_nest--;
    cur_class = saved_class;
    if (class_nest == 0) {
        DeferredBody *d = def_head;
        def_head = def_tail = 0;
        run_eager_list();
        for (; d; d = d->next) {
            Scope2 *s2 = scope2_cur;
            Ent *de = d->e;
            if (de->owner && de->owner->inst && (de->flags & EF_MEMBER) && !(de->flags & (EF_VIRTUAL | EF_DTOR))) {
                /* a member of a class template instance: generated when it is first used */
                lazy_set(de, d->fd, d->fnode, d->ft, de->scope);
                continue;
            }
            scope2_cur = d->e->scope;
            define_function(d->fd, d->e, d->fnode, d->ft);
            scope2_cur = s2;
        }
    }
}

/* the C struct of a class: the vptr, the base subobjects, the data members */
static void emit_struct(Ent *cls) {
    ClassInfo *ci = cls->ci;
    Field *f;
    int i;
    if (cls->flags & EF_LIB) return;
    if (inst_active_count() > 0 && pending_helpers.len && !cur_fn) {
        /* the instances that the class needs (its bases and members) come before it */
        emit(pending_helpers.s);
        sb_init(&pending_helpers);
    }
    emit_line_none();
    emitf("%s {\n", class_tag(cls));
    if (ci->own_vptr) emit("void **__vptr;\n");
    for (i = 0; i < ci->nbases; i++) emitf("%s %s;\n", class_tag(ci->bases[i].cls), ci->bases[i].fname);
    if (!ci->fields && !ci->own_vptr && ci->nbases == 0) emit("int __empty;\n");
    for (f = ci->fields; f; f = f->next)
        emitf("%s;\n", c_decl(ty_is_ref(f->type) ? ty_ptr(f->type->base) : f->type, f->name->s));
    emit("};\n");
}

/* ------------------------------------------------------------ virtual functions */

static int same_signature(Ent *f, Name *name, Type *ft, int is_const) {
    int i;
    if (f->name != name || f->type->nparams != ft->nparams || f->type->variadic != ft->variadic) return 0;
    if ((!!(f->flags & EF_CONST_METHOD)) != is_const && name != nm_dtor) return 0;
    for (i = 0; i < ft->nparams; i++)
        if (!ty_same(ty_adjust_param(f->type->params[i]), ty_adjust_param(ft->params[i]))) return 0;
    return 1;
}

/* a virtual function of a base class (or of its bases) with this signature */
static Ent *find_virtual_in_bases(ClassInfo *ci, Name *name, Type *ft, int is_const) {
    int i, k;
    for (i = 0; i < ci->nbases; i++) {
        ClassInfo *bc = ci->bases[i].cls->ci;
        if (name == nm_dtor) {
            if (bc->dtor && bc->dtor->vowner) return bc->dtor;
        } else {
            for (k = 0; k < bc->nmethods; k++) {
                Ent *g = bc->methods[k];
                if (g->vowner && same_signature(g, name, ft, is_const)) return g;
            }
        }
        {
            Ent *r = find_virtual_in_bases(bc, name, ft, is_const);
            if (r) return r;
        }
    }
    return 0;
}

/* the path of the vptr member inside an object of class x: ".__vptr" or through the primary base */
static const char *vptr_path(Type *x) {
    ClassInfo *ci = x->ent->ci;
    if (ci->own_vptr) return ".__vptr";
    if (ci->nbases > 0 && ci->bases[0].cls->ci->poly) return fmt_str(".%s%s", ci->bases[0].fname, vptr_path(ci->bases[0].cls->type));
    return 0;
}

static void add_vslot(ClassInfo *ci, Name *name, Type *ft, int is_const, Ent *decl) {
    ci->vslots = xrealloc(ci->vslots, sizeof(VSlot) * (ci->nvslots + 1));
    ci->vslots[ci->nvslots].name = name;
    ci->vslots[ci->nvslots].ft = ft;
    ci->vslots[ci->nvslots].is_const = is_const;
    ci->vslots[ci->nvslots].decl = decl;
    ci->nvslots++;
}

static void analyze_virtuals(Ent *cls) {
    ClassInfo *ci = cls->ci;
    int i;
    Ent *all[256];
    int n = 0;
    ci->own_vptr = ci->poly && !(ci->nbases > 0 && ci->bases[0].cls->ci->poly);
    if (ci->poly && ci->nbases > 0 && ci->bases[0].cls->ci->poly) {
        ClassInfo *pb = ci->bases[0].cls->ci;
        for (i = 0; i < pb->nvslots; i++) add_vslot(ci, pb->vslots[i].name, pb->vslots[i].ft, pb->vslots[i].is_const, pb->vslots[i].decl);
    }
    for (i = 0; i < ci->nmethods && n < 255; i++) all[n++] = ci->methods[i];
    if (ci->dtor && ci->dtor->owner == cls) {
        int found = 0;
        for (i = 0; i < n; i++) if (all[i] == ci->dtor) found = 1;
        if (!found) all[n++] = ci->dtor;
    }
    for (i = 0; i < n; i++) {
        Ent *f = all[i], *g;
        int is_const = (f->flags & EF_CONST_METHOD) != 0;
        if (f->flags & (EF_CTOR | EF_CONV)) {
            if (f->flags & (EF_VIRTUAL | EF_PURE | EF_OVERRIDE)) serr(0, "'%s' cannot be virtual", f->name->s);
            continue;
        }
        if (f->flags & EF_STATIC) {
            if (f->flags & (EF_VIRTUAL | EF_OVERRIDE)) serr(0, "a static member function cannot be virtual");
            continue;
        }
        g = find_virtual_in_bases(ci, f->name, f->type, is_const);
        if (g) {
            if (g->flags & EF_FINAL) error_at(f->file, f->line, "'%s' overrides a function that is final", ent_qualified_name(f));
            f->flags |= EF_VIRTUAL;
            f->vowner = g->vowner;
            f->vslot = g->vslot;
        } else if (f->flags & EF_VIRTUAL) {
            f->vowner = cls;
            f->vslot = ci->nvslots;
            add_vslot(ci, f->name, f->type, is_const, f);
        } else if (f->flags & EF_OVERRIDE) {
            error_at(f->file, f->line, "'%s' is marked 'override' but does not override a base function", ent_qualified_name(f));
        }
        if ((f->flags & EF_PURE) && !f->vowner) error_at(f->file, f->line, "'%s' is declared pure but is not virtual", ent_qualified_name(f));
    }
    if (ci->dtor && ci->dtor->vowner) ci->has_virtual_dtor = 1;
}

/* the function that overrides the virtual function (name, ft, is_const) in the complete class d: the first
   one found in d's own scope or in the bases; *path = the path to the subobject of its class */
static Ent *final_overrider(Type *d, ClassInfo *ci, Name *name, Type *ft, int is_const, const char *prefix, const char **path) {
    int k, i;
    if (name == nm_dtor) {
        if (ci->dtor && ci->dtor->vowner) {
            *path = prefix;
            return ci->dtor;
        }
    } else {
        for (k = 0; k < ci->nmethods; k++) {
            Ent *g = ci->methods[k];
            if (g->vowner && same_signature(g, name, ft, is_const)) {
                *path = prefix;
                return g;
            }
        }
    }
    for (i = 0; i < ci->nbases; i++) {
        Ent *r = final_overrider(d, ci->bases[i].cls->ci, name, ft, is_const, fmt_str("%s.%s", prefix, ci->bases[i].fname), path);
        if (r) return r;
    }
    return 0;
}

/* is the subobject at path q at the same address as the one at path p?  (equal paths, or a chain of
   first bases of classes without their own vptr) */
static int zero_chain(Type *d, const char *p, const char *q) {
    /* q = p + a chain of ".__b0" through classes without an own vptr, or the reverse */
    const char *longer, *shorter;
    size_t lp = strlen(p), lq = strlen(q), ls;
    Type *t = d;
    const char *c;
    if (strcmp(p, q) == 0) return 1;
    if (lp > lq) { longer = p; shorter = q; } else { longer = q; shorter = p; }
    ls = strlen(shorter);
    if (strncmp(longer, shorter, ls) != 0) return 0;
    /* walk down the shorter path, then through the rest */
    c = shorter;
    while (*c) {
        int idx;
        const char *e;
        ClassInfo *ci = t->ent->ci;
        c += 4;                      /* ".__b" */
        idx = 0;
        while (*c >= '0' && *c <= '9') idx = idx * 10 + (*c++ - '0');
        (void)e;
        t = ci->bases[idx].cls->type;
    }
    c = longer + ls;
    while (*c) {
        int idx = 0;
        ClassInfo *ci = t->ent->ci;
        c += 4;
        while (*c >= '0' && *c <= '9') idx = idx * 10 + (*c++ - '0');
        if (idx != 0 || ci->own_vptr) return 0;
        t = ci->bases[idx].cls->type;
    }
    return 1;
}

static int thunk_count = 0;

/* the C parameter types of a virtual function as the vtable's C functions take them (this first) */
static Type *vtable_fn_type(Ent *f, Type *this_class) {
    Type *ft = f->type;
    Type **ps = anew(sizeof(Type *) * (ft->nparams + 3));
    int n = 0, i;
    if (ft->base->kind == TY_CLASS) ps[n++] = ty_ptr(ty_unqual(ft->base));
    ps[n++] = ty_ptr(this_class);
    for (i = 0; i < ft->nparams; i++) {
        Type *p = ft->params[i];
        if (ty_is_ref(p)) p = ty_ptr(p->base);
        else if (p->kind == TY_CLASS) p = ty_ptr(p);
        ps[n++] = p;
    }
    return ty_func(c_return_type(f, ft), ps, n, ft->variadic);
}

/* a thunk: a function with the C type of the vtable entry that adjusts this and calls the overrider.
   group_path: where the vtable's subobject (class x) is in the complete class d; target_path: where the
   overrider's class is. */
static const char *emit_thunk(Type *d, const char *group_path, Type *x, Ent *target, const char *target_cname, const char *target_path, Ent *slot_decl, int is_dtor) {
    const char *name = fmt_str("%s__thunk%d", d->ent->cname, thunk_count++);
    Type *ft = slot_decl->type;
    StrBuf call, params;
    StrBuf *saved = out;
    const char *complete, *target_this, *dtag = class_tag(d->ent);
    Type *rt = is_dtor ? T_VOID : c_return_type(slot_decl, ft);
    int i;
    sb_init(&call);
    sb_init(&params);
    if (!is_dtor && ft->base->kind == TY_CLASS) {
        sb_puts(&params, c_decl(ty_ptr(ty_unqual(ft->base)), "__ret"));
        sb_puts(&params, ", ");
        sb_puts(&call, "__ret, ");
    }
    sb_puts(&params, c_decl(ty_ptr(x), "this"));
    if (group_path[0])
        complete = fmt_str("((%s *)((char *)this - ((char *)&((%s *)0)->%s - (char *)0)))", dtag, dtag, group_path + 1);
    else complete = fmt_str("((%s *)this)", dtag);
    target_this = target_path[0] ? fmt_str("(&%s->%s)", complete, target_path + 1) : complete;
    sb_puts(&call, target_this);
    if (!is_dtor) {
        for (i = 0; i < ft->nparams; i++) {
            sb_printf(&params, ", %s", c_param_type_text(ft->params[i], fmt_str("a%d", i)));
            sb_printf(&call, ", a%d", i);
        }
        if (ft->variadic) sb_puts(&params, ", ...");
    }
    (void)target;
    out = &pending_helpers;
    emitf("static %s {\n", c_decl(rt, fmt_str("%s(%s)", name, params.s)));
    if (rt->kind == TY_VOID) emitf("%s(%s);\n}\n", target_cname, call.s);
    else emitf("return %s(%s);\n}\n", target_cname, call.s);
    out = saved;
    return name;
}

static void collect_groups(Ent *d, Type *x, const char *path, int primary_of_parent, ClassInfo *dci, int *k) {
    ClassInfo *xc = x->ent->ci;
    int i;
    if (xc->poly && !primary_of_parent) {
        dci->vgroups = xrealloc(dci->vgroups, sizeof(VGroup) * (dci->nvgroups + 1));
        dci->vgroups[dci->nvgroups].path = path;
        dci->vgroups[dci->nvgroups].x = x;
        dci->vgroups[dci->nvgroups].name = *k == 0 ? fmt_str("%s__vtbl", d->cname) : fmt_str("%s__vtbl%d", d->cname, *k);
        dci->nvgroups++;
        (*k)++;
    }
    for (i = 0; i < xc->nbases; i++) {
        ClassInfo *bc = xc->bases[i].cls->ci;
        collect_groups(d, xc->bases[i].cls->type, fmt_str("%s.%s", path, xc->bases[i].fname), i == 0 && xc->poly && bc->poly, dci, k);
    }
}

/* the vtables of a polymorphic class, their thunks, and the deleting destructor */
static void emit_vtables(Ent *cls) {
    ClassInfo *ci = cls->ci;
    int g, k = 0, i;
    StrBuf *saved = out;
    if (!ci->poly) return;
    collect_groups(cls, cls->type, "", 0, ci, &k);
    if (cls->flags & EF_LIB) return;
    /* the deleting destructor: calls the destructor and frees the object */
    if (ci->has_virtual_dtor) {
        out = &pending_helpers;
        emitf("static void %s__deldtor(%s) {\n%s(this);\nfree(this);\n}\n", cls->cname, c_decl(ty_ptr(cls->type), "this"), ci->dtor->cname);
        out = saved;
    }
    for (g = 0; g < ci->nvgroups; g++) {
        VGroup *vg = &ci->vgroups[g];
        ClassInfo *xc = vg->x->ent->ci;
        StrBuf tbl;
        sb_init(&tbl);
        {
            /* slot 0: the run-time type information of the complete class and the offset of this subobject in it */
            const char *vi = fmt_str("%s__vi", vg->name);
            out = &pending_helpers;
            emitf("static struct __cppc_vtinfo %s = { %s, 0 };\n", vi, type_desc(cls->type));
            out = saved;
            if (vg->path[0]) {
                sb_printf(&out_init, "%s.off = (long)((char *)&((%s *)0)->%s - (char *)0);\n", vi, class_tag(cls), vg->path + 1);
                dyn_init_count++;
            }
            sb_printf(&tbl, "(void *)&%s", vi);
        }
        for (i = 0; i < xc->nvslots; i++) {
            VSlot *vs = &xc->vslots[i];
            const char *path = 0, *target, *entry;
            Ent *f = final_overrider(cls->type, ci, vs->name, vs->ft, vs->is_const, "", &path);
            Ent *slot_decl = vs->decl;
            if (!f || (f->flags & EF_PURE)) {
                ci->abstract = 1;
                sb_puts(&tbl, ", (void *)__cppc_pure");
                continue;
            }
            target = vs->name == nm_dtor ? fmt_str("%s__deldtor", f->owner->cname) : f->cname;
            if (zero_chain(cls->type, vg->path, path)) entry = target;
            else {
                entry = emit_thunk(cls->type, vg->path, vg->x, f, target, path, slot_decl, vs->name == nm_dtor);
            }
            sb_printf(&tbl, ", (void *)%s", entry);
        }
        out = &pending_helpers;
        emitf("static void *%s[] = { %s };\n", vg->name, tbl.s);
        out = saved;
    }
    /* a class with a pure virtual function (not overridden) is abstract */
    for (g = 0; g < ci->nvgroups; g++) { (void)g; }
}

/* the statements that set the vptrs of the object *this to this class's vtables */
static void emit_set_vptrs(Ent *cls) {
    ClassInfo *ci = cls->ci;
    int g;
    for (g = 0; g < ci->nvgroups; g++) {
        VGroup *vg = &ci->vgroups[g];
        emitf("(*this)%s%s = %s;\n", vg->path, vptr_path(vg->x), vg->name);
    }
}

/* ------------------------------------------------------------ implicit special members */

typedef struct {
    StrBuf body, tmps;
    StrBuf *saved_out, *saved_temps;
    Ent *saved_fn, *saved_class;
    Type *saved_ret;
    Scope2 *saved_scope;
    int saved_tc, saved_ctor, saved_dtor, saved_const, saved_nf;
} GenCtx;

static void gen_begin(GenCtx *g, Ent *f) {
    g->saved_out = out;
    g->saved_temps = temps;
    g->saved_fn = cur_fn;
    g->saved_class = cur_class;
    g->saved_ret = cur_ret;
    g->saved_scope = scope2_cur;
    g->saved_tc = temp_count;
    g->saved_ctor = cur_is_ctor;
    g->saved_dtor = cur_is_dtor;
    g->saved_const = cur_method_const;
    g->saved_nf = nftemps;
    sb_init(&g->body);
    sb_init(&g->tmps);
    out = &g->body;
    temps = &g->tmps;
    cur_fn = f;
    cur_ret = f->type->base;
    cur_class = f->owner;
    cur_is_ctor = (f->flags & EF_CTOR) != 0;
    cur_is_dtor = (f->flags & EF_DTOR) != 0;
    cur_method_const = 0;
    temp_count = 0;
    nftemps = 0;
    nrvo_name = 0;
    scope2_cur = scope2_new(SS_BLOCK, f->scope);
    scope2_cur->is_func = 1;
}

static void gen_end(GenCtx *g, Ent *f) {
    out = g->saved_out;
    temps = g->saved_temps;
    cur_line_file = -1;
    if (pending_helpers.len) {
        emit(pending_helpers.s);
        sb_init(&pending_helpers);
    }
    emitf("%s {\n", c_function_decl(f, 0, f->type));
    emit(g->tmps.s);
    emit(g->body.s);
    emit("}\n");
    cur_fn = g->saved_fn;
    cur_class = g->saved_class;
    cur_ret = g->saved_ret;
    scope2_cur = g->saved_scope;
    temp_count = g->saved_tc;
    cur_is_ctor = g->saved_ctor;
    cur_is_dtor = g->saved_dtor;
    cur_method_const = g->saved_const;
    nftemps = g->saved_nf;
}

/* the assignment of src to the object dst (an lvalue C text) of class type t */
static const char *class_assign_text(Type *t, const char *dst, Val *src, Node *at) {
    ClassInfo *ci = cinfo(t);
    Ent *set = sfind(ci->scope, intern_str("operator="));
    Val *args[1];
    Ent *f;
    int amb;
    args[0] = src;
    if (set) {
        f = resolve_overload_obj(set, args, 1, t, &amb);
        if (f) {
            Val *r = call_function(f, fmt_str("(&%s)", dst), args, 1, at, 0);
            return r->c;
        }
    }
    if (!src->lval && !ci->user_move_assign && ci->implicit_massign) return fmt_str("(*%s(&%s, &%s))", ci->implicit_massign->cname, dst, src->c);
    if (ci->assign_deleted && !(ci->user_copy_assign && !src->lval))
        serr(at, "use of deleted function '%s::operator=(const %s&)': the copy assignment is deleted", ent_qualified_name(t->ent), t->ent->name->s);
    if (ci->implicit_assign) return fmt_str("(*%s(&%s, &%s))", ci->implicit_assign->cname, dst, src->c);
    return fmt_str("(%s = %s)", dst, src->c);
}

static const char *class_assign_text_fwd(Type *t, const char *dst, Val *src, Node *at) {
    return class_assign_text(t, dst, src, at);
}

static void gen_implicit_ctor(Ent *cls, Ent *f) {
    GenCtx g;
    int saved_pushed = cur_ctor_pushed;
    gen_begin(&g, f);
    emit_ctor_prologue(f, 0);
    emit_ctor_return();
    gen_end(&g, f);
    cur_ctor_pushed = saved_pushed;
}

static void gen_implicit_dtor(Ent *cls, Ent *f) {
    GenCtx g;
    gen_begin(&g, f);
    if (cls->ci->poly) emit_set_vptrs(cls);
    emit_member_dtors(f);
    emit_base_dtors(f);
    gen_end(&g, f);
}

static void gen_implicit_copy_move(Ent *cls, Ent *f, int mv) {
    GenCtx g;
    ClassInfo *ci = cls->ci;
    Field *fl;
    gen_begin(&g, f);
    {
        int bi;
        for (bi = 0; bi < ci->nbases; bi++) {
            BaseSpec *b = &ci->bases[bi];
            Val *sv = val_new(b->cls->type, fmt_str("(*__p0).%s", b->fname));
            const char *text;
            sv->lval = !mv;
                sv->xvalue = mv;
            text = copy_into(b->cls->type, fmt_str("(&this->%s)", b->fname), sv, 0);
            if (text) emitf("%s;\n", text);
        }
        if (ci->poly) emit_set_vptrs(cls);
    }
    for (fl = ci->fields; fl; fl = fl->next) {
        Type *t = fl->type;
        const char *src = fmt_str("(*__p0).%s", fl->name->s);
        const char *dst = fmt_str("this->%s", fl->name->s);
        if (ci->key == KW_union) break;
        if (t->kind == TY_ARRAY) {
            Type *el = strip_arrays(t);
            i64 n = 1;
            Type *x = t;
            while (x->kind == TY_ARRAY) { n *= x->len; x = x->base; }
            if (el->kind == TY_CLASS && type_nontrivial_copy(el)) {
                Val *sv = val_new(el, fmt_str("((%s)(*__p0).%s)[__i]", c_decl(ty_ptr(el), ""), fl->name->s));
                const char *text;
                sv->lval = !mv;
                sv->xvalue = mv;
                text = copy_into(el, fmt_str("(&((%s)this->%s)[__i])", c_decl(ty_ptr(el), ""), fl->name->s), sv, 0);
                emitf("{ long __i; for (__i = 0; __i < %lld; __i++) %s; }\n", n, text ? text : "(void)0");
            } else {
                emitf("memcpy(this->%s, (*__p0).%s, sizeof(this->%s));\n", fl->name->s, fl->name->s, fl->name->s);
            }
        } else if (t->kind == TY_CLASS) {
            Val *sv = val_new(t, src);
            const char *text;
            sv->lval = !mv;
                sv->xvalue = mv;
            text = copy_into(t, fmt_str("(&%s)", dst), sv, 0);
            if (text) emitf("%s;\n", text);
        } else emitf("%s = %s;\n", dst, src);
    }
    if (ci->key == KW_union) emit("*this = *__p0;\n");
    emit("return this;\n");
    gen_end(&g, f);
}

static void gen_implicit_assign_move(Ent *cls, Ent *f, int mv) {
    GenCtx g;
    ClassInfo *ci = cls->ci;
    Field *fl;
    gen_begin(&g, f);
    {
        int bi;
        for (bi = 0; bi < ci->nbases; bi++) {
            BaseSpec *b = &ci->bases[bi];
            Val *sv = val_new(b->cls->type, fmt_str("(*__p0).%s", b->fname));
            sv->lval = !mv;
                sv->xvalue = mv;
            emitf("%s;\n", class_assign_text(b->cls->type, fmt_str("this->%s", b->fname), sv, 0));
        }
    }
    for (fl = ci->fields; fl; fl = fl->next) {
        Type *t = fl->type;
        const char *src = fmt_str("(*__p0).%s", fl->name->s);
        const char *dst = fmt_str("this->%s", fl->name->s);
        if (ty_is_ref(t) || (t->cv & CV_CONST)) serr(0, "the implicit assignment of '%s' is deleted (member '%s')", ent_qualified_name(cls), fl->name->s);
        if (t->kind == TY_ARRAY) {
            Type *el = strip_arrays(t);
            i64 n = 1;
            Type *x = t;
            while (x->kind == TY_ARRAY) { n *= x->len; x = x->base; }
            if (el->kind == TY_CLASS) {
                Val *sv = val_new(el, fmt_str("((%s)(*__p0).%s)[__i]", c_decl(ty_ptr(el), ""), fl->name->s));
                sv->lval = !mv;
                sv->xvalue = mv;
                emitf("{ long __i; for (__i = 0; __i < %lld; __i++) %s; }\n", n,
                      class_assign_text(el, fmt_str("((%s)this->%s)[__i]", c_decl(ty_ptr(el), ""), fl->name->s), sv, 0));
            } else emitf("memcpy(this->%s, (*__p0).%s, sizeof(this->%s));\n", fl->name->s, fl->name->s, fl->name->s);
        } else if (t->kind == TY_CLASS) {
            Val *sv = val_new(t, src);
            sv->lval = !mv;
                sv->xvalue = mv;
            emitf("%s;\n", class_assign_text(t, dst, sv, 0));
        } else emitf("%s = %s;\n", dst, src);
    }
    if (ci->key == KW_union) emit("*this = *__p0;\n");
    emit("return this;\n");
    gen_end(&g, f);
}
static void gen_implicit_copy(Ent *cls, Ent *f) { gen_implicit_copy_move(cls, f, 0); }
static void gen_implicit_move(Ent *cls, Ent *f) { gen_implicit_copy_move(cls, f, 1); }
static void gen_implicit_assign(Ent *cls, Ent *f) { gen_implicit_assign_move(cls, f, 0); }
static void gen_implicit_massign(Ent *cls, Ent *f) { gen_implicit_assign_move(cls, f, 1); }

static void emit_implicit_members(Ent *cls) {
    ClassInfo *ci = cls->ci;
    Ent *d;
    if (ci->implicit_ctor && !(ci->implicit_ctor->flags & EF_DELETED)) {
        gen_implicit_ctor(cls, ci->implicit_ctor);
        ci->implicit_ctor->flags |= EF_DEFINED;
    }
    if (ci->implicit_copy) {
        gen_implicit_copy(cls, ci->implicit_copy);
        ci->implicit_copy->flags |= EF_DEFINED;
    }
    if (ci->implicit_assign) {
        gen_implicit_assign(cls, ci->implicit_assign);
        ci->implicit_assign->flags |= EF_DEFINED;
    }
    if (ci->implicit_move) {
        gen_implicit_move(cls, ci->implicit_move);
        ci->implicit_move->flags |= EF_DEFINED;
    }
    if (ci->implicit_massign) {
        gen_implicit_massign(cls, ci->implicit_massign);
        ci->implicit_massign->flags |= EF_DEFINED;
    }
    d = ci->dtor;
    if (d && (d->flags & EF_IMPLICIT) && !(d->flags & EF_DELETED)) {
        gen_implicit_dtor(cls, d);
        d->flags |= EF_DEFINED;
    }
    /* the prototypes of the generated functions, for the code of the class's own member functions */
}

static void process_toplevel_list(Node *n) {
    for (; n; n = n->next) {
        StrBuf *saved_temps = temps;
        process_decl(n);
        temps = saved_temps;
    }
}

/* ------------------------------------------------------------ statements */

static const char *cond_text(Val *v, Node *at) {
    if (is_class_operand(v)) v = convert(v, T_BOOL, at);
    v = rvalue(v);
    if (!ty_is_scalar(v->t)) serr(at, "the condition has type '%s', which is not a scalar", ty_str(v->t));
    if (v->t->kind == TY_NULLPTR) serr(at, "the condition is a null pointer constant");
    return v->c;
}

static void block_stmts(Node *list) {
    for (; list; list = list->next) stmt(list);
}

static void push_scope(void) {
    scope2_cur = scope2_new(SS_BLOCK, scope2_cur);
}

static void pop_scope(void) {
    emit_scope_dtors_text(scope2_cur);
    scope2_cur = scope2_cur->parent;
}


/* the text of a condition: the temporaries of the condition are destroyed after it is evaluated */
static const char *cond_full(Node *e, Node *n) {
    int mark = nftemps;
    const char *c = cond_text(expr(e), n);
    if (nftemps > mark) {
        const char *tmp = new_temp(T_INT);
        return fmt_str("(%s = %s, %s%s)", tmp, c, dtor_expr_from(mark), tmp);
    }
    return c;
}

static void mark_loop_scope(int cont) {
    scope2_cur->brk = 1;
    if (cont) scope2_cur->cont = 1;
}

/* is there a local object with a destructor between here and the function? */
static int pending_local_dtors(void) {
    Scope2 *sc;
    for (sc = scope2_cur; sc && sc->kind == SS_BLOCK; sc = sc->parent) {
        if (sc->ndtors > 0) return 1;
        if (sc->is_func) break;
    }
    return 0;
}

/* a local declaration of a statement: N_DECL */
static void local_decl(Node *d) {
    process_decl(d);
}

static void cond_decl(Node *d) {
    int saved = in_cond_decl;
    in_cond_decl = 1;
    local_decl(d);
    in_cond_decl = saved;
}

static void stmt(Node *n) {
    Val *v;
    Node *c;
    switch (n->kind) {
    case N_BLOCK:
        emit_line(n);
        emit("{\n");
        push_scope();
        block_stmts(n->a);
        pop_scope();
        emit("}\n");
        return;
    case N_EXPR_STMT:
        {
            int mark = nftemps;
            emit_line(n);
            v = expr(n->a);
            emitf("%s;\n", v->c);
            flush_temps_stmt(mark);
        }
        return;
    case N_NULL_STMT:
        emit(";\n");
        return;
    case N_DECL_STMT:
        emit_line(n);
        local_decl(n->a);
        return;
    case N_IF:
        emit_line(n);
        push_scope();
        emit("{\n");
        if (n->d) stmt(n->d);
        if (n->flags & IF_CONSTEXPR) {
            /* if constexpr: only the branch that the constant selects is analysed */
            Val *cv = expr(n->a);
            if (is_class_operand(cv)) cv = convert(cv, T_BOOL, n);
            if (!cv->is_const) serr(n, "the condition of 'if constexpr' is not a constant expression");
            if (cv->cv) {
                push_scope();
                stmt(n->b);
                pop_scope();
            } else if (n->c) {
                push_scope();
                stmt(n->c);
                pop_scope();
            }
            emit("}\n");
            pop_scope();
            return;
        }
        if (n->a->kind == N_DECL) {
            Node *id = n->a->b;
            Ent *e;
            cond_decl(n->a);
            e = sfind(scope2_cur, id_last(declarator_name_id_of(id->a))->name);
            emitf("if (%s) ", e->cname);
        } else {
            emitf("if (%s) ", cond_full(n->a, n));
        }
        emit("{\n");
        push_scope();
        stmt(n->b);
        pop_scope();
        emit("}\n");
        if (n->c) {
            emit("else {\n");
            push_scope();
            stmt(n->c);
            pop_scope();
            emit("}\n");
        }
        emit("}\n");
        pop_scope();
        return;
    case N_WHILE:
        emit_line(n);
        push_scope();
        if (n->a->kind == N_DECL) {
            Node *id = n->a->b;
            Ent *e;
            emit("for (;;) {\n");
            cond_decl(n->a);
            e = sfind(scope2_cur, id_last(declarator_name_id_of(id->a))->name);
            emitf("if (!%s) break;\n", e->cname);
        } else emitf("while (%s) {\n", cond_full(n->a, n));
        loop_depth++;
        push_scope();
        mark_loop_scope(1);
        stmt(n->b);
        pop_scope();
        loop_depth--;
        emit("}\n");
        pop_scope();
        return;
    case N_DO:
        emit_line(n);
        emit("do {\n");
        loop_depth++;
        push_scope();
        mark_loop_scope(1);
        stmt(n->a);
        pop_scope();
        loop_depth--;
        emitf("} while (%s);\n", cond_full(n->b, n));
        return;
    case N_FOR:
        emit_line(n);
        push_scope();
        emit("{\n");
        if (n->a) {
            if (n->a->kind == N_DECL) local_decl(n->a);
            else stmt(n->a);
        }
        emit("for (;");
        if (n->b) {
            if (n->b->kind == N_DECL) serr(n, "a declaration in the condition of 'for' is not supported yet");
            emitf(" %s", cond_full(n->b, n));
        }
        emit(";");
        if (n->c) {
            int mark = nftemps;
            const char *ic = expr(n->c)->c;
            emitf(" %s", nftemps > mark ? fmt_str("(%s, %s0)", ic, dtor_expr_from(mark)) : ic);
        }
        emit(") {\n");
        loop_depth++;
        push_scope();
        mark_loop_scope(1);
        stmt(n->d);
        pop_scope();
        loop_depth--;
        emit("}\n}\n");
        pop_scope();
        return;
    case N_RANGEFOR:
        if (n->a->kind == N_BINDING) {
            /* for (auto &[a, b] : r) body  is  for (auto &__rf : r) { auto &&[a, b] = __rf; body } */
            Node *b = n->a, *nf = anew(sizeof(Node)), *d = anew(sizeof(Node)), *idl = anew(sizeof(Node)), *dn = anew(sizeof(Node));
            Node *declr = dn, *bind = anew(sizeof(Node)), *blk = anew(sizeof(Node)), *body = anew(sizeof(Node)), *rid, *part;
            Name *rn = intern_str(fmt_str("__rf%d", label_count++));
            *nf = *n;
            nf->next = 0;
            d->kind = N_DECL;
            d->file = n->file;
            d->line = n->line;
            d->a = b->a;
            idl->kind = N_INITDECL;
            dn->kind = N_D_NAME;
            rid = anew(sizeof(Node));
            part = anew(sizeof(Node));
            rid->kind = N_ID;
            part->kind = N_PART;
            part->name = rn;
            rid->a = part;
            dn->a = rid;
            if (b->op == P_AMP || b->op == P_ANDAND) {
                Node *r = anew(sizeof(Node));
                r->kind = b->op == P_AMP ? N_D_REF : N_D_RREF;
                r->a = dn;
                declr = r;
            }
            idl->a = declr;
            d->b = idl;
            nf->a = d;
            *bind = *b;
            bind->next = 0;
            bind->op = P_ANDAND;
            bind->b = anew(sizeof(Node));
            *bind->b = *rid;
            {
                Node *bs = anew(sizeof(Node));
                Node *ts = anew(sizeof(Node));
                *ts = *b->a;
                ts->val = BT_AUTO;
                ts->flags &= ~(DS_CONST | DS_VOLATILE);
                ts->a = 0;
                bind->a = ts;
                bs->kind = N_DECL_STMT;
                bs->file = n->file;
                bs->line = n->line;
                bs->a = bind;
                *body = *n->d;
                body->next = 0;
                bs->next = body;
                blk->kind = N_BLOCK;
                blk->file = n->file;
                blk->line = n->line;
                blk->a = bs;
            }
            nf->d = blk;
            stmt(nf);
            return;
        }
        {
            int rmark = nftemps;
            Val *rng = expr(n->b);
            if (rng->braced) rng = braced_materialize(rng, braced_il_type(rng, n), n);
            Type *et, *vt;
            Node *id = n->a->b, *nid = 0, *dummy = 0;
            int dsflags;
            Name *vname;
            Type *base;
            const char *b, *e;
            int by_ref;
            int tmp = label_count++;
            emit_line(n);
            if (rng->t->kind == TY_CLASS) {
                range_for_class(n, rng);
                flush_temps_stmt(rmark);
                return;
            }
            if (rng->t->kind != TY_ARRAY || rng->t->len < 0) serr(n, "the range of the loop is not an array or a class with begin() and end()");
            et = rng->t->base;
            base = decl_specs_type(n->a->a, &dsflags);
            vt = apply_declarator(base, id->a, &nid, &dummy);
            by_ref = ty_is_ref(vt) || (contains_auto(vt) && vt->kind == TY_REF);
            vname = id_last(nid)->name;
            if (contains_auto(vt)) {
                Val *ev = val_new(et, "x");
                ev->lval = 1;
                vt = deduce_auto(vt, ev, n);
                by_ref = ty_is_ref(vt);
            }
            b = fmt_str("__b%d", tmp);
            e = fmt_str("__e%d", tmp);
            emitf("{\n%s = %s;\n%s = %s + %lld;\n", c_decl(ty_ptr(et), b), rng->c, c_decl(ty_ptr(et), e), rng->c, rng->t->len);
            emitf("for (; %s != %s; ++%s) {\n", b, e, b);
            push_scope();
            {
                Ent *ve = ent_new(EK_VAR, vname, vt, scope2_cur);
                ve->cname = vname->s;
                if (by_ref) {
                    if (!ty_same(vt->base, et)) serr(n, "cannot bind '%s' to an element of type '%s'", ty_str(vt), ty_str(et));
                    emitf("%s = %s;\n", c_decl(ty_ptr(vt->base), vname->s), b);
                } else {
                    Val *ev = val_new(et, fmt_str("(*%s)", b));
                    ev->lval = 1;
                    emitf("%s = %s;\n", c_decl(vt, vname->s), convert(ev, vt, n)->c);
                }
                scope_add(scope2_cur, ve);
            }
            loop_depth++;
            mark_loop_scope(1);
            stmt(n->d);
            loop_depth--;
            pop_scope();
            emit("}\n}\n");
            return;
        }
    case N_SWITCH:
        emit_line(n);
        push_scope();
        emit("{\n");
        if (n->d) stmt(n->d);
        if (n->a->kind == N_DECL) {
            Node *id = n->a->b;
            Ent *e;
            cond_decl(n->a);
            e = sfind(scope2_cur, id_last(declarator_name_id_of(id->a))->name);
            emitf("switch (%s) {\n", e->cname);
        } else {
            {
                int mark = nftemps;
                Val *sv = expr(n->a);
                if (is_class_operand(sv)) sv = convert(sv, T_INT, n);
                v = rvalue(sv);
                if (!ty_is_integral(v->t) && v->t->kind != TY_ENUM) serr(n, "the switch condition has type '%s'", ty_str(v->t));
                if (nftemps > mark) {
                    const char *tmp = new_temp(T_LONG);
                    emitf("%s = %s;\n%sswitch (%s) {\n", tmp, v->c, dtor_text_from(mark), tmp);
                } else emitf("switch (%s) {\n", v->c);
            }
        }
        loop_depth++;
        push_scope();
        scope2_cur->brk = 1;
        stmt(n->b);
        pop_scope();
        loop_depth--;
        emit("}\n}\n");
        pop_scope();
        return;
    case N_CASE:
        emit_line(n);
        v = expr(n->a);
        if (!v->is_const) serr(n, "the case label is not an integral constant expression");
        emitf("case %lld:\n", v->cv);
        if (n->b) stmt(n->b);
        return;
    case N_DEFAULT:
        emit_line(n);
        emit("default:\n");
        if (n->a) stmt(n->a);
        return;
    case N_BREAK:
        emit_line(n);
        emit_dtors_to_loop(0);
        emit("break;\n");
        return;
    case N_CONTINUE:
        emit_line(n);
        emit_dtors_to_loop(1);
        emit("continue;\n");
        return;
    case N_RETURN:
        {
            int mark = nftemps;
            emit_line(n);
            if (!cur_ret) serr(n, "'return' outside a function");
            if (contains_auto(cur_ret)) {
                /* a deduced return type: the first return statement decides */
                if (!n->a) cur_ret = subst_auto(cur_ret, T_VOID);
                else {
                    int m0 = nftemps;
                    Val *dv;
                    if (n->a->kind == N_INITLIST) serr(n, "cannot deduce the return type from a braced list");
                    dv = expr(n->a);
                    nftemps = m0;                    /* the probe constructed nothing */
                    cur_ret = deduce_auto(cur_ret, dv, n);
                }
            }
            if (!n->a) {
                if (cur_ret->kind != TY_VOID && !cur_is_ctor) serr(n, "'return' with no value in a function returning '%s'", ty_str(cur_ret));
                emit_dtors_to_function();
                if (cur_is_dtor) emit("goto __dtor_end;\n");
                else if (cur_is_ctor) emit_ctor_return();
                else emit("return;\n");
                return;
            }
            if (n->a->kind == N_INITLIST && cur_ret->kind == TY_CLASS) {
                Val *bv = braced_val(n->a);
                const char *text = list_init_text(ty_unqual(cur_ret), "__ret", bv, n);
                emitf("%s;\n", text);
                flush_temps_stmt(mark);
                emit_dtors_to_function();
                emit("return __ret;\n");
                return;
            }
            if (cur_ret->kind == TY_CLASS) {
                Val *cv;
                if (n->a->kind == N_ID) {
                    Ent *re = resolve_id(n->a, 0);
                    if (re && (re->flags & EF_NRVO)) {
                        emit_dtors_to_function();
                        if (type_needs_dtor(cur_ret)) emit("__CPPC_POP();\n");
                        emit("return __ret;\n");
                        return;
                    }
                }
                cv = expr_hinted(n->a, "__ret", cur_ret);
                if (dest_used) emitf("%s;\n", cv->c);
                else {
                    const char *text;
                    Val *src = cv;
                    if (n->a->kind == N_ID && cv->lval && cv->var && cv->var->kind == EK_VAR && !(cv->var->flags & EF_STATIC) && !ty_is_ref(cv->var->type)
                        && cv->var->scope && cv->var->scope->kind == SS_BLOCK && src->t->kind == TY_CLASS && ty_same(src->t, cur_ret)) {
                        /* return local; moves the local object */
                        src = anew(sizeof(Val));
                        *src = *cv;
                        src->lval = 0;
                        src->xvalue = 1;
                    }
                    if (!(src->t->kind == TY_CLASS && ty_same(src->t, cur_ret))) src = class_prvalue(cv, ty_unqual(cur_ret), n);
                    text = copy_into(ty_unqual(cur_ret), "__ret", src, n);
                    if (text) emitf("%s;\n", text);
                }
                dest_used = 0;
                flush_temps_stmt(mark);
                emit_dtors_to_function();
                emit("return __ret;\n");
                return;
            }
            v = n->a->kind == N_INITLIST ? braced_materialize(braced_val(n->a), cur_ret, n) : expr(n->a);
            if (cur_ret->kind == TY_VOID) {
                if (v->t->kind != TY_VOID) serr(n, "'return' with a value in a function returning void");
                emitf("%s;\n", v->c);
                flush_temps_stmt(mark);
                emit_dtors_to_function();
                emit("return;\n");
                return;
            }
            {
                const char *rt;
                int cleanup;
                if (ty_is_ref(cur_ret)) {
                    if (!(v->lval || (v->xvalue && cur_ret->kind == TY_RREF)) || !ty_same(v->t, cur_ret->base)) serr(n, "cannot bind a reference return to this expression");
                    rt = fmt_str("&%s", v->c);
                } else rt = convert(v, cur_ret, n)->c;
                cleanup = nftemps > mark || pending_local_dtors();
                if (!cleanup) {
                    emitf("return %s;\n", rt);
                    return;
                }
                {
                    Type *ct = ty_is_ref(cur_ret) ? ty_ptr(cur_ret->base) : ty_unqual(cur_ret);
                    const char *tmp = new_temp(ct);
                    emitf("%s = %s;\n", tmp, rt);
                    flush_temps_stmt(mark);
                    emit_dtors_to_function();
                    emitf("return %s;\n", tmp);
                }
            }
        }
        return;
    case N_GOTO:
        emit_line(n);
        emitf("goto %s;\n", n->name->s);
        return;
    case N_LABEL:
        emitf("%s:;\n", n->name->s);
        if (n->a) stmt(n->a);
        return;
    case N_TRY:
        try_stmt(n);
        return;
    }
    serr(n, "unsupported statement (%s)", node_kind_name(n->kind));
    (void)c;
}

/* ------------------------------------------------------------ classes: queries */

static ClassInfo *cinfo(Type *t) {
    return t->ent->ci;
}

static Type *strip_arrays(Type *t) {
    while (t->kind == TY_ARRAY) t = t->base;
    return t;
}

static int type_needs_dtor(Type *t) {
    t = strip_arrays(t);
    return t->kind == TY_CLASS && t->ent->ci && t->ent->ci->needs_dtor;
}

static int type_nontrivial_copy(Type *t) {
    t = strip_arrays(t);
    return t->kind == TY_CLASS && t->ent->ci && t->ent->ci->nontrivial_copy;
}

static int type_needs_ctor(Type *t) {
    t = strip_arrays(t);
    return t->kind == TY_CLASS && t->ent->ci && t->ent->ci->needs_ctor;
}

static int is_class_type(Type *t) {
    return t->kind == TY_CLASS;
}

static void require_complete(Type *t, Node *at) {
    Type *b = strip_arrays(t);
    if (b->kind == TY_CLASS && (!b->ent->ci || !b->ent->ci->complete))
        serr(at, "invalid use of incomplete type '%s'", ty_str(b));
}

/* ------------------------------------------------------------ base classes */

static const char *base_path_found;
static int base_path_count;

/* collects the paths from class 'from' to the base class 'to' (each base subobject is a member __bN) */
static void base_paths_dfs(Type *from, Ent *to, const char *prefix) {
    ClassInfo *ci = from->ent->ci;
    int i;
    if (!ci) return;
    for (i = 0; i < ci->nbases; i++) {
        BaseSpec *b = &ci->bases[i];
        const char *p = fmt_str("%s.%s", prefix, b->fname);
        if (b->cls == to) {
            if (base_path_count == 0) base_path_found = p;
            base_path_count++;
        } else base_paths_dfs(b->cls->type, to, p);
    }
}

/* the path ".__b1.__b0" of the base subobject of class 'to' in an object of class 'from'; "" if the same class;
   0 if 'to' is not a base.  *count (if not 0) receives the number of subobjects (more than one: ambiguous). */
static const char *base_path(Type *from, Ent *to, int *count) {
    if (from->ent == to) {
        if (count) *count = 1;
        return "";
    }
    base_path_found = 0;
    base_path_count = 0;
    base_paths_dfs(from, to, "");
    if (count) *count = base_path_count;
    return base_path_found;
}

static int derived_to_base(Type *from, Type *to) {
    int n;
    const char *p;
    if (from->kind != TY_CLASS || to->kind != TY_CLASS || from->ent == to->ent) return 0;
    p = base_path(from, to->ent, &n);
    return p != 0 && n == 1;
}

/* the C text of the base subobject of the object text obj (an lvalue) */
static const char *base_subobject(const char *obj, Type *from, Type *to, Node *at) {
    int n;
    const char *p = base_path(from, to->ent, &n);
    if (!p) serr(at, "'%s' is not a base of '%s'", ty_str(to), ty_str(from));
    if (n > 1) serr(at, "'%s' is an ambiguous base of '%s'", ty_str(to), ty_str(from));
    return fmt_str("(%s%s)", obj, p);
}

/* the C text of the pointer to the base subobject of the object that the pointer text p points to */
static const char *upcast_pointer(const char *p, Type *from, Type *to, Node *at) {
    int n;
    const char *path = base_path(from, to->ent, &n);
    const char *tmp;
    if (!path) serr(at, "'%s' is not a base of '%s'", ty_str(to), ty_str(from));
    if (n > 1) serr(at, "'%s' is an ambiguous base of '%s'", ty_str(to), ty_str(from));
    if (path[0] == 0) return p;
    tmp = new_temp(ty_ptr(from));
    return fmt_str("(%s = %s, %s ? &%s->%s : 0)", tmp, p, tmp, tmp, path + 1);
}

/* a Val for the base class part of a class value */
static Val *upcast_value(Val *v, Type *to, Node *at) {
    Val *r = val_new(ty_cv(ty_unqual(to), v->t->cv), base_subobject(v->c, v->t, to, at));
    r->lval = v->lval;
    r->is_temp = v->is_temp;
    return r;
}

static void require_concrete(Type *t, Node *at) {
    Type *b = strip_arrays(t);
    if (b->kind == TY_CLASS && b->ent->ci && b->ent->ci->abstract)
        serr(at, "cannot create an object of the abstract class '%s'", ty_str(b));
}

/* ------------------------------------------------------------ temporaries */

#define MAX_FTEMPS 512
static struct { const char *name; Type *cls; const char *guard; } ftemps[MAX_FTEMPS];

/* a temporary object of a class type of the function being written; destroyed at the end of the full expression */
static const char *new_class_temp(Type *cls) {
    const char *n = new_temp(ty_unqual(cls));
    if (cinfo(cls)->needs_dtor) {
        if (nftemps >= MAX_FTEMPS) serr(0, "too many temporaries in one expression");
        ftemps[nftemps].name = n;
        ftemps[nftemps].cls = cls;
        ftemps[nftemps].guard = 0;
        if (cond_depth > 0) {
            /* constructed only if this operand is evaluated: a flag tells the destruction */
            const char *g = fmt_str("__g%d", temp_count++);
            sb_printf(temps, "int %s = 0;\n", g);
            ftemps[nftemps].guard = g;
        }
        nftemps++;
    }
    return n;
}

/* the destructor calls of the temporaries created since 'mark' (the newest first); forgets them */
static const char *dtor_text_from(int mark) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    for (i = nftemps - 1; i >= mark; i--) {
        if (ftemps[i].guard)
            sb_printf(&sb, "if (%s) { %s = 0; %s(&%s); }\n", ftemps[i].guard, ftemps[i].guard, cinfo(ftemps[i].cls)->dtor->cname, ftemps[i].name);
        else sb_printf(&sb, "%s(&%s);\n", cinfo(ftemps[i].cls)->dtor->cname, ftemps[i].name);
    }
    nftemps = mark;
    return sb.s;
}

/* the same as a C expression: (dtor(&t1), dtor(&t2), 0) */
static const char *dtor_expr_from(int mark) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    for (i = nftemps - 1; i >= mark; i--) {
        if (ftemps[i].guard)
            sb_printf(&sb, "(%s ? (%s = 0, %s(&%s), 0) : 0), ", ftemps[i].guard, ftemps[i].guard, cinfo(ftemps[i].cls)->dtor->cname, ftemps[i].name);
        else sb_printf(&sb, "%s(&%s), ", cinfo(ftemps[i].cls)->dtor->cname, ftemps[i].name);
    }
    nftemps = mark;
    return sb.s;
}

/* the C pointer text of the object a class prvalue is built in: the hint of node n, or a new temporary */
static const char *prvalue_dest(Type *cls, Node *n) {
    if (dest_hint && n && n == dest_node && ty_same(cls, dest_type)) {
        const char *d = dest_hint;
        dest_hint = 0;
        dest_used = 1;
        return d;
    }
    {
        const char *name = new_class_temp(cls);
        if (nftemps > 0 && ftemps[nftemps - 1].name == name && ftemps[nftemps - 1].guard)
            return fmt_str("(%s = 1, &%s)", ftemps[nftemps - 1].guard, name);
        return fmt_str("(&%s)", name);
    }
}

/* ------------------------------------------------------------ special member access */

static Ent *class_ctors(Type *cls) {
    return sfind(cinfo(cls)->scope, nm_ctor);
}

/* a call of constructor f on the object at dest */
static const char *ctor_text(Ent *f, const char *dest, Val **args, int nargs, Node *at);

/* the pointer expression that constructs a cls object at dest from the arguments; 0 if nothing has to be done */
static const char *ctor_call(Type *cls, const char *dest, Val **args, int nargs, Node *at) {
    ClassInfo *ci = cinfo(cls);
    Ent *set, *f;
    int amb;
    int br = brace_init;
    brace_init = 0;
    require_complete(cls, at);
    if (br && nargs > 0) {
        /* list-initialization prefers a constructor that takes a std::initializer_list */
        Type *ilt = 0;
        Ent *ilc = find_il_ctor(cls, &ilt);
        if (ilc && il_viable(ilt, args, nargs)) {
            const char *d2 = prvalue_dest(ilt, 0);
            const char *t2 = il_dest_text(ilt, args, nargs, d2, at);
            Val *a[1];
            a[0] = val_new(ilt, fmt_str("(*%s)", t2));
            a[0]->is_temp = 1;
            return ctor_text(ilc, dest, a, 1, at);
        }
    }
    if (nargs == 1 && is_class_type(args[0]->t) && ty_same(args[0]->t, cls) && !args[0]->fn) {
        int rv = !args[0]->lval;
        if (rv && !ci->user_move_ctor && ci->implicit_move) return fmt_str("%s(%s, &%s)", ci->implicit_move->cname, dest, args[0]->c);
        if (!(rv && ci->user_move_ctor)) {
            if (ci->copy_deleted && !(rv && ci->implicit_move) && !ci->user_copy_ctor)
                serr(at, "use of deleted function '%s::%s(const %s&)': the copy constructor is deleted", ent_qualified_name(cls->ent), cls->ent->name->s, cls->ent->name->s);
            if (ci->implicit_copy && !ci->user_copy_ctor) return fmt_str("%s(%s, &%s)", ci->implicit_copy->cname, dest, args[0]->c);
        }
    }
    set = class_ctors(cls);
    if (!set) {
        if (nargs == 0) {
            if (ci->implicit_ctor) return fmt_str("%s(%s)", ci->implicit_ctor->cname, dest);
            return 0;
        }
        if (nargs == 1 && is_class_type(args[0]->t) && ty_same(args[0]->t, cls)) {
            /* trivially copyable: a struct copy */
            return fmt_str("(*(%s) = %s, (%s))", dest, args[0]->c, dest);
        }
        if (ci->aggregate) serr(at, "aggregate initialization needs a braced list for '%s'", ty_str(cls));
        serr(at, "no matching constructor for '%s'", ty_str(cls));
    }
    f = resolve_overload(set, args, nargs, &amb);
    if (!f) {
        if (nargs == 1 && is_class_type(args[0]->t) && ty_same(args[0]->t, cls) && !ci->user_copy_ctor) {
            if (ci->implicit_copy) return fmt_str("%s(%s, &%s)", ci->implicit_copy->cname, dest, args[0]->c);
            return fmt_str("(*(%s) = %s, (%s))", dest, args[0]->c, dest);
        }
        if (nargs == 0 && ci->implicit_ctor && !ci->user_ctor) return fmt_str("%s(%s)", ci->implicit_ctor->cname, dest);
        if (amb) serr(at, "call of overloaded constructor of '%s' is ambiguous", ty_str(cls));
        serr(at, "no matching constructor for '%s'", ty_str(cls));
    }
    if (f->flags & EF_DELETED) serr(at, "use of deleted function '%s'", ent_qualified_name(f));
    return ctor_text(f, dest, args, nargs, at);
}

static const char *default_construct_text(Type *cls, const char *dest, Node *at) {
    return ctor_call(cls, dest, 0, 0, at);
}

/* the C text that constructs dest from the object src of the same class */
static const char *copy_into(Type *cls, const char *dest, Val *src, Node *at) {
    Val *args[1];
    args[0] = src;
    return ctor_call(cls, dest, args, 1, at);
}

/* ------------------------------------------------------------ conversions of class objects */

static int in_class_conv = 0;

/* the conversion functions of a class */
static int find_conversion(Type *from, Type *to, Ent **best) {
    ClassInfo *ci = cinfo(from);
    Ent *f;
    int rank = CONV_NONE;
    Name *nm = intern_str("operator conv");
    Ent *set = ci->scope ? sfind(ci->scope, nm) : 0;
    *best = 0;
    for (f = set; f; f = f->overload) {
        Val probe;
        int r;
        Type *rt = ty_unqual(ty_strip_ref(f->type->base));
        memset(&probe, 0, sizeof probe);
        probe.t = rt;
        probe.lval = ty_is_ref(f->type->base);
        in_class_conv++;
        r = value_conversion(&probe, to);
        in_class_conv--;
        if (r != CONV_NONE && (rank == CONV_NONE || r < rank)) {
            rank = r;
            *best = f;
        }
    }
    return rank;
}

/* the rank of converting v to the type 'to' when a class is involved */
static int class_conversion(Val *v, Type *to) {
    Type *from = v->t;
    if (to->kind == TY_CLASS && from->kind == TY_CLASS && !ty_same(from, to) && derived_to_base(from, to)) return CONV_CONVERSION;
    if (in_class_conv) return CONV_NONE;
    if (lambda_invoker(from, to)) return CONV_USER;
    if (to->kind == TY_CLASS) {
        Ent *set, *f;
        ClassInfo *ci;
        if (!to->ent->ci || !to->ent->ci->complete) return CONV_NONE;
        ci = cinfo(to);
        set = class_ctors(to);
        in_class_conv++;
        for (f = set; f; f = f->overload) {
            int r;
            Ent *cf = f;
            if (f->kind == EK_FTEMPLATE) {
                Val *a1[1];
                if (f->flags & EF_EXPLICIT) continue;
                a1[0] = v;
                cf = deduce_function(f, a1, 1, 0);
                if (!cf) continue;
            }
            if ((cf->flags & EF_EXPLICIT) || cf->type->nparams < 1 || func_min_args(cf) > 1) continue;
            r = arg_conversion(v, cf->type->params[0]);
            if (r != CONV_NONE && r <= CONV_CONVERSION) {
                in_class_conv--;
                return CONV_USER;
            }
        }
        in_class_conv--;
        if (from->kind == TY_CLASS) {
            Ent *cf;
            if (find_conversion(from, to, &cf) != CONV_NONE && cf) return CONV_USER;
        }
        (void)ci;
        return CONV_NONE;
    }
    if (from->kind == TY_CLASS && from->ent->ci && from->ent->ci->complete) {
        Ent *cf;
        int r = find_conversion(from, to, &cf);
        if (r != CONV_NONE && r <= CONV_CONVERSION) return CONV_USER;
    }
    return CONV_NONE;
}

/* a class prvalue of type cls from the value a (copy, move, or a user-defined conversion) */
static Val *class_prvalue(Val *a, Type *cls, Node *at) {
    Val *r;
    const char *dest, *text;
    Type *tu = ty_unqual(cls);
    if (a->t->kind == TY_CLASS && !ty_same(a->t, tu) && derived_to_base(a->t, tu)) {
        /* slicing: the base part is copied */
        a = upcast_value(a, tu, at);
        a->is_temp = 0;
        a->lval = 1;
    }
    if (a->t->kind == TY_CLASS && ty_same(a->t, tu)) {
        if (a->is_temp && !a->lval) return a;
        dest = prvalue_dest(tu, 0);
        text = copy_into(tu, dest, a, at);
        r = val_new(tu, fmt_str("(*%s)", text ? text : dest));
        r->is_temp = 1;
        return r;
    }
    /* a converting constructor or a conversion function */
    {
        Val *args[1];
        Ent *cf = 0;
        args[0] = a;
        if (a->t->kind == TY_CLASS && find_conversion(a->t, tu, &cf) != CONV_NONE && cf) {
            Val *cv = call_function(cf, fmt_str("(&%s)", a->c), 0, 0, at, 0);
            return class_prvalue(cv, tu, at);
        }
        dest = prvalue_dest(tu, 0);
        text = ctor_call(tu, dest, args, 1, at);
        r = val_new(tu, fmt_str("(*%s)", text ? text : dest));
        r->is_temp = 1;
        return r;
    }
}

/* v converted to 'to' when a class is involved (to is not a reference) */
static Val *convert_class_any(Val *v, Type *to, Node *at) {
    Type *tu = ty_unqual(to);
    Ent *cf = 0;
    if (tu->kind == TY_CLASS) return class_prvalue(v, tu, at);
    {
        Ent *inv = lambda_invoker(v->t, tu);
        if (inv) {
            Val *r = val_new(tu, inv->cname);
            r->static_init = 1;
            return r;
        }
    }
    /* from a class to something else */
    if (v->t->kind == TY_CLASS && find_conversion(v->t, tu, &cf) != CONV_NONE && cf) {
        Val *cv = call_function(cf, fmt_str("(&%s)", v->c), 0, 0, at, 0);
        return convert(cv, tu, at);
    }
    serr(at, "cannot convert '%s' to '%s'", ty_str(v->t), ty_str(to));
    return 0;
}

/* ------------------------------------------------------------ calls */

/* the C parameter declarations of the function: this, the object for a class result, then the parameters */
static const char *c_param_type_text(Type *pt, const char *name) {
    if (ty_is_ref(pt)) return c_decl(ty_ptr(pt->base), name);
    if (pt->kind == TY_CLASS) return c_decl(ty_ptr(pt), name);
    return c_decl(pt, name);
}

static int is_nonstatic_member(Ent *f) {
    return (f->flags & EF_MEMBER) && !(f->flags & EF_STATIC);
}

/* the C return type of a function: pointers for references, classes (the result object) and constructors */
static Type *c_return_type(Ent *f, Type *ft) {
    Type *r = ft->base;
    if (f && (f->flags & EF_CTOR)) return ty_ptr(ty_unqual(f->owner->type));
    if (ty_is_ref(r)) return ty_ptr(r->base);
    if (r->kind == TY_CLASS) return ty_ptr(r);
    return r;
}

/* the C text of a call of the function f; this_c is the pointer to the object of a member function */
static Val *call_function(Ent *f, const char *this_c, Val **args, int nargs, Node *at, Node *hint) {
    Type *ft;
    StrBuf sb;
    const char *argtext, *dest = 0, *call;
    Type *rt;
    use_at = at;
    use_function(f);
    ft = f->type;
    if (contains_auto(ft->base)) serr(at, "use of '%s' before its return type is deduced", ent_qualified_name(f));
    rt = ft->base;
    int virt = f->vowner && !force_nonvirtual && this_c && is_nonstatic_member(f) && !(f->flags & EF_CTOR);
    const char *tmp = 0;
    sb_init(&sb);
    if (f->flags & EF_DELETED) serr(at, "use of deleted function '%s'", ent_qualified_name(f));
    if (rt->kind == TY_CLASS) {
        require_complete(rt, at);
        dest = prvalue_dest(rt, hint);
        sb_puts(&sb, dest);
    }
    if (is_nonstatic_member(f)) {
        if (!this_c) serr(at, "cannot call the member function '%s' without an object", ent_qualified_name(f));
        if (sb.len) sb_puts(&sb, ", ");
        if (virt) {
            tmp = new_temp(ty_ptr(f->vowner->type));
            sb_puts(&sb, tmp);
        } else sb_puts(&sb, this_c);
    }
    argtext = arg_list_text(ft, f, args, nargs, at);
    if (argtext[0]) {
        if (sb.len) sb_puts(&sb, ", ");
        sb_puts(&sb, argtext);
    }
    if (virt) {
        const char *objp = this_c;
        Type *fty = vtable_fn_type(f, f->vowner->type);
        if (f->owner != f->vowner) objp = upcast_pointer(this_c, f->owner->type, f->vowner->type, at);
        call = fmt_str("(%s = %s, ((%s)(*(void ***)%s)[%d])(%s))", tmp, objp, c_decl(ty_ptr(fty), ""), tmp, f->vslot + 1, sb.s);
    } else call = fmt_str("%s(%s)", f->cname, sb.s);
    if (rt->kind == TY_CLASS) {
        Val *r = val_new(ty_unqual(rt), fmt_str("(*%s)", call));
        r->is_temp = 1;
        return r;
    }
    return call_result(rt, call);
}

static const char *ctor_text(Ent *f, const char *dest, Val **args, int nargs, Node *at) {
    const char *argtext;
    use_at = at;
    use_function(f);
    argtext = arg_list_text(f->type, f, args, nargs, at);
    return fmt_str("%s(%s%s%s)", f->cname, dest, argtext[0] ? ", " : "", argtext);
}

/* ------------------------------------------------------------ the driver */

static const char *out_dir = "";

static const char *line_file_name(int file) {
    const char *p;
    size_t n = strlen(out_dir);
    if (file < 0 || file >= nfiles) return "?";
    p = files[file].path;
    if (n > 0 && strncmp(p, out_dir, n) == 0) return p + n;
    return p;
}

