/*
 * sema_query.c - query expressions (spec 11.21): lowered to calls of the
 * System.Linq.Enumerable methods (lib/Linq.vb) with lambdas, then bound.
 *
 * The range variables in scope are kept in a Vars list.  With one range
 * variable x, the elements of the sequence are its values and the lambdas
 * take a parameter x.  With several, the elements are objects of an
 * anonymous type with a member per variable, the lambdas take a parameter
 * $it, and the names of the variables in the clause expressions are
 * rewritten to $it.name.
 */

#define MAX_VARS 32

typedef struct {
    Name *names[MAX_VARS];
    const char *texts[MAX_VARS];
    int n;
} Vars;

static Name *w_it, *w_group_param;

/* ------------------------------------------------------------ unbound syntax */

static Node *syn(int kind, Node *at) {
    Node *n = xalloc(sizeof(Node));
    n->kind = kind;
    n->file = at->file;
    n->line = at->line;
    n->col = at->col;
    return n;
}

static Node *syn_name(Name *nm, const char *text, Node *at) {
    Node *n = syn(N_NAME, at);
    n->name = nm;
    n->text = text;
    return n;
}

static Node *syn_member(Node *a, const char *name, Node *at) {
    Node *n = syn(N_MEMBER, at);
    n->a = a;
    n->name = intern_word(name);
    n->text = name;
    return n;
}

static Node *syn_arg(Node *x) {
    Node *a = syn(N_ARG, x);
    a->a = x;
    return a;
}

/* recv.method(args...) */
static Node *syn_call(Node *recv, const char *method, Node *at, Node *a1, Node *a2, Node *a3, Node *a4) {
    Node *n = syn(N_INVOKE, at);
    n->a = syn_member(recv, method, at);
    n->list = nv_new();
    if (a1) nv_push(n->list, syn_arg(a1));
    if (a2) nv_push(n->list, syn_arg(a2));
    if (a3) nv_push(n->list, syn_arg(a3));
    if (a4) nv_push(n->list, syn_arg(a4));
    return n;
}

static int var_index(Vars *v, Name *n) {
    int i;
    for (i = 0; i < v->n; i++) if (v->names[i] == n) return i;
    return -1;
}

static void var_add(Vars *v, Name *n, const char *text, Node *at) {
    if (var_index(v, n) >= 0) serr(at, "Range variable '%s' is already declared.", text);
    if (v->n == MAX_VARS) serr(at, "Too many range variables.");
    v->names[v->n] = n;
    v->texts[v->n] = text;
    v->n++;
}

/* the names of the range variables in e become $it.name */
static void rewrite_vars(Node *e, Vars *v) {
    int i;
    if (!e) return;
    if (e->kind == N_NAME && !e->list && var_index(v, e->name) >= 0) {
        Node *it = syn_name(w_it, "$it", e);
        e->kind = N_MEMBER;
        e->a = it;
        return;
    }
    if (e->kind == N_QUERY) return;     /* a nested query has its own range variables */
    rewrite_vars(e->a, v);
    rewrite_vars(e->b, v);
    rewrite_vars(e->c, v);
    rewrite_vars(e->d, v);
    if (e->list) for (i = 0; i < e->list->n; i++) rewrite_vars(e->list->v[i], v);
    if (e->list2) for (i = 0; i < e->list2->n; i++) rewrite_vars(e->list2->v[i], v);
    if (e->list3) for (i = 0; i < e->list3->n; i++) rewrite_vars(e->list3->v[i], v);
}

static Node *syn_param(Name *n, const char *text, Node *at) {
    Node *p = syn(N_PARAM, at);
    p->name = n;
    p->text = text;
    return p;
}

/* the parameter for an element with the range variables v */
static Node *elem_param(Vars *v, Node *at) {
    if (v->n == 1) return syn_param(v->names[0], v->texts[0], at);
    return syn_param(w_it, "$it", at);
}

/* Function(elem[, p2]) body, where body refers to the range variables v
   (and to p2) */
static Node *lambda_over(Vars *v, Node *body, Name *p2, const char *p2text, Node *at) {
    Node *l = syn(N_LAMBDA, at), *b = copy_tree(body);
    l->op = K_Function;
    l->list2 = nv_new();
    nv_push(l->list2, elem_param(v, at));
    if (p2) nv_push(l->list2, syn_param(p2, p2text, at));
    if (v->n > 1) rewrite_vars(b, v);
    l->b = b;
    return l;
}

/* New With {.a = a, ...} for the range variables v, and more members */
static Node *syn_anon(Node *at) {
    Node *n = syn(N_ANONNEW, at), *init = syn(N_OBJINIT, at);
    init->list = nv_new();
    n->b = init;
    return n;
}

static void anon_add(Node *anon, Name *n, const char *text, Node *value, int key) {
    Node *fi = syn(N_FIELDINIT, value);
    fi->name = n;
    fi->text = text;
    fi->a = value;
    if (key) fi->flags |= F_KEY;
    nv_push(anon->b->list, fi);
}

static void anon_add_vars(Node *anon, Vars *v, Node *at) {
    int i;
    for (i = 0; i < v->n; i++) anon_add(anon, v->names[i], v->texts[i], syn_name(v->names[i], v->texts[i], at), 0);
}

/* the name a Select or Group By item gets without "name =" */
static int inferred_name(Node *e, Name **n, const char **text) {
    if (e->kind == N_NAME || e->kind == N_MEMBER) {
        *n = e->name;
        *text = e->text;
        return 1;
    }
    return 0;
}

static void item_name(Node *item, Name **n, const char **text) {
    if (item->name) {
        *n = item->name;
        *text = item->text;
        return;
    }
    if (!inferred_name(item->b, n, text))
        serr(item->b, "Range variable name can be inferred only from a simple or qualified name with no arguments.");
}

/* an element expression of several named items: an anonymous type; of one
   item: its value */
static Node *items_value(NodeVec *items, Vars *nv, Node *at) {
    int i;
    nv->n = 0;
    if (items->n == 1) {
        Name *n;
        const char *t;
        if (items->v[0]->name || inferred_name(items->v[0]->b, &n, &t)) {
            item_name(items->v[0], &n, &t);
            var_add(nv, n, t, items->v[0]);
        } else var_add(nv, intern("$value"), "$value", at);
        return items->v[0]->b;
    }
    {
        Node *anon = syn_anon(at);
        for (i = 0; i < items->n; i++) {
            Name *n;
            const char *t;
            item_name(items->v[i], &n, &t);
            var_add(nv, n, t, items->v[i]);
            anon_add(anon, n, t, items->v[i]->b, 0);
        }
        return anon;
    }
}

/* ------------------------------------------------------------ Into */

/* an Into item over the group coll (whose elements have the range
   variables ev): Group, or an aggregate function Count(), Sum(e), ... */
static Node *aggregate_item(Node *coll, Node *item, Vars *ev, Name **name, const char **text) {
    Node *e = item->b;
    if (e->kind == N_NAME && e->name == intern_word("group") && !e->list) {
        *name = item->name ? item->name : intern_word("group");
        *text = item->name ? item->text : "Group";
        return copy_tree(coll);
    }
    if (e->kind == N_INVOKE && e->a->kind == N_NAME) {
        const char *fn = e->a->text;
        Node *call = 0;
        *name = item->name ? item->name : e->a->name;
        *text = item->name ? item->text : fn;
        if (!e->list || e->list->n == 0) call = syn_call(copy_tree(coll), fn, e, 0, 0, 0, 0);
        else if (e->list->n == 1 && e->list->v[0]->a)
            call = syn_call(copy_tree(coll), fn, e, lambda_over(ev, e->list->v[0]->a, 0, 0, e), 0, 0, 0);
        else serr(e, "Aggregate function '%s' takes at most one argument.", fn);
        return call;
    }
    serr(e, "'Group' or an aggregate function (Count, Sum, Min, Max, Average, ...) expected.");
    return 0;
}

/* ------------------------------------------------------------ the operators */

typedef struct {
    Node *src;
    Vars v;
} Q;

static void q_from(Q *q, Node *r, int first) {
    Node *coll = r->b;
    if (r->a) {
        /* From x As T In c: the elements converted to T */
        Node *cast = syn(N_INVOKE, r), *m = syn_member(coll, "Cast", r);
        m->list = nv_new();
        nv_push(m->list, r->a);
        cast->a = m;
        cast->list = nv_new();
        coll = cast;
    }
    if (first) {
        q->src = coll;
        q->v.n = 0;
        var_add(&q->v, r->name, r->text, r);
        return;
    }
    {
        /* SelectMany(Function(old) coll, Function(old, y) New With {old..., y}) */
        Node *anon = syn_anon(r);
        Vars nv = q->v;
        anon_add_vars(anon, &q->v, r);
        anon_add(anon, r->name, r->text, syn_name(r->name, r->text, r), 0);
        q->src = syn_call(q->src, "SelectMany", r, lambda_over(&q->v, coll, 0, 0, r),
                          lambda_over(&q->v, anon, r->name, r->text, r), 0, 0);
        var_add(&nv, r->name, r->text, r);
        q->v = nv;
    }
}

/* the key of a Join: one condition, or an anonymous type of Key members */
static Node *join_key(NodeVec *conds, int inner, Node *at) {
    int i;
    Node *anon;
    if (conds->n == 1) return inner ? conds->v[0]->b : conds->v[0]->a;
    anon = syn_anon(at);
    for (i = 0; i < conds->n; i++) {
        char buf[16];
        snprintf(buf, sizeof buf, "Key%d", i);
        anon_add(anon, intern_word(buf), xstrdup(buf), inner ? conds->v[i]->b : conds->v[i]->a, 1);
    }
    return anon;
}

static void q_join(Q *q, Node *op, int group) {
    Node *r = op->a, *res;
    Vars iv;
    Vars nv = q->v;
    if (op->list && op->list->n) not_yet(op, "Nested 'Join' clauses are");
    iv.n = 0;
    var_add(&iv, r->name, r->text, r);
    if (!group) {
        res = syn_anon(op);
        anon_add_vars(res, &q->v, op);
        anon_add(res, r->name, r->text, syn_name(r->name, r->text, op), 0);
        q->src = syn_call(q->src, "Join", op, r->b, lambda_over(&q->v, join_key(op->list2, 0, op), 0, 0, op),
                          lambda_over(&iv, join_key(op->list2, 1, op), 0, 0, op),
                          lambda_over(&q->v, res, r->name, r->text, op));
        var_add(&nv, r->name, r->text, r);
    } else {
        /* GroupJoin(..., Function(old, $group) New With {old..., into items}) */
        int i;
        Node *coll = syn_name(w_group_param, "$group", op);
        res = syn_anon(op);
        anon_add_vars(res, &q->v, op);
        for (i = 0; i < op->list3->n; i++) {
            Name *n;
            const char *t;
            Node *x = aggregate_item(coll, op->list3->v[i], &iv, &n, &t);
            anon_add(res, n, t, x, 0);
            var_add(&nv, n, t, op->list3->v[i]);
        }
        q->src = syn_call(q->src, "GroupJoin", op, r->b, lambda_over(&q->v, join_key(op->list2, 0, op), 0, 0, op),
                          lambda_over(&iv, join_key(op->list2, 1, op), 0, 0, op),
                          lambda_over(&q->v, res, w_group_param, "$group", op));
    }
    q->v = nv;
}

static void q_group_by(Q *q, Node *op) {
    Vars ev, kv, nv;
    Node *key, *elem = 0, *res, *g = syn_name(w_group_param, "$group", op);
    int i;
    /* the elements: Group items By ..., else the range variables */
    if (op->list && op->list->n) elem = items_value(op->list, &ev, op);
    else ev = q->v;
    /* the key: one item, or an anonymous type of Key members */
    kv.n = 0;
    if (op->list2->n == 1) {
        Name *n;
        const char *t;
        item_name(op->list2->v[0], &n, &t);
        var_add(&kv, n, t, op->list2->v[0]);
        key = op->list2->v[0]->b;
    } else {
        key = syn_anon(op);
        for (i = 0; i < op->list2->n; i++) {
            Name *n;
            const char *t;
            item_name(op->list2->v[i], &n, &t);
            var_add(&kv, n, t, op->list2->v[i]);
            anon_add(key, n, t, op->list2->v[i]->b, 1);
        }
    }
    if (elem) q->src = syn_call(q->src, "GroupBy", op, lambda_over(&q->v, key, 0, 0, op), lambda_over(&q->v, elem, 0, 0, op), 0, 0);
    else q->src = syn_call(q->src, "GroupBy", op, lambda_over(&q->v, key, 0, 0, op), 0, 0, 0);
    /* Select(Function($group) New With {keys..., into items...}) */
    res = syn_anon(op);
    nv.n = 0;
    for (i = 0; i < kv.n; i++) {
        Node *k = syn_member(syn_name(w_group_param, "$group", op), "Key", op);
        if (kv.n > 1) k = syn_member(k, kv.texts[i], op);
        anon_add(res, kv.names[i], kv.texts[i], k, 0);
        var_add(&nv, kv.names[i], kv.texts[i], op);
    }
    for (i = 0; i < op->list3->n; i++) {
        Name *n;
        const char *t;
        Node *x = aggregate_item(g, op->list3->v[i], &ev, &n, &t);
        anon_add(res, n, t, x, 0);
        var_add(&nv, n, t, op->list3->v[i]);
    }
    {
        Node *l = syn(N_LAMBDA, op);
        l->op = K_Function;
        l->list2 = nv_new();
        nv_push(l->list2, syn_param(w_group_param, "$group", op));
        l->b = res;
        q->src = syn_call(q->src, "Select", op, l, 0, 0, 0);
    }
    q->v = nv;
}

static void q_op(Q *q, Node *op) {
    const char *k = op->text;
    int i;
    if (strcmp(k, "From") == 0) {
        for (i = 0; i < op->list->n; i++) q_from(q, op->list->v[i], 0);
    } else if (strcmp(k, "Where") == 0) {
        q->src = syn_call(q->src, "Where", op, lambda_over(&q->v, op->b, 0, 0, op), 0, 0, 0);
    } else if (strcmp(k, "Select") == 0) {
        Vars nv;
        Node *val = items_value(op->list, &nv, op);
        q->src = syn_call(q->src, "Select", op, lambda_over(&q->v, val, 0, 0, op), 0, 0, 0);
        q->v = nv;
    } else if (strcmp(k, "Let") == 0) {
        for (i = 0; i < op->list->n; i++) {
            Node *it = op->list->v[i], *anon = syn_anon(op);
            Vars nv = q->v;
            anon_add_vars(anon, &q->v, op);
            anon_add(anon, it->name, it->text, it->b, 0);
            q->src = syn_call(q->src, "Select", op, lambda_over(&q->v, anon, 0, 0, op), 0, 0, 0);
            var_add(&nv, it->name, it->text, it);
            q->v = nv;
        }
    } else if (strcmp(k, "Order By") == 0) {
        for (i = 0; i < op->list->n; i++) {
            Node *key = op->list->v[i];
            const char *m = i == 0 ? ((key->flags & F_DESC) ? "OrderByDescending" : "OrderBy")
                                   : ((key->flags & F_DESC) ? "ThenByDescending" : "ThenBy");
            q->src = syn_call(q->src, m, key, lambda_over(&q->v, key->a, 0, 0, key), 0, 0, 0);
        }
    } else if (strcmp(k, "Distinct") == 0) {
        q->src = syn_call(q->src, "Distinct", op, 0, 0, 0, 0);
    } else if (strcmp(k, "Take") == 0 || strcmp(k, "Skip") == 0) {
        q->src = syn_call(q->src, k, op, copy_tree(op->b), 0, 0, 0);
    } else if (strcmp(k, "Take While") == 0 || strcmp(k, "Skip While") == 0) {
        q->src = syn_call(q->src, k[0] == 'T' ? "TakeWhile" : "SkipWhile", op, lambda_over(&q->v, op->b, 0, 0, op), 0, 0, 0);
    } else if (strcmp(k, "Join") == 0) {
        q_join(q, op, 0);
    } else if (strcmp(k, "Group Join") == 0) {
        q_join(q, op, 1);
    } else if (strcmp(k, "Group By") == 0) {
        q_group_by(q, op);
    } else if (strcmp(k, "Aggregate") == 0) {
        not_yet(op, "'Aggregate' after the first clause of a query is");
    } else serr(op, "Query operator '%s' is not supported.", k);
}

/* the query as an unbound expression */
static Node *lower_query(Node *e) {
    Q q;
    Node *first = e->list->v[0];
    int i;
    if (!w_it) {
        w_it = intern("$it");
        w_group_param = intern("$group");
    }
    memset(&q, 0, sizeof q);
    for (i = 0; i < first->list->n; i++) q_from(&q, first->list->v[i], i == 0);
    if (strcmp(first->text, "Aggregate") == 0) {
        /* Aggregate x In c [operators] Into items: a value (one item) or an
           anonymous type of the values */
        Node *res;
        for (i = 0; first->list2 && i < first->list2->n; i++) q_op(&q, first->list2->v[i]);
        if (first->list3->n == 1) {
            Name *n;
            const char *t;
            res = aggregate_item(q.src, first->list3->v[0], &q.v, &n, &t);
        } else {
            res = syn_anon(first);
            for (i = 0; i < first->list3->n; i++) {
                Name *n;
                const char *t;
                Node *x = aggregate_item(q.src, first->list3->v[i], &q.v, &n, &t);
                anon_add(res, n, t, x, 0);
            }
        }
        for (i = 1; i < e->list->n; i++) q_op(&q, e->list->v[i]);
        if (e->list->n > 1) not_yet(e->list->v[1], "Query operators after 'Aggregate ... Into' are");
        return res;
    }
    for (i = 1; i < e->list->n; i++) q_op(&q, e->list->v[i]);
    return q.src;
}

static Node *bind_query(Node *e, int flags) {
    /* the source must be a collection (bound here, used as it is) */
    Node *r = e->list->v[0]->list->v[0], *src = rvalue(r->b);
    Type *t = src->type;
    if (t->kind != TY_ARRAY && t->kind != TY_OBJECT && !(ty_user(t) && t->kind != TY_ENUM))
        serr(r->b, "Expression of type '%s' is not queryable. Make sure you are not missing an assembly reference "
                   "and/or namespace import for the LINQ provider.", type_name(t));
    r->b = src;
    return bind(lower_query(e), flags);
}
