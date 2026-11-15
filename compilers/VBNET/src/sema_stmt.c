/*
 * sema_stmt.c - statements (spec 10): local declarations, assignments,
 * calls, conditionals, loops, branches and labels.
 */

/* ------------------------------------------------------------ control context */

typedef struct {
    int kind;           /* K_For (also For Each), K_Do, K_While, K_Select, K_Try, ... */
    Node *node;
} Ctl;

static Ctl ctls[256];
static int nctl;

static void ctl_push(int kind, Node *n) {
    if (nctl == 256) serr(n, "Statements are nested too deeply.");
    ctls[nctl].kind = kind;
    ctls[nctl].node = n;
    nctl++;
}

/* labels and GoTo statements of the method, with the statements that
   contain them (GoTo cannot jump into a For, Try, Using, With or SyncLock) */
typedef struct {
    Node *node;
    Name *name;
    Node **path;
    int npath;
} Jump;

static Jump *labels, *gotos;
static int nlabels, ngotos, cap_labels, cap_gotos;

static Name *label_name(Node *n) {
    char buf[32];
    if (n->name) return n->name;
    snprintf(buf, sizeof buf, "%llu", n->ival);
    n->text = xstrdup(buf);
    return intern(buf);
}

static void add_jump(Jump **v, int *n, int *cap, Node *node) {
    Jump *j;
    int i, k = 0;
    if (*n == *cap) {
        *cap = *cap ? 2 * *cap : 16;
        *v = xrealloc(*v, sizeof(Jump) * *cap);
    }
    j = &(*v)[(*n)++];
    j->node = node;
    j->name = label_name(node);
    j->path = xalloc(sizeof(Node *) * (nctl + 1));
    for (i = 0; i < nctl; i++)
        if (ctls[i].kind != K_Do && ctls[i].kind != K_While && ctls[i].kind != K_Select)
            j->path[k++] = ctls[i].node;
    j->npath = k;
}

static void check_gotos(void) {
    int i, j, k;
    for (i = 0; i < ngotos; i++) {
        Jump *g = &gotos[i], *l = 0;
        for (j = 0; j < nlabels; j++) if (labels[j].name == g->name) l = &labels[j];
        if (!l) serr(g->node, "Label '%s' is not defined.", g->node->text);
        for (k = 0; k < l->npath; k++) {
            Node *b = l->path[k];
            if (k >= g->npath || g->path[k] != b) {
                const char *what = b->kind == N_TRY ? "a 'Try', 'Catch' or 'Finally'" :
                                   b->kind == N_USING ? "a 'Using'" : b->kind == N_WITH ? "a 'With'" :
                                   b->kind == N_SYNCLOCK ? "a 'SyncLock'" : "a 'For' or 'For Each'";
                serr(g->node, "'GoTo %s' is not valid because '%s' is inside %s statement that does not contain this statement.",
                     g->node->text, g->node->text, what);
            }
        }
        g->node->c = l->node;
    }
}

/* ------------------------------------------------------------ locals */

static Sym *declare_local(Name *n, const char *spelling, Type *t, Node *at, int kind) {
    Scope *sc;
    Sym *s;
    for (sc = cur_scope; sc; sc = sc->parent) {
        s = scope_find(sc, n);
        if (s) {
            if (s->kind == S_PARAM) serr(at, "'%s' is already declared as a parameter of this method.", spelling);
            if (s->kind == S_RETVAL) serr(at, "Local variable cannot have the same name as the function containing it.");
            if (sc == cur_scope) serr(at, "Local variable '%s' is already declared in the current block.", spelling);
            serr(at, "Variable '%s' hides a variable in an enclosing block.", spelling);
        }
        if (sc->kind == SC_METHOD) break;
    }
    s = new_sym(kind, n, spelling);
    s->type = t;
    s->decl = at;
    s->owner = cur_method;
    s->file = at->file;
    s->scope = cur_scope;
    if (kind == S_LOCAL) {
        s->index = cur_method->locals.n;
        sv_push(&cur_method->locals, s);
    }
    scope_add(cur_scope, s);
    return s;
}

/* Option Explicit Off: an undeclared name is a local of the method */
static Sym *implicit_local(Node *at) {
    Scope *sc = cur_scope, *save = cur_scope;
    Type *t = at->tchar ? tchar_type(at->tchar, at) : ty[TY_OBJECT];
    Sym *s;
    while (sc->kind != SC_METHOD) sc = sc->parent;
    cur_scope = sc;
    s = declare_local(at->name, at->text, t, at, S_LOCAL);
    cur_scope = save;
    return s;
}

static void bind_block(NodeVec *v);
static void bind_stmt(Node *s);

/* the type of a declared variable: As clause, type character, array
   modifiers and bounds */
static Type *var_type(Node *vi, Type *as, Node *at) {
    Type *t = as;
    int i;
    if (vi->tchar) {
        Type *tc = tchar_type(vi->tchar, vi);
        if (as) serr(vi, "Type character '%c' cannot be used in a declaration with an explicit type.", vi->tchar);
        t = tc;
    }
    if (!t) return 0;
    if (vi->list2)
        for (i = vi->list2->n - 1; i >= 0; i--) t = ty_array(t, (int)vi->list2->v[i]->ival);
    (void)at;
    return t;
}

static void bind_bounds(NodeVec *b) {
    int i;
    if (!b) return;
    for (i = 0; i < b->n; i++) {
        Node *x = b->v[i];
        if (x->kind == N_BOUND) {
            Node *lo = rvalue(x->a);
            if (!is_const(lo) || !ty_integral(lo->type) || lo->ival != 0) serr(x, "Array lower bounds can be only '0'.");
            x = x->b;
        }
        b->v[i] = coerce(rvalue(x), ty[TY_INTEGER]);
    }
}

/* a constant's value: the initializer converted to its type */
static Node *const_value(Node *init, Type *t, Node *at) {
    Node *v;
    if (!init) serr(at, "Constants must have a value.");
    v = rvalue(init);
    if (v->kind == N_COLLINIT && !v->type) serr(v, "Constant expression is required.");
    if (!t) t = v->type->kind == TY_NOTHING ? ty[TY_OBJECT] : v->type;
    if (t->kind == TY_ARRAY || (t->kind == TY_OBJECT && v->type->kind != TY_NOTHING))
        serr(at, "Constants must be of an intrinsic or enumerated type, not a class, structure, type parameter, or array type.");
    v = coerce(v, t);
    if (!is_const(v)) serr(v, "Constant expression is required.");
    return v;
}

/* Dim, Static, Const */
static void bind_local(Node *st) {
    int i, j, is_const_decl = (st->mods & M_CONST) != 0;
    for (i = 0; i < st->list->n; i++) {
        Node *d = st->list->v[i];
        Type *as = d->a ? bind_type(d->a) : 0;
        Node *asnew = 0;
        if (d->flags & F_ASNEW) {
            /* Dim a, b As New T(...): a new object for each variable */
            asnew = d->b;
            d->b = 0;
            as = bind_type(asnew->a);
        }
        if (d->b && d->list->n > 1)
            serr(d, "Explicit initialization is not permitted with multiple variables declared with a single type specifier.");
        for (j = 0; j < d->list->n; j++) {
            Node *vi = d->list->v[j];
            Type *t = var_type(vi, as, vi);
            Node *init = 0;
            Sym *s;
            if (vi->list && d->b) serr(vi, "Explicit initialization is not permitted for arrays declared with explicit bounds.");
            if (asnew && vi->list) serr(vi, "Arrays cannot be declared with 'New'.");
            bind_bounds(vi->list);
            if (asnew) {
                Node *nw = xalloc(sizeof(Node));
                *nw = *asnew;
                init = bind(nw, 0);
                s = declare_local(vi->name, vi->text, t, vi, S_LOCAL);
                if (st->mods & M_STATIC) {
                    s->mods |= M_STATIC;
                    s->index = prog_fields.n;
                    sv_push(&prog_fields, s);
                }
                vi->sym = s;
                vi->type = t;
                vi->b = init;
                continue;
            }
            if (is_const_decl) {
                if (vi->list2) serr(vi, "Constants must be of an intrinsic or enumerated type, not a class, structure, type parameter, or array type.");
                if (!t && opts->strict && !opts->infer)
                    serr(vi, "Option Strict On requires all variable declarations to have an 'As' clause.");
                init = const_value(d->b, t, vi);
                s = declare_local(vi->name, vi->text, init->type, vi, S_CONST);
                s->cval = init;
                s->state = 2;
            } else {
                if (d->b) {
                    init = bind(d->b, 0);
                    if (init->kind == N_COLLINIT && !init->type) init = bind_collinit(init, t);
                    else init = value(init);
                    if (!t && is_pending(init)) init = infer_pending(init);
                    if (!t) {
                        if (opts->infer) t = init->type->kind == TY_NOTHING ? ty[TY_OBJECT] : init->type;
                        else {
                            if (opts->strict) serr(vi, "Option Strict On requires all variable declarations to have an 'As' clause.");
                            t = ty[TY_OBJECT];
                        }
                    }
                    init = coerce(init, t);
                } else if (!t) {
                    if (opts->strict) serr(vi, "Option Strict On requires all variable declarations to have an 'As' clause.");
                    t = ty[TY_OBJECT];
                }
                s = declare_local(vi->name, vi->text, t, vi, S_LOCAL);
                if (st->mods & M_STATIC) {
                    s->mods |= M_STATIC;
                    s->index = prog_fields.n;
                    sv_push(&prog_fields, s);
                    if (!init && t->kind == TY_STRUCT) {
                        init = lit_node(N_NEW, t, vi);     /* the zero value, once */
                        init->list = nv_new();
                    }
                }
            }
            vi->sym = s;
            vi->type = s->type;
            vi->b = init;
        }
        d->b = 0;
    }
}

/* ------------------------------------------------------------ assignments, calls */

static int compound_op(int k) {
    switch (k) {
    case P_PLUSEQ: return P_PLUS;
    case P_MINUSEQ: return P_MINUS;
    case P_STAREQ: return P_STAR;
    case P_SLASHEQ: return P_SLASH;
    case P_BACKSLASHEQ: return P_BACKSLASH;
    case P_CARETEQ: return P_CARET;
    case P_AMPEQ: return P_AMP;
    case P_SHLEQ: return P_SHL;
    case P_SHREQ: return P_SHR;
    }
    return 0;
}

static Node *assign_target(Node *t) {
    Node *b = bind(t, 0);
    if (b->cls == EC_VAR) return b;
    if (b->cls == EC_PROP) {
        if (!b->sym->setter) serr(t, "Property '%s' is 'ReadOnly'.", b->sym->spelling);
        return b;
    }
    if ((b->kind == N_NAME || b->kind == N_MEMBER) && b->sym && b->sym->kind == S_FIELD && (b->sym->mods & M_READONLY))
        serr(t, "'ReadOnly' variable cannot be the target of an assignment.");
    if (b->kind == N_MEMBER && b->sym && b->sym->kind == S_FIELD)
        serr(t, "Expression is a value and therefore cannot be the target of an assignment.");
    if (b->cls == EC_METHOD || b->cls == EC_TYPE || b->cls == EC_NS || b->cls == EC_VOID) {
        if (b->cls == EC_METHOD && (b->sym->mods & M_READONLY))
            serr(t, "Property '%s' is 'ReadOnly'.", b->sym->spelling);
        if (b->cls != EC_METHOD) value(b);
        serr(t, "Expression is a value and therefore cannot be the target of an assignment.");
    }
    if (is_const(b) && (t->kind == N_NAME || t->kind == N_MEMBER))
        serr(t, "Constant cannot be the target of an assignment.");
    if (b->kind == N_INVOKE && b->sym && (b->sym->mods & M_READONLY))
        serr(t, "Property '%s' is 'ReadOnly'.", b->sym->spelling);
    serr(t, "Expression is a value and therefore cannot be the target of an assignment.");
    return 0;
}

static Node *new_binary(int op, Node *a, Node *b, Node *at) {
    Node *n = lit_node(N_BINARY, 0, at);
    n->cls = EC_NONE;
    n->op = op;
    n->a = a;
    n->b = b;
    return n;
}

static Node *withevents_hooks(Node *target, int kind, Node *at);
static Node *assign_node(Node *target, Node *value, Node *at);

static void bind_assign(Node *st) {
    Node *t = assign_target(st->a), *v;
    if (t->cls == EC_PROP) {
        /* a property: a call of its setter (with the getter for x op= y) */
        Node *call, *arg;
        NodeVec *args = nv_new();
        int i;
        if (st->op == P_EQ) {
            v = bind(st->b, 0);
            if (v->kind == N_COLLINIT && !v->type) v = bind_collinit(v, t->type);
            v = value(v);
        } else v = bind(new_binary(compound_op(st->op), value(t), st->b, st), 0);
        for (i = 0; i < t->list->n; i++) nv_push(args, t->list->v[i]);
        arg = lit_node(N_ARG, v->type, st);
        arg->a = v;
        nv_push(args, arg);
        check_access(t->sym->setter, st);
        call = resolve_call(t->sym->setter, t->a, args, st);
        st->kind = N_CALLSTMT;
        st->a = call;
        st->b = 0;
        return;
    }
    if (st->op == P_EQ) {
        v = bind(st->b, 0);
        if (v->kind == N_COLLINIT && !v->type) v = bind_collinit(v, t->type);
        v = coerce(value(v), t->type);
    } else {
        Node *ref = lit_node(N_TARGET, t->type, st);
        v = coerce(bind(new_binary(compound_op(st->op), ref, st->b, st), 0), t->type);
    }
    st->a = t;
    st->b = v;
    if (t->sym && t->sym->kind == S_FIELD && (t->sym->mods & M_WITHEVENTS)) {
        Node *rm = withevents_hooks(t, N_REMOVEHANDLER, st), *add;
        if (rm) {
            Node *as = assign_node(t, v, st);
            add = withevents_hooks(t, N_ADDHANDLER, st);
            st->kind = N_BLOCK;
            st->list = nv_new();
            nv_push(st->list, rm);
            nv_push(st->list, as);
            nv_push(st->list, add);
        }
    }
}

static Node *ctor_first_stmt;   /* the statement that may call MyBase.New / Me.New */

static void bind_call_stmt(Node *st) {
    Node *x = bind(st->a, 0);
    if (x->cls == EC_METHOD) x = resolve_call(x->sym, x->a, nv_new(), st);
    if (x->kind == N_NOTHING && (x->flags & F_DIRECT)) {
        /* MyBase.New() with Object as the base */
        if (st != ctor_first_stmt) serr(st, "Constructor call is valid only as the first statement in an instance constructor.");
        st->a = x;
        return;
    }
    if (x->kind != N_INVOKE || !x->sym) serr(st, "Expression is not a method.");
    if ((x->sym->sflags & SF_CTOR) && st != ctor_first_stmt)
        serr(st, "Constructor call is valid only as the first statement in an instance constructor.");
    st->a = x;
}

static Node *bool_cond(Node *e) {
    return coerce(rvalue(e), ty[TY_BOOLEAN]);
}

/* ------------------------------------------------------------ Select Case */

static void bind_select(Node *st) {
    Node *sel = rvalue(st->a);
    int i, j;
    if (sel->kind == N_COLLINIT && !sel->type) sel = bind_collinit(sel, 0);
    st->a = sel;
    ctl_push(K_Select, st);
    for (i = 0; i < st->list->n; i++) {
        Node *cs = st->list->v[i];
        if (cs->list) {
            for (j = 0; j < cs->list->n; j++) {
                Node *cl = cs->list->v[j], *ref = lit_node(N_SELREF, sel->type, cl), *cmp;
                if (cl->kind == N_CASEVAL) cmp = new_binary(P_EQ, ref, cl->a, cl);
                else if (cl->kind == N_CASEIS) cmp = new_binary(cl->op, ref, cl->a, cl);
                else {
                    Node *ref2 = lit_node(N_SELREF, sel->type, cl);
                    cmp = new_binary(K_AndAlso, new_binary(P_GE, ref, cl->a, cl), new_binary(P_LE, ref2, cl->b, cl), cl);
                }
                cl->a = coerce(bind(cmp, 0), ty[TY_BOOLEAN]);
                cl->b = 0;
                cl->type = ty[TY_BOOLEAN];
            }
        }
        bind_block(cs->list2);
    }
    nctl--;
}

/* ------------------------------------------------------------ loops */

static int loop_var_ok(Type *t) {
    return ty_numeric(t) || t->kind == TY_OBJECT || t->kind == TY_ENUM;
}

/* a name of a variable (not of a type, namespace or method) */
static int is_variable(Sym *s) {
    return s && (s->kind == S_LOCAL || s->kind == S_PARAM || s->kind == S_FIELD || s->kind == S_RETVAL || s->kind == S_PROP);
}

/* the control variable of a For or For Each loop; elem: the type to infer
   for a new variable (NULL: from the bounds, given in v/n) */
static Node *loop_var(Node *lv, Type *infer_t) {
    if (lv->kind == N_LOOPVAR) {
        Type *t = bind_type(lv->a);
        Sym *s;
        if (lv->flags & F_NULLABLE) not_yet(lv, "Nullable types are");
        s = declare_local(lv->name, lv->text, t, lv, S_LOCAL);
        return sym_ref(s, lv, 0);
    }
    if (lv->kind == N_NAME && !is_variable(lookup(lv->name, lv, 0)) && (opts->infer || !opts->explicit_)) {
        Type *t = lv->tchar ? tchar_type(lv->tchar, lv) : opts->infer ? infer_t : ty[TY_OBJECT];
        Sym *s;
        if (!t) t = ty[TY_OBJECT];
        s = declare_local(lv->name, lv->text, t, lv, S_LOCAL);
        return sym_ref(s, lv, 0);
    }
    return assign_target(lv);
}

static void check_next(Node *st, Node *var) {
    Node *nx;
    if (!st->list2 || st->list2->n == 0) return;
    nx = bind(st->list2->v[0], 0);
    if (nx->kind != var->kind || nx->sym != var->sym || (var->kind != N_NAME && var->sym == 0))
        serr(st->list2->v[0], "Next control variable does not match For loop control variable '%s'.",
             var->text ? var->text : "");
}

static void bind_for(Node *st) {
    Scope *save = cur_scope;
    Node *from, *to, *step = 0, *var;
    Type *t;
    cur_scope = new_scope(SC_BLOCK, cur_scope, cur_method);
    cur_scope->materialized = 1;
    st->scope = cur_scope;
    from = rvalue(st->b);
    to = rvalue(st->c);
    if (st->d) step = rvalue(st->d);
    {
        Node *v[3];
        int n = 0;
        v[n++] = from;
        v[n++] = to;
        if (step) v[n++] = step;
        t = dominant(v, n);
    }
    var = loop_var(st->a, t);
    if (!loop_var_ok(var->type))
        serr(st->a, "'For' loop control variable cannot be of type '%s' because the type does not support the required operators.",
             type_name(var->type));
    if (var->type->kind == TY_OBJECT) not_yet(st->a, "'For' loops with an 'Object' control variable are");
    no_decimal(st->a, var->type);
    st->a = var;
    st->b = coerce(from, var->type);
    st->c = coerce(to, var->type);
    if (step) st->d = coerce(step, var->type);
    ctl_push(K_For, st);
    bind_block(st->list);
    nctl--;
    check_next(st, var);
    cur_scope = save;
}

/* ------------------------------------------------------------ rewriting helpers */

static int nhidden;

/* a local the compiler adds ($t1, ...) */
static Sym *hidden_local(Type *t, Node *at) {
    char buf[32];
    snprintf(buf, sizeof buf, "$t%d", ++nhidden);
    return declare_local(intern(buf), xstrdup(buf), t, at, S_LOCAL);
}

static Node *assign_node(Node *target, Node *value, Node *at) {
    Node *n = lit_node(N_ASSIGN, 0, at);
    n->op = P_EQ;
    n->a = target;
    n->b = value;
    n->cls = EC_NONE;
    return n;
}

static Node *block_node(Node *at) {
    Node *n = lit_node(N_BLOCK, 0, at);
    n->list = nv_new();
    n->cls = EC_NONE;
    return n;
}

static Node *stmt_of_call(Node *call, Node *at) {
    Node *n = lit_node(N_CALLSTMT, 0, at);
    n->a = call;
    n->cls = EC_NONE;
    return n;
}

static Sym *idisposable_sym(Node *at) {
    Sym *s = scope_find(ns_path("System")->members, intern_word("idisposable"));
    if (!s || s->kind != S_TYPE) serr(at, "The type 'System.IDisposable' is not defined (lib/System.vb is missing).");
    return s;
}

/* If v IsNot Nothing Then CType(v, IDisposable).Dispose() (checked: with
   TryCast, for an enumerator that need not implement IDisposable) */
static Node *dispose_stmt(Sym *v, Node *at, int checked) {
    Sym *id = idisposable_sym(at), *dm = scope_find(id->members, intern_word("dispose"));
    Node *cast, *cast2, *cond, *ifn;
    if (v->type->kind == TY_STRUCT) {
        cast = coerce(sym_ref(v, at, 0), id->type);
        return stmt_of_call(resolve_call(dm, cast, nv_new(), at), at);
    }
    if (checked) {
        cast = lit_node(N_CONV, id->type, at);
        cast->a = sym_ref(v, at, 0);
        cast->flags |= F_TRYCAST | F_EXPLICIT;
        cast2 = lit_node(N_CONV, id->type, at);
        cast2->a = sym_ref(v, at, 0);
        cast2->flags |= F_TRYCAST | F_EXPLICIT;
    } else {
        cast = coerce(sym_ref(v, at, 0), id->type);
        cast2 = coerce(sym_ref(v, at, 0), id->type);
    }
    cond = lit_node(N_BINARY, ty[TY_BOOLEAN], at);
    cond->op = K_IsNot;
    cond->a = cast;
    cond->b = lit_node(N_NOTHING, id->type, at);
    ifn = lit_node(N_IF, 0, at);
    ifn->cls = EC_NONE;
    ifn->a = cond;
    ifn->list = nv_new();
    nv_push(ifn->list, stmt_of_call(resolve_call(dm, cast2, nv_new(), at), at));
    return ifn;
}

/* Try (body) Finally (fin) End Try */
static Node *try_node(NodeVec *body, NodeVec *fin, Node *at) {
    Node *t = lit_node(N_TRY, 0, at);
    t->cls = EC_NONE;
    t->list = body;
    t->list2 = nv_new();
    t->list3 = fin;
    t->flags |= F_HASFINALLY;
    return t;
}

/* a member of a value of a class, structure or interface type, or NULL */
static Node *member_expr(Node *recv, const char *name, Node *at) {
    Node *m;
    Type *t = recv->type;
    Name *n = intern_word(name);
    Sym *s = 0;
    if (t->kind == TY_CLASS || t->kind == TY_INTERFACE) s = type_lookup(t->sym, n);
    else if (t->kind == TY_STRUCT) s = scope_find(t->sym->members, n);
    if (!s || (s->kind != S_METHOD && s->kind != S_PROP) || !accessible(s) || !(s->sflags & SF_INSTANCE)) return 0;
    m = lit_node(N_MEMBER, 0, at);
    m->cls = EC_NONE;
    m->name = n;
    m->text = name;
    m->a = recv;
    return bind(m, 0);
}

static Node *call_member(Node *recv, const char *name, Node *at) {
    Node *m = member_expr(recv, name, at);
    if (!m) return 0;
    if (m->cls == EC_METHOD) return resolve_call(m->sym, m->a, nv_new(), at);
    return value(m);
}

/* For Each over a collection: its enumerator (GetEnumerator, MoveNext,
   Current), the loop in a Try that disposes the enumerator */
static void bind_foreach_enum(Node *st, Node *coll) {
    Node *ge = call_member(coll, "GetEnumerator", st), *mv, *cur, *loop, *var, *ref, *body;
    Sym *e, *ie = scope_find(ns_path("System.Collections")->members, intern_word("ienumerable"));
    Scope *save = cur_scope;
    if (!ge && ie && (conv_class(coll->type, ie->type) == CV_WIDEN || (coll->type->kind == TY_OBJECT && !opts->strict)))
        ge = call_member(coerce(coll, ie->type), "GetEnumerator", st);
    if (!ge || !ge->type || !ty_user(ge->type)) {
        serr(st->b, "Expression is of type '%s', which is not a collection type.", type_name(coll->type));
    }
    cur_scope = new_scope(SC_BLOCK, cur_scope, cur_method);
    e = hidden_local(ge->type, st);
    mv = call_member(sym_ref(e, st, 0), "MoveNext", st);
    cur = call_member(sym_ref(e, st, 0), "Current", st);
    if (!mv || mv->type != ty[TY_BOOLEAN] || !cur || !cur->type)
        serr(st->b, "Expression is of type '%s', which is not a collection type.", type_name(coll->type));
    loop = xalloc(sizeof(Node));
    *loop = *st;
    loop->flags |= F_ENUM;
    var = loop_var(st->a, cur->type);
    loop->a = var;
    loop->b = cur;
    loop->d = mv;
    ref = lit_node(N_TARGET, cur->type, st);
    loop->c = coerce(ref, var->type);
    ctl_push(K_For, loop);
    bind_block(loop->list);
    nctl--;
    check_next(loop, var);
    cur_scope = save;
    body = block_node(st);
    nv_push(body->list, loop);
    st->kind = N_BLOCK;
    st->list = nv_new();
    st->list->scope = e->scope;
    e->scope->materialized = 1;
    nv_push(st->list, assign_node(sym_ref(e, st, 0), ge, st));
    {
        NodeVec *tb = nv_new(), *fin = nv_new();
        nv_push(tb, body);
        nv_push(fin, dispose_stmt(e, st, 1));
        nv_push(st->list, try_node(tb, fin, st));
    }
}

static void bind_foreach(Node *st) {
    Scope *save = cur_scope;
    Node *coll = rvalue(st->b), *var, *ref;
    Type *elem;
    if (coll->kind == N_COLLINIT && !coll->type) coll = bind_collinit(coll, 0);
    if (coll->type->kind == TY_ARRAY) elem = coll->type->elem;
    else if (coll->type->kind == TY_STRING) elem = ty[TY_CHAR];
    else if (ty_user(coll->type) || coll->type->kind == TY_OBJECT) {
        bind_foreach_enum(st, coll);
        return;
    } else {
        serr(st->b, "Expression is of type '%s', which is not a collection type.", type_name(coll->type));
        elem = 0;
    }
    cur_scope = new_scope(SC_BLOCK, cur_scope, cur_method);
    cur_scope->materialized = 1;
    st->scope = cur_scope;
    var = loop_var(st->a, elem);
    st->a = var;
    st->b = coll;
    ref = lit_node(N_TARGET, elem, st);
    st->c = coerce(ref, var->type);         /* the conversion of an element to the variable */
    ctl_push(K_For, st);
    bind_block(st->list);
    nctl--;
    check_next(st, var);
    cur_scope = save;
}

/* ------------------------------------------------------------ branches */

static const char *exit_name(int k) { return tok_spelling_of(k); }

static void bind_exit(Node *st, int is_continue) {
    int k = st->op, i, want = k;
    const char *kw = is_continue ? "Continue" : "Exit";
    for (i = nctl - 1; i >= 0; i--) {
        if (ctls[i].kind == want) break;
        if (ctls[i].kind == K_Finally) serr(st, "Branching out of a 'Finally' is not valid.");
    }
    if (k == K_Sub || k == K_Function || k == K_Property) {
        int mk = cur_method->decl ? cur_method->decl->op : K_Sub;
        if (cur_method->sflags & SF_CTOR) mk = K_Sub;
        if (cur_method->sflags & SF_ACCESSOR) {
            if (k == K_Property) return;
            mk = K_Property;
        }
        if (k == K_Sub && mk != K_Sub) serr(st, "'Exit Sub' is not valid in a Function or Property.");
        if (k == K_Function && mk != K_Function) serr(st, "'Exit Function' is not valid in a Sub or Property.");
        if (k == K_Property && mk != K_Property) serr(st, "'Exit Property' is not valid in a Function or Sub.");
        return;
    }
    for (i = nctl - 1; i >= 0; i--) {
        if (ctls[i].kind == want) {
            st->c = ctls[i].node;
            return;
        }
    }
    if (k == K_Select) serr(st, "'Exit Select' can only appear inside a 'Select' statement.");
    serr(st, "'%s %s' can only appear inside a '%s' statement.", kw, exit_name(k), exit_name(k));
}

static int finally_open(void);

static void bind_return(Node *st) {
    Sym *m = cur_method;
    if (finally_open()) serr(st, "Branching out of a 'Finally' is not valid.");
    if (!m->type) {
        if (st->a) serr(st, "'Return' statement in a Sub or a Set cannot return a value.");
        return;
    }
    if (!st->a) serr(st, "'Return' statement in a Function, Get, or Operator must return a value.");
    st->a = bind(st->a, 0);
    if (st->a->kind == N_COLLINIT && !st->a->type) st->a = bind_collinit(st->a, m->type);
    st->a = coerce(value(st->a), m->type);
}

/* ------------------------------------------------------------ arrays */

static void bind_redim(Node *st) {
    int i;
    for (i = 0; i < st->list->n; i++) {
        Node *c = st->list->v[i];
        Node *t = assign_target(c->a);
        if (t->type->kind != TY_ARRAY) serr(c, "'ReDim' statement requires an array.");
        if (c->list->n != t->type->rank) serr(c, "'ReDim' cannot change the number of dimensions of an array.");
        bind_bounds(c->list);
        c->a = t;
        c->type = t->type;
    }
}

static void bind_erase(Node *st) {
    int i;
    for (i = 0; i < st->list->n; i++) {
        Node *t = assign_target(st->list->v[i]);
        if (t->type->kind != TY_ARRAY) serr(st->list->v[i], "'Erase' statement requires an array.");
        st->list->v[i] = t;
    }
}

/* ------------------------------------------------------------ exceptions */

static int catch_depth;     /* Catch blocks around the statement (for Throw without an operand) */

static Sym *exception_sym(Node *at) {
    Sym *ns = ns_path("System"), *s = scope_find(ns->members, intern_word("exception"));
    if (!s || s->kind != S_TYPE) serr(at, "The type 'System.Exception' is not defined (lib/System.vb is missing).");
    return s;
}

static int finally_open(void) {
    int i;
    for (i = 0; i < nctl; i++) if (ctls[i].kind == K_Finally) return 1;
    return 0;
}

static void bind_try(Node *st) {
    Sym *ex = exception_sym(st);
    int i;
    ctl_push(K_Try, st);
    bind_block(st->list);
    for (i = 0; i < st->list2->n; i++) {
        Node *cn = st->list2->v[i];
        Scope *save = cur_scope;
        Type *t = cn->a ? bind_type(cn->a) : ex->type;
        cur_scope = new_scope(SC_BLOCK, cur_scope, cur_method);
        if (t->kind != TY_CLASS || !derives(t->sym, ex))
            serr(cn->a ? cn->a : cn, "'Catch' cannot catch type '%s' because it is not 'System.Exception' or a class that "
                                    "inherits from 'System.Exception'.", type_name(t));
        cn->type = t;
        if (cn->name) {
            if (cn->a) cn->sym = declare_local(cn->name, cn->text, t, cn, S_LOCAL);
            else {
                Node *v = assign_target(cn);
                if (v->kind != N_NAME || v->type->kind != TY_CLASS || !derives(v->type->sym, ex))
                    serr(cn, "'Catch' cannot catch type '%s' because it is not 'System.Exception' or a class that "
                             "inherits from 'System.Exception'.", type_name(v->type));
                cn->sym = v->sym;
            }
        }
        if (cn->b) cn->b = bool_cond(cn->b);
        catch_depth++;
        bind_block(cn->list);
        catch_depth--;
        cur_scope = save;
    }
    nctl--;
    if (st->list3) {
        int save = catch_depth;
        catch_depth = 0;
        ctl_push(K_Finally, st);
        bind_block(st->list3);
        nctl--;
        catch_depth = save;
    }
}

static void bind_throw(Node *st) {
    Sym *ex = exception_sym(st);
    if (!st->a) {
        if (!catch_depth)
            serr(st, "'Throw' statement cannot omit operand outside a 'Catch' statement or inside a 'Finally' statement.");
        return;
    }
    st->a = rvalue(st->a);
    if (st->a->type->kind == TY_NOTHING) st->a = coerce(st->a, ex->type);
    if (st->a->type->kind != TY_CLASS || !derives(st->a->type->sym, ex))
        serr(st->a, "Value of type '%s' cannot be converted to 'System.Exception'.", type_name(st->a->type));
}

/* ------------------------------------------------------------ With, Using, SyncLock */

static Sym *with_stack[64];
static int nwith;

/* the object of the innermost With (.member) */
static Node *with_object(Node *at) {
    if (!nwith) serr(at, "Leading '.' or '!' can only appear inside a 'With' statement.");
    return sym_ref(with_stack[nwith - 1], at, 0);
}

static Node *marker(int kind, Node *at) {
    Node *m = lit_node(kind, 0, at);
    m->cls = EC_NONE;
    return m;
}

static void bind_with(Node *st) {
    Node *v = rvalue(st->a), *init, *body;
    Sym *h;
    if (v->kind == N_COLLINIT && !v->type) v = bind_collinit(v, 0);
    h = hidden_local(v->type, st);
    init = assign_node(sym_ref(h, st, 0), v, st);
    if (v->type->kind == TY_STRUCT && v->cls == EC_VAR) init->flags |= F_DIRECT;   /* the variable itself */
    if (nwith == 64) serr(st, "'With' statements are nested too deeply.");
    with_stack[nwith++] = h;
    ctl_push(K_With, marker(N_WITH, st));
    body = block_node(st);
    body->list = st->list;
    bind_block(body->list);
    nctl--;
    nwith--;
    st->kind = N_BLOCK;
    st->list = nv_new();
    nv_push(st->list, init);
    nv_push(st->list, body);
}

static void bind_synclock(Node *st) {
    Node *v = rvalue(st->a), *body;
    Sym *h;
    if (!ty_ref(v->type) || v->type->kind == TY_NOTHING)
        serr(st->a, "'SyncLock' operand cannot be of type '%s' because '%s' is not a reference type.", type_name(v->type),
             type_name(v->type));
    h = hidden_local(ty[TY_OBJECT], st);
    ctl_push(K_SyncLock, marker(N_SYNCLOCK, st));
    body = block_node(st);
    body->list = st->list;
    bind_block(body->list);
    nctl--;
    st->kind = N_BLOCK;
    st->list = nv_new();
    nv_push(st->list, assign_node(sym_ref(h, st, 0), coerce(v, ty[TY_OBJECT]), st));
    nv_push(st->list, body);
}

static void check_disposable(Type *t, Node *at) {
    Sym *id = idisposable_sym(at);
    if (t != id->type && conv_class(t, id->type) != CV_WIDEN)
        serr(at, "'Using' operand of type '%s' must implement 'System.IDisposable'.", type_name(t));
}

/* Using r1 [, r2 ...] / Using expression: each resource group is
   initialized, then the rest is in a Try that disposes it */
static void bind_using(Node *st) {
    Scope *save = cur_scope;
    NodeVec *inits = nv_new(), *groups = nv_new();
    Node *body;
    int i, j;
    Scope *using_scope;
    cur_scope = new_scope(SC_BLOCK, cur_scope, cur_method);
    using_scope = cur_scope;
    if (st->list) {
        for (i = 0; i < st->list->n; i++) {
            Node *loc = lit_node(N_LOCAL, 0, st->list->v[i]);
            NodeVec *syms = nv_new();
            loc->cls = EC_NONE;
            loc->mods = M_DIM;
            loc->list = nv_new();
            nv_push(loc->list, st->list->v[i]);
            if (!(st->list->v[i]->flags & F_ASNEW) && !st->list->v[i]->b)
                serr(st->list->v[i], "'Using' resource variable must have an explicit initialization.");
            bind_local(loc);
            for (j = 0; j < st->list->v[i]->list->n; j++) {
                Node *vi = st->list->v[i]->list->v[j];
                check_disposable(vi->sym->type, vi);
                nv_push(syms, vi);
            }
            nv_push(inits, loc);
            loc->list2 = syms;
            nv_push(groups, loc);
        }
    } else {
        Node *v = rvalue(st->a), *loc;
        Sym *h;
        check_disposable(v->type, st->a);
        h = hidden_local(v->type, st);
        loc = assign_node(sym_ref(h, st, 0), v, st);
        loc->list2 = nv_new();
        {
            Node *vi = lit_node(N_VARID, v->type, st);
            vi->sym = h;
            nv_push(loc->list2, vi);
        }
        nv_push(groups, loc);
    }
    ctl_push(K_Using, marker(N_USING, st));
    body = block_node(st);
    body->list = st->list2;
    bind_block(body->list);
    nctl--;
    cur_scope = save;
    /* from the innermost group out: init; Try rest Finally dispose */
    {
        NodeVec *inner = nv_new();
        nv_push(inner, body);
        for (i = groups->n - 1; i >= 0; i--) {
            Node *g = groups->v[i];
            NodeVec *fin = nv_new(), *outer = nv_new();
            for (j = g->list2->n - 1; j >= 0; j--) nv_push(fin, dispose_stmt(g->list2->v[j]->sym, st, 0));
            g->list2 = 0;
            nv_push(outer, g);
            nv_push(outer, try_node(inner, fin, st));
            inner = outer;
        }
        st->kind = N_BLOCK;
        st->list = inner;
        inner->scope = using_scope;
        using_scope->materialized = 1;
    }
}

/* ------------------------------------------------------------ events */

static Node *make_delegate(Node *e, Type *t);
static int is_pending(Node *e);

/* the field of an event (on the object of the event expression) */
static Node *event_field(Node *ev, Node *at) {
    return sym_ref(ev->sym->field, at, ev->a);
}

/* AddHandler e, handler / RemoveHandler e, handler:
   e.EEvent = CType([Delegate].Combine(e.EEvent, handler), T) */
static void bind_handler(Node *st) {
    Node *ev = bind(st->a, 0), *h, *call, *f;
    Sym *dt = scope_find(ns_path("System")->members, intern_word("delegate")), *m;
    NodeVec *args = nv_new();
    Node *a1, *a2;
    if (ev->cls != EC_EVENT)
        serr(st->a, "'AddHandler' or 'RemoveHandler' statement event operand must be a dot-qualified expression or a simple name.");
    h = value(bind(st->b, 0));
    h = coerce(h, ev->type);
    m = scope_find(dt->members, intern_word(st->kind == N_ADDHANDLER ? "combine" : "remove"));
    a1 = lit_node(N_ARG, ev->type, st);
    a1->a = event_field(ev, st);
    a2 = lit_node(N_ARG, ev->type, st);
    a2->a = h;
    nv_push(args, a1);
    nv_push(args, a2);
    call = resolve_call(m, 0, args, st);
    f = event_field(ev, st);
    f->cls = EC_VAR;
    st->kind = N_ASSIGN;
    st->op = P_EQ;
    st->a = f;
    st->b = explicit_conv(call, ev->type, st);
}

/* RaiseEvent E(args): If EEvent IsNot Nothing Then EEvent.Invoke(args) */
static void bind_raiseevent(Node *st) {
    Node *nm = lit_node(N_NAME, 0, st), *ev, *cond, *call;
    nm->cls = EC_NONE;
    nm->name = st->name;
    nm->text = st->text;
    ev = bind(nm, 0);
    if (ev->cls != EC_EVENT) serr(st, "'%s' is not an event of '%s'.", st->text, cur_type ? cur_type->spelling : cur_module ? cur_module->spelling : "");
    cond = lit_node(N_BINARY, ty[TY_BOOLEAN], st);
    cond->op = K_IsNot;
    cond->a = event_field(ev, st);
    cond->b = lit_node(N_NOTHING, ev->type, st);
    call = resolve_call(delegate_invoke(ev->type), event_field(ev, st), bind_args(st->list), st);
    st->kind = N_IF;
    st->a = cond;
    st->list = nv_new();
    nv_push(st->list, stmt_of_call(call, st));
    st->list2 = 0;
    st->list3 = 0;
    st->flags = 0;
}

/* an unbound statement: AddHandler <target>.<event>, AddressOf <handler> */
static Node *handler_stmt(int kind, Node *target, Name *event, const char *etext, Sym *handler, Node *at) {
    Node *st = lit_node(kind, 0, at), *mem = lit_node(N_MEMBER, 0, at), *ao = lit_node(N_ADDRESSOF, 0, at), *hn = lit_node(N_NAME, 0, at);
    st->cls = mem->cls = ao->cls = hn->cls = EC_NONE;
    st->type = mem->type = ao->type = hn->type = 0;
    mem->a = target;
    mem->name = event;
    mem->text = etext;
    hn->name = handler->name;
    hn->text = handler->spelling;
    ao->a = hn;
    st->a = mem;
    st->b = ao;
    return st;
}

/* the handlers of a WithEvents field (Handles field.E): its value changes */
static Node *withevents_hooks(Node *target, int kind, Node *at) {
    Sym *f = target->sym;
    Node *ifn, *cond;
    int i;
    if (!f || f->kind != S_FIELD || !(f->mods & M_WITHEVENTS) || !f->body || !f->body->n) return 0;
    ifn = lit_node(N_IF, 0, at);
    ifn->cls = EC_NONE;
    ifn->type = 0;
    cond = lit_node(N_BINARY, 0, at);
    cond->cls = EC_NONE;
    cond->type = 0;
    cond->op = K_IsNot;
    cond->a = target;
    cond->b = lit_node(N_NOTHING, 0, at);
    cond->b->cls = EC_NONE;
    cond->b->type = 0;
    ifn->a = cond;
    ifn->list = nv_new();
    for (i = 0; i < f->body->n; i++) {
        Node *h = f->body->v[i];
        nv_push(ifn->list, handler_stmt(kind, target, h->name, h->text, h->sym, at));
    }
    bind_stmt(ifn);
    return ifn;
}

/* ------------------------------------------------------------ statements */

static void bind_stmt(Node *s) {
    int i;
    switch (s->kind) {
    case N_LOCAL: bind_local(s); return;
    case N_ASSIGN: bind_assign(s); return;
    case N_CALLSTMT: bind_call_stmt(s); return;
    case N_IF:
        s->a = bool_cond(s->a);
        bind_block(s->list);
        if (s->list2)
            for (i = 0; i < s->list2->n; i++) {
                Node *ei = s->list2->v[i];
                ei->a = bool_cond(ei->a);
                bind_block(ei->list);
            }
        if (s->list3) bind_block(s->list3);
        return;
    case N_SELECT: bind_select(s); return;
    case N_WHILE:
        s->a = bool_cond(s->a);
        ctl_push(K_While, s);
        bind_block(s->list);
        nctl--;
        return;
    case N_DO:
        if (s->a) s->a = bool_cond(s->a);
        ctl_push(K_Do, s);
        bind_block(s->list);
        nctl--;
        return;
    case N_FOR: bind_for(s); return;
    case N_FOREACH: bind_foreach(s); return;
    case N_EXIT: bind_exit(s, 0); return;
    case N_CONTINUE: bind_exit(s, 1); return;
    case N_RETURN: bind_return(s); return;
    case N_GOTO: add_jump(&gotos, &ngotos, &cap_gotos, s); return;
    case N_LABEL: {
        Name *n = label_name(s);
        for (i = 0; i < nlabels; i++)
            if (labels[i].name == n) serr(s, "Label '%s' is already defined in the current method.", s->text);
        add_jump(&labels, &nlabels, &cap_labels, s);
        return;
    }
    case N_STOP: case N_END: return;
    case N_REDIM: bind_redim(s); return;
    case N_ERASE: bind_erase(s); return;
    case N_TRY: bind_try(s); return;
    case N_THROW: bind_throw(s); return;
    case N_WITH: bind_with(s); return;
    case N_USING: bind_using(s); return;
    case N_SYNCLOCK: bind_synclock(s); return;
    case N_BLOCK: bind_block(s->list); return;
    case N_ADDHANDLER: case N_REMOVEHANDLER: bind_handler(s); return;
    case N_RAISEEVENT: bind_raiseevent(s); return;
    }
    serr(s, "Statement is not supported.");
}

static void bind_block(NodeVec *v) {
    Scope *save = cur_scope;
    int i;
    if (!v) return;
    cur_scope = new_scope(SC_BLOCK, cur_scope, cur_method);
    cur_scope->materialized = 1;
    v->scope = cur_scope;
    for (i = 0; i < v->n; i++) bind_stmt(v->v[i]);
    cur_scope = save;
}
