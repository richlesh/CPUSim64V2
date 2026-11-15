/*
 * types.c - the C++ types of the semantic analysis: construction, comparison,
 * the C spelling of a type, and name mangling.
 *
 * Output types: bool is _Bool, enums are int, float and long double are
 * double, references are pointers, nullptr_t is void *; const and volatile
 * are not written (the C11 compiler is not asked to check them).
 */

enum {
    TY_VOID = 1, TY_BOOL, TY_CHAR, TY_SCHAR, TY_UCHAR, TY_WCHAR, TY_CHAR16, TY_CHAR32,
    TY_SHORT, TY_USHORT, TY_INT, TY_UINT, TY_LONG, TY_ULONG, TY_LLONG, TY_ULLONG,
    TY_FLOAT, TY_DOUBLE, TY_LDOUBLE, TY_NULLPTR,
    TY_PTR, TY_REF, TY_RREF, TY_ARRAY, TY_FUNC, TY_ENUM, TY_CLASS,
    TY_AUTO,
    TY_ERROR
};

#define CV_CONST 1
#define CV_VOLATILE 2

typedef struct Ent Ent;
typedef struct Type Type;
struct Type {
    int kind;
    int cv;
    Type *base;          /* pointee, referent, element, return type */
    i64 len;             /* array length, -1 if unknown */
    Type **params;
    int nparams;
    int variadic;
    Ent *ent;            /* the enum or class */
};

static Type *builtin_types[TY_ERROR + 1];

static Type *ty_builtin(int kind) {
    if (!builtin_types[kind]) {
        builtin_types[kind] = anew(sizeof(Type));
        builtin_types[kind]->kind = kind;
    }
    return builtin_types[kind];
}

#define T_VOID ty_builtin(TY_VOID)
#define T_BOOL ty_builtin(TY_BOOL)
#define T_CHAR ty_builtin(TY_CHAR)
#define T_INT ty_builtin(TY_INT)
#define T_UINT ty_builtin(TY_UINT)
#define T_LONG ty_builtin(TY_LONG)
#define T_ULONG ty_builtin(TY_ULONG)
#define T_LLONG ty_builtin(TY_LLONG)
#define T_DOUBLE ty_builtin(TY_DOUBLE)
#define T_NULLPTR ty_builtin(TY_NULLPTR)

static Type *ty_derive(int kind, Type *base) {
    Type *t = anew(sizeof(Type));
    t->kind = kind;
    t->base = base;
    t->len = -1;
    return t;
}

static Type *ty_ptr(Type *base) { return ty_derive(TY_PTR, base); }
/* reference collapsing: a reference to a reference is one reference (T& & = T&, T&& & = T&, T&& && = T&&) */
static Type *ty_ref(Type *base) {
    if (base->kind == TY_REF || base->kind == TY_RREF) base = base->base;
    return ty_derive(TY_REF, base);
}
static Type *ty_rref(Type *base) {
    if (base->kind == TY_REF) return base;
    if (base->kind == TY_RREF) return base;
    return ty_derive(TY_RREF, base);
}

static Type *ty_array(Type *base, i64 len) {
    Type *t = ty_derive(TY_ARRAY, base);
    t->len = len;
    return t;
}

static Type *ty_func(Type *ret, Type **params, int n, int variadic) {
    Type *t = ty_derive(TY_FUNC, ret);
    t->params = params;
    t->nparams = n;
    t->variadic = variadic;
    return t;
}

/* a copy with extra cv-qualifiers */
static Type *ty_cv(Type *t, int cv) {
    Type *c;
    if ((t->cv | cv) == t->cv) return t;
    c = anew(sizeof(Type));
    *c = *t;
    c->cv |= cv;
    return c;
}

static Type *ty_unqual(Type *t) {
    Type *c;
    if (!t->cv) return t;
    c = anew(sizeof(Type));
    *c = *t;
    c->cv = 0;
    return c;
}

static int ty_is_ref(Type *t) { return t->kind == TY_REF || t->kind == TY_RREF; }
static int ty_is_integral(Type *t) { return t->kind >= TY_BOOL && t->kind <= TY_ULLONG; }
static int ty_is_float(Type *t) { return t->kind >= TY_FLOAT && t->kind <= TY_LDOUBLE; }
static int ty_is_enum(Type *t) { return t->kind == TY_ENUM; }
static int ty_is_arith(Type *t) { return ty_is_integral(t) || ty_is_float(t); }
static int ty_is_scalar(Type *t) {
    return ty_is_arith(t) || t->kind == TY_PTR || t->kind == TY_ENUM || t->kind == TY_NULLPTR;
}
static int ty_is_unsigned(Type *t) {
    switch (t->kind) {
    case TY_BOOL: case TY_UCHAR: case TY_CHAR16: case TY_CHAR32: case TY_USHORT: case TY_UINT:
    case TY_ULONG: case TY_ULLONG:
        return 1;
    }
    return 0;
}

/* removes a reference */
static Type *ty_strip_ref(Type *t) {
    return ty_is_ref(t) ? t->base : t;
}

/* the conversion rank of an integral type (C++ [conv.rank]) */
static int ty_int_rank(Type *t) {
    switch (t->kind) {
    case TY_BOOL: return 1;
    case TY_CHAR: case TY_SCHAR: case TY_UCHAR: return 2;
    case TY_SHORT: case TY_USHORT: return 3;
    case TY_WCHAR: case TY_CHAR16: case TY_CHAR32: return 3;
    case TY_INT: case TY_UINT: return 4;
    case TY_LONG: case TY_ULONG: return 5;
    case TY_LLONG: case TY_ULLONG: return 6;
    }
    return 4;
}

/* integral promotion: types smaller than int become int; every value fits since int is a word */
static Type *ty_promote(Type *t) {
    if (ty_is_enum(t)) return T_INT;
    if (ty_is_integral(t) && ty_int_rank(t) < 4) return T_INT;
    if (t->kind == TY_FLOAT) return T_DOUBLE;
    return ty_unqual(t);
}

/* the usual arithmetic conversions of two arithmetic (or enum) types */
static Type *ty_common(Type *a, Type *b) {
    int ra, rb, ua, ub;
    a = ty_promote(a);
    b = ty_promote(b);
    if (ty_is_float(a) || ty_is_float(b)) {
        if (a->kind == TY_LDOUBLE || b->kind == TY_LDOUBLE) return ty_builtin(TY_LDOUBLE);
        return T_DOUBLE;
    }
    ra = ty_int_rank(a);
    rb = ty_int_rank(b);
    ua = ty_is_unsigned(a);
    ub = ty_is_unsigned(b);
    if (ra == rb) {
        if (ua) return a;
        if (ub) return b;
        return a;
    }
    /* different ranks: the higher rank wins (every type can represent the lower one's values here) */
    return ra > rb ? a : b;
}

static int ty_equal(Type *a, Type *b) {
    int i;
    if (a == b) return 1;
    if (a->kind != b->kind || a->cv != b->cv) return 0;
    switch (a->kind) {
    case TY_PTR: case TY_REF: case TY_RREF:
        return ty_equal(a->base, b->base);
    case TY_ARRAY:
        return a->len == b->len && ty_equal(a->base, b->base);
    case TY_FUNC:
        if (a->nparams != b->nparams || a->variadic != b->variadic || !ty_equal(a->base, b->base)) return 0;
        for (i = 0; i < a->nparams; i++)
            if (!ty_equal(a->params[i], b->params[i])) return 0;
        return 1;
    case TY_ENUM: case TY_CLASS:
        return a->ent == b->ent;
    }
    return 1;
}

/* the same type ignoring the top-level cv */
static int ty_same(Type *a, Type *b) {
    return ty_equal(ty_unqual(a), ty_unqual(b));
}

/* the parameter type after array-to-pointer and function-to-pointer adjustment */
static Type *ty_adjust_param(Type *t) {
    if (t->kind == TY_ARRAY) return ty_ptr(t->base);
    if (t->kind == TY_FUNC) return ty_ptr(t);
    return ty_unqual(t);
}

/* ------------------------------------------------------------ spelling */

static const char *ty_builtin_name(int kind) {
    switch (kind) {
    case TY_VOID: return "void";
    case TY_BOOL: return "bool";
    case TY_CHAR: return "char";
    case TY_SCHAR: return "signed char";
    case TY_UCHAR: return "unsigned char";
    case TY_WCHAR: return "wchar_t";
    case TY_CHAR16: return "char16_t";
    case TY_CHAR32: return "char32_t";
    case TY_SHORT: return "short";
    case TY_USHORT: return "unsigned short";
    case TY_INT: return "int";
    case TY_UINT: return "unsigned int";
    case TY_LONG: return "long";
    case TY_ULONG: return "unsigned long";
    case TY_LLONG: return "long long";
    case TY_ULLONG: return "unsigned long long";
    case TY_FLOAT: return "float";
    case TY_DOUBLE: return "double";
    case TY_LDOUBLE: return "long double";
    case TY_NULLPTR: return "std::nullptr_t";
    }
    return "?";
}

static const char *ent_qualified_name(Ent *e);

/* the C++ spelling of a type, for messages */
static void ty_spell(StrBuf *sb, Type *t) {
    int i;
    switch (t->kind) {
    case TY_PTR:
        ty_spell(sb, t->base);
        sb_puts(sb, "*");
        break;
    case TY_REF:
        ty_spell(sb, t->base);
        sb_puts(sb, "&");
        break;
    case TY_RREF:
        ty_spell(sb, t->base);
        sb_puts(sb, "&&");
        break;
    case TY_ARRAY:
        ty_spell(sb, t->base);
        if (t->len >= 0) sb_printf(sb, "[%lld]", t->len);
        else sb_puts(sb, "[]");
        break;
    case TY_FUNC:
        ty_spell(sb, t->base);
        sb_puts(sb, "(");
        for (i = 0; i < t->nparams; i++) {
            if (i) sb_puts(sb, ", ");
            ty_spell(sb, t->params[i]);
        }
        if (t->variadic) sb_puts(sb, t->nparams ? ", ..." : "...");
        sb_puts(sb, ")");
        break;
    case TY_ENUM: case TY_CLASS:
        sb_puts(sb, ent_qualified_name(t->ent));
        break;
    default:
        sb_puts(sb, ty_builtin_name(t->kind));
    }
    if (t->cv & CV_CONST) sb_puts(sb, " const");
    if (t->cv & CV_VOLATILE) sb_puts(sb, " volatile");
}

static const char *ty_str(Type *t) {
    StrBuf sb;
    sb_init(&sb);
    ty_spell(&sb, t);
    return sb.s;
}

/* the C spelling of the base types */
static const char *c_base_name(Type *t) {
    switch (t->kind) {
    case TY_VOID: return "void";
    case TY_BOOL: return "_Bool";
    case TY_CHAR: return "char";
    case TY_SCHAR: return "signed char";
    case TY_UCHAR: return "unsigned char";
    case TY_WCHAR: return "int";
    case TY_CHAR16: case TY_CHAR32: return "unsigned int";
    case TY_SHORT: return "short";
    case TY_USHORT: return "unsigned short";
    case TY_INT: case TY_ENUM: return "int";
    case TY_UINT: return "unsigned int";
    case TY_LONG: return "long";
    case TY_ULONG: return "unsigned long";
    case TY_LLONG: return "long long";
    case TY_ULLONG: return "unsigned long long";
    case TY_FLOAT: case TY_DOUBLE: case TY_LDOUBLE: return "double";
    case TY_NULLPTR: return "void *";
    }
    return "int";
}

static const char *class_c_name(Ent *e);
static const char *class_tag(Ent *e);

/* the C declaration of a name of type t (name may be empty for an abstract declarator) */
static const char *c_decl(Type *t, const char *inner) {
    StrBuf sb;
    int i;
    switch (t->kind) {
    case TY_PTR: case TY_REF: case TY_RREF:
        {
            StrBuf in;
            sb_init(&in);
            sb_puts(&in, "*");
            sb_puts(&in, inner);
            if (t->base->kind == TY_ARRAY || t->base->kind == TY_FUNC) {
                StrBuf p;
                sb_init(&p);
                sb_puts(&p, "(");
                sb_puts(&p, in.s);
                sb_puts(&p, ")");
                return c_decl(t->base, p.s);
            }
            return c_decl(t->base, in.s);
        }
    case TY_ARRAY:
        {
            StrBuf in;
            sb_init(&in);
            sb_puts(&in, inner);
            if (t->len >= 0) sb_printf(&in, "[%lld]", t->len);
            else sb_puts(&in, "[]");
            return c_decl(t->base, in.s);
        }
    case TY_FUNC:
        {
            StrBuf in;
            sb_init(&in);
            sb_puts(&in, inner);
            sb_puts(&in, "(");
            for (i = 0; i < t->nparams; i++) {
                if (i) sb_puts(&in, ", ");
                sb_puts(&in, c_decl(ty_adjust_param(t->params[i]), ""));
            }
            if (t->variadic) sb_puts(&in, t->nparams ? ", ..." : "...");
            else if (t->nparams == 0) sb_puts(&in, "void");
            sb_puts(&in, ")");
            return c_decl(t->base, in.s);
        }
    case TY_CLASS:
        sb_init(&sb);
        sb_puts(&sb, class_tag(t->ent));
        if (inner[0]) {
            sb_putc(&sb, ' ');
            sb_puts(&sb, inner);
        }
        return sb.s;
    }
    sb_init(&sb);
    sb_puts(&sb, c_base_name(t));
    if (inner[0]) {
        sb_putc(&sb, ' ');
        sb_puts(&sb, inner);
    }
    return sb.s;
}

/* ------------------------------------------------------------ mangling */

/* appends the mangled form of a type: a short code per type */
static void mangle_type(StrBuf *sb, Type *t) {
    int i;
    switch (t->kind) {
    case TY_VOID: sb_puts(sb, "v"); break;
    case TY_BOOL: sb_puts(sb, "b"); break;
    case TY_CHAR: sb_puts(sb, "c"); break;
    case TY_SCHAR: sb_puts(sb, "sc"); break;
    case TY_UCHAR: sb_puts(sb, "uc"); break;
    case TY_WCHAR: sb_puts(sb, "w"); break;
    case TY_CHAR16: sb_puts(sb, "c16"); break;
    case TY_CHAR32: sb_puts(sb, "c32"); break;
    case TY_SHORT: sb_puts(sb, "s"); break;
    case TY_USHORT: sb_puts(sb, "us"); break;
    case TY_INT: sb_puts(sb, "i"); break;
    case TY_UINT: sb_puts(sb, "ui"); break;
    case TY_LONG: sb_puts(sb, "l"); break;
    case TY_ULONG: sb_puts(sb, "ul"); break;
    case TY_LLONG: sb_puts(sb, "ll"); break;
    case TY_ULLONG: sb_puts(sb, "ull"); break;
    case TY_FLOAT: sb_puts(sb, "f"); break;
    case TY_DOUBLE: sb_puts(sb, "d"); break;
    case TY_LDOUBLE: sb_puts(sb, "ld"); break;
    case TY_NULLPTR: sb_puts(sb, "n"); break;
    case TY_PTR: sb_puts(sb, "P"); mangle_type(sb, t->base); break;
    case TY_REF: sb_puts(sb, "R"); mangle_type(sb, t->base); break;
    case TY_RREF: sb_puts(sb, "O"); mangle_type(sb, t->base); break;
    case TY_ARRAY: sb_printf(sb, "A%lld_", t->len); mangle_type(sb, t->base); break;
    case TY_FUNC:
        sb_puts(sb, "F");
        mangle_type(sb, t->base);
        for (i = 0; i < t->nparams; i++) mangle_type(sb, t->params[i]);
        if (t->variadic) sb_puts(sb, "z");
        sb_puts(sb, "E");
        break;
    case TY_ENUM: case TY_CLASS:
        sb_printf(sb, "T%s_", class_c_name(t->ent));
        break;
    }
    if (t->cv & CV_CONST) sb_puts(sb, "K");
}
