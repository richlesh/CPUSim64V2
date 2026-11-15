/*
 * types.c - VB.NET types and the conversions between them (Visual Basic
 * Language Specification 8, "Conversions").
 */

static Type prim_types[TY_NPRIM];
Type *ty[TY_NPRIM];

static const char *prim_name[TY_NPRIM] = {
    "Void", "Boolean", "SByte", "Byte", "Short", "UShort", "Integer", "UInteger",
    "Long", "ULong", "Decimal", "Single", "Double", "Date", "Char", "String", "Object",
    "Nothing"
};

static const char *prim_sysname[TY_NPRIM] = {
    "Void", "Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
    "Int64", "UInt64", "Decimal", "Single", "Double", "DateTime", "Char", "String", "Object",
    "Object"
};

void types_init(void) {
    int k;
    for (k = 0; k < TY_NPRIM; k++) {
        prim_types[k].kind = k;
        prim_types[k].name = prim_name[k];
        prim_types[k].sysname = prim_sysname[k];
        ty[k] = &prim_types[k];
    }
}

Type *ty_array(Type *elem, int rank) {
    Type *t;
    for (t = elem->arrays; t; t = t->next_array)
        if (t->rank == rank) return t;
    t = xalloc(sizeof(Type));
    t->kind = TY_ARRAY;
    t->elem = elem;
    t->rank = rank;
    t->next_array = elem->arrays;
    elem->arrays = t;
    return t;
}

/* Integer, Integer(), Integer(,), Integer()(,): the outermost array first */
static void type_name_to(StrBuf *b, Type *t) {
    Type *e = t;
    while (e->kind == TY_ARRAY) e = e->elem;
    sb_puts(b, e->name);
    for (e = t; e->kind == TY_ARRAY; e = e->elem) {
        int i;
        sb_putc(b, '(');
        for (i = 1; i < e->rank; i++) sb_putc(b, ',');
        sb_putc(b, ')');
    }
}

const char *type_name(Type *t) {
    StrBuf b;
    if (!t) return "Void";
    if (t->kind != TY_ARRAY) return t->name;
    sb_init(&b);
    type_name_to(&b, t);
    return b.s;
}

int ty_integral(Type *t) { return t && t->kind >= TY_SBYTE && t->kind <= TY_ULONG; }

int ty_signed(Type *t) {
    return t && (t->kind == TY_SBYTE || t->kind == TY_SHORT || t->kind == TY_INTEGER || t->kind == TY_LONG);
}

int ty_unsigned(Type *t) {
    return t && (t->kind == TY_BYTE || t->kind == TY_USHORT || t->kind == TY_UINTEGER || t->kind == TY_ULONG);
}

int ty_float(Type *t) { return t && (t->kind == TY_SINGLE || t->kind == TY_DOUBLE); }

int ty_numeric(Type *t) { return ty_integral(t) || ty_float(t) || (t && t->kind == TY_DECIMAL); }

int ty_bits(Type *t) {
    switch (t->kind) {
    case TY_SBYTE: case TY_BYTE: return 8;
    case TY_SHORT: case TY_USHORT: return 16;
    case TY_INTEGER: case TY_UINTEGER: return 32;
    }
    return 64;
}

int ty_ref(Type *t) {
    return t && (t->kind == TY_STRING || t->kind == TY_OBJECT || t->kind == TY_ARRAY || t->kind == TY_NOTHING ||
                 t->kind == TY_CLASS || t->kind == TY_INTERFACE);
}

/* the class a derives from the class b (or is b) */
static int derives(Sym *a, Sym *b) {
    for (; a; a = a->base) if (a == b) return 1;
    return 0;
}

/* the interface a is (or inherits) the interface b */
static int iface_derives(Sym *a, Sym *b) {
    int i;
    if (a == b) return 1;
    for (i = 0; i < a->ifaces.n; i++) if (iface_derives(a->ifaces.v[i], b)) return 1;
    return 0;
}

/* the class or structure t implements the interface i */
int implements(Sym *t, Sym *i) {
    int k;
    for (; t; t = t->base)
        for (k = 0; k < t->ifaces.n; k++) if (iface_derives(t->ifaces.v[k], i)) return 1;
    return 0;
}

int ty_user(Type *t) {
    return t && (t->kind == TY_ENUM || t->kind == TY_CLASS || t->kind == TY_STRUCT || t->kind == TY_INTERFACE);
}

Type *ty_under(Type *t) { return (t && t->kind == TY_ENUM) ? t->elem : t; }

/* the numeric widening conversions (spec 8.8); the order of the kinds
   SByte .. Double is that of the enum */
static int num_widens(int f, int t) {
    switch (f) {
    case TY_SBYTE:
        return t == TY_SHORT || t == TY_INTEGER || t == TY_LONG || t == TY_DECIMAL || t == TY_SINGLE || t == TY_DOUBLE;
    case TY_BYTE:
        return t >= TY_SHORT && t <= TY_DOUBLE;
    case TY_SHORT:
        return t == TY_INTEGER || t == TY_LONG || t == TY_DECIMAL || t == TY_SINGLE || t == TY_DOUBLE;
    case TY_USHORT:
        return t >= TY_INTEGER && t <= TY_DOUBLE;
    case TY_INTEGER:
        return t == TY_LONG || t == TY_DECIMAL || t == TY_SINGLE || t == TY_DOUBLE;
    case TY_UINTEGER:
        return t >= TY_LONG && t <= TY_DOUBLE;
    case TY_LONG: case TY_ULONG:
        return t == TY_DECIMAL || t == TY_SINGLE || t == TY_DOUBLE;
    case TY_DECIMAL:
        return t == TY_SINGLE || t == TY_DOUBLE;
    case TY_SINGLE:
        return t == TY_DOUBLE;
    }
    return 0;
}

/* the conversion from one type to another: identity, widening, narrowing,
   or none (spec 8.8 and 8.9); not counting the constant expression
   conversions, which the caller handles */
int conv_class(Type *f, Type *t) {
    if (f == t) return CV_IDENT;
    if (!f || !t) return CV_NONE;
    if (f->kind == TY_NOTHING) return CV_WIDEN;
    if (t->kind == TY_OBJECT) return CV_WIDEN;
    if (f->kind == TY_OBJECT) return t->kind == TY_NOTHING ? CV_NONE : CV_NARROW;
    if (f->kind == TY_ENUM || t->kind == TY_ENUM) {
        int c;
        if (f->kind == TY_ENUM && t->kind == TY_ENUM) return CV_NARROW;
        if (f->kind == TY_ENUM) {
            c = conv_class(f->elem, t);
            return c == CV_IDENT ? CV_WIDEN : c;
        }
        c = conv_class(f, t->elem);
        return c == CV_NONE ? CV_NONE : CV_NARROW;
    }
    if (f->kind == TY_CLASS && t->kind == TY_CLASS) {
        if (derives(f->sym, t->sym)) return CV_WIDEN;
        if (derives(t->sym, f->sym)) return CV_NARROW;
        return CV_NONE;
    }
    if (t->kind == TY_INTERFACE && f->kind == TY_ARRAY) {
        /* T() to IEnumerable(Of T) and IEnumerable (through ArrayWrapper(Of T)) */
        Sym *ts = t->sym;
        if (f->rank != 1) return CV_NONE;
        if (ts->tmpl && ts->tmpl->name == intern_word("ienumerable") && ts->ntargs == 1 && ts->targs[0] == f->elem)
            return CV_WIDEN;
        if (!ts->tmpl && ts->name == intern_word("ienumerable")) return CV_WIDEN;
        return CV_NONE;
    }
    if (t->kind == TY_INTERFACE) {
        if (f->kind == TY_INTERFACE) return iface_derives(f->sym, t->sym) ? CV_WIDEN : CV_NARROW;
        if (f->kind == TY_CLASS || f->kind == TY_STRUCT) {
            if (implements(f->sym, t->sym)) return CV_WIDEN;
            return (f->kind == TY_CLASS && !(f->sym->mods & M_NOTINHERITABLE)) ? CV_NARROW : CV_NONE;
        }
        return CV_NONE;
    }
    if (f->kind == TY_INTERFACE) {
        if (t->kind == TY_CLASS) return (!(t->sym->mods & M_NOTINHERITABLE) || implements(t->sym, f->sym)) ? CV_NARROW : CV_NONE;
        if (t->kind == TY_STRUCT) return implements(t->sym, f->sym) ? CV_NARROW : CV_NONE;
        return CV_NONE;
    }
    if (f->kind == TY_CLASS || t->kind == TY_CLASS || f->kind == TY_STRUCT || t->kind == TY_STRUCT)
        return CV_NONE;
    if (f->kind == TY_ARRAY || t->kind == TY_ARRAY) {
        if (f->kind == TY_ARRAY && t->kind == TY_ARRAY) {
            int c;
            if (f->rank != t->rank) return CV_NONE;
            /* array covariance for reference element types */
            if (!ty_ref(f->elem) || !ty_ref(t->elem)) return CV_NONE;
            c = conv_class(f->elem, t->elem);
            return c == CV_IDENT ? CV_IDENT : c;
        }
        if (f->kind == TY_ARRAY && f->rank == 1 && f->elem->kind == TY_CHAR && t->kind == TY_STRING) return CV_WIDEN;
        if (t->kind == TY_ARRAY && t->rank == 1 && t->elem->kind == TY_CHAR && f->kind == TY_STRING) return CV_NARROW;
        return CV_NONE;
    }
    if (ty_numeric(f) && ty_numeric(t)) return num_widens(f->kind, t->kind) ? CV_WIDEN : CV_NARROW;
    switch (f->kind) {
    case TY_BOOLEAN:
        return (ty_numeric(t) || t->kind == TY_STRING) ? CV_NARROW : CV_NONE;
    case TY_CHAR:
        return t->kind == TY_STRING ? CV_WIDEN : CV_NONE;
    case TY_STRING:
        return (ty_numeric(t) || t->kind == TY_BOOLEAN || t->kind == TY_CHAR || t->kind == TY_DATE) ? CV_NARROW : CV_NONE;
    case TY_DATE:
        return t->kind == TY_STRING ? CV_NARROW : CV_NONE;
    }
    if (ty_numeric(f)) return (t->kind == TY_BOOLEAN || t->kind == TY_STRING) ? CV_NARROW : CV_NONE;
    return CV_NONE;
}
