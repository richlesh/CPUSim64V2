/*
 * sema_names.c - the analysis context, name lookup (spec 4.7, "Type and
 * Namespace Names", and 11.4.4, "Simple Name Expressions"), type names and
 * implicit conversions.
 */

static Sym *cur_ns;         /* the namespace of the declaration being analyzed */
static Sym *cur_module;     /* the module, or NULL */
static Sym *cur_method;     /* the method, or NULL */
static Sym *cur_type;       /* the class or structure, or NULL */
static Scope *cur_tpscope;  /* the type parameters of a generic method instance, or NULL */
static NodeVec *call_targs; /* the explicit type arguments of the next call (Max(Of Integer)(...)) */
static Sym *generic_select(Sym *s, NodeVec *targs, Node *at);     /* sema_generic.c */
static int lookup_arity = -1;   /* the number of type arguments of the name looked up, or -1 */

/* does a type name (or one of the types of the same name) take n type arguments? */
static int has_arity(Sym *s, int n) {
    Sym *o;
    if (s->kind != S_TYPE) return 1;
    for (o = s; o; o = o->overload)
        if (((o->sflags & SF_GENERIC) && o->decl->list ? o->decl->list->n : 0) == n) return 1;
    return 0;
}
static Scope *cur_scope;    /* the innermost scope */

static void set_file(int file) {
    opts = &file_opts[file];
}

/* ------------------------------------------------------------ lookup */

static int derives(Sym *a, Sym *b);

static int accessible(Sym *s) {
    Sym *o;
    if (!(s->mods & (M_PRIVATE | M_PROTECTED))) return 1;
    if ((s->mods & M_PROTECTED) && (s->mods & M_FRIEND)) return 1;
    /* Private: only inside the type; Protected: also inside derived types */
    for (o = cur_method ? cur_method->owner : cur_type ? cur_type : cur_module; o; o = o->owner) {
        if (o == s->owner) return 1;
        if ((s->mods & M_PROTECTED) && o->kind == S_TYPE && s->owner && s->owner->kind == S_TYPE && derives(o, s->owner))
            return 1;
    }
    return 0;
}

static void check_access(Sym *s, Node *at) {
    if (!accessible(s))
        serr(at, "'%s' is not accessible in this context because it is '%s'.", s->spelling,
             (s->mods & M_PRIVATE) ? "Private" : "Protected");
}

static Sym *type_lookup(Sym *t, Name *n);

/* a member of the modules of a namespace (module members are promoted to
   their namespace, spec 7.7) */
static Sym *promoted(Sym *ns, Name *n, Node *at) {
    Sym *m, *found = 0, *where = 0;
    for (m = ns->members->syms; m; m = m->next) {
        Sym *s;
        if (m->kind != S_MODULE) continue;
        s = scope_find(m->members, n);
        if (!s || !accessible(s)) continue;
        if (found)
            serr(at, "'%s' is ambiguous between declarations in Modules '%s' and '%s'.", s->spelling,
                 qual_name(where), qual_name(m));
        found = s;
        where = m;
    }
    return found;
}

/* a name in a namespace: a nested namespace, a type or module, or a member
   of one of its modules */
static Sym *ns_member(Sym *ns, Name *n, Node *at) {
    Sym *s = scope_find(ns->members, n);
    if (s) return s;
    return promoted(ns, n, at);
}

/* the imports of the file: Imports X = Y aliases, then the imported
   namespaces (and types) */
static Sym *import_lookup(Name *n, Node *at) {
    int i;
    Sym *found = 0;
    for (i = 0; i < opts->naliases; i++)
        if (opts->alias_names[i] == n) return opts->alias_syms[i];
    for (i = 0; i < opts->imports.n; i++) {
        Sym *im = opts->imports.v[i], *s;
        if (im->kind == S_NAMESPACE) {
            s = scope_find(im->members, n);
            if (s && s->kind == S_NAMESPACE) s = 0;     /* imports do not import namespaces */
            if (!s) s = promoted(im, n, at);
        } else s = scope_find(im->members, n);
        if (s && lookup_arity >= 0 && !has_arity(s, lookup_arity)) s = 0;
        if (s && found && s != found && s->kind != S_NAMESPACE)
            serr(at, "'%s' is ambiguous, imported from the namespaces or types '%s' and '%s'.", s->spelling,
                 qual_name(found->owner), qual_name(im));
        if (s && !found) found = s;
    }
    return found;
}

/* a simple name; want_call: the name is the target of an argument list,
   so that the name of the enclosing Function is a recursive call rather
   than its return variable */
static Sym *lookup(Name *n, Node *at, int want_call) {
    Scope *sc;
    Sym *s, *ns;
    for (sc = cur_scope; sc; sc = sc->parent) {
        s = scope_find(sc, n);
        if (s && !(s->kind == S_RETVAL && want_call)) return s;
        if (sc->kind == SC_METHOD) break;
    }
    if (cur_tpscope) {
        s = scope_find(cur_tpscope, n);
        if (s) return s;
    }
    /* the members of the enclosing types */
    for (ns = cur_type; ns && ns->kind == S_TYPE; ns = ns->owner) {
        s = type_lookup(ns, n);
        if (s) return s;
    }
    if (cur_type) {
        /* the instance methods of System.Object (ToString, ...) */
        s = scope_find(ty[TY_OBJECT]->sym->members, n);
        if (s) return s;
    }
    if (cur_module) {
        s = scope_find(cur_module->members, n);
        if (s) return s;
    }
    for (ns = cur_ns; ns; ns = ns->owner) {
        s = ns_member(ns, n, at);
        if (s) return s;
    }
    return import_lookup(n, at);
}

/* the member of a namespace, module or type */
static Sym *builtin_extension(Sym *t);

static Sym *member_of(Sym *c, Name *n, Node *at) {
    Sym *ext;
    if (c->kind == S_NAMESPACE) return ns_member(c, n, at);
    if ((ext = builtin_extension(c)) != 0 && !scope_find(c->members, n)) {
        Sym *s = scope_find(ext->members, n);
        if (s && s->kind == S_METHOD && (s->mods & M_SHARED)) return s;
    }
    if (c->kind == S_TYPE && c->type && (c->type->kind == TY_CLASS || c->type->kind == TY_INTERFACE))
        return type_lookup(c, n);
    return scope_find(c->members, n);
}

/* ------------------------------------------------------------ type names */

static Type *bind_type(Node *tn);

static Sym *bind_type_name(Node *tn) {
    Sym *s = 0;
    int i;
    for (i = 0; i < tn->list->n; i++) {
        Node *p = tn->list->v[i];
        Sym *nx;
        if (i == 0) {
            lookup_arity = p->list ? p->list->n : 0;
            nx = (tn->flags & F_GLOBAL) ? scope_find(ns_global->members, p->name) : lookup(p->name, p, 0);
            lookup_arity = -1;
        }
        else {
            if (s->kind != S_NAMESPACE && s->kind != S_TYPE && s->kind != S_MODULE) nx = 0;
            else nx = member_of(s, p->name, p);
        }
        if (!nx || (nx->kind != S_NAMESPACE && nx->kind != S_TYPE && nx->kind != S_MODULE)) {
            StrBuf b;
            int k;
            sb_init(&b);
            for (k = 0; k <= i; k++) {
                if (k) sb_putc(&b, '.');
                sb_puts(&b, tn->list->v[k]->text);
            }
            serr(tn, "Type '%s' is not defined.", b.s);
        }
        if (nx->kind == S_TYPE) nx = generic_select(nx, p->list, p);
        else if (p->list) serr(p, "'%s' has no type parameters and so cannot have type arguments.", p->text);
        s = nx;
    }
    return s;
}

static Type *bind_type(Node *tn) {
    Type *t;
    Sym *s;
    switch (tn->kind) {
    case N_TPRIM:
        switch (tn->op) {
        case K_Boolean: t = ty[TY_BOOLEAN]; break;
        case K_SByte: t = ty[TY_SBYTE]; break;
        case K_Byte: t = ty[TY_BYTE]; break;
        case K_Short: t = ty[TY_SHORT]; break;
        case K_UShort: t = ty[TY_USHORT]; break;
        case K_Integer: t = ty[TY_INTEGER]; break;
        case K_UInteger: t = ty[TY_UINTEGER]; break;
        case K_Long: t = ty[TY_LONG]; break;
        case K_ULong: t = ty[TY_ULONG]; break;
        case K_Single: t = ty[TY_SINGLE]; break;
        case K_Double: t = ty[TY_DOUBLE]; break;
        case K_Char: t = ty[TY_CHAR]; break;
        case K_String: t = ty[TY_STRING]; break;
        case K_Object: t = ty[TY_OBJECT]; break;
        case K_Decimal: serr(tn, "Type 'Decimal' is not supported."); return 0;
        default: serr(tn, "Type 'Date' is not supported."); return 0;
        }
        break;
    case N_TARRAY:
        t = ty_array(bind_type(tn->a), (int)tn->ival);
        break;
    case N_TNULLABLE:
        not_yet(tn, "Nullable types are");
        return 0;
    default:
        s = bind_type_name(tn);
        if (s->kind == S_NAMESPACE) serr(tn, "'%s' is a namespace and cannot be used as a type.", qual_name(s));
        if (s->kind == S_MODULE) serr(tn, "Module '%s' cannot be used as a type.", s->spelling);
        if (!s->type) serr(tn, "Type '%s' is not supported yet.", qual_name(s));
        t = s->type;
        no_decimal(tn, t);
        break;
    }
    tn->type = t;
    return t;
}

/* the type that a type character gives a name, or NULL */
static Type *tchar_type(int c, Node *at) {
    switch (c) {
    case '%': return ty[TY_INTEGER];
    case '&': return ty[TY_LONG];
    case '!': return ty[TY_SINGLE];
    case '#': return ty[TY_DOUBLE];
    case '$': return ty[TY_STRING];
    case '@': serr(at, "Type 'Decimal' is not supported.");
    }
    return 0;
}

/* ------------------------------------------------------------ conversions */

/* the conversion of e (a value) to t, folded for constants */
static Node *make_conv(Node *e, Type *t, int expl) {
    Node *c;
    if (e->type == t) return e;
    if (is_const(e)) {
        c = const_convert(e, t, e);
        if (c) return c;
    }
    c = lit_node(N_CONV, t, e);
    c->a = e;
    if (expl) c->flags |= F_EXPLICIT;
    return c;
}

static Node *bind_collinit(Node *e, Type *target);
static Node *user_conversion(Node *e, Type *t, int expl);
static int is_pending(Node *e);                     /* sema_lambda.c */
static int is_delegate(Type *t);
static int pending_conv(Node *e, Type *t);
static Node *make_delegate(Node *e, Type *t);
static Node *infer_pending(Node *e);

/* a pending lambda or AddressOf converted to t */
static Node *pending_to(Node *e, Type *t) {
    if (is_delegate(t)) return make_delegate(e, t);
    if (t->kind == TY_OBJECT && e->kind == N_LAMBDA) return make_conv(infer_pending(e), t, 0);
    if (e->kind == N_LAMBDA)
        serr(e, "Lambda expression cannot be converted to '%s' because '%s' is not a delegate type.", type_name(t), type_name(t));
    serr(e, "'AddressOf' expression cannot be converted to '%s' because '%s' is not a delegate type.", type_name(t), type_name(t));
    return 0;
}

/* the conversion class of a value to t, counting constant conversions */
static int conv_class_of(Node *e, Type *t) {
    int k;
    if (is_pending(e)) return pending_conv(e, t);
    k = conv_class(e->type, t);
    if (k == CV_NARROW && const_widens(e, t)) return CV_WIDEN;
    return k;
}

static void no_conversion(Node *at, Type *f, Type *t) {
    if (f->kind == TY_ARRAY && t->kind == TY_ARRAY && f->rank != t->rank)
        serr(at, "Value of type '%s' cannot be converted to '%s' because the array types have different numbers of dimensions.",
             type_name(f), type_name(t));
    serr(at, "Value of type '%s' cannot be converted to '%s'.", type_name(f), type_name(t));
}

static Sym *instantiate_type(Sym *tmpl, Type **args, int n, Node *at);
static Node *resolve_call(Sym *group, Node *recv, NodeVec *args, Node *at);

/* an array as an IEnumerable(Of T): New ArrayWrapper(Of T)(a) */
static Node *array_wrap(Node *e, Type *t) {
    Sym *ns = ns_path("System.Collections.Generic"), *tm = scope_find(ns->members, intern_word("arraywrapper"));
    Type *el = e->type->elem;
    Sym *inst;
    Node *nw, *call, *arg;
    NodeVec *args = nv_new();
    if (!tm) serr(e, "lib/Collections.vb has no ArrayWrapper(Of T)");
    inst = instantiate_type(tm, &el, 1, e);
    arg = lit_node(N_ARG, e->type, e);
    arg->a = e;
    nv_push(args, arg);
    call = resolve_call(inst->ctors, 0, args, e);
    nw = lit_node(N_NEW, inst->type, e);
    nw->sym = call->sym;
    nw->list = call->list;
    return make_conv(nw, t, 0);
}

/* the implicit conversion of a value to t (spec 8.1): an error if there is
   none, or if it is narrowing and Option Strict is On */
static Node *coerce(Node *e, Type *t) {
    int k;
    if (e->kind == N_COLLINIT && !e->type) return bind_collinit(e, t);
    if (is_pending(e)) return pending_to(e, t);
    k = conv_class_of(e, t);
    if (k == CV_NONE && (ty_user(e->type) || ty_user(t))) {
        Node *u = user_conversion(e, t, 0);
        if (u) return u;
    }
    if (k == CV_NONE) no_conversion(e, e->type, t);
    if (k == CV_NARROW && opts->strict)
        serr(e, "Option Strict On disallows implicit conversions from '%s' to '%s'.", type_name(e->type), type_name(t));
    if (e->type->kind == TY_ARRAY && t->kind == TY_INTERFACE) return array_wrap(e, t);
    return make_conv(e, t, 0);
}

/* an explicit conversion (CInt, CType, ...) */
static Node *explicit_conv(Node *e, Type *t, Node *at) {
    if (e->kind == N_COLLINIT && !e->type) return bind_collinit(e, t);
    if (is_pending(e)) return pending_to(e, t);
    if (conv_class(e->type, t) == CV_NONE && (ty_user(e->type) || ty_user(t))) {
        Node *u = user_conversion(e, t, 1);
        if (u) return u;
    }
    if (conv_class(e->type, t) == CV_NONE) no_conversion(at, e->type, t);
    return make_conv(e, t, 1);
}

/* the dominant type of a set of expressions (spec 8.13): the one type that
   all the others widen to; NULL if there is none */
static Type *dominant(Node **v, int n) {
    int i, j;
    Type *best = 0;
    for (i = 0; i < n; i++) {
        Type *c = v[i]->type;
        int ok = 1;
        if (c->kind == TY_NOTHING) continue;
        for (j = 0; j < n && ok; j++) {
            int k = conv_class_of(v[j], c);
            if (k != CV_IDENT && k != CV_WIDEN) ok = 0;
        }
        if (ok && (!best || conv_class(c, best) == CV_WIDEN)) best = c;
    }
    if (!best) {
        for (i = 0; i < n; i++) if (v[i]->type->kind != TY_NOTHING) return 0;
        return ty[TY_OBJECT];
    }
    return best;
}
