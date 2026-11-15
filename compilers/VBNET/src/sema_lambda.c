/*
 * sema_lambda.c - delegates, AddressOf and lambda expressions (spec 7.10,
 * 11.4.5 and 11.20).
 *
 * AddressOf and lambdas have no type of their own: they are "pending" until
 * a conversion to a delegate type (coerce) makes them a NewDelegate node
 * (N_DLGNEW: sym the method, a the target object).  A lambda becomes a
 * method of the enclosing module or class (flag SF_LAMBDA) whose Me is its
 * environment: the variables of enclosing methods that it uses are marked
 * SF_CAPTURED, and the code generator keeps them in environment objects.
 * Without a target type, a lambda with typed parameters gets a Func or
 * Action type.
 */

static int nlambda;
/* tentative (declared in sema_generic.c): > 0 while a copy of a lambda is bound to infer its type */

static int is_delegate(Type *t) {
    return t && t->kind == TY_CLASS && t->sym && (t->sym->sflags & SF_DELEGATE);
}

static Sym *delegate_invoke(Type *t) {
    Sym *inv = t->sym->invoke;
    advance(t->sym, 4);         /* its signature */
    return inv;
}

static int is_pending(Node *e) {
    return e && (e->kind == N_LAMBDA || e->kind == N_ADDRESSOF) && !e->type;
}

/* "Delegate Function Func(arg As Integer) As Integer" for messages */
static const char *delegate_sig(Type *t) {
    Sym *inv = delegate_invoke(t);
    StrBuf b;
    int i;
    sb_init(&b);
    sb_printf(&b, "Delegate %s %s(", inv->type ? "Function" : "Sub", type_name(t));
    for (i = 0; i < inv->params.n; i++) {
        Sym *p = inv->params.v[i];
        if (i) sb_puts(&b, ", ");
        if (p->mods & M_BYREF) sb_puts(&b, "ByRef ");
        sb_printf(&b, "%s As %s", p->spelling, type_name(p->type));
    }
    sb_putc(&b, ')');
    if (inv->type) sb_printf(&b, " As %s", type_name(inv->type));
    return b.s;
}

/* a parameter or result type that needs no code to convert (identity, or a
   reference conversion) */
static int same_rep(Type *from, Type *to) {
    int c = conv_class(from, to);
    if (c == CV_IDENT) return 1;
    return c == CV_WIDEN && ty_ref(from) && ty_ref(to) && to->kind != TY_INTERFACE && from->kind != TY_ARRAY;
}

/* can method m be the target of a delegate with Invoke inv? */
static int method_fits(Sym *m, Sym *inv) {
    int i;
    if (m->kind != S_METHOD || (m->sflags & SF_GENERIC) || m->params.n != inv->params.n) return 0;
    for (i = 0; i < m->params.n; i++) {
        Sym *a = inv->params.v[i], *b = m->params.v[i];
        if (((a->mods & M_BYREF) != 0) != ((b->mods & M_BYREF) != 0)) return 0;
        if (!same_rep(a->type, b->type)) return 0;
    }
    if (inv->type) return m->type && same_rep(m->type, inv->type);
    return 1;
}

/* ------------------------------------------------------------ lambdas */

static Sym *root_method(void) {
    Sym *m = cur_method;
    while (m && (m->sflags & SF_LAMBDA)) m = m->outer;
    return m;
}

/* a variable of an enclosing method used inside a lambda */
static void note_capture(Sym *s, Node *at) {
    if (!cur_method || !(cur_method->sflags & SF_LAMBDA) || s->owner == cur_method || (s->mods & M_STATIC)) return;
    if (s->kind == S_PARAM && (s->mods & M_BYREF))
        serr(at, "'ByRef' parameter '%s' cannot be used in a lambda expression.", s->spelling);
    s->sflags |= SF_CAPTURED;
}

/* Me inside a lambda: the Me of the enclosing method, captured */
static Node *lambda_me(Node *at) {
    Sym *root = root_method();
    Node *n;
    if (!root || !(root->sflags & SF_INSTANCE) || !cur_type) serr(at, "'Me' is valid only within an instance method.");
    if (cur_type->type->kind == TY_STRUCT)
        serr(at, "Instance members and 'Me' cannot be used within a lambda expression in structures.");
    if (!root->me_sym) {
        Sym *me = new_sym(S_LOCAL, intern("$me"), "Me");
        me->type = cur_type->type;
        me->owner = root;
        me->scope = root->scope;
        root->me_sym = me;
    }
    root->me_sym->sflags |= SF_CAPTURED;
    n = lit_node(N_NAME, cur_type->type, at);
    n->sym = root->me_sym;
    n->text = "Me";
    n->cls = EC_VALUE;
    return n;
}

typedef struct {
    Sym *cm;
    Scope *sc;
    int nctl, catch_depth, nlabels, ngotos, cap_labels, cap_gotos;
    Jump *labels, *gotos;
} LambdaCtx;

static void enter_lambda(LambdaCtx *c, Sym *l) {
    c->cm = cur_method;
    c->sc = cur_scope;
    c->nctl = nctl;
    c->catch_depth = catch_depth;
    c->labels = labels;
    c->gotos = gotos;
    c->nlabels = nlabels;
    c->ngotos = ngotos;
    c->cap_labels = cap_labels;
    c->cap_gotos = cap_gotos;
    labels = gotos = 0;
    nlabels = ngotos = cap_labels = cap_gotos = 0;
    nctl = 0;
    catch_depth = 0;
    cur_method = l;
    cur_scope = l->scope;
}

static void leave_lambda(LambdaCtx *c) {
    check_gotos();
    cur_method = c->cm;
    cur_scope = c->sc;
    nctl = c->nctl;
    catch_depth = c->catch_depth;
    labels = c->labels;
    gotos = c->gotos;
    nlabels = c->nlabels;
    ngotos = c->ngotos;
    cap_labels = c->cap_labels;
    cap_gotos = c->cap_gotos;
}

/* binds a lambda: the parameter types from inv (or ptypes, or the lambda's
   As clauses), the result type from inv (NULL: inferred); returns the
   lambda's method */
static Sym *bind_lambda(Node *e, Sym *inv, Type **ptypes, Type *dtype) {
    Sym *l = new_sym(S_METHOD, 0, 0), *owner = cur_type ? cur_type : cur_module;
    NodeVec *ps = e->list2 ? e->list2 : nv_new();
    LambdaCtx ctx;
    Type *rtype = 0;
    char buf[32];
    int i, is_func = e->op == K_Function;
    snprintf(buf, sizeof buf, "$lambda%d", ++nlambda);
    l->name = intern(buf);
    l->spelling = xstrdup(buf);
    l->decl = e;
    l->owner = owner;
    l->outer = cur_method;
    l->file = e->file;
    l->mods = M_PRIVATE;
    l->sflags = SF_INSTANCE | SF_LAMBDA | SF_BOUND;
    l->vslot = -1;
    l->tparams = cur_tpscope;
    if (!owner) serr(e, "Lambda expressions are not valid here.");
    /* before the lambdas inside it: their code needs its environments */
    if (!tentative) sv_push(&owner->methods, l);
    if (inv && ps->n != inv->params.n)
        serr(e, "Nested %s does not have a signature that is compatible with delegate '%s'.",
             is_func ? "function" : "sub", delegate_sig(dtype));
    l->scope = new_scope(SC_LAMBDA, cur_scope, l);
    l->scope->materialized = 1;
    for (i = 0; i < ps->n; i++) {
        Node *pn = ps->v[i];
        Sym *p = new_sym(S_PARAM, pn->name, pn->text);
        Type *t = pn->a ? bind_type(pn->a) : 0;
        if (pn->list2) {
            int k;
            for (k = pn->list2->n - 1; k >= 0 && t; k--) t = ty_array(t, (int)pn->list2->v[k]->ival);
        }
        if (inv) {
            Sym *ip = inv->params.v[i];
            if (t && !same_rep(ip->type, t))
                serr(pn, "Nested %s does not have a signature that is compatible with delegate '%s'.",
                     is_func ? "function" : "sub", delegate_sig(dtype));
            if (!t) t = ip->type;
            p->mods = ip->mods & (M_BYREF | M_BYVAL);
        } else if (ptypes && ptypes[i]) {
            if (!t) t = ptypes[i];
        }
        if (!t) serr(pn, "Lambda parameter '%s' needs an 'As' clause: its type cannot be inferred.", pn->text);
        if (!p->mods) p->mods = M_BYVAL;
        p->type = t;
        p->owner = l;
        p->index = i;
        p->decl = pn;
        p->file = pn->file;
        p->scope = l->scope;
        pn->sym = p;
        pn->type = t;
        sv_push(&l->params, p);
        scope_add(l->scope, p);
    }
    if (inv) {
        if (!inv->type && is_func && (e->flags & F_MULTILINE))
            serr(e, "Nested function does not have a signature that is compatible with delegate '%s'.", delegate_sig(dtype));
        if (inv->type && !is_func)
            serr(e, "Nested sub does not have a signature that is compatible with delegate '%s'.", delegate_sig(dtype));
        rtype = inv->type;
    }
    if (is_func && !rtype && e->a) rtype = bind_type(e->a);
    enter_lambda(&ctx, l);
    if (is_func && !(e->flags & F_MULTILINE)) {
        /* Function(x) expression */
        Node *x = bind(e->b, 0), *st;
        if (rtype || (inv && !inv->type)) x = value(x);
        else x = rvalue(x);
        if (!rtype && !(inv && !inv->type)) rtype = x->type->kind == TY_NOTHING ? ty[TY_OBJECT] : x->type;
        if (inv && !inv->type && is_pending(x)) x = infer_pending(x);
        l->body = nv_new();
        if (inv && !inv->type) {
            /* a function lambda for a Sub delegate: the value is discarded */
            st = lit_node(N_CALLSTMT, 0, e);
            st->cls = EC_NONE;
            st->a = x;
            nv_push(l->body, st);
        } else {
            Sym *r = new_sym(S_RETVAL, intern("$ret"), "$ret");
            r->type = rtype;
            r->owner = l;
            r->index = l->locals.n;
            sv_push(&l->locals, r);
            l->retval = r;
            l->type = rtype;
            st = lit_node(N_RETURN, 0, e);
            st->cls = EC_NONE;
            st->a = coerce(x, rtype);
            nv_push(l->body, st);
        }
    } else {
        if (is_func) {
            Sym *r;
            if (!rtype) not_yet(e, "Multi-line function lambdas without a target type or 'As' clause are");
            r = new_sym(S_RETVAL, intern("$ret"), "$ret");
            r->type = rtype;
            r->owner = l;
            r->index = l->locals.n;
            sv_push(&l->locals, r);
            l->retval = r;
            l->type = rtype;
        }
        bind_block(e->list3);
        l->body = e->list3;
    }
    leave_lambda(&ctx);
    return l;
}

/* the Func or Action type of a lambda without a target type */
static Type *lambda_default_type(Node *e, Sym *l) {
    Sym *ns = ns_path("System"), *tm;
    Type *args[8];
    int n = l->params.n, i;
    const char *name = l->type ? "func" : "action";
    for (i = 0; i < n; i++) args[i] = l->params.v[i]->type;
    if (l->type) args[n++] = l->type;
    if (n > 4) not_yet(e, "Lambdas with more than 3 parameters without a target type are");
    tm = scope_find(ns->members, intern_word(name));
    if (!tm) serr(e, "lib/System.vb has no %s.", name);
    for (; tm; tm = tm->overload) {
        int ar = (tm->sflags & SF_GENERIC) && tm->decl->list ? tm->decl->list->n : 0;
        if (ar == n) return n ? instantiate_type(tm, args, n, e)->type : tm->type;
    }
    serr(e, "lib/System.vb has no %s with %d type arguments.", name, n);
    return 0;
}

static Node *dlg_node(Type *t, Sym *m, Node *target, Node *at) {
    Node *n = lit_node(N_DLGNEW, t, at);
    n->sym = m;
    n->a = target;
    return n;
}

/* a delegate to a built-in member: a lambda that calls it (the receiver is
   captured in a hidden local) */
static Sym *wrap_builtin(Sym *m, Sym *inv, Node *recv, Node *at) {
    Sym *l = new_sym(S_METHOD, 0, 0), *owner = cur_type ? cur_type : cur_module;
    NodeVec *args = nv_new();
    LambdaCtx ctx;
    Node *call, *st;
    char buf[32];
    int i;
    if (recv) not_yet(at, "'AddressOf' of an instance member of a built-in type is");
    snprintf(buf, sizeof buf, "$lambda%d", ++nlambda);
    l->name = intern(buf);
    l->spelling = xstrdup(buf);
    l->decl = at;
    l->owner = owner;
    l->outer = cur_method;
    l->file = at->file;
    l->mods = M_PRIVATE;
    l->sflags = SF_INSTANCE | SF_LAMBDA | SF_BOUND;
    l->vslot = -1;
    if (!tentative) sv_push(&owner->methods, l);
    l->scope = new_scope(SC_LAMBDA, cur_scope, l);
    l->scope->materialized = 1;
    enter_lambda(&ctx, l);
    for (i = 0; i < inv->params.n; i++) {
        Sym *p = new_sym(S_PARAM, inv->params.v[i]->name, inv->params.v[i]->spelling);
        Node *a = lit_node(N_ARG, 0, at);
        p->type = inv->params.v[i]->type;
        p->mods = M_BYVAL;
        p->owner = l;
        p->index = i;
        p->scope = l->scope;
        sv_push(&l->params, p);
        scope_add(l->scope, p);
        a->cls = EC_NONE;
        a->type = 0;
        a->a = sym_ref(p, at, 0);
        nv_push(args, a);
    }
    call = resolve_call(m, 0, args, at);
    l->body = nv_new();
    if (inv->type) {
        Sym *r = new_sym(S_RETVAL, intern("$ret"), "$ret");
        r->type = inv->type;
        r->owner = l;
        r->index = l->locals.n;
        sv_push(&l->locals, r);
        l->retval = r;
        l->type = inv->type;
        st = lit_node(N_RETURN, 0, at);
        st->cls = EC_NONE;
        st->a = coerce(call, inv->type);
    } else st = stmt_of_call(call, at);
    nv_push(l->body, st);
    leave_lambda(&ctx);
    return l;
}

/* a pending AddressOf or lambda converted to the delegate type t */
static Node *make_delegate(Node *e, Type *t) {
    Sym *inv = delegate_invoke(t);
    if (e->kind == N_ADDRESSOF) {
        Node *g = e->a;
        Sym *m, *found = 0;
        SymVec all;
        int i;
        memset(&all, 0, sizeof all);
        collect_cands(g->sym, &all);
        for (i = 0; i < all.n && !found; i++) {
            m = all.v[i];
            if (accessible(m) && method_fits(m, inv)) found = m;
        }
        if (!found)
            serr(e, "Method '%s' does not have a signature compatible with delegate '%s'.", method_sig(g->sym), delegate_sig(t));
        if ((found->sflags & SF_INSTANCE) && !g->a)
            serr(e, "Reference to a non-shared member requires an object reference.");
        if (found->rt || found->builtin || !found->decl) return dlg_node(t, wrap_builtin(found, inv, g->a, e), 0, e);
        if (!(found->sflags & SF_INSTANCE)) found->sflags |= SF_THUNK;
        return dlg_node(t, found, (found->sflags & SF_INSTANCE) ? g->a : 0, e);
    }
    return dlg_node(t, bind_lambda(e, inv, 0, t), 0, e);
}

/* a pending AddressOf or lambda without a target delegate type */
static Node *infer_pending(Node *e) {
    Sym *l;
    Node *copy;
    if (e->kind == N_ADDRESSOF)
        serr(e, "'AddressOf' expression cannot be converted to 'Object' because 'Object' is not a delegate type.");
    /* the type from a tentative binding of a copy, then the real one */
    copy = copy_tree(e);
    tentative++;
    l = bind_lambda(copy, 0, 0, 0);
    tentative--;
    return make_delegate(e, lambda_default_type(e, l));
}

/* the conversion class of a pending AddressOf or lambda to t */
static int pending_conv(Node *e, Type *t) {
    if (t->kind == TY_OBJECT) return e->kind == N_LAMBDA ? CV_WIDEN : CV_NONE;
    if (!is_delegate(t)) return CV_NONE;
    {
        Sym *inv = delegate_invoke(t);
        if (e->kind == N_ADDRESSOF) {
            SymVec all;
            int i;
            memset(&all, 0, sizeof all);
            collect_cands(e->a->sym, &all);
            for (i = 0; i < all.n; i++) if (method_fits(all.v[i], inv)) return CV_WIDEN;
            return CV_NONE;
        }
        if ((e->list2 ? e->list2->n : 0) != inv->params.n) return CV_NONE;
        if (inv->type && e->op == K_Sub) return CV_NONE;
        return CV_WIDEN;
    }
}

/* the parameter and result types for a lambda passed to a generic method:
   the delegate type syntax Func(Of A, R), Action(Of A), Predicate(Of A),
   Comparison(Of A), Converter(Of A, R) and the type parameters known so far */
static int lambda_shape(Node *tn, Node **pnodes, int *np, Node **rnode) {
    Node *last;
    Name *n;
    int k;
    if (!tn || tn->kind != N_TNAME) return 0;
    last = tn->list->v[tn->list->n - 1];
    n = last->name;
    k = last->list ? last->list->n : 0;
    *rnode = 0;
    if (n == intern_word("func") && k >= 1) {
        for (*np = 0; *np < k - 1; (*np)++) pnodes[*np] = last->list->v[*np];
        *rnode = last->list->v[k - 1];
        return 1;
    }
    if (n == intern_word("action")) {
        for (*np = 0; *np < k; (*np)++) pnodes[*np] = last->list->v[*np];
        return 1;
    }
    if ((n == intern_word("predicate") && k == 1) || (n == intern_word("comparison") && k == 1)) {
        pnodes[0] = last->list->v[0];
        pnodes[1] = last->list->v[0];
        *np = n == intern_word("predicate") ? 1 : 2;
        return 1;
    }
    if (n == intern_word("converter") && k == 2) {
        pnodes[0] = last->list->v[0];
        *np = 1;
        *rnode = last->list->v[1];
        return 1;
    }
    return 0;
}
