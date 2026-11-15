/*
 * sema_ops.c - the types of the operators (spec 11.12 to 11.17) and the
 * folding of constant operations.
 */

/* the type of an operation on two numeric (or Boolean) operands: + - *
   Mod and the operand type of the relational and bitwise operators.
   Boolean counts as SByte, except that two Booleans give Short. */
static Type *num_dominant(Type *a, Type *b) {
    int ka = a->kind, kb = b->kind;
    if (ka == TY_BOOLEAN && kb == TY_BOOLEAN) return ty[TY_SHORT];
    if (ka == TY_BOOLEAN) ka = TY_SBYTE, a = ty[TY_SBYTE];
    if (kb == TY_BOOLEAN) kb = TY_SBYTE, b = ty[TY_SBYTE];
    if (ka == TY_DOUBLE || kb == TY_DOUBLE) return ty[TY_DOUBLE];
    if (ka == TY_SINGLE || kb == TY_SINGLE) return ty[TY_SINGLE];
    if (ka == TY_DECIMAL || kb == TY_DECIMAL) return ty[TY_DECIMAL];
    if (ty_signed(a) == ty_signed(b)) return ty_bits(a) >= ty_bits(b) ? a : b;
    {
        Type *s = ty_signed(a) ? a : b, *u = ty_signed(a) ? b : a;
        if (ty_bits(s) > ty_bits(u)) return s;
        switch (ty_bits(u)) {
        case 8: return ty[TY_SHORT];
        case 16: return ty[TY_INTEGER];
        case 32: return ty[TY_LONG];
        }
        return ty[TY_DECIMAL];
    }
}

static int num_or_bool(Type *t) { return ty_numeric(t) || t->kind == TY_BOOLEAN; }

static const char *op_spelling(int op) { return tok_spelling_of(op); }

static void op_undefined2(Node *at, int op, Type *a, Type *b) {
    serr(at, "Operator '%s' is not defined for types '%s' and '%s'.", op_spelling(op), type_name(a), type_name(b));
}

static void op_undefined1(Node *at, int op, Type *a) {
    serr(at, "Operator '%s' is not defined for type '%s'.", op_spelling(op), type_name(a));
}

static void late_bound(Node *at, int op) {
    serr(at, "Late binding is not supported: operator '%s' with an operand of type 'Object'.", op_spelling(op));
}

static void no_decimal(Node *at, Type *t) {
    if (t && t->kind == TY_DECIMAL) serr(at, "Type 'Decimal' is not supported.");
    if (t && t->kind == TY_DATE) serr(at, "Type 'Date' is not supported.");
}

/* String (or Char) mixed with numbers under Option Strict Off: the
   operation is done in Double (spec 11.12.2) */
static Type *str_num(Node *at, Type *a, Type *b, int op) {
    Type *s = (a->kind == TY_STRING) ? a : b;
    Type *o = s == a ? b : a;
    if (!num_or_bool(o) && o->kind != TY_STRING) op_undefined2(at, op, a, b);
    if (opts->strict) serr(at, "Option Strict On disallows implicit conversions from '%s' to 'Double'.", type_name(s));
    return ty[TY_DOUBLE];
}

/* the operand type of a binary operator; *res receives the result type.
   Shifts: the operand type is that of the left operand (the right one is
   converted to Integer). */
static Type *binop_types(Node *at, int op, Type *a, Type *b, Type **res) {
    Type *t = 0;
    int as = a->kind == TY_STRING, bs = b->kind == TY_STRING;
    if (a->kind == TY_OBJECT || b->kind == TY_OBJECT) {
        if (op == K_Is || op == K_IsNot) {
            *res = ty[TY_BOOLEAN];
            return ty[TY_OBJECT];
        }
        if (op == P_AMP || op == K_AndAlso || op == K_OrElse) {
            /* converted to String / Boolean: narrowing from Object */
        } else late_bound(at, op);
    }
    switch (op) {
    case P_PLUS:
        if ((as || a->kind == TY_CHAR) && (bs || b->kind == TY_CHAR)) t = ty[TY_STRING];
        else if (as || bs) t = str_num(at, a, b, op);
        else if (num_or_bool(a) && num_or_bool(b)) t = num_dominant(a, b);
        *res = t;
        break;
    case P_MINUS: case P_STAR: case K_Mod:
        if (as || bs) t = str_num(at, a, b, op);
        else if (num_or_bool(a) && num_or_bool(b)) t = num_dominant(a, b);
        *res = t;
        break;
    case P_SLASH:
        if (as || bs) t = str_num(at, a, b, op);
        else if (num_or_bool(a) && num_or_bool(b)) {
            t = num_dominant(a, b);
            if (!ty_float(t) && t->kind != TY_DECIMAL) t = ty[TY_DOUBLE];
        }
        *res = t;
        break;
    case P_BACKSLASH:
        if (as || bs) {
            str_num(at, a, b, op);
            t = ty[TY_LONG];
        } else if (num_or_bool(a) && num_or_bool(b)) {
            t = num_dominant(a, b);
            if (!ty_integral(t)) t = ty[TY_LONG];
        }
        *res = t;
        break;
    case P_CARET:
        if (as || bs) t = str_num(at, a, b, op);
        else if (num_or_bool(a) && num_or_bool(b)) t = ty[TY_DOUBLE];
        *res = t;
        break;
    case P_AMP:
        if (a->kind == TY_ARRAY || b->kind == TY_ARRAY) break;
        if ((ty_user(a) && a->kind != TY_ENUM) || (ty_user(b) && b->kind != TY_ENUM)) break;
        if (a->kind == TY_NOTHING && b->kind == TY_NOTHING) break;
        t = ty[TY_STRING];
        *res = t;
        break;
    case P_SHL: case P_SHR:
        if (as) {
            if (opts->strict) serr(at, "Option Strict On disallows implicit conversions from 'String' to 'Long'.");
            t = ty[TY_LONG];
        } else if (a->kind == TY_BOOLEAN) t = ty[TY_SHORT];
        else if (ty_integral(a)) t = a;
        else if (ty_numeric(a)) t = ty[TY_LONG];
        if (t && !(num_or_bool(b) || bs)) t = 0;
        *res = t;
        break;
    case P_EQ: case P_NE: case P_LT: case P_GT: case P_LE: case P_GE:
        if ((as || a->kind == TY_CHAR) && (bs || b->kind == TY_CHAR))
            t = (a->kind == TY_CHAR && b->kind == TY_CHAR) ? ty[TY_CHAR] : ty[TY_STRING];
        else if (as || bs) t = str_num(at, a, b, op);
        else if (a->kind == TY_BOOLEAN && b->kind == TY_BOOLEAN) t = a;
        else if (num_or_bool(a) && num_or_bool(b)) t = num_dominant(a, b);
        else if (a->kind == TY_NOTHING && (b->kind == TY_NOTHING || ty_ref(b))) t = b->kind == TY_NOTHING ? ty[TY_STRING] : b;
        else if (b->kind == TY_NOTHING && ty_ref(a)) t = a;
        if (t && t->kind == TY_ARRAY) t = 0;
        *res = ty[TY_BOOLEAN];
        break;
    case K_Like:
        if ((as || a->kind == TY_CHAR || a->kind == TY_NOTHING || num_or_bool(a)) &&
            (bs || b->kind == TY_CHAR || b->kind == TY_NOTHING || num_or_bool(b)))
            t = ty[TY_STRING];
        *res = ty[TY_BOOLEAN];
        break;
    case K_Is: case K_IsNot:
        if (!ty_ref(a) || !ty_ref(b)) {
            Type *bad = ty_ref(a) ? b : a;
            serr(at, "'%s' operator does not accept operands of type '%s'. Operands must be reference or nullable types.",
                 op_spelling(op), type_name(bad));
        }
        t = ty[TY_OBJECT];
        *res = ty[TY_BOOLEAN];
        break;
    case K_And: case K_Or: case K_Xor:
        if (a->kind == TY_BOOLEAN && b->kind == TY_BOOLEAN) t = a;
        else if (as || bs) {
            str_num(at, a, b, op);
            t = ty[TY_LONG];
        } else if (num_or_bool(a) && num_or_bool(b)) {
            t = num_dominant(a, b);
            if (!ty_integral(t)) t = ty[TY_LONG];
        }
        *res = t;
        break;
    case K_AndAlso: case K_OrElse:
        t = ty[TY_BOOLEAN];
        if (!(num_or_bool(a) || as || a->kind == TY_OBJECT) || !(num_or_bool(b) || bs || b->kind == TY_OBJECT)) t = 0;
        *res = t;
        break;
    }
    if (!t) op_undefined2(at, op, a, b);
    no_decimal(at, t);
    return t;
}

/* the operand (and result) type of a unary operator */
static Type *unop_type(Node *at, int op, Type *a) {
    Type *t = 0;
    if (a->kind == TY_OBJECT) late_bound(at, op);
    if (a->kind == TY_STRING) {
        if (opts->strict) {
            serr(at, "Option Strict On disallows implicit conversions from 'String' to '%s'.", op == K_Not ? "Long" : "Double");
        }
        return op == K_Not ? ty[TY_LONG] : ty[TY_DOUBLE];
    }
    switch (op) {
    case P_MINUS:
        switch (a->kind) {
        case TY_BOOLEAN: case TY_BYTE: t = ty[TY_SHORT]; break;
        case TY_USHORT: t = ty[TY_INTEGER]; break;
        case TY_UINTEGER: t = ty[TY_LONG]; break;
        case TY_ULONG: t = ty[TY_DECIMAL]; break;
        default: if (ty_numeric(a)) t = a;
        }
        break;
    case P_PLUS:
        if (a->kind == TY_BOOLEAN) t = ty[TY_SHORT];
        else if (ty_numeric(a)) t = a;
        break;
    case K_Not:
        if (a->kind == TY_BOOLEAN || ty_integral(a)) t = a;
        else if (ty_numeric(a)) t = ty[TY_LONG];
        break;
    }
    if (!t) op_undefined1(at, op, a);
    no_decimal(at, t);
    return t;
}

/* ------------------------------------------------------------ folding */

static int add_ovf(i64 a, i64 b, i64 *r) {
    if ((b > 0 && a > I64_MAX - b) || (b < 0 && a < I64_MIN - b)) return 1;
    *r = a + b;
    return 0;
}

static int mul_ovf(i64 a, i64 b, i64 *r) {
    i64 p;
    if (a == 0 || b == 0) { *r = 0; return 0; }
    if (a == -1) { if (b == I64_MIN) return 1; *r = -b; return 0; }
    if (b == -1) { if (a == I64_MIN) return 1; *r = -a; return 0; }
    p = (i64)((u64)a * (u64)b);
    if (p / b != a) return 1;
    *r = p;
    return 0;
}

static void div_zero(Node *at) {
    serr(at, "Division by zero occurred while evaluating this expression.");
}

/* folds an integral operation in type t (both operands of type t) */
static Node *fold_int(Node *at, int op, Type *t, Node *x, Node *y) {
    Node *r = lit_node(N_INT, t, at);
    if (ty_unsigned(t)) {
        u64 a = x->ival, b = y->ival, v = 0;
        switch (op) {
        case P_PLUS: v = a + b; if (v < a) not_representable(at, t); break;
        case P_MINUS: if (b > a) not_representable(at, t); v = a - b; break;
        case P_STAR: v = a * b; if (a && v / a != b) not_representable(at, t); break;
        case P_BACKSLASH: if (!b) div_zero(at); v = a / b; break;
        case K_Mod: if (!b) div_zero(at); v = a % b; break;
        case K_And: v = a & b; break;
        case K_Or: v = a | b; break;
        case K_Xor: v = a ^ b; break;
        default: return 0;
        }
        if (!fits_unsigned(t, v)) not_representable(at, t);
        r->ival = v;
    } else {
        i64 a = (i64)x->ival, b = (i64)y->ival, v = 0;
        switch (op) {
        case P_PLUS: if (add_ovf(a, b, &v)) not_representable(at, t); break;
        case P_MINUS:
            if (b == I64_MIN) { if (a >= 0) not_representable(at, t); v = a - b; }
            else if (add_ovf(a, -b, &v)) not_representable(at, t);
            break;
        case P_STAR: if (mul_ovf(a, b, &v)) not_representable(at, t); break;
        case P_BACKSLASH:
            if (!b) div_zero(at);
            if (a == I64_MIN && b == -1) not_representable(at, t);
            v = a / b;
            break;
        case K_Mod:
            if (!b) div_zero(at);
            v = (b == -1) ? 0 : a % b;
            break;
        case K_And: v = a & b; break;
        case K_Or: v = a | b; break;
        case K_Xor: v = a ^ b; break;
        default: return 0;
        }
        if (!fits_signed(t, v)) not_representable(at, t);
        r->ival = (u64)v;
    }
    return r;
}

static int str_compare(const char *a, const char *b) {
    int c = strcmp(a ? a : "", b ? b : "");
    return c < 0 ? -1 : c > 0 ? 1 : 0;
}

static Node *bool_node(Node *at, int v) {
    Node *r = lit_node(N_BOOL, ty[TY_BOOLEAN], at);
    r->ival = v != 0;
    return r;
}

static int rel_result(int op, int c) {
    switch (op) {
    case P_EQ: return c == 0;
    case P_NE: return c != 0;
    case P_LT: return c < 0;
    case P_GT: return c > 0;
    case P_LE: return c <= 0;
    }
    return c >= 0;
}

/* folds a binary operation whose operands are constants of type t (shifts:
   the right operand is Integer); NULL if it is not folded */
static Node *fold_binary(Node *at, int op, Type *t, Node *x, Node *y) {
    Node *r;
    if (op == P_SHL || op == P_SHR) {
        int b = ty_bits(t), n = (int)((i64)y->ival & (b - 1));
        r = lit_node(N_INT, t, at);
        if (op == P_SHL) r->ival = wrap_to(t, x->ival << n);
        else if (ty_unsigned(t)) r->ival = x->ival >> n;
        else r->ival = (u64)((i64)x->ival >> n);
        return r;
    }
    if (op == P_EQ || op == P_NE || op == P_LT || op == P_GT || op == P_LE || op == P_GE) {
        int c;
        if (t->kind == TY_STRING) {
            if (opts->compare_text) return 0;
            c = str_compare(x->text, y->text);
        } else if (ty_float(t)) {
            if (x->dval != x->dval || y->dval != y->dval) return bool_node(at, op == P_NE);
            c = x->dval < y->dval ? -1 : x->dval > y->dval ? 1 : 0;
        } else if (ty_unsigned(t) || t->kind == TY_CHAR) c = x->ival < y->ival ? -1 : x->ival > y->ival ? 1 : 0;
        else if (t->kind == TY_BOOLEAN) {
            /* True is -1, so True < False */
            i64 a = x->ival ? -1 : 0, b = y->ival ? -1 : 0;
            c = a < b ? -1 : a > b ? 1 : 0;
        } else {
            i64 a = (i64)x->ival, b = (i64)y->ival;
            c = a < b ? -1 : a > b ? 1 : 0;
        }
        return bool_node(at, rel_result(op, c));
    }
    if (t->kind == TY_BOOLEAN) {
        int a = x->ival != 0, b = y->ival != 0;
        switch (op) {
        case K_And: case K_AndAlso: return bool_node(at, a && b);
        case K_Or: case K_OrElse: return bool_node(at, a || b);
        case K_Xor: return bool_node(at, a != b);
        }
        return 0;
    }
    if (t->kind == TY_STRING) {
        StrBuf b;
        if (op != P_AMP && op != P_PLUS) return 0;
        sb_init(&b);
        sb_puts(&b, x->text ? x->text : "");
        sb_puts(&b, y->text ? y->text : "");
        r = lit_node(N_STRING, t, at);
        r->text = b.s;
        return r;
    }
    if (ty_integral(t)) return fold_int(at, op, t, x, y);
    if (ty_float(t)) {
        double a = x->dval, b = y->dval, v;
        switch (op) {
        case P_PLUS: v = a + b; break;
        case P_MINUS: v = a - b; break;
        case P_STAR: v = a * b; break;
        case P_SLASH: v = a / b; break;
        case K_Mod: v = fmod(a, b); break;
        case P_CARET: v = pow(a, b); break;
        default: return 0;
        }
        r = lit_node(N_FLOAT, t, at);
        r->dval = t->kind == TY_SINGLE ? round_single(v) : v;
        return r;
    }
    return 0;
}

static Node *fold_unary(Node *at, int op, Type *t, Node *x) {
    Node *r;
    if (t->kind == TY_BOOLEAN) return op == K_Not ? bool_node(at, !x->ival) : 0;
    if (ty_float(t)) {
        if (op == K_Not) return 0;
        r = lit_node(N_FLOAT, t, at);
        r->dval = op == P_MINUS ? -x->dval : x->dval;
        return r;
    }
    if (!ty_integral(t)) return 0;
    r = lit_node(N_INT, t, at);
    if (op == K_Not) r->ival = wrap_to(t, ~x->ival);
    else if (op == P_PLUS) r->ival = x->ival;
    else {
        if (ty_unsigned(t)) {
            if (x->ival) not_representable(at, t);
            r->ival = 0;
        } else {
            i64 v = (i64)x->ival;
            if (v == I64_MIN || !fits_signed(t, -v)) not_representable(at, t);
            r->ival = (u64)(-v);
        }
    }
    return r;
}
