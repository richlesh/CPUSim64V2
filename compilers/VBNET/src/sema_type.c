/*
 * sema_type.c - classes and structures (spec 7.5, 7.6 and 9): their
 * declarations, fields, methods, constructors and properties.
 *
 * Instance fields have a slot (Sym.index) in the object: an object p of a
 * class (or the box of a structure value) has its fields at p[0 .. size-1].
 * Instance methods receive the object (Me) as an extra parameter.  The
 * instance field initializers form a method of their own (Type.init) that
 * every constructor calls first; a class without Sub New gets an implicit
 * Public Sub New().  Properties have getter and setter methods; an
 * auto-implemented property is its backing field _Name.
 */

static void declare_type(Node *d, Sym *container);

static Sym *type_member(Sym *t, int kind, Node *n, Name *name, const char *spelling, int mods) {
    Sym *s = new_sym(kind, name, spelling);
    s->decl = n;
    s->owner = t;
    s->mods = mods;
    s->file = n->file;
    if (!(mods & M_SHARED)) s->sflags |= SF_INSTANCE;
    return s;
}

/* an accessor or constructor: a method that is not looked up by name */
static Sym *hidden_method(Sym *t, Node *n, const char *spelling, int mods) {
    Sym *s = type_member(t, S_METHOD, n, intern_word(spelling), spelling, mods);
    sv_push(&t->methods, s);
    return s;
}

static void add_type_member(Sym *t, Sym *s, Node *at) {
    Sym *old = scope_find(t->members, s->name);
    if (old) {
        if (old->kind == S_METHOD && s->kind == S_METHOD) {
            while (old->overload) old = old->overload;
            old->overload = s;
            return;
        }
        serr(at, "'%s' is already declared as %s in this %s.", s->spelling, member_kind_text(old),
             t->type->kind == TY_STRUCT ? "structure" : "class");
    }
    scope_add(t->members, s);
}

static void declare_field(Sym *t, Node *d, int kind) {
    int j, k;
    for (j = 0; j < d->list->n; j++) {
        Node *decl = d->list->v[j];
        for (k = 0; k < decl->list->n; k++) {
            Node *vi = decl->list->v[k];
            int mods = d->mods | (kind == S_CONST ? M_SHARED : 0);
            Sym *s = type_member(t, kind, vi, vi->name, vi->text, mods);
            vi->parent = decl;
            vi->sym = s;
            add_type_member(t, s, vi);
            if (kind == S_FIELD && (s->sflags & SF_INSTANCE)) {
                s->index = t->fields.n;
                sv_push(&t->fields, s);
            } else if (kind == S_FIELD) sv_push(&t->methods, s);   /* bound with the shared fields */
        }
    }
}

static void declare_property(Sym *t, Node *d) {
    Sym *p = type_member(t, S_PROP, d, d->name, d->text, d->mods);
    int i, has_get = 0, has_set = 0;
    d->sym = p;
    add_type_member(t, p, d);
    if ((d->mods & M_DEFAULT) && !(d->list2 && d->list2->n))
        serr(d, "Properties with no required parameters cannot be declared 'Default'.");
    if (t->type->kind == TY_INTERFACE || (d->mods & M_MUSTOVERRIDE)) {
        /* no accessors: Get unless WriteOnly, Set unless ReadOnly */
        if (!(d->mods & M_WRITEONLY)) {
            p->getter = hidden_method(t, d, d->text, d->mods);
            p->getter->sflags |= SF_ACCESSOR;
            p->getter->field = p;
        }
        if (!(d->mods & M_READONLY)) {
            p->setter = hidden_method(t, d, d->text, d->mods);
            p->setter->sflags |= SF_ACCESSOR;
            p->setter->field = p;
        }
        return;
    }
    if (d->flags & F_AUTO) {
        if (d->mods & (M_OVERRIDABLE | M_OVERRIDES | M_MUSTOVERRIDE)) not_yet(d, "Overridable auto-implemented properties are");
        /* the backing field _Name */
        StrBuf b;
        Sym *f;
        if (d->list2 && d->list2->n) serr(d, "Auto-implemented properties cannot have parameters.");
        sb_init(&b);
        sb_putc(&b, '_');
        sb_puts(&b, d->text);
        f = type_member(t, S_FIELD, d, intern_word(b.s), b.s, (d->mods & M_SHARED) | M_PRIVATE);
        if (scope_find(t->members, f->name)) serr(d, "'%s' is already declared in this class.", b.s);
        scope_add(t->members, f);
        if (f->sflags & SF_INSTANCE) {
            f->index = t->fields.n;
            sv_push(&t->fields, f);
        } else sv_push(&t->methods, f);
        p->field = f;
        p->sflags |= SF_AUTO;
        return;
    }
    for (i = 0; i < d->list3->n; i++) {
        Node *a = d->list3->v[i];
        int mods = (d->mods & ~(M_PUBLIC | M_PRIVATE | M_FRIEND | M_PROTECTED)) |
                   (a->mods ? a->mods : (d->mods & (M_PUBLIC | M_PRIVATE | M_FRIEND | M_PROTECTED)));
        Sym *m;
        if (a->op == K_Get) {
            if (has_get++) serr(a, "'Get' is already declared.");
            m = hidden_method(t, a, d->text, mods);
            p->getter = m;
        } else {
            if (has_set++) serr(a, "'Set' is already declared.");
            m = hidden_method(t, a, d->text, mods);
            p->setter = m;
        }
        m->sflags |= SF_ACCESSOR;
        m->field = p;
        a->sym = m;
    }
    if ((d->mods & M_READONLY) && has_set) serr(d, "Properties declared 'ReadOnly' cannot have a 'Set'.");
    if ((d->mods & M_WRITEONLY) && has_get) serr(d, "Properties declared 'WriteOnly' cannot have a 'Get'.");
    if ((d->mods & M_READONLY) && !has_get) serr(d, "Property declared 'ReadOnly' must have a 'Get'.");
    if ((d->mods & M_WRITEONLY) && !has_set) serr(d, "Property declared 'WriteOnly' must have a 'Set'.");
    if (!(d->mods & (M_READONLY | M_WRITEONLY)) && (!has_get || !has_set))
        serr(d, "Property without a 'ReadOnly' or 'WriteOnly' specifier must provide both a 'Get' and a 'Set'.");
}

/* the operator methods: "operator +", "operator ctype", ... */
static Name *operator_name(Node *m) {
    StrBuf b;
    Name *n;
    sb_init(&b);
    sb_puts(&b, "operator ");
    sb_puts(&b, m->op ? tok_spelling_of(m->op) : m->text);
    n = intern_word(b.s);
    free(b.s);
    return n;
}

static void declare_type_template(Node *d, Sym *container);    /* sema_generic.c */
static Node *copy_tree(Node *n);

/* a delegate type: a class derived from System.MulticastDelegate with the
   method Invoke (the code generator calls the methods of the invocation list) */
static void declare_delegate(Node *d, Sym *container) {
    Sym *t = new_type_sym(d, container, TY_CLASS), *inv;
    t->mods = d->mods | M_NOTINHERITABLE;
    t->sflags |= SF_DELEGATE;
    t->body = nv_new();
    nv_push(t->body, d);
    inv = type_member(t, S_METHOD, d, intern_word("invoke"), "Invoke", M_PUBLIC);
    inv->rt = "@invoke";
    inv->vslot = -1;
    add_type_member(t, inv, d);
    sv_push(&t->methods, inv);
    t->invoke = inv;
}

/* Event E(params) [As T]: the event, its delegate type (EEventHandler for
   the parameter form) and its field EEvent */
static void declare_event(Sym *c, Node *d) {
    int in_module = c->kind == S_MODULE;
    int mods = d->mods | (in_module ? M_SHARED : 0);
    Sym *ev, *f;
    StrBuf b;
    if (d->flags & F_CUSTOM) not_yet(d, "Custom events are");
    ev = new_sym(S_EVENT, d->name, d->text);
    ev->decl = d;
    ev->owner = c;
    ev->mods = mods;
    ev->file = d->file;
    if (!(mods & M_SHARED)) ev->sflags |= SF_INSTANCE;
    if (!d->a) {
        /* the delegate type: Delegate Sub EEventHandler(params) */
        Node *dn = lit_node(N_DELEGATE, 0, d);
        dn->cls = EC_NONE;
        dn->type = 0;
        dn->op = K_Sub;
        dn->mods = M_PUBLIC;
        dn->list2 = d->list2 ? d->list2 : nv_new();
        sb_init(&b);
        sb_puts(&b, d->text);
        sb_puts(&b, "EventHandler");
        dn->text = b.s;
        dn->name = intern_word(b.s);
        declare_type(dn, c);
        ev->field = 0;
        ev->invoke = dn->sym;       /* the delegate type's symbol, until the types are bound */
    }
    sb_init(&b);
    sb_puts(&b, d->text);
    sb_puts(&b, "Event");
    f = new_sym(S_FIELD, intern_word(b.s), b.s);
    f->decl = d;
    f->owner = c;
    f->mods = M_PRIVATE | (mods & M_SHARED);
    f->file = d->file;
    if (!(mods & M_SHARED)) f->sflags |= SF_INSTANCE;
    if (scope_find(c->members, f->name)) serr(d, "'%s' is already declared in this class.", b.s);
    scope_add(c->members, f);
    if (f->sflags & SF_INSTANCE) {
        f->index = c->fields.n;
        sv_push(&c->fields, f);
    } else sv_push(&c->methods, f);         /* bound with the shared fields */
    ev->field = f;
    if (c->kind == S_MODULE) add_member(c, ev, d);
    else add_type_member(c, ev, d);
    d->sym = ev;
}

/* the delegate type of an event */
static void bind_event_type(Sym *ev) {
    Node *d = ev->decl;
    Type *t;
    if (d->a) {
        enter(ev);
        t = bind_type(d->a);
        if (t->kind != TY_CLASS || !(t->sym->sflags & SF_DELEGATE))
            serr(d->a, "Event '%s' cannot be of type '%s' because it is not a delegate type.", d->text, type_name(t));
    } else t = ev->invoke->type;
    ev->type = t;
    ev->field->type = t;
    d->type = t;
}

static void declare_type(Node *d, Sym *container) {
    int is_struct = d->kind == N_STRUCTURE, i;
    if (d->list && d->list->n && d != inst_decl) {
        declare_type_template(d, container);
        return;
    }
    if (d->kind == N_DELEGATE) {
        declare_delegate(d, container);
        return;
    }
    {
    int kind = is_struct ? TY_STRUCT : d->kind == N_INTERFACE ? TY_INTERFACE : TY_CLASS;
    Sym *t = d == inst_decl ? 0 : scope_find(container->members, d->name);
    int is_iface = d->kind == N_INTERFACE;
    Sym *ctors = 0;
    if (t && t->kind == S_TYPE && t->type->kind == kind && ((t->mods | d->mods) & M_PARTIAL)) {
        /* another part of a Partial type */
        t->mods |= d->mods;
        d->sym = t;
        ctors = t->ctors;
    } else {
        t = new_type_sym(d, container, kind);
        t->mods = d->mods;
        t->body = nv_new();
    }
    nv_push(t->body, d);    /* the parts */
    for (i = 0; i < d->list3->n; i++) {
        Node *m = d->list3->v[i];
        switch (m->kind) {
        case N_METHOD: {
            Sym *s;
            s = type_member(t, S_METHOD, m, m->name, m->text, m->mods);
            add_type_member(t, s, m);
            m->sym = s;
            if (m->list && m->list->n) {
                /* a generic method: a template */
                if (m->mods & (M_OVERRIDABLE | M_OVERRIDES | M_MUSTOVERRIDE) || is_iface)
                    not_yet(m, "Overridable and interface generic methods are");
                s->sflags |= SF_GENERIC;
                break;
            }
            sv_push(&t->methods, s);
            break;
        }
        case N_CTOR: {
            Sym *s = type_member(t, S_METHOD, m, intern_word("new"), "New", m->mods);
            s->sflags |= SF_CTOR;
            if (m->mods & M_SHARED) {
                if (m->list2->n) serr(m, "Shared 'Sub New' cannot have any parameters.");
                if (t->sctor) serr(m, "Shared 'Sub New' is already declared.");
                t->sctor = s;
            } else {
                if (is_struct && m->list2->n == 0)
                    serr(m, "Structures cannot declare a non-shared 'Sub New' with no parameters.");
                if (!ctors) ctors = t->ctors;
                if (ctors) {
                    Sym *c = ctors;
                    while (c->overload) c = c->overload;
                    c->overload = s;
                } else ctors = s;
            }
            sv_push(&t->methods, s);
            m->sym = s;
            break;
        }
        case N_FIELD: declare_field(t, m, S_FIELD); break;
        case N_CONSTDECL: declare_field(t, m, S_CONST); break;
        case N_PROPERTY: declare_property(t, m); break;
        case N_ENUM: declare_enum(m, t); break;
        case N_CLASS: case N_STRUCTURE: case N_INTERFACE: declare_type(m, t); break;
        case N_EVENT:
            if (is_iface) not_yet(m, "Interface events are");
            declare_event(t, m);
            break;
        case N_OPERATOR: {
            Sym *s = type_member(t, S_METHOD, m, operator_name(m), 0, m->mods | M_SHARED);
            s->spelling = s->name->key;
            if (!(m->mods & M_PUBLIC) || !(m->mods & M_SHARED))
                serr(m, "Operators must be declared 'Public' and 'Shared'.");
            if (is_iface) serr(m, "Statement is not valid in an interface.");
            m->op = m->op ? m->op : 0;
            add_type_member(t, s, m);
            sv_push(&t->methods, s);
            m->sym = s;
            break;
        }
        case N_DELEGATE: declare_type(m, t); break;
        default: not_yet(m, "This kind of member is");
        }
    }
    if (is_iface) {
        /* interface members are Public and MustOverride */
        Sym *s;
        for (s = t->members->syms; s; s = s->next) s->mods |= M_PUBLIC | M_MUSTOVERRIDE;
        for (i = 0; i < t->methods.n; i++) t->methods.v[i]->mods |= M_PUBLIC | M_MUSTOVERRIDE;
        return;
    }
    t->ctors = ctors;
    }
}

/* after all the parts: the implicit Public Sub New() of a class without one */
static void implicit_ctor(Sym *t) {
    Sym *c;
    if (t->ctors || t->type->kind != TY_CLASS || (t->sflags & SF_DELEGATE)) return;
    c = type_member(t, S_METHOD, t->decl, intern_word("new"), "New", M_PUBLIC);
    c->sflags |= SF_CTOR;
    sv_push(&t->methods, c);
    t->ctors = c;
}

/* ------------------------------------------------------------ bases, layout */

/* a member of a type or of its base classes */
static Sym *type_lookup(Sym *t, Name *n) {
    Sym *s;
    int i;
    for (; t; t = t->base) {
        s = scope_find(t->members, n);
        if (s) return s;
        if (t->type->kind == TY_INTERFACE)
            for (i = 0; i < t->ifaces.n; i++) {
                s = type_lookup(t->ifaces.v[i], n);
                if (s) return s;
            }
    }
    return 0;
}

static void bind_base(Sym *t) {
    int p;
    if (t->sflags & SF_DELEGATE) {
        Sym *b = scope_find(ns_path("System")->members, intern_word("multicastdelegate"));
        if (!b || b->kind != S_TYPE) serr(t->decl, "lib/System.vb has no System.MulticastDelegate");
        t->base = b;
        return;
    }
    for (p = 0; p < (t->body ? t->body->n : 1); p++) {
    Node *d = t->body ? t->body->v[p] : t->decl;
    Sym *b;
    int i;
    enter(t);
    set_file(d->file);
    if (t->type->kind == TY_CLASS && d->a) {
        if (t->base) serr(d->a, "'Inherits' can appear only once within a 'Class' statement and can only be followed by 'Implements'.");
        Type *bt = bind_type(d->a);
        if (bt->kind != TY_CLASS) serr(d->a, "Classes can inherit only from other classes.");
        b = bt->sym;
        if (b->mods & M_NOTINHERITABLE)
            serr(d->a, "'%s' cannot inherit from class '%s' because '%s' is declared 'NotInheritable'.", t->spelling,
                 b->spelling, b->spelling);
        t->base = b;
        for (; b; b = b->base)
            if (b == t) serr(d->a, "Class '%s' cannot inherit from itself.", t->spelling);
    }
    if (d->list2)
        for (i = 0; i < d->list2->n; i++) {
            Type *it = bind_type(d->list2->v[i]);
            if (it->kind != TY_INTERFACE)
                serr(d->list2->v[i], t->type->kind == TY_INTERFACE ? "Interface can inherit only from another interface."
                                                                   : "Implemented type must be an interface.");
            sv_push(&t->ifaces, it->sym);
        }
    }
}

/* the fields of the base class first */
static void layout(Sym *t) {
    int i, off;
    if (t->laid_out) return;
    t->laid_out = 1;
    if (t->base) layout(t->base);
    off = t->base ? t->base->size : 0;
    for (i = 0; i < t->fields.n; i++) t->fields.v[i]->index = off + i;
    t->size = off + t->fields.n;
}

/* ------------------------------------------------------------ signatures */

static Type *bind_signature_params(Sym *m, NodeVec *params);

/* the type of a property: As clause, type character or Object */
static Type *prop_type(Node *d) {
    Type *t = d->a ? bind_type(d->a) : 0;
    if (d->tchar) {
        if (t) serr(d, "Type character '%c' cannot be used in a declaration with an explicit type.", d->tchar);
        t = tchar_type(d->tchar, d);
    }
    if (!t) {
        if (opts->strict) serr(d, "Option Strict On requires all Function, Property, and Operator declarations to have an 'As' clause.");
        t = ty[TY_OBJECT];
    }
    return t;
}

static void bind_type_signatures(Sym *t) {
    Sym *s;
    int i;
    for (s = t->members->syms; s; s = s->next) if (s->kind == S_EVENT) bind_event_type(s);
    for (s = t->members->syms; s; s = s->next) {
        if (s->kind != S_PROP) continue;
        enter(s);
        s->type = prop_type(s->decl);
        s->decl->type = s->type;
        if (s->field) s->field->type = s->type;
        if (s->getter) {
            enter(s->getter);
            bind_signature_params(s->getter, s->decl->list2);
            s->getter->type = s->type;
        }
        if (s->setter) {
            Node *a = s->setter->decl;
            Sym *v;
            enter(s->setter);
            bind_signature_params(s->setter, s->decl->list2);
            if (a->kind == N_ACCESSOR && a->list2 && a->list2->n) {
                Node *pn = a->list2->v[0];
                Type *vt = pn->a ? bind_type(pn->a) : s->type;
                if (a->list2->n > 1) serr(a, "'Set' parameter must be the only parameter.");
                if (vt != s->type) serr(pn, "'Set' parameter must have the same type as the containing property.");
                v = new_sym(S_PARAM, pn->name, pn->text);
                pn->sym = v;
            } else v = new_sym(S_PARAM, intern_word("value"), "Value");
            v->type = s->type;
            v->owner = s->setter;
            v->mods = M_BYVAL;
            v->index = s->setter->params.n;
            sv_push(&s->setter->params, v);
        }
    }
    for (i = 0; i < t->methods.n; i++) {
        Sym *m = t->methods.v[i];
        if (m->kind != S_METHOD || (m->sflags & SF_ACCESSOR)) continue;
        if (!m->decl || m->decl->kind == N_CLASS || m->decl->kind == N_STRUCTURE) continue;  /* implicit New */
        if (m->sflags & SF_CTOR) {
            enter(m);
            bind_signature_params(m, m->decl->list2);
        } else bind_signature(m);
    }
}

/* ------------------------------------------------------------ fields */

/* an instance field: its type, and its initializer as an assignment in the
   type's initializer method */
static void bind_instance_field(Sym *t, Sym *f) {
    Node *vi = f->decl, *d = vi->parent, *init = 0;
    Type *as;
    if (vi->kind == N_EVENT) return;       /* the field of an event: no initializer */
    if (vi->kind == N_PROPERTY) {
        /* the backing field of an auto-implemented property */
        if (!vi->c) return;
        enter(t->initm);
        cur_method = t->initm;
        cur_scope = t->initm->members;
        init = vi->c;
        if (init->kind == N_NEW) not_yet(init, "'As New' properties are");
        init = bind(init, 0);
        if (init->kind == N_COLLINIT && !init->type) init = bind_collinit(init, f->type);
        init = coerce(value(init), f->type);
    } else {
        enter(t->initm);
        cur_method = t->initm;
        cur_scope = t->initm->members;
        as = d->a ? bind_type(d->a) : 0;
        if (d->flags & F_ASNEW) {
            init = bind(d->b, 0);
            as = init->type;
        } else if (d->b) {
            if (d->list->n > 1)
                serr(d, "Explicit initialization is not permitted with multiple variables declared with a single type specifier.");
            if (vi->list) serr(vi, "Explicit initialization is not permitted for arrays declared with explicit bounds.");
        }
        f->type = var_type(vi, as, vi);
        if (!f->type) {
            if (opts->strict) serr(vi, "Option Strict On requires all variable declarations to have an 'As' clause.");
            f->type = ty[TY_OBJECT];
        }
        vi->type = f->type;
        bind_bounds(vi->list);
        if (d->b && !(d->flags & F_ASNEW)) {
            init = bind(d->b, 0);
            if (init->kind == N_COLLINIT && !init->type) init = bind_collinit(init, f->type);
            init = coerce(value(init), f->type);
        } else if (vi->list) {
            init = lit_node(N_NEWARRAY, f->type, vi);
            init->list = vi->list;
        }
    }
    if (init && t->type->kind == TY_STRUCT)
        serr(vi, "Initializers on structure members are valid only for 'Shared' members and constants.");
    if (!init && f->type->kind == TY_STRUCT && t->type->kind == TY_CLASS) {
        /* a field of a structure type: its zero value */
        init = lit_node(N_NEW, f->type, vi);
        init->list = nv_new();
    }
    if (init) {
        Node *as_ = lit_node(N_ASSIGN, 0, vi), *me = lit_node(N_ME, t->type, vi), *tg = lit_node(N_MEMBER, f->type, vi);
        tg->a = me;
        tg->sym = f;
        tg->cls = EC_VAR;
        as_->op = P_EQ;
        as_->a = tg;
        as_->b = init;
        nv_push(t->init_body, as_);
        if (f->mods & M_WITHEVENTS) {
            Node *add = withevents_hooks(tg, N_ADDHANDLER, vi);
            if (add) nv_push(t->init_body, add);
        }
    }
}

/* the method that runs the instance field initializers */
static void make_type_init(Sym *t) {
    Sym *m = new_sym(S_METHOD, intern_word("$init"), "$init");
    m->owner = t;
    m->sflags = SF_INSTANCE;
    m->file = t->file;
    m->decl = t->decl;
    m->members = new_scope(SC_METHOD, 0, m);
    m->members->materialized = 1;
    m->scope = m->members;
    t->initm = m;
    t->init_body = nv_new();
}

/* Handles clauses: Me.E / MyBase.E are connected in the initializer method
   (run by every constructor); field.E is noted on the WithEvents field */
static void bind_handles(Sym *t) {
    int i, k;
    for (i = 0; i < t->methods.n; i++) {
        Sym *m = t->methods.v[i];
        Node *cl;
        if (m->kind != S_METHOD || !m->decl || m->decl->kind != N_METHOD) continue;
        cl = m->decl->b;
        if (!cl || cl->op != K_Handles) continue;
        for (k = 0; k < cl->list->n; k++) {
            Node *h = cl->list->v[k], *base = h->a;
            if (base->kind == N_NAME) {
                Sym *f = type_lookup(t, base->name);
                if (!f || f->kind != S_FIELD || !(f->mods & M_WITHEVENTS))
                    serr(h, "Handles clause requires a WithEvents variable defined in the containing type or one of its base types.");
                if (!f->body) f->body = nv_new();
                {
                    Node *rec = lit_node(N_MEMBER, 0, h);
                    rec->name = h->name;
                    rec->text = h->text;
                    rec->sym = m;
                    nv_push(f->body, rec);
                }
            } else {
                Node *st;
                if (!(m->sflags & SF_INSTANCE)) serr(h, "Events of shared WithEvents variables cannot be handled by non-shared methods.");
                enter(t->initm);
                cur_method = t->initm;
                cur_scope = t->initm->members;
                st = handler_stmt(N_ADDHANDLER, copy_tree(base), h->name, h->text, m, h);
                bind_stmt(st);
                nv_push(t->init_body, st);
            }
        }
    }
}

static void bind_type_fields(Sym *t) {
    Sym *s;
    int i;
    make_type_init(t);
    bind_handles(t);
    for (s = t->members->syms; s; s = s->next)
        if (s->kind == S_CONST) eval_const(s, s->decl);
    for (i = 0; i < t->fields.n; i++) bind_instance_field(t, t->fields.v[i]);
    for (i = 0; i < t->methods.n; i++) {
        Sym *f = t->methods.v[i];
        if (f->kind != S_FIELD) continue;
        if (f->decl->kind == N_EVENT) {
            f->index = prog_fields.n;
            sv_push(&prog_fields, f);
        } else if (f->decl->kind == N_PROPERTY) {
            /* a shared auto-implemented property */
            enter(f);
            f->type = f->owner == t ? f->type : f->type;
            f->index = prog_fields.n;
            sv_push(&prog_fields, f);
            if (f->decl->c) {
                Node *init = bind(f->decl->c, 0);
                f->init = coerce(value(init), f->type);
            }
        } else bind_field(f);
    }
    if (t->type->kind == TY_STRUCT && t->size == 0)
        serr(t->decl, "Structure '%s' must contain at least one instance member variable or at least one instance "
                      "event declaration not marked 'Custom'.", t->spelling);
    t->initm->body = t->init_body;
    cur_method = 0;
    cur_scope = 0;
}

/* ------------------------------------------------------------ virtual methods */

static const char *method_word(Sym *m) {
    if (m->sflags & SF_ACCESSOR) return "property";
    return m->type ? "function" : "sub";
}

/* the Object method that m overrides: slot 0 ToString, 1 Equals, 2 GetHashCode */
static int object_slot(Sym *m) {
    if (m->name == intern_word("tostring") && m->params.n == 0 && m->type == ty[TY_STRING]) return 0;
    if (m->name == intern_word("equals") && m->params.n == 1 && m->params.v[0]->type == ty[TY_OBJECT] &&
        m->type == ty[TY_BOOLEAN]) return 1;
    if (m->name == intern_word("gethashcode") && m->params.n == 0 && m->type == ty[TY_INTEGER]) return 2;
    return -1;
}

/* the method of a base class that m (Overrides) overrides */
static Sym *find_overridden(Sym *t, Sym *m) {
    Sym *b;
    for (b = t->base; b; b = b->base) {
        int i;
        for (i = 0; i < b->methods.n; i++) {
            Sym *c = b->methods.v[i];
            if (c->kind != S_METHOD || c->name != m->name || (c->sflags & SF_CTOR)) continue;
            if (((c->sflags & SF_ACCESSOR) != 0) != ((m->sflags & SF_ACCESSOR) != 0)) continue;
            if ((m->sflags & SF_ACCESSOR) && ((c->field->getter == c) != (m->field->getter == m))) continue;
            if (same_params(c, m)) return c;
        }
    }
    return 0;
}

static void assign_vslots(Sym *t) {
    int i;
    if (t->nvirt) return;
    if (t->base) assign_vslots(t->base);
    if (t->base) {
        for (i = 0; i < t->base->vtab.n; i++) sv_push(&t->vtab, t->base->vtab.v[i]);
    } else {
        sv_push(&t->vtab, 0);       /* ToString, Equals, GetHashCode: the defaults */
        sv_push(&t->vtab, 0);
        sv_push(&t->vtab, 0);
    }
    for (i = 0; i < t->methods.n; i++) {
        Sym *m = t->methods.v[i];
        int mods = m->mods;
        m->vslot = -1;
        if (m->kind != S_METHOD || (m->sflags & SF_CTOR) || !(m->sflags & SF_INSTANCE) || m == t->initm) continue;
        if (mods & M_OVERRIDES) {
            Sym *b = find_overridden(t, m);
            int os = b ? -1 : object_slot(m);
            if (!b && os < 0)
                serr(m->decl, "%s '%s' cannot be declared 'Overrides' because it does not override a %s in a base class.",
                     method_word(m), m->spelling, method_word(m));
            if (b) {
                if (!(b->mods & (M_OVERRIDABLE | M_MUSTOVERRIDE | M_OVERRIDES)))
                    serr(m->decl, "'%s' cannot override '%s' because it is not declared 'Overridable'.", method_sig(m), method_sig(b));
                if (b->mods & M_NOTOVERRIDABLE)
                    serr(m->decl, "'%s' cannot override '%s' because it is declared 'NotOverridable'.", method_sig(m), method_sig(b));
                if (b->type != m->type)
                    serr(m->decl, "'%s' cannot override '%s' because they differ by their return types.", method_sig(m), method_sig(b));
                m->vslot = b->vslot;
                m->overrides = b;
            } else m->vslot = os;
            t->vtab.v[m->vslot] = m;
        } else if (mods & (M_OVERRIDABLE | M_MUSTOVERRIDE)) {
            if (mods & M_NOTOVERRIDABLE) serr(m->decl, "'NotOverridable' cannot be specified for methods that do not override another method.");
            if ((mods & M_MUSTOVERRIDE) && t->type->kind == TY_CLASS && !(t->mods & M_MUSTINHERIT))
                serr(t->decl, "Class '%s' must be declared 'MustInherit' because it contains methods declared 'MustOverride'.",
                     t->spelling);
            m->vslot = t->vtab.n;
            sv_push(&t->vtab, m);
        }
    }
    t->nvirt = t->vtab.n;
    if (t->type->kind == TY_CLASS && !(t->mods & M_MUSTINHERIT))
        for (i = 0; i < t->vtab.n; i++)
            if (t->vtab.v[i] && (t->vtab.v[i]->mods & M_MUSTOVERRIDE))
                serr(t->decl, "Class '%s' must either be declared 'MustInherit' or override the following inherited "
                              "'MustOverride' member(s): %s.", t->spelling, method_sig(t->vtab.v[i]));
}

/* ------------------------------------------------------------ interfaces */

static void iface_slots(Sym *it) {
    int i, n = 0;
    if (it->nvirt || it->laid_out) return;
    it->laid_out = 1;
    for (i = 0; i < it->methods.n; i++) {
        Sym *m = it->methods.v[i];
        if (m->kind == S_METHOD) m->vslot = n++;
    }
    it->nvirt = n;
}

static void add_iface(SymVec *v, Sym *it) {
    int i;
    for (i = 0; i < v->n; i++) if (v->v[i] == it) return;
    sv_push(v, it);
    for (i = 0; i < it->ifaces.n; i++) add_iface(v, it->ifaces.v[i]);
}

/* does the Implements clause of m name the interface member im? */
static int names_member(Sym *m, Sym *it, Sym *im) {
    Node *d = m->decl, *cl;
    int i;
    if (m->sflags & SF_ACCESSOR) d = m->field->decl;
    cl = d ? d->b : 0;
    if (!cl || cl->op != K_Implements) return 0;
    for (i = 0; i < cl->list->n; i++) {
        Node *tn = cl->list->v[i], *last = tn->list->v[tn->list->n - 1];
        Node part;
        Sym *named;
        Name *mname = last->name;
        Name *target = (im->sflags & SF_ACCESSOR) ? im->field->name : im->name;
        if (mname != target) continue;
        /* the interface: the name without its last part */
        part = *tn;
        part.list = nv_new();
        {
            int k;
            for (k = 0; k + 1 < tn->list->n; k++) nv_push(part.list, tn->list->v[k]);
        }
        named = bind_type(&part)->sym;
        if (named == it || iface_derives(named, it)) {
            /* the member that the name finds in the named interface (a derived
               interface's member hides a base's) */
            Sym *f = type_lookup(named, mname), *o;
            if (im->sflags & SF_ACCESSOR) {
                if (f == im->field) return 1;
                continue;
            }
            for (o = f; o; o = o->overload) if (o == im) return 1;
        }
    }
    return 0;
}

static void check_implements(Sym *t) {
    Sym *c;
    int i, k;
    for (c = t; c; c = c->base)
        for (i = 0; i < c->ifaces.n; i++) add_iface(&t->all_ifaces, c->ifaces.v[i]);
    t->itabs = xalloc(sizeof(Sym **) * (t->all_ifaces.n + 1));
    for (i = 0; i < t->all_ifaces.n; i++) {
        Sym *it = t->all_ifaces.v[i];
        iface_slots(it);
        t->itabs[i] = xalloc(sizeof(Sym *) * (it->nvirt + 1));
        for (k = 0; k < it->methods.n; k++) {
            Sym *im = it->methods.v[k], *impl = 0;
            if (im->kind != S_METHOD) continue;
            for (c = t; c && !impl; c = c->base) {
                int j;
                for (j = 0; j < c->methods.n && !impl; j++) {
                    Sym *m = c->methods.v[j];
                    if (m->kind != S_METHOD || !names_member(m, it, im)) continue;
                    if ((m->sflags & SF_ACCESSOR) && ((m->field->getter == m) != (im->field->getter == im))) continue;
                    if (!same_params(m, im) || m->type != im->type)
                        serr(m->decl, "'%s' cannot implement '%s' because there is no matching %s on interface '%s'.",
                             m->spelling, im->spelling, method_word(im), it->spelling);
                    impl = m;
                }
            }
            if (!impl)
                serr(t->decl, "%s '%s' must implement '%s' for interface '%s'.", t->type->kind == TY_STRUCT ? "Structure" : "Class",
                     t->spelling, method_sig(im), it->spelling);
            /* a virtual implementation: the class's override */
            if (impl->vslot >= 0 && impl->vslot < t->vtab.n && t->vtab.v[impl->vslot]) impl = t->vtab.v[impl->vslot];
            t->itabs[i][im->vslot] = impl;
        }
    }
}
