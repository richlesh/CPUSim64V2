/*
 * lambda.c - lambda expressions.
 *
 * A lambda becomes a class (the closure type) that is written as an AST and
 * analysed like any other class: one field per capture, and a const member
 * function operator() with the body.  A lambda with auto parameters gets a
 * member function template.  The object is built with the values of the
 * captures.  Inside the body a captured name is the field; a name of the
 * enclosing function that is not captured is an error.
 */

typedef struct CapEntry CapEntry;
struct CapEntry {
    Name *name;
    int by_ref;
    Node *init;              /* an init-capture's initializer (0: the variable of the same name) */
    int is_this;
    Val *val;                /* the value that initializes the field */
    Type *type;
};

typedef struct LamCache LamCache;
struct LamCache {
    Node *n;
    Ent *fn;
    Ent *cls;
    LamCache *next;
};
static LamCache *lam_cache = 0;
static int lambda_count = 0;

typedef struct CapScan CapScan;
struct CapScan {
    Ent *vars[64];
    int nvars;
    int uses_this;
    Name *skip[32];                       /* the parameters of the lambda */
    int nskip;
};

static int is_outer_local(Ent *e) {
    return e && e->kind == EK_VAR && e->scope && e->scope->kind == SS_BLOCK && !(e->flags & EF_STATIC)
        && !(e->has_val && ty_is_integral(ty_unqual(ty_strip_ref(e->type))) && !ty_is_ref(e->type));
}

static void scan_captures(Node *n, CapScan *cs);

static void scan_list(Node *n, CapScan *cs) {
    for (; n; n = n->next) scan_captures(n, cs);
}

static void scan_captures(Node *n, CapScan *cs) {
    if (n->kind == N_THIS) cs->uses_this = 1;
    if (n->kind == N_ID && n->a && !n->a->next && (n->a->flags & 7) == PART_NAME && n->a->name && !(n->flags & ID_GLOBAL)) {
        int i, skipped = 0;
        Ent *e;
        for (i = 0; i < cs->nskip; i++)
            if (cs->skip[i] == n->a->name) skipped = 1;
        if (!skipped) {
            e = lookup_unqual(scope2_cur, n->a->name);
            if (is_outer_local(e)) {
                for (i = 0; i < cs->nvars; i++)
                    if (cs->vars[i] == e) break;
                if (i == cs->nvars && cs->nvars < 64) cs->vars[cs->nvars++] = e;
            } else if (e && e->kind == EK_FIELD && in_lambda_class() && e->owner == cur_class) {
                /* a captured variable of the enclosing lambda: the inner lambda captures it too */
                for (i = 0; i < cs->nvars; i++)
                    if (cs->vars[i] == e) break;
                if (i == cs->nvars && cs->nvars < 64) cs->vars[cs->nvars++] = e;
            } else if (e && (e->kind == EK_FIELD || (e->kind == EK_FUNC && (e->flags & EF_MEMBER) && !(e->flags & EF_STATIC))) && have_this()) cs->uses_this = 1;
        }
    }
    if (n->kind == N_MEMBER) {
        scan_captures(n->a, cs);
        return;                            /* the member name is not a variable */
    }
    if (n->kind == N_D_NAME) return;
    scan_list(n->a, cs);
    scan_list(n->b, cs);
    scan_list(n->c, cs);
    scan_list(n->d, cs);
}

/* the return type of the lambda as the specifiers and the declarator chain of operator() */
static Node *lambda_declarator(Node *lam, Node *funcdecl, int *auto_params) {
    Node *p, *params = 0, *tail = 0;
    int k = 0;
    for (p = lam->b; p; p = p->next) {
        Node *np = clone_node(p);
        if (np->a && np->a->val == BT_AUTO && !np->a->a) {
            /* a generic lambda: the type is a template parameter */
            Node *ts = clone_node(np->a), *id = mk_idn(intern_str(fmt_str("__Auto%d", k++)), p);
            ts->val = 0;
            ts->a = id;
            np->a = ts;
            (*auto_params)++;
        }
        if (tail) tail->next = np;
        else params = np;
        tail = np;
    }
    funcdecl->b = params;
    return funcdecl;
}

static Val *lambda_expr(Node *n) {
    CapEntry caps[80];
    int ncaps = 0, i;
    CapScan cs;
    LamCache *lc;
    Ent *cls = 0;
    Node *cap;
    int default_ref = 0, default_val = 0, this_explicit = 0;
    Val *vals[80];
    Type *rt;
    memset(&cs, 0, sizeof cs);
    for (lc = lam_cache; lc; lc = lc->next)
        if (lc->n == n && lc->fn == cur_fn) cls = lc->cls;
    for (cap = n->a; cap; cap = cap->next) {
        if (!cap->name && cap->op == P_ASSIGN) default_val = 1;
        else if (!cap->name && cap->op == P_AMP) default_ref = 1;
    }
    {
        Node *p;
        for (p = n->b; p; p = p->next) {
            Node *idn = declarator_name_id_of(p->b);
            if (idn && idn->a && idn->a->name && cs.nskip < 32) cs.skip[cs.nskip++] = idn->a->name;
        }
    }
    scan_captures(n->d, &cs);
    /* the explicit captures first, in order */
    for (cap = n->a; cap; cap = cap->next) {
        CapEntry *ce;
        if (!cap->name) continue;
        if (ncaps >= 80) serr(n, "too many captures");
        ce = &caps[ncaps++];
        memset(ce, 0, sizeof *ce);
        if (cap->name->s[0] == 't' && strcmp(cap->name->s, "this") == 0) {
            ce->is_this = 1;
            this_explicit = 1;
            ce->name = intern_str("__this");
            continue;
        }
        ce->name = cap->name;
        ce->by_ref = cap->op == P_AMP;
        ce->init = cap->a;
    }
    /* the variables that the body uses and that the default captures take */
    if (default_val || default_ref) {
        for (i = 0; i < cs.nvars; i++) {
            int j, have = 0;
            for (j = 0; j < ncaps; j++)
                if (caps[j].name == cs.vars[i]->name && !caps[j].is_this) have = 1;
            if (have) continue;
            if (ncaps >= 80) serr(n, "too many captures");
            memset(&caps[ncaps], 0, sizeof(CapEntry));
            caps[ncaps].name = cs.vars[i]->name;
            caps[ncaps].by_ref = default_ref;
            ncaps++;
        }
        if (cs.uses_this && !this_explicit && have_this()) {
            if (ncaps >= 80) serr(n, "too many captures");
            memset(&caps[ncaps], 0, sizeof(CapEntry));
            caps[ncaps].is_this = 1;
            caps[ncaps].name = intern_str("__this");
            ncaps++;
        }
    }
    /* the values and types of the fields */
    for (i = 0; i < ncaps; i++) {
        CapEntry *ce = &caps[i];
        Val *v;
        if (ce->is_this) {
            if (!have_this()) serr(n, "'this' cannot be captured here");
            v = val_new(ty_ptr(this_class_type()), cur_this_text());
            ce->type = v->t;
            ce->val = v;
            continue;
        }
        if (ce->init) {
            v = expr(ce->init);
            if (ce->by_ref) {
                if (!v->lval) serr(n, "an init-capture by reference needs an lvalue");
                ce->type = ty_ref(v->t);
            } else ce->type = deduce_auto(auto_type(), v, n);
        } else {
            Node *id = mk_idn(ce->name, n);
            Ent *e = lookup_unqual(scope2_cur, ce->name);
            if (!e || (e->kind != EK_VAR && e->kind != EK_FIELD)) serr(n, "'%s' cannot be captured", ce->name->s);
            v = expr(id);
            if (ce->by_ref) ce->type = ty_ref(v->t);
            else ce->type = ty_unqual(v->t);
            if (v->t->cv & CV_CONST) ce->type = ty_cv(ce->type, CV_CONST);
        }
        ce->val = v;
    }
    for (i = 0; i < ncaps; i++) vals[i] = caps[i].val;
    if (!cls) {
        Scope2 *ls, *saved = scope2_cur;
        Node *members = 0, *mtail = 0, *cn, *fd, *dfn, *dn, *idn, *part, *spec;
        int auto_params = 0;
        InstState st;
        Name *cname = intern_str(fmt_str("__lambda%d", lambda_count++));
        Node *tparams = 0, *ttail = 0;
        ls = tscope_new(scope2_cur);
        for (i = 0; i < ncaps; i++) {
            Ent *te = ent_new(EK_TYPEDEF, intern_str(fmt_str("__CT%d", i)), caps[i].type, ls);
            Node *d = mk_node(N_DECL, n), *ts = mk_node(N_TYPE, n), *idl = mk_node(N_INITDECL, n), *dnm = mk_node(N_D_NAME, n);
            scope_add(ls, te);
            ts->a = mk_idn(te->name, n);
            dnm->a = mk_idn(caps[i].name, n);
            idl->a = dnm;
            d->a = ts;
            d->b = idl;
            if (mtail) mtail->next = d;
            else members = d;
            mtail = d;
        }
        /* R operator()(params) const { body } */
        fd = mk_node(N_FUNCDEF, n);
        dfn = mk_node(N_D_FUNC, n);
        if (!(n->flags & LF_MUTABLE)) dfn->flags |= FQ_CONST;
        lambda_declarator(n, dfn, &auto_params);
        dn = mk_node(N_D_NAME, n);
        idn = mk_node(N_ID, n);
        part = mk_node(N_PART, n);
        part->flags = PART_OPERATOR;
        part->op = P_LPAREN;
        part->text = "()";
        idn->a = part;
        dn->a = idn;
        dfn->a = dn;
        spec = mk_node(N_TYPE, n);
        if (n->c) {
            Node *rd;
            spec = clone_node(n->c->a);
            /* a return type with a declarator ( -> int * ): the declarator chain ends at the function declarator */
            if (n->c->b) {
                Node *c0 = 0, *cl = 0, *x;
                for (x = n->c->b; x && x->kind != N_D_NAME; x = x->a) {
                    Node *cc = clone_node(x);
                    if (cl) cl->a = cc;
                    else c0 = cc;
                    cl = cc;
                }
                if (cl) {
                    cl->a = dfn;
                    rd = c0;
                } else rd = dfn;
                fd->b = rd;
            } else fd->b = dfn;
        } else {
            spec->val = BT_AUTO;
            fd->b = dfn;
        }
        fd->a = spec;
        fd->d = n->d;
        if (auto_params) {
            Node *tp, *tm;
            int k;
            for (k = 0; k < auto_params; k++) {
                tp = mk_node(N_TPARAM, n);
                tp->name = intern_str(fmt_str("__Auto%d", k));
                tp->op = OPKW + KW_class;
                if (ttail) ttail->next = tp;
                else tparams = tp;
                ttail = tp;
            }
            tm = mk_node(N_TEMPLATE, n);
            tm->a = tparams;
            tm->b = fd;
            fd = tm;
        }
        if (mtail) mtail->next = fd;
        else members = fd;
        if (ncaps == 0 && !auto_params) {
            /* static auto __invoke(params) { return __lambdaN()(params); }: the target of the conversion to a function pointer */
            Node *inv = mk_node(N_FUNCDEF, n), *idf = mk_node(N_D_FUNC, n), *ispec = mk_node(N_TYPE, n), *p, *plist = 0, *ptail = 0;
            Node *args = 0, *atail = 0, *fc, *call, *ret, *blk, *tid;
            int k = 0;
            for (p = n->b; p; p = p->next, k++) {
                Node *np = clone_node(p);
                Name *an = intern_str(fmt_str("__a%d", k));
                Node *c0 = 0, *cl = 0, *x, *aid;
                for (x = p->b; x && x->kind != N_D_NAME; x = x->a) {
                    Node *cc = clone_node(x);
                    if (cl) cl->a = cc;
                    else c0 = cc;
                    cl = cc;
                }
                {
                    Node *nm = mk_node(N_D_NAME, n);
                    nm->a = mk_idn(an, n);
                    if (cl) cl->a = nm;
                    else c0 = nm;
                }
                np->b = c0;
                np->c = 0;
                if (ptail) ptail->next = np;
                else plist = np;
                ptail = np;
                aid = mk_idn(an, n);
                if (atail) atail->next = aid;
                else args = aid;
                atail = aid;
            }
            idf->b = plist;
            {
                Node *nm = mk_node(N_D_NAME, n);
                nm->a = mk_idn(intern_str("__invoke"), n);
                idf->a = nm;
            }
            ispec->val = BT_AUTO;
            ispec->flags = DS_STATIC;
            inv->a = ispec;
            inv->b = idf;
            tid = mk_node(N_TYPEID, n);
            tid->a = mk_node(N_TYPE, n);
            tid->a->a = mk_idn(cname, n);
            fc = mk_node(N_FUNCCAST, n);
            fc->b = tid;
            call = mk_node(N_CALL, n);
            call->a = fc;
            call->b = args;
            ret = mk_node(N_RETURN, n);
            ret->a = call;
            blk = mk_node(N_BLOCK, n);
            blk->a = ret;
            inv->d = blk;
            {
                Node *last = members;
                while (last && last->next) last = last->next;
                if (last) last->next = inv;
                else members = inv;
            }
        }
        cn = mk_node(N_CLASS, n);
        cn->op = OPKW + KW_struct;
        cn->a = mk_idn(cname, n);
        cn->c = members;
        cn->flags = CL_DEFINITION;
        inst_begin(&st);
        scope2_cur = ls;
        lambda_outer_next = 0;
        for (i = 0; i < ncaps; i++)
            if (caps[i].is_this) lambda_outer_next = cur_this_class();
        lambda_depth++;
        cls = declare_class_node(cn);
        lambda_depth--;
        lambda_outer_next = 0;
        inst_end(&st);
        scope2_cur = saved;
        lc = anew(sizeof(LamCache));
        lc->n = n;
        lc->fn = cur_fn;
        lc->cls = cls;
        lc->next = lam_cache;
        lam_cache = lc;
        cls->ci->is_lambda = 1;
    }
    {
        Val bv;
        const char *dest, *text;
        Val *r;
        rt = cls->type;
        memset(&bv, 0, sizeof bv);
        bv.braced = n;
        bv.bvals = vals;
        bv.nbvals = ncaps;
        dest = prvalue_dest(rt, n);
        text = list_init_text(rt, dest, &bv, n);
        r = val_new(rt, fmt_str("(*%s)", text));
        r->is_temp = 1;
        return r;
    }
}

/* the static function that a captureless lambda converts to when it is converted to the function pointer type 'to' */
static Ent *lambda_invoker(Type *from, Type *to) {
    ClassInfo *ci;
    Ent *inv;
    if (from->kind != TY_CLASS || to->kind != TY_PTR || to->base->kind != TY_FUNC) return 0;
    ci = from->ent->ci;
    if (!ci || !ci->is_lambda || !ci->complete) return 0;
    inv = lookup_in(ci->scope, intern_str("__invoke"), 0);
    if (!inv || inv->kind != EK_FUNC || !ty_equal(inv->type, to->base)) return 0;
    return inv;
}
