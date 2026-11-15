/*
 * braced.c - braced-init-lists as values and std::initializer_list.
 *
 * A braced list that is not the initializer of a declaration (a function
 * argument, a returned value, the right side of an assignment) has no type
 * of its own: expr() returns a Val with the list's elements (already
 * analysed) and the target type decides what the list means: an aggregate,
 * a constructor call, a std::initializer_list or a scalar.  All the code is
 * written as C expressions, so it can be used inside any expression: the
 * temporaries are destroyed at the end of the full expression.
 */

static Node *clone_node(Node *n) {
    Node *c = anew(sizeof(Node));
    *c = *n;
    c->next = 0;
    return c;
}

static Name *nm_initializer_list = 0;
static Node empty_braced_node;

static Type *inst_arg_type(Ent *cls, int i);

static Val *braced_val(Node *n) {
    Val *v = val_new(T_VOID, "");
    Val *tmp[MAXARGS];
    int cnt, k;
    cnt = expr_args(n->a, tmp, 0, MAXARGS);
    v->braced = n;
    v->bvals = anew(sizeof(Val *) * (cnt + 1));
    for (k = 0; k < cnt; k++) v->bvals[k] = tmp[k];
    v->nbvals = cnt;
    return v;
}

static int is_init_list_type(Type *t) {
    Ent *te;
    t = ty_unqual(t);
    if (t->kind != TY_CLASS) return 0;
    te = inst_template(t->ent);
    if (!te) return 0;
    if (!nm_initializer_list) nm_initializer_list = intern_str("initializer_list");
    return te->name == nm_initializer_list;
}

static Type *init_list_elem(Type *t) {
    return inst_arg_type(ty_unqual(t)->ent, 0);
}

/* the constructor of the class that takes a std::initializer_list */
static Ent *find_il_ctor(Type *cls, Type **ilt) {
    Ent *f, *set;
    ClassInfo *ci = cls->ent->ci;
    if (!ci || !ci->complete) return 0;
    set = sfind(ci->scope, nm_ctor);
    for (f = set; f; f = f->overload) {
        Type *p;
        if (f->kind != EK_FUNC || f->type->nparams < 1) continue;
        p = ty_unqual(ty_strip_ref(f->type->params[0]));
        if (is_init_list_type(p) && func_min_args(f) <= 1 && f->type->nparams == 1) {
            if (ilt) *ilt = p;
            return f;
        }
    }
    return 0;
}

static int il_viable(Type *ilt, Val **vals, int n) {
    Type *e = init_list_elem(ilt);
    int i;
    for (i = 0; i < n; i++)
        if (arg_conversion(vals[i], e) == CONV_NONE) return 0;
    return 1;
}

static const char *list_init_text(Type *cls, const char *dest, Val *bv, Node *at);
static Val *braced_materialize(Val *bv, Type *to, Node *at);

/* the C expression that initializes the object (or array) at dest from the value v; its value is not used */
static const char *init_from_val_text(Type *t, const char *dest, Val *v, Node *at) {
    t = ty_unqual(t);
    if (t->kind == TY_ARRAY) {
        StrBuf sb;
        i64 i;
        if (!v->braced) serr(at, "an array must be initialized with a braced list");
        if (t->len >= 0 && v->nbvals > t->len) serr(at, "too many initializers for '%s'", ty_str(t));
        sb_init(&sb);
        sb_puts(&sb, "(");
        for (i = 0; i < (t->len >= 0 ? t->len : v->nbvals); i++) {
            const char *ed = fmt_str("(&(*%s)[%lld])", dest, i);
            if (i < v->nbvals) sb_puts(&sb, init_from_val_text(t->base, ed, v->bvals[i], at));
            else if (t->base->kind == TY_CLASS) {
                const char *d = default_construct_text(t->base, ed, at);
                sb_puts(&sb, d ? d : "0");
            } else sb_printf(&sb, "(*%s = 0)", ed);
            sb_puts(&sb, ", ");
        }
        sb_puts(&sb, "0)");
        return sb.s;
    }
    if (v->braced) {
        if (t->kind == TY_CLASS) return list_init_text(t, dest, v, at);
        if (v->nbvals == 0) return fmt_str("(*(%s) = 0)", dest);
        if (v->nbvals > 1) serr(at, "too many initializers for '%s'", ty_str(t));
        return init_from_val_text(t, dest, v->bvals[0], at);
    }
    if (ty_is_ref(t)) return fmt_str("(*(%s) = %s)", dest, bind_argument(v, t, at));
    if (t->kind == TY_CLASS) {
        const char *text;
        if (v->t->kind == TY_CLASS && ty_same(v->t, t)) text = copy_into(t, dest, v, at);
        else if (v->t->kind != TY_CLASS) {
            Val *args[1];
            args[0] = v;
            if (class_conversion(v, t) == CONV_NONE) serr(at, "cannot convert '%s' to '%s'", ty_str(v->t), ty_str(t));
            text = ctor_call(t, dest, args, 1, at);
        } else {
            Val *cv = class_prvalue(v, t, at);
            text = copy_into(t, dest, cv, at);
        }
        return text ? text : dest;
    }
    return fmt_str("(*(%s) = %s)", dest, convert(v, t, at)->c);
}

/* a destructor call for a part of a temporary (an element of an array): at the end of the full expression */
static void register_part_temp(const char *name, Type *cls) {
    if (!cinfo(cls)->needs_dtor) return;
    if (nftemps >= MAX_FTEMPS) serr(0, "too many temporaries in one expression");
    ftemps[nftemps].name = name;
    ftemps[nftemps].cls = cls;
    ftemps[nftemps].guard = 0;
    nftemps++;
}

/* the C expression that constructs a std::initializer_list at dest from the elements; the backing array is a temporary */
static const char *il_dest_text(Type *ilt, Val **vals, int n, const char *dest, Node *at) {
    Type *e = init_list_elem(ilt);
    Type *arrt = ty_array(e, n > 0 ? n : 1);
    const char *arr = new_temp(arrt);
    StrBuf sb;
    Val *args[2];
    const char *ctor;
    int i;
    sb_init(&sb);
    sb_puts(&sb, "(");
    for (i = 0; i < n; i++) {
        sb_puts(&sb, init_from_val_text(e, fmt_str("(&%s[%d])", arr, i), vals[i], at));
        sb_puts(&sb, ", ");
        if (ty_unqual(e)->kind == TY_CLASS) register_part_temp(fmt_str("%s[%d]", arr, i), ty_unqual(e));
    }
    args[0] = val_new(ty_ptr(ty_cv(ty_unqual(e), CV_CONST)), fmt_str("((%s)%s)", c_decl(ty_ptr(ty_unqual(e)), ""), arr));
    args[1] = const_val(T_ULONG, n);
    ctor = ctor_call(ty_unqual(ilt), dest, args, 2, at);
    sb_puts(&sb, ctor ? ctor : dest);
    sb_puts(&sb, ")");
    return sb.s;
}

/* the aggregate's fields from the values */
static const char *aggregate_vals_text(Type *cls, const char *dest, Val **vals, int n, Node *at) {
    ClassInfo *ci = cinfo(cls);
    Field *f;
    int i = 0;
    StrBuf sb;
    sb_init(&sb);
    sb_puts(&sb, "(");
    for (f = ci->fields; f; f = f->next, i++) {
        const char *fd = fmt_str("(&(*%s).%s)", dest, f->name->s);
        if (i < n && f->type->kind == TY_ARRAY && !vals[i]->braced && !(vals[i]->t->kind == TY_ARRAY)) {
            /* brace elision: the next initializers are the elements of the array */
            Val ev;
            i64 len = f->type->len, k;
            memset(&ev, 0, sizeof ev);
            ev.braced = &empty_braced_node;
            ev.bvals = vals + i;
            for (k = 0; i + k < n && (len < 0 || k < len); k++) ev.nbvals++;
            sb_puts(&sb, init_from_val_text(f->type, fd, &ev, at));
            i += (int)ev.nbvals - 1;
        } else if (i < n) sb_puts(&sb, init_from_val_text(f->type, fd, vals[i], at));
        else if (f->init) sb_puts(&sb, init_from_val_text(f->type, fd, f->init->kind == N_INITLIST ? braced_val(f->init) : expr(f->init), at));
        else if (f->type->kind == TY_CLASS) {
            const char *d = default_construct_text(f->type, fd, at);
            sb_puts(&sb, d ? d : "0");
        } else if (f->type->kind == TY_ARRAY) {
            Val *empty = val_new(T_VOID, "");
            empty->braced = &empty_braced_node;
            sb_puts(&sb, init_from_val_text(f->type, fd, empty, at));
        } else if (ty_is_ref(f->type)) serr(at, "the reference member '%s' must be initialized", f->name->s);
        else sb_printf(&sb, "(*%s = %s)", fd, ty_is_float(f->type) ? "0.0" : (f->type->kind == TY_PTR ? "((void *)0)" : "0"));
        sb_puts(&sb, ", ");
    }
    if (i < n) serr(at, "too many initializers for '%s'", ty_str(cls));
    sb_puts(&sb, dest);
    sb_puts(&sb, ")");
    return sb.s;
}

/* the C expression (of the value dest) that list-initializes the object at dest of class cls from the braced list bv */
static const char *list_init_text(Type *cls, const char *dest, Val *bv, Node *at) {
    ClassInfo *ci;
    Type *ilt = 0;
    Ent *ilc;
    require_complete(cls, at);
    ci = cinfo(cls);
    if (is_init_list_type(cls)) return il_dest_text(cls, bv->bvals, bv->nbvals, dest, at);
    ilc = bv->nbvals > 0 ? find_il_ctor(cls, &ilt) : 0;
    if (ilc && il_viable(ilt, bv->bvals, bv->nbvals)) {
        const char *d2 = prvalue_dest(ilt, 0);
        const char *t2 = il_dest_text(ilt, bv->bvals, bv->nbvals, d2, at);
        Val *a[1];
        a[0] = val_new(ilt, fmt_str("(*%s)", t2));
        a[0]->is_temp = 1;
        return ctor_text(ilc, dest, a, 1, at);
    }
    if (ci->aggregate && !class_ctors(cls)) return aggregate_vals_text(cls, dest, bv->bvals, bv->nbvals, at);
    {
        const char *t = ctor_call(cls, dest, bv->bvals, bv->nbvals, at);
        return t ? t : dest;
    }
}

static int braced_conversion(Val *bv, Type *p) {
    Type *tb = p;
    int i;
    if (ty_is_ref(p)) {
        tb = p->base;
        if (p->kind == TY_REF && !(tb->cv & CV_CONST)) return CONV_NONE;
    }
    tb = ty_unqual(tb);
    if (tb->kind == TY_CLASS) {
        ClassInfo *ci = tb->ent->ci;
        Type *ilt = 0;
        if (!ci || !ci->complete) return CONV_NONE;
        if (is_init_list_type(tb)) return il_viable(tb, bv->bvals, bv->nbvals) ? CONV_USER : CONV_NONE;
        if (bv->nbvals > 0 && find_il_ctor(tb, &ilt) && il_viable(ilt, bv->bvals, bv->nbvals)) return CONV_USER;
        if (ci->aggregate && !class_ctors(tb)) {
            Field *f;
            int nf = 0;
            for (f = ci->fields; f; f = f->next) nf++;
            return bv->nbvals <= nf ? CONV_USER : CONV_NONE;
        }
        {
            Ent *set = class_ctors(tb);
            int amb;
            if (!set) return bv->nbvals == 0 ? CONV_USER : CONV_NONE;
            if (bv->nbvals == 0) return (cinfo(tb)->implicit_ctor || resolve_overload(set, 0, 0, &amb)) ? CONV_USER : CONV_NONE;
            return resolve_overload(set, bv->bvals, bv->nbvals, &amb) ? CONV_USER : CONV_NONE;
        }
    }
    if (tb->kind == TY_ARRAY) return CONV_NONE;
    if (bv->nbvals == 0) return CONV_EXACT;
    if (bv->nbvals != 1) return CONV_NONE;
    for (i = 0; i < 1; i++)
        if (bv->bvals[i]->braced) return CONV_NONE;
    return arg_conversion(bv->bvals[0], tb);
}

/* the value that the braced list has when it initializes an object of type 'to' (a class: a temporary) */
static Val *braced_materialize(Val *bv, Type *to, Node *at) {
    Type *tb = ty_unqual(ty_strip_ref(to));
    if (tb->kind == TY_CLASS) {
        const char *dest = prvalue_dest(tb, 0);
        const char *text = list_init_text(tb, dest, bv, at);
        Val *r = val_new(tb, fmt_str("(*%s)", text));
        r->is_temp = 1;
        return r;
    }
    if (bv->nbvals == 0) return tb->kind == TY_PTR ? val_new(tb, "((void *)0)") : (ty_is_float(tb) ? val_new(tb, "0.0") : const_val(tb, 0));
    if (bv->nbvals > 1) serr(at, "too many initializers for '%s'", ty_str(tb));
    return convert(bv->bvals[0], tb, at);
}

/* ------------------------------------------------------------ range-for over a class */

static Node *mk_node(int kind, Node *at) {
    Node *n = anew(sizeof(Node));
    n->kind = kind;
    n->file = at->file;
    n->line = at->line;
    return n;
}

static Node *mk_idn(Name *nm, Node *at) {
    Node *id = mk_node(N_ID, at), *p = mk_node(N_PART, at);
    p->name = nm;
    id->a = p;
    return id;
}

/* auto name = init; */
static Node *mk_auto_decl(Name *nm, Node *init, Node *at) {
    Node *d = mk_node(N_DECL, at), *ts = mk_node(N_TYPE, at), *idl = mk_node(N_INITDECL, at), *dn = mk_node(N_D_NAME, at);
    Node *st = mk_node(N_DECL_STMT, at);
    ts->val = BT_AUTO;
    dn->a = mk_idn(nm, at);
    idl->a = dn;
    idl->b = init;
    d->a = ts;
    d->b = idl;
    st->a = d;
    return st;
}

/* for (D x : range) body  with a class range: auto __b = range.begin(); auto __e = range.end(); for (; __b != __e; ++__b) { D x = *__b; body } */
static void range_for_class(Node *n, Val *rng) {
    int k = label_count++;
    Name *rn = intern_str(fmt_str("__range%d", k)), *bn = intern_str(fmt_str("__begin%d", k)), *en = intern_str(fmt_str("__end%d", k));
    Ent *re;
    Node *call, *mem, *bg, *loop, *decl, *dnode, *body, *blk;
    Type *rt = ty_unqual(rng->t);
    if (!class_member(rt, intern_str("begin")) || !class_member(rt, intern_str("end")))
        serr(n, "the range of the loop (type '%s') has no begin() and end()", ty_str(rng->t));
    emit("{\n");
    push_scope();
    emitf("%s = &%s;\n", c_decl(ty_ptr(rng->t), fmt_str("__r%d", k)), rng->c);
    re = ent_new(EK_VAR, rn, ty_ref(rng->t), scope2_cur);
    re->cname = fmt_str("__r%d", k);
    scope_add(scope2_cur, re);
    /* auto __begin = __range.begin(); */
    mem = mk_node(N_MEMBER, n);
    mem->op = P_DOT;
    mem->a = mk_idn(rn, n);
    mem->b = mk_idn(intern_str("begin"), n);
    call = mk_node(N_CALL, n);
    call->a = mem;
    stmt(mk_auto_decl(bn, call, n));
    mem = mk_node(N_MEMBER, n);
    mem->op = P_DOT;
    mem->a = mk_idn(rn, n);
    mem->b = mk_idn(intern_str("end"), n);
    call = mk_node(N_CALL, n);
    call->a = mem;
    stmt(mk_auto_decl(en, call, n));
    /* the loop */
    loop = mk_node(N_FOR, n);
    bg = mk_node(N_BINARY, n);
    bg->op = P_NE;
    bg->a = mk_idn(bn, n);
    bg->b = mk_idn(en, n);
    loop->b = bg;
    bg = mk_node(N_UNARY, n);
    bg->op = P_INC;
    bg->a = mk_idn(bn, n);
    loop->c = bg;
    /* the body: D x = *__begin; original body */
    decl = anew(sizeof(Node));
    *decl = *n->a;
    decl->next = 0;
    dnode = anew(sizeof(Node));
    *dnode = *n->a->b;
    dnode->next = 0;
    bg = mk_node(N_UNARY, n);
    bg->op = P_STAR;
    bg->a = mk_idn(bn, n);
    dnode->b = bg;
    decl->b = dnode;
    blk = mk_node(N_DECL_STMT, n);
    blk->a = decl;
    body = anew(sizeof(Node));
    *body = *n->d;
    body->next = 0;
    blk->next = body;
    loop->d = mk_node(N_BLOCK, n);
    loop->d->a = blk;
    stmt(loop);
    pop_scope();
    emit("}\n");
}

/* using Base::Base; : a constructor of the derived class for each constructor of the base class that forwards its arguments */
static int inherit_constructors(Ent *cls, Node *m, int access) {
    ClassInfo *ci = cls->ci;
    Node *id = m->a, *p0, *lp;
    Ent *be, *f;
    int i, found = 0;
    if (!id || !id->a || !id->a->next) return 0;
    lp = id_last(id);
    for (p0 = id->a; p0->next && p0->next->next; p0 = p0->next) ;
    if ((lp->flags & 7) != PART_NAME || (p0->flags & 7) != PART_NAME || lp->name != p0->name) return 0;
    be = 0;
    {
        /* the class named by the prefix */
        Node tmpid = *id;
        Node *q;
        Node *copy_head = 0, *copy_tail = 0;
        for (q = id->a; q != lp; q = q->next) {
            Node *c = clone_node(q);
            if (copy_tail) copy_tail->next = c;
            else copy_head = c;
            copy_tail = c;
        }
        tmpid.a = copy_head;
        be = resolve_id(&tmpid, 0);
    }
    if (be && be->kind == EK_TYPEDEF && be->type->kind == TY_CLASS) be = be->type->ent;
    if (!be || be->kind != EK_CLASS || !be->ci) return 0;
    for (i = 0; i < ci->nbases; i++)
        if (ci->bases[i].cls == be) found = 1;
    if (!found) serr(m, "'%s' is not a direct base of '%s'", ent_qualified_name(be), ent_qualified_name(cls));
    for (f = sfind(be->ci->scope, nm_ctor); f; f = f->overload) {
        Node *plist = 0, *ptail = 0, *args = 0, *atail = 0, *fnode, *fd, *ci_node, *tn;
        Type *ft;
        Ent *e;
        int k, skip = 0;
        if (f->kind != EK_FUNC || (f->flags & EF_DELETED) || (f->flags & EF_IMPLICIT)) continue;
        if (f->type->nparams == 1 && ty_is_ref(f->type->params[0]) && ty_same(f->type->params[0]->base, be->type)) continue;   /* copy and move */
        for (k = 0; k < f->type->nparams; k++)
            if (f->type->params[k]->kind == TY_RREF) skip = 1;
        if (skip) continue;
        for (k = 0; k < f->type->nparams; k++) {
            Node *pn = mk_node(N_PARAM, m), *ts = mk_node(N_TYPE, m), *dn = mk_node(N_D_NAME, m);
            Name *an = intern_str(fmt_str("__p%d", k));
            Node *aid = mk_idn(an, m);
            ts->val = BT_INT;
            dn->a = mk_idn(an, m);
            pn->a = ts;
            pn->b = dn;
            if (ptail) ptail->next = pn;
            else plist = pn;
            ptail = pn;
            if (atail) atail->next = aid;
            else args = aid;
            atail = aid;
        }
        fnode = mk_node(N_D_FUNC, m);
        fnode->b = plist;
        ft = ty_func(T_VOID, f->type->params, f->type->nparams, f->type->variadic);
        e = ent_new(EK_FUNC, nm_ctor, ft, ci->scope);
        e->flags |= EF_MEMBER | EF_CTOR;
        e->owner = cls;
        e->access = access;
        e->cname = function_cname(ci->scope, cls, nm_ctor, EF_CTOR, ft, 0);
        e->defaults = anew(sizeof(Node *) * (ft->nparams + 1));
        e->file = m->file;
        e->line = m->line;
        if (find_member_overload(sfind(ci->scope, nm_ctor), ft, 0, EF_CTOR)) continue;      /* the class declares this constructor itself */
        scope_add(ci->scope, e);
        ci->methods = xrealloc(ci->methods, sizeof(Ent *) * (ci->nmethods + 1));
        ci->methods[ci->nmethods++] = e;
        emit_prototype(e, fnode, 0);
        fd = mk_node(N_FUNCDEF, m);
        fd->a = mk_node(N_TYPE, m);
        ci_node = mk_node(N_CTORINIT, m);
        tn = mk_idn(be->name, m);
        ci_node->a = tn;
        ci_node->b = args;
        fd->c = ci_node;
        fd->d = mk_node(N_BLOCK, m);
        {
            DeferredBody *d = anew(sizeof(DeferredBody));
            d->e = e;
            d->fd = fd;
            d->fnode = fnode;
            d->ft = ft;
            if (def_tail) def_tail->next = d;
            else def_head = d;
            def_tail = d;
        }
    }
    return 1;
}

/* new (place) T(args): the object is built at the address; no allocation */
static Val *placement_new(Node *n, Type *t) {
    Val *pv, *r;
    const char *tmp, *place;
    Node *list = n->c ? (n->c->kind == N_INITLIST ? n->c->a : n->c) : 0;
    if (n->a->next) serr(n, "placement new with several placement arguments is not supported");
    if (n->flags & 4) serr(n, "placement new of an array is not supported");
    pv = rvalue(expr(n->a));
    if (pv->t->kind != TY_PTR) serr(n, "the placement argument of new is not a pointer");
    place = pv->c;
    tmp = new_temp(ty_ptr(t));
    if (t->kind == TY_CLASS) {
        Val *args[MAXARGS];
        int na;
        const char *text;
        require_complete(t, n);
        require_concrete(t, n);
        if ((n->flags & 2) && cinfo(t)->aggregate && !class_ctors(t)) {
            Val bv;
            Val *vv[MAXARGS];
            int k;
            na = expr_args(list, vv, 0, MAXARGS);
            memset(&bv, 0, sizeof bv);
            bv.braced = n;
            bv.bvals = vv;
            bv.nbvals = na;
            text = list_init_text(ty_unqual(t), tmp, &bv, n);
            (void)k;
            return val_new(ty_ptr(t), fmt_str("(%s = (%s)%s, %s, %s)", tmp, c_decl(ty_ptr(t), ""), place, text, tmp));
        }
        na = expr_args(list, args, 0, MAXARGS);
        brace_init = (n->flags & 2) != 0;
        text = ctor_call(ty_unqual(t), tmp, args, na, n);
        brace_init = 0;
        r = val_new(ty_ptr(t), fmt_str("(%s = (%s)%s, %s, %s)", tmp, c_decl(ty_ptr(t), ""), place, text ? text : tmp, tmp));
        return r;
    }
    {
        Val *iv = 0, *sv[2];
        int sn = expr_args(list, sv, 0, 2);
        if (sn > 1) serr(n, "unsupported initializer in new-expression");
        if (sn == 1) iv = convert(sv[0], t, n);
        r = val_new(ty_ptr(t), fmt_str("(%s = (%s)%s, *%s = %s, %s)", tmp, c_decl(ty_ptr(t), ""), place, tmp, iv ? iv->c : (ty_is_float(t) ? "0.0" : "0"), tmp));
        return r;
    }
}

/* auto [a, b] = e;  a hidden variable holds e (or refers to it) and each name refers to an element */
static int binding_count = 0;

static void binding_decl(Node *b) {
    Name *hn = intern_str(fmt_str("__sb%d", binding_count++));
    Node *d = mk_node(N_DECL, b), *idl = mk_node(N_INITDECL, b), *dn = mk_node(N_D_NAME, b), *declr = dn;
    Ent *he;
    Type *ht;
    Node *nm;
    int i = 0, count = 0;
    dn->a = mk_idn(hn, b);
    if (b->op == 0 && b->b && b->b->kind != N_INITLIST) {
        /* an array is not copied: the names refer to its elements (a copy is only visible if the array changes) */
        int m = nftemps;
        Val *pv;
        unevaluated++;
        pv = expr(b->b);
        unevaluated--;
        nftemps = m;
        if (pv->t->kind == TY_ARRAY) b->op = P_AMP;
    }
    if (b->op == P_AMP || b->op == P_ANDAND) {
        Node *r = mk_node(b->op == P_AMP ? N_D_REF : N_D_RREF, b);
        r->a = dn;
        declr = r;
    }
    idl->a = declr;
    idl->b = b->b;
    if (b->b && b->b->kind == N_INITLIST) idl->flags = 1;
    d->a = b->a;
    d->b = idl;
    process_decl(d);
    he = lookup_in(scope2_cur, hn, 0);
    if (!he) serr(b, "the structured binding has no object");
    ht = ty_strip_ref(he->type);
    for (nm = b->c; nm; nm = nm->next) count++;
    if (ht->kind == TY_ARRAY) {
        if (ht->len != count) serr(b, "the structured binding has %d names but the array has %lld elements", count, ht->len);
    } else if (ht->kind == TY_CLASS) {
        ClassInfo *ci = cinfo(ht);
        Field *f;
        int nf = 0;
        if (ci->nbases) serr(b, "structured bindings of classes with base classes are not supported");
        for (f = ci->fields; f; f = f->next) nf++;
        if (nf != count) serr(b, "the structured binding has %d names but '%s' has %d members", count, ty_str(ht), nf);
    } else serr(b, "cannot decompose an object of type '%s'", ty_str(ht));
    for (nm = b->c; nm; nm = nm->next, i++) {
        /* auto &&name = __sbN.field;  (or __sbN[i]) */
        Node *ad = mk_node(N_DECL, b), *ts = mk_node(N_TYPE, b), *il = mk_node(N_INITDECL, b), *rr = mk_node(N_D_RREF, b), *nd = mk_node(N_D_NAME, b);
        Node *init;
        ts->val = BT_AUTO;
        nd->a = nm;
        rr->a = nd;
        il->a = rr;
        if (ht->kind == TY_ARRAY) {
            init = mk_node(N_INDEX, b);
            init->a = mk_idn(hn, b);
            init->b = mk_node(N_NUM, b);
            init->b->text = fmt_str("%d", i);
        } else {
            Field *f = cinfo(ht)->fields;
            int k;
            for (k = 0; k < i; k++) f = f->next;
            init = mk_node(N_MEMBER, b);
            init->op = P_DOT;
            init->a = mk_idn(hn, b);
            init->b = mk_idn(f->name, b);
        }
        il->b = init;
        ad->a = ts;
        ad->b = il;
        process_decl(ad);
    }
}
