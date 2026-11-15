/*
 * sema_call.c - method calls: argument matching and overload resolution
 * (spec 11.8, "Invocation Expressions" and "Overload Resolution").
 */

static Node *value(Node *e);
static Node *bind(Node *e, int flags);

/* "Public Sub Foo(x As Integer, Optional y As String = ...)" for messages */
static const char *method_sig(Sym *m) {
    StrBuf b;
    int i;
    sb_init(&b);
    sb_puts(&b, (m->mods & M_PRIVATE) ? "Private " : (m->mods & M_FRIEND) ? "Friend " : "Public ");
    if ((m->mods & M_SHARED) && m->owner && m->owner->kind == S_TYPE) sb_puts(&b, "Shared ");
    sb_puts(&b, m->type ? "Function " : "Sub ");
    sb_puts(&b, m->spelling);
    sb_putc(&b, '(');
    for (i = 0; i < m->params.n; i++) {
        Sym *p = m->params.v[i];
        if (i) sb_puts(&b, ", ");
        if (p->mods & M_BYREF) sb_puts(&b, "ByRef ");
        if (p->mods & M_OPTIONAL) sb_puts(&b, "Optional ");
        if (p->mods & M_PARAMARRAY) sb_puts(&b, "ParamArray ");
        sb_printf(&b, "%s As %s", p->spelling, type_name(p->type));
    }
    sb_putc(&b, ')');
    if (m->type) sb_printf(&b, " As %s", type_name(m->type));
    return b.s;
}

/* why a candidate does not apply */
enum { MF_OK, MF_TOO_MANY, MF_MISSING, MF_NAMED, MF_DUP, MF_TYPE, MF_STRICT, MF_OMITTED };

typedef struct {
    Sym *m;
    int fail;
    Sym *fail_param;
    Node *fail_arg;
    int narrowing;
    int cnarrow;            /* narrowing conversions of constants that fit (ranked below widening) */
    int nexact;             /* lambdas whose result type is the delegate's */
    int expanded;           /* ParamArray in the expanded form */
    int ndefault;           /* parameters without an argument (defaults, empty ParamArray) */
    Node **slots;           /* the argument of each parameter (NULL: default or ParamArray) */
    Node **extra;           /* expanded ParamArray: the arguments */
    int nextra;
    Type **argparam;        /* the parameter type that each argument converts to */
} Match;

/* matches the bound arguments (ARG nodes, a = value or NULL if omitted)
   against method m; expanded: put trailing arguments into a ParamArray */
static void match(Match *mt, Sym *m, NodeVec *args, int expanded) {
    int np = m->params.n, i, pa = np > 0 && (m->params.v[np - 1]->mods & M_PARAMARRAY);
    int nfixed = expanded ? np - 1 : np;
    memset(mt, 0, sizeof(Match));
    mt->m = m;
    mt->expanded = expanded;
    mt->slots = xalloc(sizeof(Node *) * (np + 1));
    mt->extra = xalloc(sizeof(Node *) * (args->n + 1));
    mt->argparam = xalloc(sizeof(Type *) * (args->n + 1));
    if (expanded && !pa) { mt->fail = MF_TOO_MANY; return; }
    for (i = 0; i < args->n; i++) {
        Node *a = args->v[i];
        Sym *p = 0;
        int k;
        if (a->name) {
            for (k = 0; k < np; k++)
                if (m->params.v[k]->name == a->name) p = m->params.v[k];
            if (!p || (p->mods & M_PARAMARRAY)) { mt->fail = MF_NAMED; mt->fail_arg = a; return; }
            if (mt->slots[p->index]) { mt->fail = MF_DUP; mt->fail_param = p; return; }
            mt->slots[p->index] = a;
        } else if (i < nfixed) {
            p = m->params.v[i];
            mt->slots[i] = a;
        } else if (expanded) {
            if (!a->a) { mt->fail = MF_OMITTED; mt->fail_arg = a; return; }
            mt->extra[mt->nextra++] = a;
            p = m->params.v[np - 1];
        } else { mt->fail = MF_TOO_MANY; return; }
        if (!a->a) {
            if (!(p->mods & M_OPTIONAL)) { mt->fail = MF_OMITTED; mt->fail_param = p; return; }
            continue;
        }
        {
            Type *pt = (expanded && i >= nfixed && !a->name) ? p->type->elem : p->type;
            int c = conv_class_of(a->a, pt);
            mt->argparam[i] = pt;
            if (c == CV_WIDEN && a->a->type && conv_class(a->a->type, pt) == CV_NARROW) mt->cnarrow++;
            if (c == CV_IDENT && a->a->kind == N_LAMBDA && !a->a->type) mt->nexact++;
            if (c == CV_NONE) { mt->fail = MF_TYPE; mt->fail_arg = a; mt->fail_param = p; return; }
            if (c == CV_NARROW) {
                if (opts->strict) { mt->fail = MF_STRICT; mt->fail_arg = a; mt->fail_param = p; return; }
                mt->narrowing++;
            }
        }
    }
    for (i = 0; i < nfixed; i++) {
        Sym *p = m->params.v[i];
        if (!mt->slots[i] || !mt->slots[i]->a) {
            if (!(p->mods & (M_OPTIONAL | M_PARAMARRAY))) {
                mt->fail = MF_MISSING;
                mt->fail_param = p;
                return;
            }
            mt->ndefault++;
        }
    }
}

/* the order of the numeric types for specificity (spec 11.8.1.1) */
static int num_rank(Type *t) {
    static const int order[] = { TY_BYTE, TY_SBYTE, TY_SHORT, TY_USHORT, TY_INTEGER, TY_UINTEGER,
                                 TY_LONG, TY_ULONG, TY_DECIMAL, TY_SINGLE, TY_DOUBLE };
    int i;
    for (i = 0; i < 11; i++) if (order[i] == t->kind) return i;
    return -1;
}

/* is parameter type a more specific than b for an argument? 1 yes, 0 the
   same, -1 no */
static int more_specific(Type *a, Type *b) {
    int ca, cb;
    if (a == b) return 0;
    ca = conv_class(a, b);
    cb = conv_class(b, a);
    if (ca == CV_WIDEN && cb != CV_WIDEN) return 1;
    if (cb == CV_WIDEN && ca != CV_WIDEN) return -1;
    if (num_rank(a) >= 0 && num_rank(b) >= 0) return num_rank(a) < num_rank(b) ? 1 : -1;
    return 0;
}

/* is candidate x better than y? */
static int better(Match *x, Match *y, int nargs) {
    int i, some = 0;
    if (x->narrowing != y->narrowing) return x->narrowing < y->narrowing;
    if (x->cnarrow != y->cnarrow) return x->cnarrow < y->cnarrow;
    for (i = 0; i < nargs; i++) {
        int c;
        if (!x->argparam[i] || !y->argparam[i]) continue;
        c = more_specific(x->argparam[i], y->argparam[i]);
        if (c < 0) return 0;
        if (c > 0) some = 1;
    }
    if (some) return 1;
    /* the normal form of a ParamArray beats the expanded one; fewer
       parameters filled in by default values is better */
    if (x->nexact != y->nexact) return x->nexact > y->nexact;
    if (x->expanded != y->expanded) return !x->expanded;
    if (x->ndefault != y->ndefault) return x->ndefault < y->ndefault;
    return 0;
}

static void report_mismatch(Match *mt, Node *at) {
    const char *sig = method_sig(mt->m);
    switch (mt->fail) {
    case MF_TOO_MANY: serr(at, "Too many arguments to '%s'.", sig);
    case MF_MISSING: case MF_OMITTED:
        if (mt->fail_param)
            serr(at, "Argument not specified for parameter '%s' of '%s'.", mt->fail_param->spelling, sig);
        serr(mt->fail_arg ? mt->fail_arg : at, "Omitted argument cannot match a ParamArray parameter.");
    case MF_NAMED: serr(mt->fail_arg, "'%s' is not a parameter of '%s'.", mt->fail_arg->text, sig);
    case MF_DUP: serr(at, "Parameter '%s' already has a matching argument.", mt->fail_param->spelling);
    case MF_TYPE: case MF_STRICT: {
        Sym *p = mt->fail_param;
        coerce(mt->fail_arg->a, (mt->expanded && (p->mods & M_PARAMARRAY)) ? p->type->elem : p->type);
    }
    }
    serr(at, "Overload resolution failed.");
}

/* the argument passed for a parameter: converted, ByRef copy-back */
static Node *pass_arg(Node *a, Sym *p, Type *pt) {
    Node *v = a->a;
    Node *r = lit_node(N_ARG, pt, a);
    r->name = p->name;
    r->text = p->spelling;
    if ((p->mods & M_BYREF) && (v->cls == EC_VAR)) {
        r->op = K_ByRef;
        if (v->type != pt) {
            /* copy-in, copy-out through a temporary of the parameter type */
            Node *tmp = lit_node(N_TARGET, pt, a);
            r->flags |= F_COPYBACK;
            r->c = v;
            r->b = coerce(tmp, v->type);
            r->a = coerce(v, pt);
        } else r->a = v;
    } else {
        r->a = coerce(v, pt);
        if (p->mods & M_BYREF) r->op = K_ByRef;     /* by reference to a temporary */
    }
    return r;
}

static Node *param_default(Sym *p, Node *at) {
    Node *d = p->cval, *c;
    if (!d) {
        Node *z = lit_node(N_NOTHING, ty[TY_NOTHING], at);
        return const_convert(z, p->type, at);
    }
    c = xalloc(sizeof(Node));
    *c = *d;
    c->file = at->file;
    c->line = at->line;
    return c;
}

/* the call node for the chosen candidate */
static Node *make_call(Match *mt, Node *target, Node *recv, NodeVec *args, Node *at) {
    Sym *m = mt->m;
    Node *call = lit_node(N_INVOKE, m->type, at);
    int i, np = m->params.n;
    call->sym = m;
    call->a = recv;
    call->list = nv_new();
    call->cls = m->type ? EC_VALUE : EC_VOID;
    (void)target;
    (void)args;
    for (i = 0; i < np; i++) {
        Sym *p = m->params.v[i];
        if (mt->expanded && i == np - 1) {
            /* the ParamArray: a new array of the remaining arguments */
            Node *arr = lit_node(N_NEWARRAY, p->type, at), *init = lit_node(N_COLLINIT, p->type, at);
            Node *ar;
            int k;
            init->list = nv_new();
            for (k = 0; k < mt->nextra; k++) nv_push(init->list, coerce(mt->extra[k]->a, p->type->elem));
            arr->b = init;
            ar = lit_node(N_ARG, p->type, at);
            ar->name = p->name;
            ar->text = p->spelling;
            ar->a = arr;
            nv_push(call->list, ar);
        } else if (mt->slots[i] && mt->slots[i]->a) {
            nv_push(call->list, pass_arg(mt->slots[i], p, p->type));
        } else {
            Node *ar = lit_node(N_ARG, p->type, at);
            ar->name = p->name;
            ar->text = p->spelling;
            if (p->mods & M_PARAMARRAY) {
                Node *arr = lit_node(N_NEWARRAY, p->type, at), *init = lit_node(N_COLLINIT, p->type, at);
                init->list = nv_new();
                arr->b = init;
                ar->a = arr;
            } else ar->a = param_default(p, at);
            nv_push(call->list, ar);
        }
    }
    return call;
}

/* binds the arguments of a call: ARG nodes with bound values */
static NodeVec *bind_args(NodeVec *args) {
    NodeVec *v = nv_new();
    int i;
    if (!args) return v;
    for (i = 0; i < args->n; i++) {
        Node *a = args->v[i];
        if (a->a && a->a->kind == N_BOUND) serr(a, "Array bounds cannot appear here.");
        if (a->a) a->a = value(bind(a->a, 0));
        nv_push(v, a);
    }
    return v;
}

static int same_params(Sym *a, Sym *b) {
    int i;
    if (a->params.n != b->params.n) return 0;
    for (i = 0; i < a->params.n; i++)
        if (a->params.v[i]->type != b->params.v[i]->type) return 0;
    return 1;
}

/* the candidates: the overloads in the type, then those of the base classes
   that no derived method with the same signature hides or overrides */
/* the class System.<Type>_Members of the library that extends a built-in type */
static Sym *builtin_extension(Sym *t) {
    char buf[80];
    Sym *s;
    if (!t || t->kind != S_TYPE || t->decl || t->owner != ns_path("System")) return 0;
    snprintf(buf, sizeof buf, "%s_members", t->spelling);
    s = scope_find(ns_path("System")->members, intern_word(buf));
    return s && s->kind == S_TYPE ? s : 0;
}

static void collect_cands(Sym *group, SymVec *v) {
    Sym *m, *t, *ext;
    int k;
    for (m = group; m; m = m->overload) sv_push(v, m);
    t = group->owner;
    if ((ext = builtin_extension(t)) != 0 && group->kind == S_METHOD) {
        for (m = scope_find(ext->members, group->name); m; m = m->overload)
            if (m->kind == S_METHOD && (m->mods & M_SHARED)) sv_push(v, m);
    }
    if (!t || t->kind != S_TYPE || group->kind != S_METHOD || (group->sflags & SF_CTOR)) return;
    for (t = t->base; t; t = t->base) {
        Sym *b = scope_find(t->members, group->name);
        if (!b || b->kind != S_METHOD) continue;
        for (; b; b = b->overload) {
            int hidden = 0;
            for (k = 0; k < v->n; k++) if (same_params(v->v[k], b)) hidden = 1;
            if (!hidden) sv_push(v, b);
        }
    }
}

static Node *resolve_vec(SymVec *cv, Sym *group, Node *recv, NodeVec *args, Node *at);
static void instantiate_cands(SymVec *all, NodeVec *args, Node *at);   /* sema_generic.c */

/* overload resolution among the methods of a group; args are bound */
static Node *resolve_call(Sym *group, Node *recv, NodeVec *args, Node *at) {
    SymVec all;
    memset(&all, 0, sizeof all);
    collect_cands(group, &all);
    return resolve_vec(&all, group, recv, args, at);
}

/* overload resolution among the candidates (group: for messages) */
static Node *resolve_vec(SymVec *cv, Sym *group, Node *recv, NodeVec *args, Node *at) {
    Match *cands;
    int n = 0, i, j, napp = 0, all_count = 1, nm = 0, ci;
    Sym *m;
    Match *best = 0;
    SymVec all;
    all.n = all.cap = cv->n;
    all.v = xalloc(sizeof(Sym *) * (cv->n + 1));
    memcpy(all.v, cv->v, sizeof(Sym *) * cv->n);
    instantiate_cands(&all, args, at);
    for (ci = 0; ci < all.n; ci++) if (accessible(all.v[ci])) nm++;
    cands = xalloc(sizeof(Match) * (2 * nm + 1));
    for (ci = 0; ci < all.n; ci++) {
        m = all.v[ci];
        if (!accessible(m)) continue;
        match(&cands[n], m, args, 0);
        if (cands[n].fail != MF_OK && m->params.n && (m->params.v[m->params.n - 1]->mods & M_PARAMARRAY)) {
            match(&cands[n], m, args, 1);
        } else if (cands[n].fail == MF_OK && m->params.n && (m->params.v[m->params.n - 1]->mods & M_PARAMARRAY) &&
                   args->n >= m->params.n) {
            n++;
            match(&cands[n], m, args, 1);
            if (cands[n].fail != MF_OK) n--;
        }
        n++;
    }
    if (n == 0) {
        check_access(group, at);
        serr(at, "'%s' is not accessible in this context.", group->spelling);
    }
    for (i = 0; i < n; i++) {
        if (cands[i].fail == MF_OK) napp++;
        else if (cands[i].fail != MF_TOO_MANY && cands[i].fail != MF_MISSING) all_count = 0;
    }
    if (napp == 0) {
        if (n == 1 || nm == 1) report_mismatch(&cands[0], at);
        if (all_count)
            serr(at, "Overload resolution failed because no accessible '%s' accepts this number of arguments.", group->spelling);
        serr(at, "Overload resolution failed because no accessible '%s' can be called with these arguments.", group->spelling);
    }
    for (i = 0; i < n; i++) {
        int ok = 1;
        if (cands[i].fail != MF_OK) continue;
        for (j = 0; j < n && ok; j++)
            if (j != i && cands[j].fail == MF_OK && cands[j].m != cands[i].m && !better(&cands[i], &cands[j], args->n)) ok = 0;
        if (ok) { best = &cands[i]; break; }
    }
    if (!best) {
        for (i = 0; i < n && !best; i++) if (cands[i].fail == MF_OK) best = &cands[i];
        for (j = 0; j < n; j++)
            if (cands[j].fail == MF_OK && cands[j].m != best->m)
                serr(at, "Overload resolution failed because no accessible '%s' is most specific for these arguments.",
                     group->spelling);
    }
    check_access(best->m, at);
    if (recv && best->m->owner == ty[TY_OBJECT]->sym && !ty_ref(recv->type))
        recv = coerce(recv, ty[TY_OBJECT]);     /* Object's methods of a value: boxed */
    return make_call(best, 0, recv, args, at);
}

/* ------------------------------------------------------------ user-defined operators */

static void add_operators(SymVec *v, Type *t, Name *n) {
    Sym *ts, *s;
    int k;
    if (!t || (t->kind != TY_CLASS && t->kind != TY_STRUCT)) return;
    for (ts = t->sym; ts; ts = ts->base)
        for (s = scope_find(ts->members, n); s; s = s->overload) {
            int dup = 0;
            for (k = 0; k < v->n; k++) if (v->v[k] == s) dup = 1;
            if (!dup) sv_push(v, s);
        }
}

static Node *arg_of(Node *v) {
    Node *a = lit_node(N_ARG, v->type, v);
    a->a = v;
    return a;
}

/* a user-defined operator for operands of class or structure types, or NULL */
static Node *user_operator(int op, Node *l, Node *r, Node *at) {
    StrBuf b;
    SymVec v;
    NodeVec *args;
    Name *n;
    if (!(l->type && (l->type->kind == TY_CLASS || l->type->kind == TY_STRUCT)) &&
        !(r && r->type && (r->type->kind == TY_CLASS || r->type->kind == TY_STRUCT)))
        return 0;
    sb_init(&b);
    sb_puts(&b, "operator ");
    sb_puts(&b, tok_spelling_of(op));
    n = intern_word(b.s);
    memset(&v, 0, sizeof v);
    add_operators(&v, l->type, n);
    if (r) add_operators(&v, r->type, n);
    if (!v.n) return 0;
    args = nv_new();
    nv_push(args, arg_of(l));
    if (r) nv_push(args, arg_of(r));
    {
        /* only the operators with this number of operands */
        SymVec w;
        int k;
        memset(&w, 0, sizeof w);
        for (k = 0; k < v.n; k++) if (v.v[k]->params.n == args->n) sv_push(&w, v.v[k]);
        if (!w.n) return 0;
        return resolve_vec(&w, w.v[0], 0, args, at);
    }
}

/* a user-defined conversion (Widening or Narrowing Operator CType) from the
   type of e to t; implicit: only a Widening one (or Narrowing with Option
   Strict Off) */
static Node *user_conversion(Node *e, Type *t, int expl) {
    SymVec v;
    Name *n = intern_word("operator ctype");
    int k;
    Sym *found = 0;
    memset(&v, 0, sizeof v);
    add_operators(&v, e->type, n);
    add_operators(&v, t, n);
    for (k = 0; k < v.n; k++) {
        Sym *m = v.v[k];
        int c;
        if (m->params.n != 1 || m->type != t) continue;
        c = conv_class_of(e, m->params.v[0]->type);
        if (c != CV_IDENT && c != CV_WIDEN) continue;
        if (!expl && (m->mods & M_NARROWING) && opts->strict) continue;
        found = m;
    }
    if (!found) return 0;
    {
        NodeVec *args = nv_new();
        nv_push(args, arg_of(e));
        return resolve_call(found, 0, args, e);
    }
}

/* ------------------------------------------------------------ extension methods */

/* the extension methods named n in the modules in scope: of the current
   namespace and its parents, and of the imported namespaces */
static void add_ext_from(SymVec *v, Sym *ns, Name *n) {
    Sym *m;
    if (!ns || ns->kind != S_NAMESPACE) return;
    for (m = ns->members->syms; m; m = m->next) {
        Sym *s;
        if (m->kind != S_MODULE) continue;
        for (s = scope_find(m->members, n); s; s = s->overload) {
            int k, dup = 0;
            if (s->kind != S_METHOD || !(s->mods & M_EXTENSION) || !accessible(s)) continue;
            for (k = 0; k < v->n; k++) if (v->v[k] == s) dup = 1;
            if (!dup) sv_push(v, s);
        }
    }
}

static void extension_cands(Name *n, SymVec *v) {
    Sym *ns;
    int i;
    for (ns = cur_ns; ns; ns = ns->owner) add_ext_from(v, ns, n);
    for (i = 0; i < opts->imports.n; i++) add_ext_from(v, opts->imports.v[i], n);
}

static int has_extensions(Name *n) {
    SymVec v;
    memset(&v, 0, sizeof v);
    extension_cands(n, &v);
    free(v.v);
    return v.n > 0;
}

/* a call of a method group: e (EC_METHOD) with the bound arguments; an
   extension group tries the instance methods, then the extension methods
   with the receiver as their first argument */
static Node *call_group(Node *e, NodeVec *args, Node *at) {
    NodeVec *targs = call_targs;
    if (e->flags & F_EXTCALL) {
        SymVec v;
        NodeVec *xargs;
        Node *recv = e->d, *a0;
        int i;
        if (e->sym) {
            /* the instance methods first, if one applies */
            SymVec all;
            int k, ok = 0;
            memset(&all, 0, sizeof all);
            collect_cands(e->sym, &all);
            for (k = 0; k < all.n && !ok; k++) {
                Match mt;
                if (all.v[k]->sflags & SF_GENERIC) continue;
                match(&mt, all.v[k], args, 0);
                if (mt.fail == MF_OK) ok = 1;
                else if (all.v[k]->params.n && (all.v[k]->params.v[all.v[k]->params.n - 1]->mods & M_PARAMARRAY)) {
                    match(&mt, all.v[k], args, 1);
                    if (mt.fail == MF_OK) ok = 1;
                }
            }
            if (ok || (all.n && (all.v[0]->sflags & SF_GENERIC))) {
                call_targs = targs;
                return resolve_vec(&all, e->sym, e->a, args, at);
            }
        }
        memset(&v, 0, sizeof v);
        extension_cands(e->name, &v);
        if (!v.n) {
            call_targs = targs;
            return resolve_call(e->sym, e->a, args, at);
        }
        xargs = nv_new();
        a0 = lit_node(N_ARG, recv->type, at);
        a0->a = recv;
        nv_push(xargs, a0);
        for (i = 0; i < args->n; i++) nv_push(xargs, args->v[i]);
        call_targs = targs;
        return resolve_vec(&v, v.v[0], 0, xargs, at);
    }
    return resolve_call(e->sym, e->a, args, at);
}
