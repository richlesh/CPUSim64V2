/*
 * sema_decl.c - declarations (spec 6, 7 and 9): namespaces, modules,
 * methods, fields and constants, the file options and imports, and the
 * passes of the semantic analysis.
 */

/* ------------------------------------------------------------ pass 1: declarations */

SymVec prog_types;
static int sema_phase = 1;  /* the pass of the semantic analysis (sema_main.c) */
static Node *inst_decl;     /* the copied declaration that declare_type makes a generic instance of */
static void declare_type(Node *d, Sym *container);     /* sema_type.c */
static void declare_event(Sym *c, Node *d);

static Sym *new_type_sym(Node *d, Sym *container, int kind) {
    Sym *s = new_sym(S_TYPE, d->name, d->text);
    Type *t = xalloc(sizeof(Type));
    Sym *old = d == inst_decl ? 0 : scope_find(container->members, d->name);
    int chained = 0;
    if (old && old->kind == S_TYPE && (old->sflags & SF_GENERIC)) {
        /* Foo and Foo(Of T) can both be declared */
        Sym *o;
        for (o = old; o; o = o->overload)
            if (!(o->sflags & SF_GENERIC)) serr(d, "'%s' is already declared as a type in this %s.", d->text,
                                                 container->kind == S_NAMESPACE ? "namespace" : "type");
        for (o = old; o->overload; o = o->overload) ;
        o->overload = s;
        chained = 1;
    } else if (old) serr(d, "'%s' is already declared as %s in this %s.", d->text, old->kind == S_TYPE ? "a type" : "a member",
                  container->kind == S_NAMESPACE ? "namespace" : container->kind == S_MODULE ? "module" : "type");
    s->decl = d;
    s->owner = container;
    s->mods = d->mods | M_SHARED;
    s->file = d->file;
    s->members = new_scope(SC_TYPE, 0, s);
    s->type = t;
    s->desc = prog_types.n;
    t->kind = kind;
    t->name = s->spelling;
    t->sym = s;
    if (d == inst_decl) inst_decl = 0;          /* a generic instance: not in its container's scope */
    else if (!chained) scope_add(container->members, s);
    sv_push(&prog_types, s);
    d->sym = s;
    return s;
}

/* Enum E [As T]: the members are constants of type E, evaluated on demand */
static void declare_enum(Node *d, Sym *container) {
    Sym *s = new_type_sym(d, container, TY_ENUM);
    int i;
    for (i = 0; i < d->list->n; i++) {
        Node *mn = d->list->v[i];
        Sym *c = new_sym(S_CONST, mn->name, mn->text);
        if (scope_find(s->members, mn->name)) serr(mn, "'%s' is already declared in this enum.", mn->text);
        c->decl = mn;
        c->owner = s;
        c->mods = M_PUBLIC | M_SHARED;
        c->file = mn->file;
        mn->sym = c;
        scope_add(s->members, c);
        sv_push(&s->fields, c);
    }
}

static const char *member_kind_text(Sym *s) {
    switch (s->kind) {
    case S_METHOD: return "a method";
    case S_FIELD: return "a variable";
    case S_CONST: return "a constant";
    case S_MODULE: return "a module";
    case S_NAMESPACE: return "a namespace";
    }
    return "a member";
}

static void add_member(Sym *container, Sym *s, Node *at) {
    Sym *old = scope_find(container->members, s->name);
    if (old) {
        if (old->kind == S_METHOD && s->kind == S_METHOD) {
            while (old->overload) old = old->overload;
            old->overload = s;
            return;
        }
        serr(at, "'%s' is already declared as %s in this %s.", s->spelling, member_kind_text(old),
             container->kind == S_MODULE ? "module" : "namespace");
    }
    scope_add(container->members, s);
}

static Sym *member_sym(int kind, Node *n, Sym *owner, int mods) {
    Sym *s = new_sym(kind, n->name, n->text);
    s->decl = n;
    s->owner = owner;
    s->mods = mods;
    s->file = n->file;
    return s;
}

static void declare_module(Node *m, Sym *ns) {
    Sym *mod = member_sym(S_MODULE, m, ns, m->mods), *old = scope_find(ns->members, m->name);
    int i, j, k;
    if (old) {
        if (old->kind == S_MODULE)
            serr(m, "module '%s' and module '%s' conflict in namespace '%s'.", m->text, old->spelling,
                 ns == ns_global ? "<Default>" : qual_name(ns));
        serr(m, "'%s' is already declared as %s in this namespace.", m->text, member_kind_text(old));
    }
    mod->members = new_scope(SC_MODULE, 0, mod);
    scope_add(ns->members, mod);
    sv_push(&prog_modules, mod);
    m->sym = mod;
    for (i = 0; i < m->list3->n; i++) {
        Node *d = m->list3->v[i];
        switch (d->kind) {
        case N_METHOD: {
            Sym *s = member_sym(S_METHOD, d, mod, d->mods | M_SHARED);
            add_member(mod, s, d);
            if (d->list && d->list->n) {
                s->sflags |= SF_GENERIC;       /* a template: instantiated by the calls */
                d->sym = s;
                break;
            }
            sv_push(&mod->methods, s);
            d->sym = s;
            break;
        }
        case N_FIELD: case N_CONSTDECL:
            for (j = 0; j < d->list->n; j++) {
                Node *decl = d->list->v[j];
                for (k = 0; k < decl->list->n; k++) {
                    Node *vi = decl->list->v[k];
                    Sym *s = member_sym(d->kind == N_FIELD ? S_FIELD : S_CONST, vi, mod, d->mods | M_SHARED);
                    vi->parent = decl;
                    if (d->kind == N_FIELD && (d->mods & M_CONST)) s->kind = S_CONST;
                    add_member(mod, s, vi);
                    if (s->kind == S_FIELD) sv_push(&mod->fields, s);
                    vi->sym = s;
                }
            }
            break;
        case N_ENUM: declare_enum(d, mod); break;
        case N_CLASS: case N_STRUCTURE: case N_INTERFACE: declare_type(d, mod); break;
        case N_PROPERTY: not_yet(d, "Properties are");
        case N_EVENT: declare_event(mod, d); break;
        case N_CTOR: not_yet(d, "Constructors are");
        case N_OPERATOR: not_yet(d, "Operators are");
        case N_DELEGATE: declare_type(d, mod); break;
        default: not_yet(d, "Nested types are");
        }
    }
}

static void declare_members(NodeVec *v, Sym *ns) {
    int i, k;
    for (i = 0; i < v->n; i++) {
        Node *m = v->v[i];
        switch (m->kind) {
        case N_NAMESPACE: {
            Sym *inner = (m->a->flags & F_GLOBAL) ? ns_global : ns;
            for (k = 0; k < m->a->list->n; k++) {
                Node *p = m->a->list->v[k];
                Sym *old = scope_find(inner->members, p->name);
                if (old && old->kind != S_NAMESPACE)
                    serr(p, "'%s' is already declared as %s in this namespace.", p->text, member_kind_text(old));
                inner = ns_child(inner, p->name, p->text);
            }
            m->sym = inner;
            declare_members(m->list, inner);
            break;
        }
        case N_MODULE: declare_module(m, ns); break;
        case N_CLASS: case N_STRUCTURE: case N_INTERFACE: declare_type(m, ns); break;
        case N_ENUM: declare_enum(m, ns); break;
        case N_DELEGATE: declare_type(m, ns); break;
        }
    }
}

/* ------------------------------------------------------------ pass 2: options, imports */

static const char *project_imports[] = {
    "Microsoft.VisualBasic", "System", "System.Collections", "System.Collections.Generic",
    "System.Diagnostics", "System.Linq", 0
};

static void bind_options(Node *f, int file) {
    Opts *o = &file_opts[file];
    int i, seen = 0;
    o->strict = 0;
    o->explicit_ = 1;
    o->infer = 1;
    o->compare_text = 0;
    opts = o;
    cur_ns = ns_global;
    cur_module = 0;
    cur_method = 0;
    cur_scope = 0;
    for (i = 0; i < f->list->n; i++) {
        Node *n = f->list->v[i];
        if (n->kind == N_OPTION) {
            int bit = n->name == intern_word("strict") ? 1 : n->name == intern_word("explicit") ? 2 :
                      n->name == intern_word("infer") ? 4 : 8;
            int on = strcmp(n->text, "On") == 0 || strcmp(n->text, "Text") == 0;
            if (seen & bit) {
                const char *w = bit == 1 ? "Strict" : bit == 2 ? "Explicit" : bit == 4 ? "Infer" : "Compare";
                serr(n, "'Option %s' statement can only appear once per file.", w);
            }
            seen |= bit;
            if (bit == 1) o->strict = on;
            else if (bit == 2) o->explicit_ = on;
            else if (bit == 4) o->infer = on;
            else o->compare_text = on;
        } else {
            Sym *s = bind_type_name(n->a);
            if (n->name) {
                o->alias_names = xrealloc(o->alias_names, sizeof(Name *) * (o->naliases + 1));
                o->alias_syms = xrealloc(o->alias_syms, sizeof(Sym *) * (o->naliases + 1));
                o->alias_names[o->naliases] = n->name;
                o->alias_syms[o->naliases] = s;
                o->naliases++;
            } else {
                if (s->kind == S_MODULE) serr(n, "'Imports' of a module is not supported; import its namespace.");
                sv_push(&o->imports, s);
            }
        }
    }
    for (i = 0; project_imports[i]; i++) sv_push(&o->imports, ns_path(project_imports[i]));
}

/* ------------------------------------------------------------ pass 3: signatures */

static void enter(Sym *s) {
    Sym *o = s->owner;
    set_file(s->file);
    cur_method = 0;
    cur_scope = 0;
    cur_type = 0;
    {
        Sym *t;
        for (t = s->kind == S_TYPE ? s : o; t; t = t->owner)
            if (t->kind == S_TYPE && t->type && (t->type->kind == TY_CLASS || t->type->kind == TY_STRUCT ||
                                                  t->type->kind == TY_INTERFACE)) {
                cur_type = t;
                break;
            }
    }
    while (o && o->kind != S_MODULE && o->kind != S_NAMESPACE) o = o->owner;
    cur_module = (o && o->kind == S_MODULE) ? o : 0;
    while (o && o->kind != S_NAMESPACE) o = o->owner;
    cur_ns = o ? o : ns_global;
    cur_tpscope = s->kind == S_METHOD ? s->tparams : 0;
}

static int same_signature(Sym *a, Sym *b) {
    int i;
    if (a->params.n != b->params.n) return 0;
    for (i = 0; i < a->params.n; i++)
        if (a->params.v[i]->type != b->params.v[i]->type) return 0;
    return 1;
}

/* the parameters of a method */
static Type *bind_signature_params(Sym *m, NodeVec *params) {
    int i, j, optional_seen = 0;
    if (!params) return 0;
    for (i = 0; i < params->n; i++) {
        Node *pn = params->v[i];
        Sym *p = new_sym(S_PARAM, pn->name, pn->text);
        Type *t = pn->a ? bind_type(pn->a) : 0;
        p->decl = pn;
        p->owner = m;
        p->file = pn->file;
        p->mods = pn->mods;
        if (!(p->mods & M_BYREF)) p->mods |= M_BYVAL;
        if (pn->tchar) {
            if (t) serr(pn, "Type character '%c' cannot be used in a declaration with an explicit type.", pn->tchar);
            t = tchar_type(pn->tchar, pn);
        }
        if (pn->list) serr(pn, "Array bounds cannot appear in type specifiers.");
        if (!t) {
            if (opts->strict) serr(pn, "Option Strict On requires that all method parameters have an 'As' clause.");
            t = ty[TY_OBJECT];
        }
        if (pn->list2)
            for (j = pn->list2->n - 1; j >= 0; j--) t = ty_array(t, (int)pn->list2->v[j]->ival);
        if (pn->flags & F_NULLABLE) t = nullable_of(t, pn);
        p->type = t;
        p->index = m->params.n;
        for (j = 0; j < m->params.n; j++)
            if (m->params.v[j]->name == p->name) serr(pn, "Parameter already declared with name '%s'.", pn->text);
        if (m->params.n && (m->params.v[m->params.n - 1]->mods & M_PARAMARRAY))
            serr(pn, "End of parameter list expected. Cannot define parameters after a paramarray parameter.");
        if (p->mods & M_PARAMARRAY) {
            if (t->kind != TY_ARRAY || t->rank != 1) serr(pn, "ParamArray parameter must be a one-dimensional array.");
            if (p->mods & M_BYREF) serr(pn, "ParamArray parameters must be declared 'ByVal'.");
            if (optional_seen) serr(pn, "ParamArray parameters cannot be combined with Optional parameters.");
        }
        if (p->mods & M_OPTIONAL) {
            optional_seen = 1;
            if (!pn->b) serr(pn, "Optional parameters must specify a default value.");
            p->cval = const_value(pn->b, t, pn);
        } else {
            if (pn->b) serr(pn, "Only 'Optional' parameters can have default values.");
            if (optional_seen && !(p->mods & M_PARAMARRAY)) serr(pn, "'Optional' expected.");
        }
        pn->sym = p;
        pn->type = t;
        sv_push(&m->params, p);
    }
    return 0;
}

static void bind_signature(Sym *m) {
    Node *d = m->decl;
    enter(m);
    if (d->list && d->kind == N_METHOD) not_yet(d, "Generic methods are");

    bind_signature_params(m, d->list2);
    if (m->mods & M_EXTENSION) {
        if (!m->owner || m->owner->kind != S_MODULE)
            serr(d, "'Extension' attribute can be applied only to 'Module', 'Sub', or 'Function' declarations.");
        if (!m->params.n || (m->params.v[0]->mods & (M_OPTIONAL | M_PARAMARRAY)))
            serr(d, "Extension method '%s' has no parameters or its first parameter is 'Optional' or 'ParamArray'.", m->spelling);
    }
    if (d->op == K_Function || d->kind == N_OPERATOR) {
        Type *t = d->a ? bind_type(d->a) : 0;
        if (d->tchar) {
            if (t) serr(d, "Type character '%c' cannot be used in a declaration with an explicit type.", d->tchar);
            t = tchar_type(d->tchar, d);
        }
        if (!t) {
            if (opts->strict) serr(d, "Option Strict On requires all Function, Property, and Operator declarations to have an 'As' clause.");
            t = ty[TY_OBJECT];
        }
        m->type = t;
    }
    d->type = m->type;
}

static void check_overloads(Sym *mod) {
    Sym *s, *a, *b;
    for (s = mod->members->syms; s; s = s->next) {
        if (s->kind != S_METHOD) continue;
        for (a = s; a; a = a->overload)
            for (b = a->overload; b; b = b->overload)
                if (!(a->sflags & SF_GENERIC) && !(b->sflags & SF_GENERIC) && same_signature(a, b)) {
                    if (a->type != b->type)
                        serr(b->decl, "'%s' and '%s' cannot overload each other because they differ only by return types.",
                             method_sig(a), method_sig(b));
                    serr(b->decl, "'%s' has multiple definitions with identical signatures.", method_sig(b));
                }
    }
}

/* ------------------------------------------------------------ pass 4: fields, constants */

static void bind_field(Sym *s) {
    Node *vi = s->decl, *d = vi->parent, *init = 0;
    Type *as, *t;
    enter(s);
    as = d->a ? bind_type(d->a) : 0;
    if (d->flags & F_ASNEW) {
        Node *nw = xalloc(sizeof(Node));
        *nw = *d->b;
        init = bind(nw, 0);
        s->type = init->type;
        s->init = init;
        s->index = prog_fields.n;
        sv_push(&prog_fields, s);
        vi->type = s->type;
        vi->b = init;
        return;
    }
    if (d->b && d->list->n > 1)
        serr(d, "Explicit initialization is not permitted with multiple variables declared with a single type specifier.");
    if (vi->list && d->b) serr(vi, "Explicit initialization is not permitted for arrays declared with explicit bounds.");
    t = var_type(vi, as, vi);
    if (!t) {
        if (opts->strict) serr(vi, "Option Strict On requires all variable declarations to have an 'As' clause.");
        t = ty[TY_OBJECT];
    }
    bind_bounds(vi->list);
    if (d->b) {
        init = bind(d->b, 0);
        if (init->kind == N_COLLINIT && !init->type) init = bind_collinit(init, t);
        init = coerce(value(init), t);
    }
    if (!init && t->kind == TY_STRUCT) {
        init = lit_node(N_NEW, t, vi);     /* the zero value */
        init->list = nv_new();
    }
    s->type = t;
    s->init = init;
    d->b = 0;
    s->index = prog_fields.n;
    sv_push(&prog_fields, s);
    vi->type = t;
    vi->b = init;
}

static void eval_const(Sym *s, Node *at) {
    Sym *sv_module = cur_module, *sv_method = cur_method, *sv_ns = cur_ns;
    Scope *sv_scope = cur_scope;
    Opts *sv_opts = opts;
    Node *vi, *d;
    Type *t;
    int sv_nctl = nctl;
    if (s->state == 2) return;
    if (s->state == 1) serr(s->decl, "The constant '%s' cannot depend on its own value.", s->spelling);
    (void)at;
    s->state = 1;
    enter(s);
    if (s->owner && s->owner->kind == S_TYPE && s->owner->type->kind == TY_ENUM) {
        Sym *e = s->owner;
        Type *et = e->type, *ut = et->elem;
        Node *mn = s->decl, *v;
        int k;
        cur_scope = e->members;     /* the other members by their simple names */
        if (mn->a) v = coerce(rvalue(mn->a), ut);
        else {
            for (k = 0; k < e->fields.n && e->fields.v[k] != s; k++) ;
            if (k == 0) v = const_convert(lit_node(N_NOTHING, ty[TY_NOTHING], mn), ut, mn);
            else {
                Node *prev, one;
                Sym *ps = e->fields.v[k - 1];
                eval_const(ps, mn);
                prev = const_convert(ps->cval, ut, mn);
                memset(&one, 0, sizeof one);
                one.kind = N_INT;
                one.type = ut;
                one.ival = 1;
                one.cls = EC_VALUE;
                v = fold_binary(mn, P_PLUS, ut, prev, &one);
            }
        }
        if (!is_const(v)) serr(mn->a ? mn->a : mn, "Constant expression is required.");
        v = copy_const(v, mn);
        v->type = et;
        s->cval = v;
        s->type = et;
        mn->type = et;
        s->state = 2;
        cur_module = sv_module;
        cur_method = sv_method;
        cur_ns = sv_ns;
        cur_scope = sv_scope;
        opts = sv_opts;
        nctl = sv_nctl;
        return;
    }
    vi = s->decl;
    d = vi->parent;
    t = d->a ? bind_type(d->a) : 0;
    t = var_type(vi, t, vi);
    if (vi->list2) serr(vi, "Constants must be of an intrinsic or enumerated type, not a class, structure, type parameter, or array type.");
    if (!t && opts->strict && !opts->infer)
        serr(vi, "Option Strict On requires all variable declarations to have an 'As' clause.");
    s->cval = const_value(d->b, t, vi);
    s->type = s->cval->type;
    vi->type = s->type;
    vi->b = s->cval;
    s->state = 2;
    if (d->list->n == 1) d->b = 0;
    cur_module = sv_module;
    cur_method = sv_method;
    cur_ns = sv_ns;
    cur_scope = sv_scope;
    opts = sv_opts;
    nctl = sv_nctl;
}

/* ------------------------------------------------------------ pass 5: bodies */

/* the base constructor that a constructor calls implicitly (without
   MyBase.New / Me.New as its first statement) */
static void ctor_chain(Sym *m, Node *first) {
    Sym *b = m->owner->base;
    NodeVec *none;
    Node *me, *call;
    if (first || !b) return;
    if (!b->ctors) return;
    none = nv_new();
    me = lit_node(N_ME, m->owner->type, m->decl);
    me->flags |= F_DIRECT;
    {
        /* does the base have a constructor that takes no arguments? */
        Sym *c;
        int ok = 0, i;
        for (c = b->ctors; c; c = c->overload) {
            int req = 0;
            for (i = 0; i < c->params.n; i++)
                if (!(c->params.v[i]->mods & (M_OPTIONAL | M_PARAMARRAY))) req++;
            if (!req && accessible(c)) ok = 1;
        }
        if (!ok)
            serr(m->decl, "First statement of this 'Sub New' must be a call to 'MyBase.New' or 'MyClass.New' because base class "
                          "'%s' of '%s' does not have an accessible 'Sub New' that can be called with no arguments.",
                 b->spelling, m->owner->spelling);
    }
    call = resolve_call(b->ctors, me, none, m->decl);
    m->cval = call;     /* the implicit MyBase.New() */
}

static void bind_method(Sym *m) {
    Node *d = m->decl;
    int i;
    enter(m);
    cur_method = m;
    cur_scope = new_scope(SC_METHOD, 0, m);
    cur_scope->materialized = 1;
    m->scope = cur_scope;
    for (i = 0; i < m->params.n; i++) {
        scope_add(cur_scope, m->params.v[i]);
        m->params.v[i]->scope = cur_scope;
    }
    if (m->type) {
        Sym *r = new_sym(S_RETVAL, m->name, m->spelling);
        r->type = m->type;
        r->owner = m;
        r->decl = d;
        r->index = m->locals.n;
        sv_push(&m->locals, r);
        scope_add(cur_scope, r);
        m->retval = r;
    }
    nctl = 0;
    nlabels = ngotos = 0;
    ctor_first_stmt = 0;
    if ((m->sflags & SF_CTOR) && (m->sflags & SF_INSTANCE) && d->list3 && d->list3->n) {
        Node *f = d->list3->v[0];
        Node *c = f->kind == N_CALLSTMT ? f->a : 0;
        if (c && c->kind == N_INVOKE) c = c->a;
        if (c && c->kind == N_MEMBER && c->name == intern_word("new") && c->a &&
            (c->a->kind == N_MYBASE || c->a->kind == N_ME || c->a->kind == N_MYCLASS))
            ctor_first_stmt = f;
    }
    bind_block(d->list3);
    if ((m->sflags & SF_CTOR) && (m->sflags & SF_INSTANCE)) ctor_chain(m, ctor_first_stmt);
    check_gotos();
    m->body = d->list3;
    cur_method = 0;
    cur_scope = 0;
}

