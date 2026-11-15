/*
 * sema_const.c - constant values: literal nodes, the type of a literal,
 * range checks and the conversion of constants (spec 11.2, "Constant
 * Expressions").
 *
 * A constant is a literal node (N_INT, N_FLOAT, N_BOOL, N_CHAR, N_STRING,
 * N_NOTHING) with its type.  An integral value is kept in ival: sign-extended
 * for the signed types, zero-extended for the unsigned ones.
 */

#include <math.h>

#define I64_MAX 0x7FFFFFFFFFFFFFFFLL
#define I64_MIN (-I64_MAX - 1)

static Node *copy_const(Node *c, Node *at);      /* sema_expr.c */

static Node *lit_node(int kind, Type *t, Node *at) {
    Node *n = xalloc(sizeof(Node));
    n->kind = kind;
    n->file = at->file;
    n->line = at->line;
    n->col = at->col;
    n->type = t;
    n->cls = EC_VALUE;
    return n;
}

static int is_const(Node *e) {
    switch (e->kind) {
    case N_INT: case N_FLOAT: case N_BOOL: case N_CHAR: case N_STRING: case N_NOTHING:
        return e->type != 0;
    }
    return 0;
}

/* truncates bits to the width of an integral type, sign-extending the
   signed ones */
static u64 wrap_to(Type *t, u64 v) {
    int b = ty_bits(t);
    if (b == 64) return v;
    v &= (1ULL << b) - 1;
    if (ty_signed(t) && (v >> (b - 1)) & 1) v |= ~((1ULL << b) - 1);
    return v;
}

/* does the integer (signed if neg_ok and the value is negative as i64)
   fit the integral type t? */
static int fits_signed(Type *t, i64 v) {
    int b = ty_bits(t);
    if (ty_unsigned(t)) return v >= 0 && (b == 64 || (u64)v < (1ULL << b));
    if (b == 64) return 1;
    return v >= -(1LL << (b - 1)) && v <= (1LL << (b - 1)) - 1;
}

static int fits_unsigned(Type *t, u64 v) {
    int b = ty_bits(t);
    if (ty_unsigned(t)) return b == 64 || v < (1ULL << b);
    return v <= (u64)((b == 64) ? I64_MAX : (1LL << (b - 1)) - 1);
}

/* the value of an integral constant fits t */
static int int_fits(Node *c, Type *t) {
    if (ty_unsigned(c->type)) return fits_unsigned(t, c->ival);
    return fits_signed(t, (i64)c->ival);
}

static double int_to_double(Node *c) {
    if (ty_unsigned(c->type)) return (double)c->ival;
    return (double)(i64)c->ival;
}

/* rounds to the nearest integer, halves to even (as CInt does) */
static double round_even(double d) {
    double f = floor(d), r = d - f;
    if (r > 0.5) return f + 1;
    if (r < 0.5) return f;
    return fmod(f, 2.0) == 0.0 ? f : f + 1;
}

/* rounds to the nearest IEEE binary32 value (Single), as the run-time
   library's vbrt_to_single does */
static double round_single(double x) {
    int e, p;
    double m, r;
    if (x != x || x == 0.0 || x - x != 0.0) return x;      /* NaN, 0, infinities */
    m = frexp(x, &e);
    p = 24;
    if (e - 1 < -126) p = 24 - (-126 - (e - 1));
    r = ldexp(round_even(ldexp(m, p)), e - p);
    if (fabs(r) >= 3.4028235677973366e+38) return x > 0 ? HUGE_VAL : -HUGE_VAL;
    return r;
}

static void not_representable(Node *at, Type *t) {
    serr(at, "Constant expression not representable in type '%s'.", type_name(t));
}

/* the text of a constant converted to String (Boolean, Char and String
   constants only: the numeric formats are left to run time) */
static const char *const_str(Node *c) {
    StrBuf b;
    if (c->kind == N_STRING) return c->text;
    if (c->kind == N_NOTHING) return 0;
    sb_init(&b);
    if (c->kind == N_CHAR) sb_put_cp(&b, (int)c->ival);
    else if (c->kind == N_BOOL) sb_puts(&b, c->ival ? "True" : "False");
    else return 0;
    return b.s;
}

/* converts the constant c to type t, or returns NULL if the conversion is
   not done at compile time; an out-of-range value is an error */
static Node *const_convert(Node *c, Type *t, Node *at) {
    Node *r;
    Type *f = c->type;
    if (f == t) return c;
    if (f->kind == TY_ENUM || t->kind == TY_ENUM) {
        /* through the underlying type */
        Node tmp = *c;
        if (f->kind == TY_ENUM) {
            tmp.type = f->elem;
            if (t->kind == TY_OBJECT || t->kind == TY_STRING) return 0;
            if (t->kind != TY_ENUM) {
                r = const_convert(&tmp, t, at);
                return r == &tmp ? copy_const(&tmp, at) : r;
            }
        }
        if (!ty_integral(tmp.type) && tmp.type->kind != TY_NOTHING) return 0;
        r = const_convert(&tmp, t->elem, at);
        if (!r) return 0;
        if (r == &tmp) r = copy_const(&tmp, at);
        r->type = t;
        return r;
    }
    if (c->kind == N_NOTHING) {
        /* Nothing converted to a value type is its default value */
        if (ty_integral(t)) { r = lit_node(N_INT, t, at); return r; }
        if (ty_float(t)) { r = lit_node(N_FLOAT, t, at); return r; }
        if (t->kind == TY_BOOLEAN) return lit_node(N_BOOL, t, at);
        if (t->kind == TY_CHAR) return lit_node(N_CHAR, t, at);
        r = lit_node(N_NOTHING, t, at);
        return r;
    }
    if (ty_integral(t)) {
        r = lit_node(N_INT, t, at);
        if (ty_integral(f)) {
            if (!int_fits(c, t)) not_representable(at, t);
            r->ival = wrap_to(t, c->ival);
            return r;
        }
        if (f->kind == TY_BOOLEAN) {
            r->ival = c->ival ? wrap_to(t, ~0ULL) : 0;     /* True is -1 */
            return r;
        }
        if (ty_float(f)) {
            double d = round_even(c->dval);
            if (d != d) not_representable(at, t);
            if (ty_unsigned(t)) {
                if (d < 0 || d >= 18446744073709551616.0) not_representable(at, t);
                r->ival = d >= 9223372036854775808.0 ? (u64)(i64)(d - 9223372036854775808.0) + (1ULL << 63) : (u64)(i64)d;
                if (!fits_unsigned(t, r->ival)) not_representable(at, t);
            } else {
                if (d < -9223372036854775808.0 || d >= 9223372036854775808.0) not_representable(at, t);
                if (!fits_signed(t, (i64)d)) not_representable(at, t);
                r->ival = (u64)(i64)d;
            }
            return r;
        }
        return 0;
    }
    if (ty_float(t)) {
        r = lit_node(N_FLOAT, t, at);
        if (ty_integral(f)) r->dval = int_to_double(c);
        else if (ty_float(f)) {
            r->dval = c->dval;
            if (t->kind == TY_SINGLE && fabs(r->dval) > 3.4028234663852886E+38 && r->dval == r->dval &&
                fabs(r->dval) != HUGE_VAL)
                not_representable(at, t);
        } else if (f->kind == TY_BOOLEAN) r->dval = c->ival ? -1.0 : 0.0;
        else return 0;
        if (t->kind == TY_SINGLE) r->dval = round_single(r->dval);
        return r;
    }
    if (t->kind == TY_BOOLEAN) {
        r = lit_node(N_BOOL, t, at);
        if (ty_integral(f)) r->ival = c->ival != 0;
        else if (ty_float(f)) r->ival = c->dval != 0.0;
        else return 0;
        return r;
    }
    if (t->kind == TY_STRING) {
        const char *s = const_str(c);
        if (!s) return 0;
        r = lit_node(N_STRING, t, at);
        r->text = s;
        return r;
    }
    return 0;
}

/* a constant that can be converted to t by a constant expression
   conversion (spec 8.8: an integral constant to a narrower integral type
   when the value fits, a Double constant to Single) */
static int const_widens(Node *c, Type *t) {
    if (!is_const(c)) return 0;
    if (ty_integral(c->type) && ty_integral(t)) return int_fits(c, t);
    if (c->type->kind == TY_DOUBLE && t->kind == TY_SINGLE) return 1;
    return 0;
}

/* the type and value of an integer literal (spec 2.4.2); out of range is
   "Overflow." */
static void type_int_literal(Node *n) {
    u64 v = n->ival;
    int dec = n->aux == 10 || n->aux == 0;
    Type *t;
    switch (n->tchar) {
    case LS_S: t = ty[TY_SHORT]; break;
    case LS_US: t = ty[TY_USHORT]; break;
    case LS_I: t = ty[TY_INTEGER]; break;
    case LS_UI: t = ty[TY_UINTEGER]; break;
    case LS_L: t = ty[TY_LONG]; break;
    case LS_UL: t = ty[TY_ULONG]; break;
    default:
        if (dec) t = v <= 0x7FFFFFFFULL ? ty[TY_INTEGER] : ty[TY_LONG];
        else t = v <= 0xFFFFFFFFULL ? ty[TY_INTEGER] : ty[TY_LONG];
        break;
    }
    if (ty_unsigned(t) || !dec) {
        int b = ty_bits(t);
        /* hex, octal and binary literals give the bit pattern */
        if (b < 64 && v >= (1ULL << b)) serr(n, "Overflow.");
        n->ival = ty_signed(t) ? wrap_to(t, v) : v;
    } else {
        if (v > (u64)I64_MAX || !fits_signed(t, (i64)v)) serr(n, "Overflow.");
    }
    n->type = t;
    n->cls = EC_VALUE;
}
