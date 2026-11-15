/*
 * sema_generic.c - generic types and methods (spec 4.9), by
 * monomorphization: a generic declaration is a template that is never
 * analyzed itself.  Each use with type arguments makes an instance from a
 * copy of the template's syntax tree, in which the type parameters are
 * names of the argument types (members of the instance type, or the
 * tparams scope of a method instance).  Instances are cached, and they go
 * through the same passes as the other types (advance() in sema_main.c).
 */

static void advance(Sym *t, int p);

/* ------------------------------------------------------------ copying trees */

static Node *copy_tree(Node *n);

static NodeVec *copy_vec(NodeVec *v) {
    NodeVec *c;
    int i;
    if (!v) return 0;
    c = nv_new();
    for (i = 0; i < v->n; i++) nv_push(c, copy_tree(v->v[i]));
    return c;
}

static Node *copy_tree(Node *n) {
    Node *c;
    if (!n) return 0;
    c = xalloc(sizeof(Node));
    *c = *n;
    c->type = 0;
    c->sym = 0;
    c->cls = EC_NONE;
    c->parent = 0;
    c->a = copy_tree(n->a);
    c->b = copy_tree(n->b);
    c->c = copy_tree(n->c);
    c->d = copy_tree(n->d);
    c->list = copy_vec(n->list);
    c->list2 = copy_vec(n->list2);
    c->list3 = copy_vec(n->list3);
    return c;
}

/* ------------------------------------------------------------ the analysis context */

typedef struct {
    Sym *ns, *module, *method, *type;
    Scope *scope, *tpscope;
    Opts *opts;
    int nctl, catch_depth, nwith;
    Node *ctor_first;
} Ctx;

static void save_ctx(Ctx *c) {
    c->ns = cur_ns;
    c->module = cur_module;
    c->method = cur_method;
    c->type = cur_type;
    c->scope = cur_scope;
    c->tpscope = cur_tpscope;
    c->opts = opts;
    c->nctl = nctl;
    c->catch_depth = catch_depth;
    c->nwith = nwith;
    c->ctor_first = ctor_first_stmt;
}

static void restore_ctx(Ctx *c) {
    cur_ns = c->ns;
    cur_module = c->module;
    cur_method = c->method;
    cur_type = c->type;
    cur_scope = c->scope;
    cur_tpscope = c->tpscope;
    opts = c->opts;
    nctl = c->nctl;
    catch_depth = c->catch_depth;
    nwith = c->nwith;
    ctor_first_stmt = c->ctor_first;
}

/* ------------------------------------------------------------ names */

static int arity(Sym *s) {
    return (s->sflags & SF_GENERIC) && s->decl->list ? s->decl->list->n : 0;
}

/* "Stack(Of T)" or "Max(Of T, U)" */
static const char *template_name(Sym *s) {
    StrBuf b;
    int i;
    sb_init(&b);
    sb_puts(&b, s->spelling);
    sb_puts(&b, "(Of ");
    for (i = 0; i < s->decl->list->n; i++) {
        if (i) sb_puts(&b, ", ");
        sb_puts(&b, s->decl->list->v[i]->text);
    }
    sb_putc(&b, ')');
    return b.s;
}

static const char *instance_name(const char *base, Type **args, int n) {
    StrBuf b;
    int i;
    sb_init(&b);
    sb_puts(&b, base);
    sb_puts(&b, "(Of ");
    for (i = 0; i < n; i++) {
        if (i) sb_puts(&b, ", ");
        sb_puts(&b, type_name(args[i]));
    }
    sb_putc(&b, ')');
    return b.s;
}

/* ------------------------------------------------------------ templates */

/* a generic type: its name in the container (several arities may share it) */
static void declare_type_template(Node *d, Sym *container) {
    Sym *s = new_sym(S_TYPE, d->name, d->text), *old = scope_find(container->members, d->name);
    Type *t = xalloc(sizeof(Type));
    s->decl = d;
    s->owner = container;
    s->mods = d->mods | M_SHARED;
    s->file = d->file;
    s->sflags |= SF_GENERIC;
    s->members = new_scope(SC_TYPE, 0, s);
    t->kind = d->kind == N_STRUCTURE ? TY_STRUCT : d->kind == N_INTERFACE ? TY_INTERFACE : TY_CLASS;
    if (d->kind == N_DELEGATE) s->sflags |= SF_DELEGATE;
    t->name = s->spelling;
    t->sym = s;
    s->type = t;
    d->sym = s;
    if (d->mods & M_PARTIAL) not_yet(d, "Partial generic types are");
    if (old) {
        Sym *o;
        if (old->kind != S_TYPE)
            serr(d, "'%s' is already declared as %s in this %s.", d->text, member_kind_text(old),
                 container->kind == S_NAMESPACE ? "namespace" : container->kind == S_MODULE ? "module" : "type");
        for (o = old; o; o = o->overload)
            if (arity(o) == arity(s)) serr(d, "'%s' is already declared in this %s.", d->text,
                                           container->kind == S_NAMESPACE ? "namespace" : "type");
        for (o = old; o->overload; o = o->overload) ;
        o->overload = s;
        return;
    }
    scope_add(container->members, s);
}

/* the type parameters as names of the argument types */
static Scope *tparam_scope(Sym *owner, NodeVec *tps, Type **args, Scope *into) {
    Scope *sc = into ? into : new_scope(SC_BLOCK, 0, owner);
    int i;
    for (i = 0; i < tps->n; i++) {
        Node *tp = tps->v[i];
        Sym *p = new_sym(S_TYPE, tp->name, tp->text);
        p->type = args[i];
        p->owner = owner;
        p->mods = M_PUBLIC | M_SHARED;
        p->decl = tp;
        p->file = tp->file;
        scope_add(sc, p);
    }
    return sc;
}

static int same_targs(Sym *inst, Type **args, int n) {
    int i;
    if (inst->ntargs != n) return 0;
    for (i = 0; i < n; i++) if (inst->targs[i] != args[i]) return 0;
    return 1;
}

static int has_default_ctor(Type *t) {
    Sym *c;
    int i;
    if (t->kind == TY_STRUCT || ty_numeric(t) || t->kind == TY_BOOLEAN || t->kind == TY_CHAR || t->kind == TY_ENUM) return 1;
    if (t->kind != TY_CLASS || (t->sym->mods & M_MUSTINHERIT)) return 0;
    for (c = t->sym->ctors; c; c = c->overload) {
        int req = 0;
        if (!(c->mods & M_PUBLIC) && (c->mods & (M_PRIVATE | M_PROTECTED | M_FRIEND))) continue;
        for (i = 0; i < c->params.n; i++) if (!(c->params.v[i]->mods & (M_OPTIONAL | M_PARAMARRAY))) req++;
        if (!req) return 1;
    }
    return !t->sym->ctors;
}

static int is_value_type(Type *t) {
    return !ty_ref(t) && t->kind != TY_NOTHING;
}

/* a primitive type stands for the generic interfaces it implements in .NET */
static int prim_implements(Type *a, Type *c) {
    Sym *ct = c->sym;
    const char *n;
    if (!ct || !ct->tmpl || ty_user(a) || a->kind == TY_ARRAY || a->kind == TY_OBJECT) return 0;
    n = ct->tmpl->name->key;
    if (strcmp(n, "icomparable") && strcmp(n, "iequatable")) return 0;
    return ct->ntargs == 1 && ct->targs[0] == a;
}

static int prim_implements_plain(Type *a, Type *c) {
    if (!c->sym || ty_user(a) || a->kind == TY_ARRAY || a->kind == TY_OBJECT || a->kind == TY_NOTHING) return 0;
    return c->sym->name == intern_word("icomparable") && !c->sym->tmpl && c->sym->owner == ns_path("System");
}

/* the constraints of the type parameters (in the instance's context) */
static void check_constraints(Sym *gen, NodeVec *tps, Type **args, Node *at) {
    int i, k;
    for (i = 0; i < tps->n; i++) {
        Node *tp = tps->v[i];
        Type *a = args[i];
        if (!tp->list) continue;
        for (k = 0; k < tp->list->n; k++) {
            Node *c = tp->list->v[k];
            if (c->kind == N_CONSTRAINT) {
                if (c->op == K_Class && !ty_ref(a))
                    serr(at, "Type argument '%s' does not satisfy the 'Class' constraint for type parameter '%s'.",
                         type_name(a), tp->text);
                if (c->op == K_Structure && !is_value_type(a))
                    serr(at, "Type argument '%s' does not satisfy the 'Structure' constraint for type parameter '%s'.",
                         type_name(a), tp->text);
                if (c->op == K_New && !has_default_ctor(a))
                    serr(at, "Type argument '%s' must have a public parameterless instance constructor to satisfy the "
                             "'New' constraint for type parameter '%s'.", type_name(a), tp->text);
            } else {
                Type *ct = bind_type(copy_tree(c));     /* the template's nodes stay unbound */
                int cv = conv_class(a, ct);
                if (cv != CV_IDENT && cv != CV_WIDEN && !prim_implements(a, ct) && !prim_implements_plain(a, ct))
                    serr(at, "Type argument '%s' does not inherit from or implement the constraint type '%s'.",
                         type_name(a), type_name(ct));
            }
        }
    }
    (void)gen;
}

/* ------------------------------------------------------------ type instances */

static Sym *instantiate_type(Sym *tmpl, Type **args, int n, Node *at) {
    Ctx ctx;
    Node *copy;
    Sym *inst;
    Type **targs;
    int i, first, last;
    for (i = 0; i < tmpl->insts.n; i++)
        if (same_targs(tmpl->insts.v[i], args, n)) return tmpl->insts.v[i];
    save_ctx(&ctx);
    copy = copy_tree(tmpl->decl);
    inst_decl = copy;
    first = prog_types.n;
    declare_type(copy, tmpl->owner);
    inst = copy->sym;
    targs = xalloc(sizeof(Type *) * (n + 1));
    for (i = 0; i < n; i++) targs[i] = args[i];
    inst->tmpl = tmpl;
    inst->targs = targs;
    inst->ntargs = n;
    inst->spelling = instance_name(tmpl->spelling, args, n);
    inst->type->name = inst->spelling;
    tparam_scope(inst, copy->list, args, inst->members);
    sv_push(&tmpl->insts, inst);
    enter(inst);
    check_constraints(inst, copy->list, args, at);
    /* the passes done for the other types (the rest follows with them),
       for the instance and the types nested in it */
    last = prog_types.n;
    for (i = first; i < last; i++) advance(prog_types.v[i], sema_phase - 1);
    restore_ctx(&ctx);
    return inst;
}

/* the type for a name with (or without) type arguments: s is a type found
   by name, possibly the first of several generic arities */
static Sym *generic_select(Sym *s, NodeVec *targs, Node *at) {
    int n = targs ? targs->n : 0, i;
    Sym *o, *found = 0;
    Type **args;
    for (o = s; o; o = o->overload)
        if (o->kind == S_TYPE && arity(o) == n) found = o;
    if (!found) {
        Sym *g = 0;
        for (o = s; o; o = o->overload) if (o->sflags & SF_GENERIC) g = o;
        if (!g) serr(at, "'%s' has no type parameters and so cannot have type arguments.", s->spelling);
        if (n < arity(g)) serr(at, "Too few type arguments to '%s'.", template_name(g));
        serr(at, "Too many type arguments to '%s'.", template_name(g));
    }
    if (!(found->sflags & SF_GENERIC)) return found;
    args = xalloc(sizeof(Type *) * (n + 1));
    for (i = 0; i < n; i++) {
        Node *a = targs->v[i];
        if (a->kind == N_TOPEN) serr(at, "Type expected.");
        args[i] = a->type ? a->type : bind_type(a);
    }
    return instantiate_type(found, args, n, at);
}

/* ------------------------------------------------------------ method instances */

static Sym *instantiate_method(Sym *tm, Type **args, int n, Node *at) {
    Ctx ctx;
    Node *copy;
    Sym *inst;
    int i;
    for (i = 0; i < tm->insts.n; i++)
        if (same_targs(tm->insts.v[i], args, n)) return tm->insts.v[i];
    save_ctx(&ctx);
    copy = copy_tree(tm->decl);
    copy->list = 0;
    inst = new_sym(S_METHOD, tm->name, tm->spelling);
    inst->decl = copy;
    inst->owner = tm->owner;
    inst->mods = tm->mods;
    inst->file = tm->file;
    inst->sflags = tm->sflags & ~SF_GENERIC;
    inst->vslot = -1;
    inst->tmpl = tm;
    inst->targs = xalloc(sizeof(Type *) * (n + 1));
    for (i = 0; i < n; i++) inst->targs[i] = args[i];
    inst->ntargs = n;
    inst->tparams = tparam_scope(inst, tm->decl->list, args, 0);
    copy->sym = inst;
    sv_push(&tm->insts, inst);
    sv_push(&tm->owner->methods, inst);
    enter(inst);
    check_constraints(inst, tm->decl->list, args, at);
    bind_signature(inst);
    restore_ctx(&ctx);
    return inst;
}

/* ------------------------------------------------------------ type inference */

typedef struct {
    NodeVec *tps;
    Type **cand[16];
    int ncand[16];
} Infer;

static int tparam_index(Infer *in, Node *tn) {
    int i;
    if (tn->kind != N_TNAME || tn->list->n != 1 || tn->list->v[0]->list || (tn->flags & F_GLOBAL)) return -1;
    for (i = 0; i < in->tps->n; i++)
        if (in->tps->v[i]->name == tn->list->v[0]->name) return i;
    return -1;
}

static void add_cand(Infer *in, int k, Type *t) {
    int i;
    for (i = 0; i < in->ncand[k]; i++) if (in->cand[k][i] == t) return;
    if (in->ncand[k] < 16) in->cand[k][in->ncand[k]++] = t;
}

static int mentions_tparam(Infer *in, Node *tn) {
    int i;
    if (!tn) return 0;
    if (tparam_index(in, tn) >= 0) return 1;
    if (tn->kind == N_TARRAY || tn->kind == N_TNULLABLE) return mentions_tparam(in, tn->a);
    if (tn->kind == N_TNAME)
        for (i = 0; i < tn->list->n; i++) {
            NodeVec *ta = tn->list->v[i]->list;
            int k;
            if (ta) for (k = 0; k < ta->n; k++) if (mentions_tparam(in, ta->v[k])) return 1;
        }
    return 0;
}

static Sym *bind_lambda(Node *e, Sym *inv, Type **ptypes, Type *dtype);     /* sema_lambda.c */
static int lambda_shape(Node *tn, Node **pnodes, int *np, Node **rnode);
static int tentative;

/* matches a parameter's type syntax against an argument's type */
static void unify(Infer *in, Node *tn, Type *at) {
    int k;
    if (!tn || !at || at->kind == TY_NOTHING) return;
    k = tparam_index(in, tn);
    if (k >= 0) {
        add_cand(in, k, at);
        return;
    }
    if (tn->kind == N_TARRAY) {
        if (at->kind == TY_ARRAY && at->rank == (int)tn->ival) unify(in, tn->a, at->elem);
        return;
    }
    if (tn->kind == N_TNAME) {
        Node *last = tn->list->v[tn->list->n - 1];
        Sym *s = at->sym;
        int i;
        if (last->list && at->kind == TY_ARRAY && at->rank == 1 && last->list->n == 1 &&
            last->name == intern_word("ienumerable")) {
            unify(in, last->list->v[0], at->elem);      /* an array is an IEnumerable(Of T) */
            return;
        }
        if (!last->list || !s) return;
        /* an instance of the generic type named here (or of its base classes and interfaces) */
        for (; s; s = s->base) {
            if (s->tmpl && s->tmpl->name == last->name && s->ntargs == last->list->n) {
                for (i = 0; i < s->ntargs; i++) unify(in, last->list->v[i], s->targs[i]);
                return;
            }
            for (i = 0; i < s->ifaces.n; i++) {
                Sym *it = s->ifaces.v[i];
                if (it->tmpl && it->tmpl->name == last->name && it->ntargs == last->list->n) {
                    int j;
                    for (j = 0; j < it->ntargs; j++) unify(in, last->list->v[j], it->targs[j]);
                    return;
                }
            }
        }
    }
}

/* an argument for parameter pn (expanded: one element of a ParamArray) */
static void unify_param(Infer *in, Node *pn, Type *at, int expanded) {
    int k;
    if (pn->list2) {
        for (k = expanded ? 1 : 0; k < pn->list2->n; k++) {
            if (!at || at->kind != TY_ARRAY || at->rank != (int)pn->list2->v[k]->ival) return;
            at = at->elem;
        }
    } else if (expanded && pn->a && pn->a->kind == N_TARRAY) {
        unify(in, pn->a->a, at);
        return;
    }
    unify(in, pn->a, at);
}

/* the type arguments of a generic method for a call, or 0 */
static int infer_targs(Sym *tm, NodeVec *args, Type **out) {
    Infer in;
    NodeVec *params = tm->decl->list2 ? tm->decl->list2 : nv_new();
    int i, k, np = params->n;
    memset(&in, 0, sizeof in);
    in.tps = tm->decl->list;
    if (in.tps->n > 16) return 0;
    for (k = 0; k < in.tps->n; k++) in.cand[k] = xalloc(sizeof(Type *) * 16);
    for (i = 0; i < args->n; i++) {
        Node *a = args->v[i];
        Node *pn = 0;
        int expanded = 0;
        if (!a->a) continue;
        if (a->name) {
            for (k = 0; k < np; k++) if (params->v[k]->name == a->name) pn = params->v[k];
        } else if (i < np) {
            pn = params->v[i];
            if (i == np - 1 && (pn->mods & M_PARAMARRAY) && a->a->type && a->a->type->kind != TY_ARRAY) expanded = 1;
        } else if (np && (params->v[np - 1]->mods & M_PARAMARRAY)) {
            pn = params->v[np - 1];
            expanded = 1;
        }
        if (pn) unify_param(&in, pn, a->a->type, expanded);
    }
    /* lambdas: their parameter types from what is known, then their result
       type (from a tentative binding of a copy) */
    {
        int round;
        for (round = 0; round < 3; round++) {
            int progress = 0;
            for (i = 0; i < args->n && i < np; i++) {
                Node *a = args->v[i], *pn = a->name ? 0 : params->v[i], *pnodes[8], *rnode;
                Type *pt[8], *rt;
                int nps, j, ok = 1;
                if (!a->a || a->a->kind != N_LAMBDA || a->a->type || !pn) continue;
                if (!lambda_shape(pn->a, pnodes, &nps, &rnode)) continue;
                if ((a->a->list2 ? a->a->list2->n : 0) != nps) continue;
                /* the lambda's own parameter types (As clauses) */
                for (j = 0; j < nps; j++) {
                    Node *lp = a->a->list2->v[j];
                    if (lp->a && !lp->list2) {
                        Type *lt = bind_type(copy_tree(lp->a));
                        if (tparam_index(&in, pnodes[j]) >= 0) {
                            if (!in.ncand[tparam_index(&in, pnodes[j])]) progress = 1;
                            add_cand(&in, tparam_index(&in, pnodes[j]), lt);
                        }
                    }
                }
                for (j = 0; j < nps && ok; j++) {
                    int k2 = tparam_index(&in, pnodes[j]);
                    if (k2 >= 0) {
                        if (!in.ncand[k2]) ok = 0;
                        else pt[j] = in.cand[k2][0];
                    } else if (mentions_tparam(&in, pnodes[j])) ok = 0;
                    else {
                        /* a type of the method's declaration: in its context */
                        Ctx c;
                        save_ctx(&c);
                        enter(tm);
                        pt[j] = bind_type(copy_tree(pnodes[j]));
                        restore_ctx(&c);
                    }
                }
                if (!ok || !rnode) continue;
                if (tparam_index(&in, rnode) >= 0 && in.ncand[tparam_index(&in, rnode)]) continue;
                {
                    Node *copy = copy_tree(a->a);
                    Sym *l;
                    tentative++;
                    l = bind_lambda(copy, 0, pt, 0);
                    tentative--;
                    rt = l->type;
                }
                if (rt) {
                    unify(&in, rnode, rt);
                    progress = 1;
                }
            }
            if (!progress) break;
        }
    }
    for (k = 0; k < in.tps->n; k++) {
        Type *best = 0;
        int c, j;
        if (!in.ncand[k]) return 0;
        for (c = 0; c < in.ncand[k] && !best; c++) {
            int ok = 1;
            for (j = 0; j < in.ncand[k]; j++) {
                int cv = conv_class(in.cand[k][j], in.cand[k][c]);
                if (cv != CV_IDENT && cv != CV_WIDEN) ok = 0;
            }
            if (ok) best = in.cand[k][c];
        }
        if (!best) return 0;
        out[k] = best;
    }
    return 1;
}

/* the candidates with the generic templates replaced by their instances */
static void instantiate_cands(SymVec *all, NodeVec *args, Node *at) {
    int i, k = 0, ngen = 0;
    NodeVec *explicit_ = call_targs;
    Sym *last_gen = 0;
    const char *first = all->n ? all->v[0]->spelling : "";
    call_targs = 0;
    for (i = 0; i < all->n; i++) {
        Sym *m = all->v[i];
        if (!(m->sflags & SF_GENERIC)) {
            if (!explicit_) all->v[k++] = m;
            continue;
        }
        ngen++;
        last_gen = m;
        {
            int n = arity(m), j;
            Type **ta = xalloc(sizeof(Type *) * (n + 1));
            if (explicit_) {
                if (explicit_->n != n) continue;
                for (j = 0; j < n; j++) {
                    if (explicit_->v[j]->kind == N_TOPEN) serr(at, "Type expected.");
                    ta[j] = explicit_->v[j]->type ? explicit_->v[j]->type : bind_type(explicit_->v[j]);
                }
            } else if (!infer_targs(m, args, ta)) continue;
            all->v[k++] = instantiate_method(m, ta, n, at);
        }
    }
    all->n = k;
    if (k == 0 && ngen) {
        if (explicit_) serr(at, "Too %s type arguments to '%s'.", explicit_->n < arity(last_gen) ? "few" : "many",
                            template_name(last_gen));
        serr(at, "Data type(s) of the type parameter(s) in method '%s' cannot be inferred from these arguments. "
                 "Specifying the data type(s) explicitly might correct this error.", template_name(last_gen));
    }
    if (k == 0 && explicit_) serr(at, "'%s' has no type parameters and so cannot have type arguments.", first);
}
