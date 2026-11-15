/*
 * sema_expr.c - expressions (spec 11): names, members, calls, indexing,
 * operators, conversions, array literals and interpolated strings.
 *
 * bind() returns the bound expression (a new node when it is folded or
 * converted) with its classification (Node.cls); value() makes a value of
 * it, calling a method group without arguments.
 */

#define BF_INVOKE 1         /* the target of an argument list */

static Node *bind_new(Node *e);
static Node *make_me(Node *at);
static Node *with_object(Node *at);
static void note_capture(Sym *s, Node *at);        /* sema_lambda.c */
static Node *lambda_me(Node *at);
static Sym *root_method(void);
static Sym *delegate_invoke(Type *t);
static int is_pending(Node *e);
static Node *make_delegate(Node *e, Type *t);

static void eval_const(Sym *s, Node *at);
static Sym *implicit_local(Node *at);

/* ------------------------------------------------------------ names */

static Node *copy_const(Node *c, Node *at) {
    Node *r = xalloc(sizeof(Node));
    *r = *c;
    r->file = at->file;
    r->line = at->line;
    r->col = at->col;
    return r;
}

/* the expression for a symbol */
static Node *sym_ref(Sym *s, Node *at, Node *recv) {
    Node *n;
    switch (s->kind) {
    case S_CONST:
        eval_const(s, at);
        return copy_const(s->cval, at);
    case S_FIELD:
        if (s->sflags & SF_INSTANCE) {
            /* recv.field */
            n = lit_node(N_MEMBER, s->type, at);
            n->name = s->name;
            n->text = s->spelling;
            n->sym = s;
            n->a = recv;
            n->cls = (recv->type->kind == TY_CLASS || recv->cls == EC_VAR || recv->kind == N_ME) ? EC_VAR : EC_VALUE;
            if ((s->mods & M_READONLY) && !(cur_method && (cur_method->sflags & SF_CTOR) && cur_method->owner == s->owner &&
                                            recv->kind == N_ME) && !(cur_method && cur_method == s->owner->initm))
                n->cls = EC_VALUE;
            return n;
        }
        n = lit_node(N_NAME, s->type, at);
        n->name = s->name;
        n->text = s->spelling;
        n->sym = s;
        n->cls = EC_VAR;
        if ((s->mods & M_READONLY) && cur_method && !((cur_method->sflags & SF_CTOR) && !(cur_method->sflags & SF_INSTANCE) &&
                                                      cur_method->owner == s->owner))
            n->cls = EC_VALUE;
        return n;
    case S_EVENT:
        n = lit_node(N_NAME, s->type, at);
        n->name = s->name;
        n->text = s->spelling;
        n->sym = s;
        n->a = recv;
        n->cls = EC_EVENT;
        return n;
    case S_PROP:
        if (s->field) return sym_ref(s->field, at, recv);
        n = lit_node(N_PROPREF, s->type, at);
        n->name = s->name;
        n->text = s->spelling;
        n->sym = s;
        n->a = recv;
        n->list = nv_new();
        n->cls = EC_PROP;
        return n;
    case S_LOCAL: case S_PARAM: case S_RETVAL:
        note_capture(s, at);
        n = lit_node(N_NAME, s->type, at);
        n->name = s->name;
        n->text = s->spelling;
        n->sym = s;
        n->cls = EC_VAR;
        return n;
    case S_METHOD:
        n = lit_node(N_NAME, 0, at);
        n->name = s->name;
        n->text = s->spelling;
        n->sym = s;
        n->a = recv;
        n->cls = EC_METHOD;
        return n;
    case S_MODULE: case S_TYPE:
        n = lit_node(N_NAME, s->type, at);
        n->name = s->name;
        n->text = s->spelling;
        n->sym = s;
        n->cls = EC_TYPE;
        return n;
    }
    n = lit_node(N_NAME, 0, at);
    n->name = s->name;
    n->text = s->spelling;
    n->sym = s;
    n->cls = EC_NS;
    return n;
}

static Node *make_me(Node *at) {
    Node *me;
    if (cur_method && (cur_method->sflags & SF_LAMBDA)) return lambda_me(at);
    me = lit_node(N_ME, cur_type->type, at);
    me->cls = cur_type->type->kind == TY_STRUCT ? EC_VAR : EC_VALUE;
    return me;
}

static Node *bind_name(Node *e, int flags) {
    Sym *s;
    lookup_arity = e->list ? e->list->n : -1;
    s = lookup(e->name, e, flags & BF_INVOKE);
    lookup_arity = -1;
    if (!s) {
        if (!opts->explicit_ && cur_method) s = implicit_local(e);
        else serr(e, "'%s' is not declared. It may be inaccessible due to its protection level.", e->text);
    }
    check_access(s, e);
    if (s->kind == S_TYPE) s = generic_select(s, e->list, e);
    else if (e->list && s->kind != S_METHOD) serr(e, "'%s' has no type parameters and so cannot have type arguments.", e->text);
    if (s->kind == S_METHOD && e->list) {
        Node *r = sym_ref(s, e, (s->sflags & SF_INSTANCE) ? make_me(e) : 0);
        r->list2 = e->list;
        return r;
    }
    if (s->sflags & SF_INSTANCE) {
        /* a member of the current object: Me.name */
        if (!root_method() || !(root_method()->sflags & SF_INSTANCE))
            serr(e, "Cannot refer to an instance member of a class from within a shared method or shared member "
                    "initializer without an explicit instance of the class.");
        return sym_ref(s, e, make_me(e));
    }
    if (e->tchar && (s->kind == S_LOCAL || s->kind == S_PARAM || s->kind == S_FIELD || s->kind == S_CONST ||
                     s->kind == S_RETVAL)) {
        Type *t = tchar_type(e->tchar, e);
        if (s->kind == S_CONST) eval_const(s, e);
        if (t != s->type)
            serr(e, "Type character '%c' does not match declared data type '%s'.", e->tchar, type_name(s->type));
    }
    return sym_ref(s, e, 0);
}

static Node *value(Node *e) {
    Node *c;
    NodeVec *none;
    switch (e->cls) {
    case EC_EVENT:
        serr(e, "'%s' is an event, and cannot be called directly. Use a 'RaiseEvent' statement to raise an event.", e->text);
    case EC_PROP:
        if (!e->sym->getter) serr(e, "Property '%s' is 'WriteOnly'.", e->sym->spelling);
        c = resolve_call(e->sym->getter, e->a, e->list, e);
        return c;
    case EC_METHOD:
        none = nv_new();
        call_targs = e->list2;
        c = resolve_call(e->sym, e->a, none, e);
        if (c->cls == EC_VOID) serr(e, "Expression does not produce a value.");
        return c;
    case EC_TYPE:
        if (e->sym && e->sym->kind == S_MODULE)
            serr(e, "'%s' is a module and cannot be referenced as an expression.", e->sym->spelling);
        serr(e, "'%s' is a type and cannot be used as an expression.", e->sym ? e->sym->spelling : type_name(e->type));
    case EC_NS:
        serr(e, "'%s' is a namespace and cannot be used as an expression.", qual_name(e->sym));
    case EC_VOID:
        serr(e, "Expression does not produce a value.");
    }
    if (e->kind == N_COLLINIT && !e->type) return bind_collinit(e, 0);
    return e;
}

static Node *rvalue(Node *e) {
    Node *r = value(bind(e, 0));
    if (is_pending(r)) r = infer_pending(r);
    return r;
}

/* ------------------------------------------------------------ members */

static Sym *array_length;

static Node *instance_member(Node *v, Node *e) {
    Type *t = v->type;
    Sym *s = 0;
    if (t->kind == TY_ARRAY) {
        if (e->name == intern_word("length")) {
            if (!array_length) {
                array_length = new_sym(S_METHOD, intern_word("length"), "Length");
                array_length->builtin = B_ARRAY_LENGTH;
                array_length->type = ty[TY_INTEGER];
                array_length->mods = M_PUBLIC | M_READONLY;
            }
            s = array_length;
        }
    } else if (t->kind == TY_CLASS || t->kind == TY_INTERFACE) s = type_lookup(t->sym, e->name);
    else if (t->sym && t->sym->members) s = scope_find(t->sym->members, e->name);
    if (!s && ty_user(t)) {
        /* the members of System.Object */
        Sym *o = ty[TY_OBJECT]->sym;
        s = scope_find(o->members, e->name);
    }
    if (!s) serr(e, "'%s' is not a member of '%s'.", e->text, type_name(t));
    check_access(s, e);
    if (s->kind == S_CONST) return copy_const((eval_const(s, e), s->cval), e);
    if (s->kind == S_TYPE) serr(e, "'%s' is a type and cannot be used as an expression.", s->spelling);
    if (s->kind == S_FIELD || s->kind == S_PROP || s->kind == S_EVENT) return sym_ref(s, e, (s->sflags & SF_INSTANCE) ? v : 0);
    if (s->kind == S_METHOD && (s->mods & M_READONLY) && !s->params.n && !(s->mods & M_SHARED)) {
        /* a builtin read-only property */
        Node *call = lit_node(N_INVOKE, s->type, e);
        call->sym = s;
        call->a = v;
        call->list = nv_new();
        return call;
    }
    return sym_ref(s, e, (s->mods & M_SHARED) ? 0 : v);
}

/* MyBase.name and MyClass.name: the member of the base class (or of the
   class itself), called without virtual dispatch */
static Node *bind_mybase(Node *e) {
    int is_base = e->a->kind == N_MYBASE;
    Sym *t, *s;
    Node *me;
    if (!cur_type || !cur_method || !(cur_method->sflags & SF_INSTANCE))
        serr(e->a, "'%s' is valid only within an instance method.", is_base ? "MyBase" : "MyClass");
    t = is_base ? cur_type->base : cur_type;
    me = make_me(e->a);
    me->flags |= F_DIRECT;
    if (e->name == intern_word("new")) {
        Sym *c = t ? t->ctors : 0;
        if (!c) {
            /* MyBase.New() of a class whose base is Object: nothing to do */
            Node *n = lit_node(N_NOTHING, ty[TY_NOTHING], e);
            n->cls = EC_VOID;
            n->flags |= F_DIRECT;
            return n;
        }
        return sym_ref(c, e, me);
    }
    s = t ? type_lookup(t, e->name) : 0;
    if (!s) s = scope_find(ty[TY_OBJECT]->sym->members, e->name);
    if (!s) serr(e, "'%s' is not a member of '%s'.", e->text, t ? t->spelling : "Object");
    check_access(s, e);
    if (s->kind == S_CONST) return copy_const((eval_const(s, e), s->cval), e);
    return sym_ref(s, e, (s->sflags & SF_INSTANCE) ? me : 0);
}

static Node *bind_member1(Node *e, int flags);

/* a member, with the explicit type arguments of a generic method */
static Node *bind_member(Node *e, int flags) {
    Node *r = bind_member1(e, flags);
    if (r->cls == EC_METHOD && e->list) r->list2 = e->list;
    return r;
}

static Node *bind_member1(Node *e, int flags) {
    Node *b;
    Sym *s;
    (void)flags;
    if (e->a && (e->a->kind == N_MYBASE || e->a->kind == N_MYCLASS)) return bind_mybase(e);
    if (e->a && e->a->kind == N_ME && e->name == intern_word("new")) {
        /* Me.New(...): another constructor of the class, as MyClass.New */
        Node tmp = *e;
        Node mc = *e->a;
        mc.kind = N_MYCLASS;
        tmp.a = &mc;
        return bind_mybase(&tmp);
    }
    if (!e->a) e->a = with_object(e);
    if (e->flags & F_QDOT) not_yet(e, "The '?.' operator is");
    b = bind(e->a, 0);
    if (b->cls == EC_NS || b->cls == EC_TYPE) {
        Sym *c = b->sym;
        s = member_of(c, e->name, e);
        if (!s) serr(e, "'%s' is not a member of '%s'.", e->text, qual_name(c));
        check_access(s, e);
        if (s->kind == S_TYPE) s = generic_select(s, e->list, e);
        if (c->kind == S_TYPE && !(s->mods & M_SHARED) && s->kind != S_TYPE)
            serr(e, "Reference to a non-shared member requires an object reference.");
        return sym_ref(s, e, 0);
    }
    b = value(b);
    if (is_pending(b)) b = infer_pending(b);
    return instance_member(b, e);
}

/* ------------------------------------------------------------ invocation, indexing */

static Node *bind_index(Node *v, NodeVec *args, Node *e) {
    Node *n;
    int i;
    for (i = 0; i < args->n; i++) {
        Node *a = args->v[i];
        if (!a->a) serr(e, "Array index expected.");
        if (a->name) serr(a, "Named arguments are not valid as array subscripts.");
        if (a->a->kind == N_BOUND) serr(a, "Array bounds cannot appear here.");
    }
    if (v->type->kind == TY_STRING) {
        /* String's default property Chars */
        if (args->n != 1) serr(e, "Too many arguments to 'Public ReadOnly Default Property Chars(index As Integer) As Char'.");
        n = lit_node(N_INDEX, ty[TY_CHAR], e);
        n->a = v;
        n->list = nv_new();
        nv_push(n->list, coerce(rvalue(args->v[0]->a), ty[TY_INTEGER]));
        return n;
    }
    if (v->type->kind != TY_ARRAY)
        serr(e, "Expression is not an array or a method, and cannot have an argument list.");
    if (args->n < v->type->rank) serr(e, "Number of indices is less than the number of dimensions of the indexed array.");
    if (args->n > v->type->rank) serr(e, "Number of indices exceeds the number of dimensions of the indexed array.");
    n = lit_node(N_INDEX, v->type->elem, e);
    n->a = v;
    n->cls = EC_VAR;
    n->list = nv_new();
    for (i = 0; i < args->n; i++) nv_push(n->list, coerce(rvalue(args->v[i]->a), ty[TY_INTEGER]));
    return n;
}

/* the Default property of a type (or of its bases and interfaces) */
static Sym *default_prop(Sym *t) {
    Sym *s, *c;
    int i;
    for (c = t; c; c = c->base) {
        for (s = c->members->syms; s; s = s->next)
            if (s->kind == S_PROP && (s->mods & M_DEFAULT)) return s;
        for (i = 0; i < c->ifaces.n; i++) {
            Sym *d = default_prop(c->ifaces.v[i]);
            if (d) return d;
        }
    }
    return 0;
}

static Node *bind_invoke(Node *e) {
    Node *t;
    if (e->flags & F_QDOT) not_yet(e, "The '?(' operator is");
    t = bind(e->a, BF_INVOKE);
    if (t->cls == EC_METHOD) {
        NodeVec *args = bind_args(e->list);
        call_targs = t->list2;
        return resolve_call(t->sym, t->a, args, e);
    }
    if (t->cls == EC_PROP && t->list->n == 0 && t->sym->decl->list2 && t->sym->decl->list2->n) {
        t->list = bind_args(e->list);
        return t;
    }
    t = value(t);
    if (is_pending(t)) t = infer_pending(t);
    if (is_delegate(t->type)) {
        /* a call of a delegate: its Invoke */
        NodeVec *args = bind_args(e->list);
        return resolve_call(delegate_invoke(t->type), t, args, e);
    }
    if (ty_user(t->type) && t->type->kind != TY_ENUM) {
        /* the default property: obj(args) */
        Sym *dp = default_prop(t->type->sym);
        Node *r;
        if (!dp)
            serr(e, "%s '%s' cannot be indexed because it has no default property.",
                 t->type->kind == TY_STRUCT ? "Structure" : t->type->kind == TY_INTERFACE ? "Interface" : "Class",
                 type_name(t->type));
        check_access(dp, e);
        r = sym_ref(dp, e, t);
        r->list = bind_args(e->list);
        return r;
    }
    return bind_index(t, e->list, e);
}

/* ------------------------------------------------------------ operators */

static Node *user_operator(int op, Node *l, Node *r, Node *at);

static Node *bind_unary(Node *e) {
    Node *x = rvalue(e->a);
    Type *t;
    {
        Node *u = user_operator(e->op, x, 0, e);
        if (u) return u;
    }
    if (x->type->kind == TY_ENUM) {
        if (e->op == K_Not) {
            /* Not of an enum keeps the enum type */
            e->a = x;
            e->type = x->type;
            e->cls = EC_VALUE;
            return e;
        }
        x = make_conv(x, x->type->elem, 0);
    }
    t = unop_type(e, e->op, x->type);
    x = make_conv(x, t, 0);
    if (is_const(x)) {
        Node *f = fold_unary(e, e->op, t, x);
        if (f) return f;
    }
    e->a = x;
    e->type = t;
    e->cls = EC_VALUE;
    return e;
}

static Node *bind_binary(Node *e) {
    Node *l = rvalue(e->a), *r = rvalue(e->b);
    Type *t, *res;
    int op = e->op;
    if (l->kind == N_COLLINIT && !l->type) l = bind_collinit(l, 0);
    if (r->kind == N_COLLINIT && !r->type) r = bind_collinit(r, 0);
    if (op != K_Is && op != K_IsNot && op != K_AndAlso && op != K_OrElse) {
        Node *u = user_operator(op, l, r, e);
        if (u) return u;
    }
    if (l->type->kind == TY_ENUM || r->type->kind == TY_ENUM) {
        if ((op == K_And || op == K_Or || op == K_Xor) && l->type == r->type) {
            /* flags: the bitwise operators on one enum type keep it */
            e->a = l;
            e->b = r;
            e->type = l->type;
            e->cls = EC_VALUE;
            return e;
        }
        /* the other operators work on the underlying type */
        if (l->type->kind == TY_ENUM) l = make_conv(l, l->type->elem, 0);
        if (r->type->kind == TY_ENUM) r = make_conv(r, r->type->elem, 0);
    }
    if (op == K_AndAlso || op == K_OrElse) {
        l = coerce(l, ty[TY_BOOLEAN]);
        r = coerce(r, ty[TY_BOOLEAN]);
        t = res = ty[TY_BOOLEAN];
    } else {
        t = binop_types(e, op, l->type, r->type, &res);
        if (op == K_Is || op == K_IsNot) {
            /* reference comparison: no conversions */
        } else if (op == P_AMP && (l->type->kind == TY_OBJECT || r->type->kind == TY_OBJECT)) {
            l = coerce(l, t);
            r = coerce(r, t);
        } else {
            l = make_conv(l, t, 0);
            r = (op == P_SHL || op == P_SHR) ? coerce(r, ty[TY_INTEGER]) : make_conv(r, t, 0);
        }
    }
    if (is_const(l) && is_const(r) && op != K_Is && op != K_IsNot && op != K_Like) {
        Node *f = fold_binary(e, op, t, l, r);
        if (f) return f;
    }
    e->a = l;
    e->b = r;
    e->type = res;
    e->cls = EC_VALUE;
    return e;
}

/* ------------------------------------------------------------ conversions */

static Type *cast_type(int op, Node *at) {
    switch (op) {
    case K_CBool: return ty[TY_BOOLEAN];
    case K_CByte: return ty[TY_BYTE];
    case K_CChar: return ty[TY_CHAR];
    case K_CDbl: return ty[TY_DOUBLE];
    case K_CInt: return ty[TY_INTEGER];
    case K_CLng: return ty[TY_LONG];
    case K_CObj: return ty[TY_OBJECT];
    case K_CSByte: return ty[TY_SBYTE];
    case K_CShort: return ty[TY_SHORT];
    case K_CSng: return ty[TY_SINGLE];
    case K_CStr: return ty[TY_STRING];
    case K_CUInt: return ty[TY_UINTEGER];
    case K_CULng: return ty[TY_ULONG];
    case K_CUShort: return ty[TY_USHORT];
    case K_CDec: serr(at, "Type 'Decimal' is not supported.");
    }
    serr(at, "Type 'Date' is not supported.");
    return 0;
}

static Node *as_value_node(Node *x, Node *at) {
    Node *p;
    if (x->cls != EC_VAR) return x;
    p = lit_node(N_PAREN, x->type, at);
    p->a = x;
    return p;
}

static Node *bind_cast(Node *e) {
    Node *x = rvalue(e->a);
    Type *t;
    if (e->op == K_CType || e->op == K_DirectCast || e->op == K_TryCast) {
        t = bind_type(e->b);
        if (e->op == K_TryCast && !ty_ref(t))
            serr(e, "'TryCast' operands must be class-constrained type parameter or reference type, but '%s' is a value type.",
                 type_name(t));
        if (e->op != K_CType && x->type != t && x->type->kind != TY_OBJECT && t->kind != TY_OBJECT &&
            x->type->kind != TY_NOTHING && !(ty_ref(x->type) && ty_ref(t) && conv_class(x->type, t) != CV_NONE) &&
            !(x->type->kind == TY_STRUCT && t->kind == TY_INTERFACE) && !(t->kind == TY_STRUCT && ty_ref(x->type)))
            no_conversion(e, x->type, t);
    } else t = cast_type(e->op, e);
    {
        Node *c = explicit_conv(x, t, e);
        if (e->op == K_TryCast && c->kind == N_CONV) c->flags |= F_TRYCAST;
        return as_value_node(c, e);
    }
}

static Node *bind_iif(Node *e) {
    Node *v[2];
    Type *t;
    if (e->c) {
        Node *c = coerce(rvalue(e->a), ty[TY_BOOLEAN]);
        v[0] = rvalue(e->b);
        v[1] = rvalue(e->c);
        t = dominant(v, 2);
        if (!t || (t->kind == TY_OBJECT && v[0]->type->kind != TY_OBJECT && v[1]->type->kind != TY_OBJECT && opts->strict))
            serr(e, "Cannot infer a common type for the second and third operands of the 'If' operator. "
                    "One must have a widening conversion to the other.");
        e->a = c;
        e->b = coerce(v[0], t);
        e->c = coerce(v[1], t);
    } else {
        v[0] = rvalue(e->a);
        v[1] = rvalue(e->b);
        if (!ty_ref(v[0]->type))
            serr(e, "First operand in a binary 'If' expression must be a nullable value type, a reference type, "
                    "or an unconstrained generic type.");
        t = dominant(v, 2);
        if (!t) serr(e, "Cannot infer a common type for the first and second operands of the binary 'If' operator. "
                        "One must have a widening conversion to the other.");
        e->a = coerce(v[0], t);
        e->b = coerce(v[1], t);
    }
    e->type = t;
    e->cls = EC_VALUE;
    return e;
}

/* ------------------------------------------------------------ arrays */

static int collinit_rank(Node *e) {
    int r = 1;
    while (e->kind == N_COLLINIT && e->list->n > 0 && e->list->v[0]->kind == N_COLLINIT) {
        r++;
        e = e->list->v[0];
    }
    return r;
}

static void collect_leaves(Node *e, int depth, Node ***v, int *n, int *cap) {
    int i;
    for (i = 0; i < e->list->n; i++) {
        Node *x = e->list->v[i];
        if (depth > 1) {
            if (x->kind != N_COLLINIT) serr(x, "Array initializer has too few dimensions.");
            collect_leaves(x, depth - 1, v, n, cap);
            continue;
        }
        if (*n == *cap) {
            *cap = *cap ? 2 * *cap : 16;
            *v = xrealloc(*v, sizeof(Node *) * *cap);
        }
        if (x->kind == N_COLLINIT && !x->type) {
            /* an element that is itself an array literal (jagged arrays) */
            x = bind_collinit(x, 0);
        } else x = rvalue(x);
        e->list->v[i] = x;
        (*v)[(*n)++] = x;
    }
}

/* element counts of a rectangular initializer: dims[0..rank-1] */
static void check_shape(Node *e, int depth, int *dims, int level) {
    int i;
    if (dims[level] < 0) dims[level] = e->list->n;
    else if (dims[level] > e->list->n)
        serr(e, "Array initializer is missing %d elements.", dims[level] - e->list->n);
    else if (dims[level] < e->list->n)
        serr(e, "Array initializer has %d too many elements.", e->list->n - dims[level]);
    if (depth > 1)
        for (i = 0; i < e->list->n; i++) check_shape(e->list->v[i], depth - 1, dims, level + 1);
}

static void convert_leaves(Node *e, int depth, Type *elem) {
    int i;
    for (i = 0; i < e->list->n; i++) {
        if (depth > 1) convert_leaves(e->list->v[i], depth - 1, elem);
        else e->list->v[i] = coerce(e->list->v[i], elem);
    }
}

/* an array literal { ... }: converted to the array type target, or typed
   by its elements (spec 11.1.1) */
static Node *bind_collinit(Node *e, Type *target) {
    int rank = (target && target->kind == TY_ARRAY) ? target->rank : collinit_rank(e);
    int n = 0, cap = 0, *dims, i;
    Node **leaves = 0;
    Type *t;
    if (e->type) return target ? coerce(e, target) : e;
    if (!e->aux) {
        collect_leaves(e, rank, &leaves, &n, &cap);
        e->aux = 1;
    }
    dims = xalloc(sizeof(int) * rank);
    for (i = 0; i < rank; i++) dims[i] = -1;
    check_shape(e, rank, dims, 0);
    if (target && target->kind == TY_ARRAY) t = target;
    else {
        Type *el = n ? dominant(leaves, n) : ty[TY_OBJECT];
        if (!el || (el->kind == TY_OBJECT && n && opts->strict)) {
            if (opts->strict)
                serr(e, "Cannot infer an element type, and Option Strict On does not allow 'Object' to be assumed.");
            el = ty[TY_OBJECT];
        }
        t = ty_array(el, rank);
    }
    convert_leaves(e, rank, t->elem);
    e->type = t;
    e->cls = EC_VALUE;
    if (target && target != t) return coerce(e, target);
    return e;
}

/* New T(bounds) { ... } */
static Node *bind_newarray(Node *e) {
    Type *t = bind_type(e->a);
    int i;
    for (i = e->list2->n - 1; i >= 1; i--) t = ty_array(t, (int)e->list2->v[i]->ival);
    t = ty_array(t, (int)e->list2->v[0]->ival);
    if (e->list) {
        for (i = 0; i < e->list->n; i++) {
            Node *b = e->list->v[i];
            if (b->kind == N_BOUND) {
                Node *lo = rvalue(b->a);
                if (!is_const(lo) || !ty_integral(lo->type) || lo->ival != 0)
                    serr(b, "Array lower bounds can be only '0'.");
                b = b->b;
            }
            e->list->v[i] = coerce(rvalue(b), ty[TY_INTEGER]);
        }
    }
    if (e->b) {
        int rank = t->rank;
        e->b = bind_collinit(e->b, t);
        if (e->list && e->b->list->n + (rank > 1 ? 0 : 0) >= 0) {
            /* constant bounds must match the initializer */
            Node *lvl = e->b;
            for (i = 0; i < e->list->n; i++) {
                Node *ub = e->list->v[i];
                int have = lvl->list->n;
                if (is_const(ub)) {
                    i64 want = (i64)ub->ival + 1;
                    if (want > have) serr(e->b, "Array initializer is missing %d elements.", (int)(want - have));
                    if (want < have) serr(e->b, "Array initializer has %d too many elements.", (int)(have - want));
                } else if (have) serr(ub, "Constant expression is required.");
                if (lvl->list->n == 0) break;
                lvl = lvl->list->v[0];
            }
        }
    }
    e->type = t;
    e->cls = EC_VALUE;
    return e;
}

/* ------------------------------------------------------------ New */

/* New T(args) [With {.a = x, ...}]: sym the constructor (NULL: a structure
   without arguments), list the arguments, b the member initializers (FIELDINIT
   nodes with sym the field or property and a the value) */
static Node *bind_new(Node *e) {
    Type *t = bind_type(e->a);
    Sym *ts;
    NodeVec *args = bind_args(e->list);
    if (t->kind != TY_CLASS && t->kind != TY_STRUCT) {
        if (t->kind == TY_ENUM || ty_numeric(t) || t->kind == TY_BOOLEAN || t->kind == TY_CHAR) {
            if (args->n) serr(e, "Too many arguments to 'Public Sub New()'.");
            return const_convert(lit_node(N_NOTHING, ty[TY_NOTHING], e), t, e);
        }
        serr(e, "'New' cannot be used with the type '%s'.", type_name(t));
    }
    ts = t->sym;
    if (ts->sflags & SF_DELEGATE) {
        /* New D(AddressOf M) / New D(lambda) */
        if (args->n != 1 || !args->v[0]->a || !is_pending(args->v[0]->a))
            serr(e, "Delegate '%s' requires an 'AddressOf' expression or lambda expression as the only argument to its constructor.",
                 type_name(t));
        return make_delegate(args->v[0]->a, t);
    }
    if (ts->mods & M_MUSTINHERIT) serr(e, "'New' cannot be used on a class that is declared 'MustInherit'.");
    if (args->n || t->kind == TY_CLASS) {
        Node *call;
        if (!ts->ctors) serr(e, "Too many arguments to 'Public Sub New()'.");
        call = resolve_call(ts->ctors, 0, args, e);
        e->sym = call->sym;
        e->list = call->list;
    } else {
        e->sym = 0;
        e->list = nv_new();
    }
    if (e->b && e->b->kind == N_COLLINIT) {
        /* From {x, {k, v}, ...}: a call of Add for each element (receiver: the new object) */
        Node *init = e->b;
        int i;
        e->list3 = nv_new();
        for (i = 0; i < init->list->n; i++) {
            Node *x = init->list->v[i], *recv = lit_node(N_TARGET, t, x), *m, *call;
            NodeVec *args = nv_new();
            if (x->kind == N_COLLINIT && !x->type) {
                int k;
                for (k = 0; k < x->list->n; k++) {
                    Node *a = lit_node(N_ARG, 0, x->list->v[k]);
                    a->cls = EC_NONE;
                    a->a = x->list->v[k];
                    nv_push(args, a);
                }
            } else {
                Node *a = lit_node(N_ARG, 0, x);
                a->cls = EC_NONE;
                a->a = x;
                nv_push(args, a);
            }
            m = lit_node(N_MEMBER, 0, x);
            m->cls = EC_NONE;
            m->name = intern_word("add");
            m->text = "Add";
            m->a = recv;
            {
                Sym *as = (t->kind == TY_CLASS || t->kind == TY_INTERFACE) ? type_lookup(ts, m->name) : scope_find(ts->members, m->name);
                if (!as || as->kind != S_METHOD)
                    serr(init, "Type '%s' cannot be initialized with a collection initializer because it does not have an "
                               "accessible 'Add' method.", type_name(t));
            }
            m = bind(m, 0);
            call = resolve_call(m->sym, m->a, bind_args(args), x);
            nv_push(e->list3, call);
        }
        e->b = 0;
    }
    if (e->b) {
        int i;
        for (i = 0; i < e->b->list->n; i++) {
            Node *fi = e->b->list->v[i];
            Sym *m = scope_find(ts->members, fi->name);
            Node *v;
            if (!m || (m->kind != S_FIELD && m->kind != S_PROP) || !(m->sflags & SF_INSTANCE))
                serr(fi, "'%s' is not a member of '%s'.", fi->text, type_name(t));
            check_access(m, fi);
            if (m->kind == S_PROP && m->field) m = m->field;
            if (m->kind == S_PROP && !m->setter) serr(fi, "Property '%s' is 'ReadOnly'.", m->spelling);
            v = bind(fi->a, 0);
            if (v->kind == N_COLLINIT && !v->type) v = bind_collinit(v, m->type);
            fi->a = coerce(value(v), m->type);
            fi->sym = m;
        }
    }
    e->type = t;
    e->cls = EC_VALUE;
    return e;
}

/* ------------------------------------------------------------ the binder */

static Node *bind(Node *e, int flags) {
    if (e->cls != EC_NONE && e->kind != N_COLLINIT) return e;     /* already bound */
    switch (e->kind) {
    case N_INT:
        if (!e->type) type_int_literal(e);
        return e;
    case N_FLOAT:
        if (e->tchar == LS_D) serr(e, "Type 'Decimal' is not supported.");
        e->type = e->tchar == LS_F ? ty[TY_SINGLE] : ty[TY_DOUBLE];
        if (e->tchar == LS_F) e->dval = round_single(e->dval);
        e->cls = EC_VALUE;
        return e;
    case N_STRING: e->type = ty[TY_STRING]; e->cls = EC_VALUE; return e;
    case N_CHAR: e->type = ty[TY_CHAR]; e->cls = EC_VALUE; return e;
    case N_BOOL: e->type = ty[TY_BOOLEAN]; e->cls = EC_VALUE; return e;
    case N_NOTHING: e->type = ty[TY_NOTHING]; e->cls = EC_VALUE; return e;
    case N_NAME: return bind_name(e, flags);
    case N_MEMBER: return bind_member(e, flags);
    case N_INVOKE: return bind_invoke(e);
    case N_PAREN: {
        Node *x = rvalue(e->a);
        if (is_const(x)) return x;
        e->a = x;
        e->type = x->type;
        e->cls = EC_VALUE;
        return e;
    }
    case N_UNARY: return bind_unary(e);
    case N_BINARY: return bind_binary(e);
    case N_CAST: return bind_cast(e);
    case N_IIF: return bind_iif(e);
    case N_COLLINIT: return e;          /* typed by the context */
    case N_NEWARRAY: return bind_newarray(e);
    case N_TYPEEXPR: {
        Type *t = bind_type(e->a);
        Node *n = sym_ref(t->sym, e, 0);
        return n;
    }
    case N_GLOBAL: {
        Node *n = lit_node(N_NAME, 0, e);
        n->sym = ns_global;
        n->text = "Global";
        n->cls = EC_NS;
        return n;
    }
    case N_NAMEOF: {
        Node *x = e->a, *s;
        if (x->kind != N_NAME && x->kind != N_MEMBER) serr(x, "This expression does not have a name.");
        bind(x, BF_INVOKE);
        s = lit_node(N_STRING, ty[TY_STRING], e);
        s->text = x->text;
        return s;
    }
    case N_INTERP: {
        int i;
        for (i = 0; i < e->list->n; i++) {
            Node *h = e->list->v[i];
            if (h->kind != N_INTERPHOLE) {
                h->type = ty[TY_STRING];
                h->cls = EC_VALUE;
                continue;
            }
            h->a = rvalue(h->a);
            if (h->a->kind == N_COLLINIT && !h->a->type) h->a = bind_collinit(h->a, 0);
            h->type = h->a->type;
            if (h->b) {
                h->b = coerce(rvalue(h->b), ty[TY_INTEGER]);
                if (!is_const(h->b)) serr(h->b, "Constant expression is required.");
            }
        }
        e->type = ty[TY_STRING];
        e->cls = EC_VALUE;
        return e;
    }
    case N_BOUND: serr(e, "Array bounds cannot appear here.");
    case N_NEW: return bind_new(e);
    case N_ANONNEW: not_yet(e, "Anonymous types are");
    case N_LAMBDA:
        e->cls = EC_VALUE;      /* pending: typed by a conversion */
        return e;
    case N_QUERY: not_yet(e, "Query expressions are");
    case N_ADDRESSOF: {
        Node *g = bind(e->a, BF_INVOKE);
        if (g->cls != EC_METHOD) serr(e->a, "'AddressOf' operand must be the name of a method (without parentheses).");
        e->a = g;
        e->cls = EC_VALUE;      /* pending */
        return e;
    }
    case N_TYPEOF: {
        Node *x = rvalue(e->a);
        Type *t = bind_type(e->b);
        if (!ty_ref(x->type))
            serr(e->a, "'TypeOf ... Is' requires its left operand to have a reference type, but this operand has the value type '%s'.",
                 type_name(x->type));
        if (conv_class(x->type, t) == CV_NONE && conv_class(t, x->type) == CV_NONE)
            serr(e, "Expression of type '%s' can never be of type '%s'.", type_name(x->type), type_name(t));
        e->a = x;
        e->type = ty[TY_BOOLEAN];
        e->cls = EC_VALUE;
        return e;
    }
    case N_GETTYPE: not_yet(e, "'GetType' is");
    case N_ME:
        if (!cur_type || !root_method() || !(root_method()->sflags & SF_INSTANCE))
            serr(e, "'Me' is valid only within an instance method.");
        return make_me(e);
    case N_MYBASE: case N_MYCLASS: not_yet(e, "'MyBase' and 'MyClass' are");
    case N_DICT: not_yet(e, "Dictionary access ('!') is");
    }
    if (e->type) return e;      /* already bound */
    serr(e, "Expression expected.");
    return e;
}
