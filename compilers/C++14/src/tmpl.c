/*
 * tmpl.c - templates.
 *
 * A template is not analysed where it is declared.  The entity of a template
 * (EK_TEMPLATE for a class or alias template, EK_FTEMPLATE for a function
 * template) keeps the AST and the scope of the definition.  An instantiation
 * analyses that AST again in a scope (SS_TMPL) that binds the template
 * parameters: a type parameter is a typedef, a non-type parameter is an
 * enumerator-like constant, a parameter pack is an EK_PACK entity with one
 * entity per element.  So the AST is never copied and a dependent name is
 * looked up when the template is instantiated.
 *
 * - Class templates are instantiated when they are named with arguments.
 *   The member functions are instantiated when they are first used (use_function),
 *   except virtual functions and destructors.
 * - Function templates are instantiated by argument deduction (deduce_function)
 *   during overload resolution, or by explicit arguments.  A substitution that
 *   fails is not an error for the call: the candidate is dropped.
 * - The code of an instance is written to a separate buffer and put before the
 *   function that is being written (pending_helpers) or at the top level.
 */

enum { TA_TYPE = 1, TA_VALUE, TA_PACK, TA_ENT };
enum { TK_CLASS = 1, TK_FUNC, TK_ALIAS, TK_VAR };

struct TArg {
    int kind;
    Type *t;                 /* TA_TYPE: the type; TA_VALUE: the type of the value */
    i64 v;
    Ent *ent;                /* TA_ENT: a function or variable whose address is the argument */
    struct TArg **elems;     /* TA_PACK */
    int n;
};
struct TInst {
    Ent *tmpl;
    TArg **args;
    int nargs;
    Ent *ent;
    TInst *next;
    Node *fnode;             /* function instance: the declarator that has the parameters */
    Type *ft;
};

typedef struct TSpec TSpec;
struct TSpec {
    Node *tnode;             /* the N_TEMPLATE */
    Node *cn;                /* the N_CLASS */
    Node *targs;             /* the argument patterns (N_TARG list) of a partial specialization */
    Scope2 *scope;
    TArg **exact;            /* an explicit specialization: its arguments */
    int nexact;
    int is_exact;
    int score;
    TSpec *next;
};

typedef struct OolDef OolDef;
struct OolDef {
    Node *tnode;
    Node *def;               /* the N_FUNCDEF or N_DECL that is defined outside its class */
    Scope2 *scope;
    OolDef *next;
};

struct Tmpl {
    int kind;
    Node *tnode;
    Node *tparams;
    int ntp;
    Node **tp;               /* the parameters */
    Node **defaults;         /* the default argument of each parameter, merged from all declarations */
    Node *classnode;         /* class template: the definition */
    Node *decl;              /* function template: the definition, or the declaration */
    Node *alias;             /* alias template: the N_ALIAS */
    Scope2 *scope;
    Ent *owner;              /* member template: the class */
    TInst *insts;
    TSpec *specs;
    OolDef *ool;
};

struct Lazy {
    Ent *ent;                /* the function (the copies of the entity get the deduced type from it) */
    Node *fd, *fnode;
    Type *ft;
    Scope2 *parent;
    int done;
};

static int tinst_count = 0, tdepth = 0;

/* the instantiations in progress, for the notes of an error */
typedef struct InstNote InstNote;
struct InstNote {
    const char *what;
    int file, line;
};
static InstNote inst_notes[160];
static int ninst_notes = 0;

static void print_inst_context(void) {
    int i;
    for (i = 0; i < ninst_notes; i++) {
        InstNote *n = &inst_notes[i];
        if (n->file >= 0 && n->file < nfiles)
            fprintf(stderr, "%s:%d: note: in instantiation of '%s' requested here\n", files[n->file].disp, n->line, n->what);
        else fprintf(stderr, "note: in instantiation of '%s'\n", n->what);
    }
}

static void note_push(const char *what, Node *at) {
    if (!err_context_hook) err_context_hook = print_inst_context;
    if (ninst_notes < 160) {
        inst_notes[ninst_notes].what = what;
        inst_notes[ninst_notes].file = at ? at->file : -1;
        inst_notes[ninst_notes].line = at ? at->line : 0;
        ninst_notes++;
    }
}

static void note_pop(void) {
    if (ninst_notes > 0) ninst_notes--;
}

static Ent *inst_template(Ent *e) {
    return e->inst ? e->inst->tmpl : 0;
}

/* the i-th template argument (a type) of a class instance */
static Type *inst_arg_type(Ent *cls, int i) {
    TArg *a;
    if (!cls->inst || i >= cls->inst->nargs) return T_INT;
    a = cls->inst->args[i];
    if (a->kind != TA_TYPE) return T_INT;
    return a->t;
}

/* ------------------------------------------------------------ state of the code generator */

typedef struct InstState InstState;
struct InstState {
    StrBuf *out, *temps;
    Ent *fn, *cls;
    Type *ret;
    Name *nrvo;
    Scope2 *scope;
    int temp_count, is_ctor, is_dtor, is_const, nftemps, cond_depth, in_cond, loop_depth, force_nv, line_file, line_no;
    int class_nest;
    const char *dest_hint;
    Node *dest_node;
    Type *dest_type;
    int dest_used;
    DeferredBody *def_tail;
    int in_fn;
    StrBuf buf, scratch;
    StrBuf pend;                 /* the helpers that were pending before the instance */
    int saved_in_fn;
    int nbuilding;
};

static void snap_save(InstState *s) {
    s->out = out;
    s->temps = temps;
    s->fn = cur_fn;
    s->cls = cur_class;
    s->ret = cur_ret;
    s->nrvo = nrvo_name;
    s->scope = scope2_cur;
    s->temp_count = temp_count;
    s->is_ctor = cur_is_ctor;
    s->is_dtor = cur_is_dtor;
    s->is_const = cur_method_const;
    s->nftemps = nftemps;
    s->cond_depth = cond_depth;
    s->in_cond = in_cond_decl;
    s->loop_depth = loop_depth;
    s->force_nv = force_nonvirtual;
    s->line_file = cur_line_file;
    s->line_no = cur_line_no;
    s->class_nest = class_nest;
    s->dest_hint = dest_hint;
    s->dest_node = dest_node;
    s->dest_type = dest_type;
    s->dest_used = dest_used;
    s->def_tail = def_tail;
}

static void snap_restore(InstState *s) {
    out = s->out;
    temps = s->temps;
    cur_fn = s->fn;
    cur_class = s->cls;
    cur_ret = s->ret;
    nrvo_name = s->nrvo;
    scope2_cur = s->scope;
    temp_count = s->temp_count;
    cur_is_ctor = s->is_ctor;
    cur_is_dtor = s->is_dtor;
    cur_method_const = s->is_const;
    nftemps = s->nftemps;
    cond_depth = s->cond_depth;
    in_cond_decl = s->in_cond;
    loop_depth = s->loop_depth;
    force_nonvirtual = s->force_nv;
    cur_line_file = s->line_file;
    cur_line_no = s->line_no;
    class_nest = s->class_nest;
    dest_hint = s->dest_hint;
    dest_node = s->dest_node;
    dest_type = s->dest_type;
    dest_used = s->dest_used;
}

static StrBuf pend_stack[64];          /* the pending helpers of each instantiation in progress (what they replaced) */
static int npend = 0;
static int cur_in_fn = 0;             /* the instance being written is inside a function (its text goes before the function) */
static int inst_active = 0;           /* instantiations in progress (their text is not written yet) */
static int running_eager = 0;
static int inst_active_count(void) {
    return inst_active;
}
static Ent **eager_list = 0;
static int neager = 0;

static TInst **inst_log = 0;            /* every instance in the order it was made */
static int ninst_log = 0, cap_inst_log = 0;

static void log_inst(TInst *in) {
    if (ninst_log == cap_inst_log) {
        cap_inst_log = cap_inst_log ? cap_inst_log * 2 : 256;
        inst_log = xrealloc(inst_log, sizeof(TInst *) * cap_inst_log);
    }
    inst_log[ninst_log++] = in;
}

static Ent **lazy_log = 0;              /* the functions whose bodies were generated, in order */
static int nlazy_log = 0, cap_lazy_log = 0;

static TInst *building[64];
static int nbuilding = 0;

/* the code of an instance goes to its own buffer: inst_end puts it before the function being written, or at the top level */
static void inst_begin(InstState *s) {
    snap_save(s);
    s->in_fn = cur_fn != 0 || cur_in_fn;
    s->saved_in_fn = cur_in_fn;
    cur_in_fn = s->in_fn;
    sb_init(&s->buf);
    sb_init(&s->scratch);
    s->pend = pending_helpers;
    if (npend < 64) pend_stack[npend++] = pending_helpers;
    inst_active++;
    sb_init(&pending_helpers);
    out = &s->buf;
    temps = &s->scratch;
    cur_fn = 0;
    cur_ret = 0;
    nrvo_name = 0;
    cond_depth = 0;
    in_cond_decl = 0;
    loop_depth = 0;
    force_nonvirtual = 0;
    dest_hint = 0;
    dest_node = 0;
    dest_type = 0;
    dest_used = 0;
    cur_line_file = -1;
}

static void inst_end(InstState *s) {
    StrBuf leftover = pending_helpers;
    if (npend > 0) npend--;
    inst_active--;       /* helpers made by the instance and not written yet */
    snap_restore(s);
    cur_in_fn = s->saved_in_fn;
    pending_helpers = s->pend;
    /* the instances that this one needed come first */
    if (leftover.len) sb_puts(&pending_helpers, leftover.s);
    if (s->buf.len) {
        if (s->in_fn) sb_puts(&pending_helpers, s->buf.s);
        else sb_puts(&out_body, s->buf.s);
    }
    cur_line_file = -1;
    if (inst_active == 0 && class_nest == 0 && neager > 0) run_eager_list();
}

/* ------------------------------------------------------------ template arguments */

static TArg *ta_type(Type *t) {
    TArg *a = anew(sizeof(TArg));
    a->kind = TA_TYPE;
    a->t = t;
    return a;
}

static TArg *ta_value(Type *t, i64 v) {
    TArg *a = anew(sizeof(TArg));
    a->kind = TA_VALUE;
    a->t = t;
    a->v = v;
    return a;
}

static TArg *ta_pack(TArg **e, int n) {
    TArg *a = anew(sizeof(TArg));
    a->kind = TA_PACK;
    a->elems = e;
    a->n = n;
    return a;
}

static int ta_equal(TArg *a, TArg *b) {
    int i;
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
    case TA_TYPE: return ty_equal(a->t, b->t);
    case TA_VALUE: return a->v == b->v;
    case TA_ENT: return a->ent->cname == b->ent->cname;
    case TA_PACK:
        if (a->n != b->n) return 0;
        for (i = 0; i < a->n; i++)
            if (!ta_equal(a->elems[i], b->elems[i])) return 0;
        return 1;
    }
    return 0;
}

static int targs_equal(TArg **a, int na, TArg **b, int nb) {
    int i;
    if (na != nb) return 0;
    for (i = 0; i < na; i++)
        if (!ta_equal(a[i], b[i])) return 0;
    return 1;
}

static void ta_spell(StrBuf *sb, TArg *a) {
    int i;
    switch (a->kind) {
    case TA_TYPE: sb_puts(sb, ty_str(a->t)); break;
    case TA_VALUE:
        if (a->t && a->t->kind == TY_BOOL) sb_puts(sb, a->v ? "true" : "false");
        else sb_printf(sb, "%lld", a->v);
        break;
    case TA_ENT: sb_puts(sb, ent_qualified_name(a->ent)); break;
    case TA_PACK:
        for (i = 0; i < a->n; i++) {
            if (i) sb_puts(sb, ", ");
            ta_spell(sb, a->elems[i]);
        }
        break;
    }
}

static const char *targs_text(TArg **a, int n) {
    StrBuf sb;
    int i;
    sb_init(&sb);
    sb_puts(&sb, "<");
    for (i = 0; i < n; i++) {
        if (i) sb_puts(&sb, ", ");
        ta_spell(&sb, a[i]);
    }
    if (sb.len > 0 && sb.s[sb.len - 1] == '>') sb_puts(&sb, " ");
    sb_puts(&sb, ">");
    return sb.s;
}

static void targ_push(TArg ***v, int *n, int *cap, TArg *a) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 8;
        *v = xrealloc(*v, sizeof(TArg *) * *cap);
    }
    (*v)[(*n)++] = a;
}

/* ------------------------------------------------------------ parameter packs */

typedef struct PackSet PackSet;
struct PackSet {
    Ent *p[8];
    int n;
};

static void walk_packs(Node *n, PackSet *ps);

static void walk_list(Node *n, PackSet *ps) {
    for (; n; n = n->next) walk_packs(n, ps);
}

static void walk_packs(Node *n, PackSet *ps) {
    if (n->kind == N_ID && n->a && !n->a->next && (n->a->flags & 7) == PART_NAME && n->a->name) {
        Ent *e = lookup_unqual(scope2_cur, n->a->name);
        if (e && e->kind == EK_PACK) {
            int i;
            for (i = 0; i < ps->n; i++)
                if (ps->p[i] == e) break;
            if (i == ps->n && ps->n < 8) ps->p[ps->n++] = e;
        }
    }
    walk_list(n->a, ps);
    walk_list(n->b, ps);
    walk_list(n->c, ps);
    walk_list(n->d, ps);
}

static int pack_count(Node *pat) {
    PackSet ps;
    int i, n;
    ps.n = 0;
    walk_packs(pat, &ps);
    if (ps.n == 0) serr(pat, "there is no parameter pack to expand");
    n = ps.p[0]->nelems;
    for (i = 1; i < ps.n; i++)
        if (ps.p[i]->nelems != n) serr(pat, "the parameter packs have different lengths");
    return n;
}

/* a scope in which every pack of the pattern names its i-th element; the caller restores scope2_cur */
static Scope2 *pack_enter(Node *pat, int i) {
    PackSet ps;
    Scope2 *sc, *saved = scope2_cur;
    int k;
    ps.n = 0;
    walk_packs(pat, &ps);
    sc = scope2_new(SS_TMPL, saved);
    for (k = 0; k < ps.n; k++) {
        Ent *c = anew(sizeof(Ent));
        *c = *ps.p[k]->elems[i];
        c->name = ps.p[k]->name;
        c->overload = 0;
        scope_add(sc, c);
    }
    scope2_cur = sc;
    return saved;
}

/* the values of a list of expressions with pack expansions (call arguments) */
static int expr_args(Node *list, Val **args, int nargs, int max) {
    Node *a;
    for (a = list; a; a = a->next) {
        if (a->kind == N_PACKEXP) {
            int cnt = pack_count(a->a), i;
            for (i = 0; i < cnt; i++) {
                Scope2 *saved = pack_enter(a->a, i);
                if (nargs >= max) serr(a, "too many arguments");
                args[nargs++] = expr(a->a);
                scope2_cur = saved;
            }
            continue;
        }
        if (nargs >= max) serr(a, "too many arguments");
        args[nargs++] = expr(a);
    }
    return nargs;
}

static Ent *pack_ent_new(Name *name, Scope2 *sc) {
    Ent *e = ent_new(EK_PACK, name, 0, sc);
    scope_add(sc, e);
    return e;
}

static void pack_append(Ent *pack, Ent *elem) {
    pack->elems = xrealloc(pack->elems, sizeof(Ent *) * (pack->nelems + 1));
    pack->elems[pack->nelems++] = elem;
}

/* a function parameter pack: the function's parameters are expanded into one parameter per element.  The
   declarator that has the new parameters is a copy; the entity of the pack is created in the current scope. */
static int declarator_has_pack(Node *d) {
    for (; d; d = d->a)
        if (d->kind == N_D_PACK) return 1;
    return 0;
}

static Node *declarator_pack_name_id(Node *d) {
    for (; d; d = d->a)
        if (d->kind == N_D_NAME) return d->a;
    return 0;
}

static int pack_param_count = 0;

static Node *expand_func_params(Node *d, Type ***ptypes, int *pn) {
    Node *p, *head = 0, *tail = 0, *nd;
    Type **types = 0;
    int n = 0, cap = 0, i;
    for (p = d->b; p; p = p->next) {
        if (declarator_has_pack(p->b)) {
            Node *idn = declarator_pack_name_id(p->b);
            Name *pname = idn && idn->a ? idn->a->name : 0;
            Ent *pe = 0;
            int cnt = pack_count(p->a);
            if (!pname) pname = intern_str(fmt_str("__pack%d", pack_param_count++));
            pe = sfind(scope2_cur, pname);
            if (!pe || pe->kind != EK_PACK) pe = pack_ent_new(pname, scope2_cur);
            pe->nelems = 0;
            for (i = 0; i < cnt; i++) {
                Scope2 *saved = pack_enter(p->a, i);
                Type *pt = decl_specs_type(p->a, 0);
                Node *sp, *tn, *dn, *idc, *part;
                pt = apply_declarator(pt, p->b, 0, 0);
                scope2_cur = saved;
                if (n == cap) {
                    cap = cap ? cap * 2 : 8;
                    types = xrealloc(types, sizeof(Type *) * cap);
                }
                types[n++] = ty_adjust_param(pt);
                sp = anew(sizeof(Node));
                *sp = *p;
                tn = anew(sizeof(Node));
                tn->kind = N_TYPE;
                tn->val = BT_INT;
                sp->a = tn;
                dn = anew(sizeof(Node));
                dn->kind = N_D_NAME;
                idc = anew(sizeof(Node));
                idc->kind = N_ID;
                part = anew(sizeof(Node));
                part->kind = N_PART;
                part->name = intern_str(fmt_str("%s__%d", pname->s, i));
                idc->a = part;
                dn->a = idc;
                sp->b = dn;
                sp->c = 0;
                sp->name = pname;
                sp->val = 1;
                sp->next = 0;
                if (tail) tail->next = sp;
                else head = sp;
                tail = sp;
            }
        } else {
            Node *sp = anew(sizeof(Node));
            Type *pt = decl_specs_type(p->a, 0);
            *sp = *p;
            sp->next = 0;
            pt = apply_declarator(pt, p->b, 0, 0);
            if (n == cap) {
                cap = cap ? cap * 2 : 8;
                types = xrealloc(types, sizeof(Type *) * cap);
            }
            types[n++] = ty_adjust_param(pt);
            if (tail) tail->next = sp;
            else head = sp;
            tail = sp;
        }
    }
    nd = anew(sizeof(Node));
    *nd = *d;
    nd->b = head;
    *ptypes = types;
    *pn = n;
    return nd;
}

/* ------------------------------------------------------------ binding the parameters */

static Scope2 *tscope_new(Scope2 *parent) {
    return scope2_new(SS_TMPL, parent);
}

static int tp_is_type(Node *tp) {
    return tp->op != 0 && !tp->b;
}

static Type *tp_value_type(Scope2 *ts, Node *tp) {
    Scope2 *saved = scope2_cur;
    Type *t;
    scope2_cur = ts;
    t = apply_declarator(decl_specs_type(tp->a->a, 0), tp->a->b, 0, 0);
    scope2_cur = saved;
    return t;
}

static Ent *tp_elem_ent(Scope2 *ts, Node *tp, TArg *a, Type *vt) {
    Ent *e;
    if (a->kind == TA_TYPE) e = ent_new(EK_TYPEDEF, tp->name, a->t, ts);
    else if (a->kind == TA_ENT) {
        e = anew(sizeof(Ent));
        *e = *a->ent;
        e->name = tp->name;
        e->overload = 0;
        e->scope = ts;
    } else {
        e = ent_new(EK_ENUMERATOR, tp->name, a->t ? a->t : vt, ts);
        e->cv = a->v;
        e->has_val = 1;
    }
    return e;
}

static void tp_bind(Scope2 *ts, Node *tp, TArg *a) {
    Type *vt = 0;
    if (!tp->name) return;
    if (!tp_is_type(tp)) vt = tp_value_type(ts, tp);
    if (a->kind == TA_PACK) {
        Ent *pe = pack_ent_new(tp->name, ts);
        int i;
        for (i = 0; i < a->n; i++) pack_append(pe, tp_elem_ent(ts, tp, a->elems[i], vt));
        return;
    }
    scope_add(ts, tp_elem_ent(ts, tp, a, vt));
}

/* ------------------------------------------------------------ evaluating template arguments */

static Ent *apply_part_args(Ent *e, Node *p);

static TArg *eval_arg_node(Node *x) {
    if (x->kind == N_TYPEID) return ta_type(typeid_type(x));
    if (x->kind == N_TYPE_EXPR) return ta_type(typeid_type(x->b));
    if (x->kind == N_ID) {
        Ent *e = resolve_id(x, 0);
        if (e && (e->kind == EK_TYPEDEF || e->kind == EK_CLASS || e->kind == EK_ENUM)) return ta_type(e->type);
        if (e && e->kind == EK_TEMPLATE) serr(x, "template template arguments are not supported yet");
    }
    {
        /* the address of a function or of a variable with static storage */
        Node *y = x;
        Ent *pe = 0;
        if (y->kind == N_UNARY && y->op == P_AMP && y->a && y->a->kind == N_ID) y = y->a;
        if (y->kind == N_ID && (y == x || x->kind == N_UNARY)) {
            pe = resolve_id(y, 0);
            if (pe && ((pe->kind == EK_FUNC && !pe->overload && (!(pe->flags & EF_MEMBER) || (pe->flags & EF_STATIC))) || (pe->kind == EK_VAR && !(pe->flags & EF_PARAM) && pe->scope && pe->scope->kind != SS_BLOCK && !pe->has_val))) {
                TArg *a = anew(sizeof(TArg));
                a->kind = TA_ENT;
                a->ent = pe;
                a->t = pe->type;
                if (pe->kind == EK_FUNC) use_function(pe);
                return a;
            }
        }
    }
    {
        Val *v = expr(x);
        if (!v->is_const) serr(x, "a template argument is not a constant expression");
        return ta_value(v->t, v->cv);
    }
}

/* the arguments of a template-id (N_TARG list), pack expansions expanded */
static void eval_targs(Node *list, TArg ***out_args, int *out_n) {
    TArg **v = 0;
    int n = 0, cap = 0;
    Node *a;
    for (a = list; a; a = a->next) {
        Node *x = a->a;
        if (x->kind == N_PACKEXP) {
            int cnt = pack_count(x->a), i;
            for (i = 0; i < cnt; i++) {
                Scope2 *saved = pack_enter(x->a, i);
                targ_push(&v, &n, &cap, eval_arg_node(x->a));
                scope2_cur = saved;
            }
        } else targ_push(&v, &n, &cap, eval_arg_node(x));
    }
    *out_args = v;
    *out_n = n;
}

static Node *tparam_default(Node *tp) {
    if (tp->a && tp->a->kind == N_PARAM) return tp->a->c;
    return tp->a;
}

/* the arguments for all the parameters of tm: the given ones, the defaults, empty packs.  ts receives the bindings. */
static int complete_args(Tmpl *tm, TArg **given, int ng, Scope2 *ts, TArg ***out_args, int *out_n, Node *at) {
    TArg **v = anew(sizeof(TArg *) * (tm->ntp + 1));
    int i, gi = 0;
    for (i = 0; i < tm->ntp; i++) {
        Node *tp = tm->tp[i];
        TArg *a = 0;
        if (tp->flags & TPF_PACK) {
            TArg **e = anew(sizeof(TArg *) * (ng - gi + 1));
            int k = 0;
            while (gi < ng) e[k++] = given[gi++];
            a = ta_pack(e, k);
        } else if (gi < ng) {
            a = given[gi++];
            if (tp_is_type(tp) && a->kind != TA_TYPE) serr(at, "the template argument for '%s' must be a type", tp->name ? tp->name->s : "?");
            if (!tp_is_type(tp) && a->kind == TA_TYPE) serr(at, "the template argument for '%s' must be a value", tp->name ? tp->name->s : "?");
            if (a->kind == TA_VALUE) a = ta_value(tp_value_type(ts, tp), a->v);
        } else {
            Node *df = tm->defaults[i];
            Scope2 *saved = scope2_cur;
            if (!df) serr(at, "too few template arguments");
            scope2_cur = ts;
            if (tp_is_type(tp)) a = ta_type(typeid_type(df));
            else {
                Val *dv = expr(df);
                if (!dv->is_const) serr(df, "a default template argument is not a constant expression");
                a = ta_value(tp_value_type(ts, tp), dv->cv);
            }
            scope2_cur = saved;
        }
        v[i] = a;
        tp_bind(ts, tp, a);
    }
    if (gi < ng) serr(at, "too many template arguments");
    *out_args = v;
    *out_n = tm->ntp;
    return 1;
}

/* ------------------------------------------------------------ matching (deduction) */

typedef struct Binds Binds;
struct Binds {
    Node **tp;
    int n;
    TArg **v;
    char *fixed;             /* given explicitly: not deduced */
};

static int binds_index(Binds *b, Name *nm) {
    int i;
    for (i = 0; i < b->n; i++)
        if (b->tp[i]->name == nm) return i;
    return -1;
}

static int bind_arg(Binds *b, int idx, TArg *a) {
    Node *tp = b->tp[idx];
    if (b->fixed[idx]) return 1;
    if (tp->flags & TPF_PACK) {
        TArg *pk = b->v[idx];
        if (!pk) {
            pk = ta_pack(0, 0);
            b->v[idx] = pk;
        }
        pk->elems = xrealloc(pk->elems, sizeof(TArg *) * (pk->n + 1));
        pk->elems[pk->n++] = a;
        return 1;
    }
    if (b->v[idx]) return ta_equal(b->v[idx], a);
    b->v[idx] = a;
    return 1;
}

static int match_type(Binds *b, Node *ts, Node *decl, Type *a);
static int lenient_cv = 0;     /* the top-level match of a function parameter: const in the pattern may be missing in the argument */
static int match_targs(Binds *b, Node *list, TArg **args, int nargs);

/* the class template of a template-id without looking at the arguments */
static Ent *template_of_id(Node *id) {
    Node *p;
    Ent *e = 0;
    Scope2 *sc = 0;
    int first = 1;
    if (id->flags & ID_GLOBAL) {
        sc = scope2_global;
        first = 0;
    }
    for (p = id->a; p; p = p->next) {
        if ((p->flags & 7) != PART_NAME) return 0;
        e = first ? lookup_unqual(scope2_cur, p->name) : (sc ? lookup_in(sc, p->name, 0) : 0);
        first = 0;
        if (!e) return 0;
        if (p->next) {
            if (p->flags & PART_ARGS) return 0;
            if (e->kind == EK_NS) sc = e->members;
            else if (e->kind == EK_CLASS && e->ci) sc = e->ci->scope;
            else return 0;
        }
    }
    if (e && (e->flags & EF_INJECTED) && e->tmpl_ent) e = e->tmpl_ent;
    return e && e->kind == EK_TEMPLATE ? e : 0;
}

/* an instance of the template (or of a class derived from an instance) among the class and its bases */
static Ent *find_instance_of(Type *t, Ent *tmpl, int depth) {
    Ent *c;
    int i;
    if (t->kind != TY_CLASS || depth > 8) return 0;
    c = t->ent;
    if (c->inst && c->inst->tmpl == tmpl) return c;
    if (!c->ci) return 0;
    for (i = 0; i < c->ci->nbases; i++) {
        Ent *r = find_instance_of(c->ci->bases[i].cls->type, tmpl, depth + 1);
        if (r) return r;
    }
    return 0;
}

static int pattern_is_value_param(Binds *b, Node *x, int *idx) {
    Node *p;
    if (x->kind != N_ID || !x->a || x->a->next) return 0;
    p = x->a;
    if ((p->flags & 7) != PART_NAME || (p->flags & PART_ARGS) || (x->flags & ID_GLOBAL)) return 0;
    *idx = binds_index(b, p->name);
    return *idx >= 0;
}

static int match_targ(Binds *b, Node *x, TArg *arg) {
    int idx;
    if (x->kind == N_TYPEID) {
        if (arg->kind != TA_TYPE) return 0;
        return match_type(b, x->a, x->b, arg->t);
    }
    if (x->kind == N_TYPE_EXPR) {
        if (arg->kind != TA_TYPE) return 0;
        return match_type(b, x->b->a, x->b->b, arg->t);
    }
    if (pattern_is_value_param(b, x, &idx)) {
        if (tp_is_type(b->tp[idx])) {
            if (arg->kind != TA_TYPE) return 0;
        } else if (arg->kind != TA_VALUE && arg->kind != TA_ENT) return 0;
        return bind_arg(b, idx, arg);
    }
    /* a literal must be equal to the argument; any other expression is not deduced */
    {
        Node *y = x;
        int neg = 0;
        if (y->kind == N_UNARY && y->op == P_MINUS && y->a) {
            neg = 1;
            y = y->a;
        }
        if (y->kind == N_NUM || y->kind == N_BOOLLIT || y->kind == N_CHARLIT) {
            Val *v = expr(y);
            if (!v->is_const) return 1;
            if (arg->kind != TA_VALUE) return 0;
            return arg->v == (neg ? -v->cv : v->cv);
        }
    }
    return 1;
}

/* the arguments with the elements of the packs in line */
static TArg **flat_args(TArg **a, int n, int *out_n) {
    TArg **v = 0;
    int m = 0, cap = 0, i, k;
    for (i = 0; i < n; i++) {
        if (a[i]->kind == TA_PACK) {
            for (k = 0; k < a[i]->n; k++) targ_push(&v, &m, &cap, a[i]->elems[k]);
        } else targ_push(&v, &m, &cap, a[i]);
    }
    *out_n = m;
    return v;
}

static int match_targs_flat(Binds *b, Node *list, TArg **args, int nargs);

static int match_targs(Binds *b, Node *list, TArg **args, int nargs) {
    int nf;
    TArg **fa = flat_args(args, nargs, &nf);
    return match_targs_flat(b, list, fa, nf);
}

static int match_targs_flat(Binds *b, Node *list, TArg **args, int nargs) {
    Node *p;
    int i = 0;
    for (p = list; p; p = p->next) {
        Node *x = p->a;
        if (x->kind == N_PACKEXP) {
            while (i < nargs) {
                if (!match_targ(b, x->a, args[i])) return 0;
                i++;
            }
            /* a pack that matches nothing is an empty pack */
            {
                PackSet ps;
                int k;
                ps.n = 0;
                /* bind the parameters of the pattern that stayed unbound to empty packs */
                for (k = 0; k < b->n; k++)
                    if ((b->tp[k]->flags & TPF_PACK) && !b->v[k] && !b->fixed[k]) {
                        Node *y = x->a;
                        int idx;
                        if (y->kind == N_TYPEID && y->a && y->a->a && y->a->a->kind == N_ID && pattern_is_value_param(b, y->a->a, &idx) && idx == k)
                            b->v[k] = ta_pack(0, 0);
                    }
            }
            continue;
        }
        if (i >= nargs) return 0;
        if (!match_targ(b, x, args[i])) return 0;
        i++;
    }
    return i == nargs;
}

/* the declarator nodes from the root to the node before the name, the pack marker left out */
static int declarator_chain(Node *d, Node **nodes, int max) {
    int k = 0;
    for (; d && d->kind != N_D_NAME; d = d->a) {
        if (d->kind == N_D_PACK) continue;
        if (k < max) nodes[k++] = d;
    }
    return k;
}

static int match_specs(Binds *b, Node *ts, Type *a) {
    Node *id;
    Node *last;
    int idx;
    int lenient = lenient_cv;
    lenient_cv = 0;
    if ((ts->flags & DS_CONST) && !(a->cv & CV_CONST) && !lenient) return 0;
    if ((ts->flags & DS_VOLATILE) && !(a->cv & CV_VOLATILE) && !lenient) return 0;
    if ((ts->flags & DS_CONST) && (a->cv & CV_CONST)) {
        Type *c = anew(sizeof(Type));
        *c = *a;
        c->cv &= ~CV_CONST;
        a = c;
    }
    if ((ts->flags & DS_VOLATILE) && (a->cv & CV_VOLATILE)) {
        Type *c = anew(sizeof(Type));
        *c = *a;
        c->cv &= ~CV_VOLATILE;
        a = c;
    }
    if (!ts->a || ts->a->kind != N_ID) return 1;
    id = ts->a;
    last = id->a;
    while (last && last->next) last = last->next;
    if (!last) return 1;
    if (!id->a->next && !(last->flags & PART_ARGS) && !(id->flags & ID_GLOBAL) && (last->flags & 7) == PART_NAME) {
        idx = binds_index(b, last->name);
        if (idx >= 0 && tp_is_type(b->tp[idx])) return bind_arg(b, idx, ta_type(a));
        return 1;
    }
    if (last->flags & PART_ARGS) {
        Ent *tent = template_of_id(id);
        Ent *inst;
        if (!tent) return 1;
        if (a->kind != TY_CLASS) return 0;
        inst = find_instance_of(a, tent, 0);
        if (!inst) return 0;
        return match_targs(b, last->b, inst->inst->args, inst->inst->nargs);
    }
    return 1;
}

/* does the pattern (specifiers + declarator chain of k nodes) match the type a? */
static int match_chain(Binds *b, Node *ts, Node **nodes, int k, Type *a) {
    int j;
    int top = lenient_cv;
    lenient_cv = 0;
    for (j = k - 1; j >= 0; j--) {
        Node *d = nodes[j];
        switch (d->kind) {
        case N_D_PTR:
            if (a->kind != TY_PTR) return 0;
            if ((d->flags & DS_CONST) && !(a->cv & CV_CONST)) return 0;
            a = a->base;
            break;
        case N_D_REF:
            if (a->kind != TY_REF) return 0;
            a = a->base;
            break;
        case N_D_RREF:
            if (a->kind != TY_RREF) return 0;
            a = a->base;
            break;
        case N_D_ARRAY:
            {
                int idx;
                if (a->kind != TY_ARRAY) return 0;
                if (d->b && pattern_is_value_param(b, d->b, &idx)) {
                    if (a->len < 0) return 0;
                    if (!bind_arg(b, idx, ta_value(T_ULONG, a->len))) return 0;
                }
                a = a->base;
            }
            break;
        case N_D_FUNC:
            {
                Node *p;
                int i = 0;
                if (a->kind != TY_FUNC) return 0;
                for (p = d->b; p; p = p->next) {
                    Node *pc[16];
                    int pk = declarator_chain(p->b, pc, 16);
                    if (p->a->val == BT_VOID && !p->a->a && !p->b) break;
                    if (declarator_has_pack(p->b)) {
                        while (i < a->nparams) {
                            if (!match_chain(b, p->a, pc, pk, a->params[i])) return 0;
                            i++;
                        }
                        {
                            int k2;
                            Node *y = p->a;
                            int idx;
                            for (k2 = 0; k2 < b->n; k2++)
                                if ((b->tp[k2]->flags & TPF_PACK) && !b->v[k2] && !b->fixed[k2] && y->a && pattern_is_value_param(b, y->a, &idx) && idx == k2)
                                    b->v[k2] = ta_pack(0, 0);
                        }
                        continue;
                    }
                    if (i >= a->nparams) return 0;
                    if (!match_chain(b, p->a, pc, pk, a->params[i])) return 0;
                    i++;
                }
                if (i != a->nparams) return 0;
                a = a->base;
            }
            break;
        default:
            return 1;
        }
    }
    lenient_cv = top;
    return match_specs(b, ts, a);
}

static int match_type(Binds *b, Node *ts, Node *decl, Type *a) {
    Node *nodes[16];
    int k = declarator_chain(decl, nodes, 16);
    return match_chain(b, ts, nodes, k, a);
}

/* ------------------------------------------------------------ trial of a substitution */

typedef struct Trial Trial;
struct Trial {
    InstState st;
    jmp_buf jb;
    jmp_buf *saved_trap;
    int nbuild, depth, npend, plen, in_fn, nlog, nlazy, late, neager;
    DeferredBody *tail;
};

static void trial_end_ok(Trial *t) {
    err_trap = t->saved_trap;
}

static void trial_failed(Trial *t) {
    err_trap = t->saved_trap;
    snap_restore(&t->st);
    cur_in_fn = t->in_fn;
    if (npend > t->npend) {
        pending_helpers = pend_stack[t->npend];
        inst_active -= npend - t->npend;
        npend = t->npend;
    }
    if (pending_helpers.len > t->plen) {
        pending_helpers.len = t->plen;
        pending_helpers.s[t->plen] = 0;
    }
    nbuilding = t->nbuild;
    if (neager > t->neager) neager = t->neager;
    late_truncate(t->late);
    while (nlazy_log > t->nlazy) {
        Ent *f = lazy_log[--nlazy_log];
        f->lazy->done = 0;
        f->flags &= ~EF_DEFINED;
    }
    while (ninst_log > t->nlog) {
        /* an instance made during the trial: its text is gone, so it is forgotten */
        TInst *in = inst_log[--ninst_log];
        Tmpl *tm = in->tmpl->tmpl;
        TInst **pp;
        for (pp = &tm->insts; *pp; pp = &(*pp)->next)
            if (*pp == in) {
                *pp = in->next;
                break;
            }
    }
    def_tail = t->tail;
    if (def_tail) def_tail->next = 0;
    else def_head = 0;
    tdepth = t->depth;
}

#define TRIAL_BEGIN(t) \
    do { \
        snap_save(&(t).st); \
        (t).saved_trap = err_trap; \
        (t).nbuild = nbuilding; \
        (t).nlog = ninst_log; \
        (t).neager = neager; \
        (t).nlazy = nlazy_log; \
        (t).late = late_len(); \
        (t).tail = def_tail; \
        (t).depth = tdepth; \
        (t).npend = npend; \
        (t).in_fn = cur_in_fn; \
        (t).plen = pending_helpers.len; \
        err_trap = &(t).jb; \
    } while (0)

/* ------------------------------------------------------------ class templates */

static const char *class_inst_cname(Tmpl *tm, Name *name) {
    return fmt_str("%s%s__I%d", tm->scope->prefix, name->s, ++tinst_count);
}

static Scope2 *new_instance_scope(Tmpl *tm) {
    return tscope_new(tm->scope);
}

static int pattern_score_node(Node *n) {
    int s = 0;
    if (!n) return 0;
    for (; n; n = n->next) {
        if (n->kind == N_D_PTR || n->kind == N_D_REF || n->kind == N_D_RREF || n->kind == N_D_ARRAY || n->kind == N_D_FUNC) s += 2;
        if (n->kind == N_NUM || n->kind == N_BOOLLIT) s += 2;
        if (n->kind == N_PART && (n->flags & PART_ARGS)) s += 3;
        if (n->kind == N_TYPE && !n->a) s += 2;
        if (n->kind == N_TYPE && (n->flags & DS_CONST)) s += 1;
        s += pattern_score_node(n->a) + pattern_score_node(n->b) + pattern_score_node(n->c) + pattern_score_node(n->d);
    }
    return s;
}

static Ent *instantiate_class(Ent *tent, TArg **given, int ngiven, Node *at);

static int try_ool_member(Ent *f);

/* virtual functions and destructors of an instance are generated with the class, but after the enclosing classes are complete */
static void eager_add(Ent *f) {
    eager_list = xrealloc(eager_list, sizeof(Ent *) * (neager + 1));
    eager_list[neager++] = f;
}

/* generates the queued members once no class is being defined and no instance is being written */
static void run_eager_list(void) {
    int i;
    if (running_eager || inst_active > 0 || class_nest > 0) return;
    running_eager = 1;
    for (i = 0; i < neager; i++) {
        Ent *f = eager_list[i];
        InstState st;
        inst_begin(&st);
        if (f->lazy) use_function(f);
        else try_ool_member(f);
        inst_end(&st);
    }
    neager = 0;
    running_eager = 0;
}

/* the partial or explicit specialization that the arguments select; binds its parameters in ts */
static TSpec *select_spec(Tmpl *tm, TArg **args, int nargs, Scope2 *ts, Node *at) {
    TSpec *sp, *best = 0;
    TArg **bestv = 0;
    for (sp = tm->specs; sp; sp = sp->next) {
        if (sp->is_exact && targs_equal(sp->exact, sp->nexact, args, nargs)) return sp;
    }
    for (sp = tm->specs; sp; sp = sp->next) {
        Binds b;
        Node *tp;
        int n = 0, i, ok;
        if (sp->is_exact) continue;
        for (tp = sp->tnode->a; tp; tp = tp->next) n++;
        b.n = n;
        b.tp = anew(sizeof(Node *) * (n + 1));
        b.v = anew(sizeof(TArg *) * (n + 1));
        b.fixed = anew(n + 1);
        for (tp = sp->tnode->a, i = 0; tp; tp = tp->next, i++) b.tp[i] = tp;
        ok = match_targs(&b, sp->targs, args, nargs);
        for (i = 0; ok && i < n; i++) {
            if (!b.v[i]) {
                if (b.tp[i]->flags & TPF_PACK) b.v[i] = ta_pack(0, 0);
                else ok = 0;
            }
        }
        if (!ok) continue;
        if (!best || sp->score > best->score) {
            best = sp;
            bestv = b.v;
        }
    }
    if (best) {
        Node *tp;
        int i;
        for (tp = best->tnode->a, i = 0; tp; tp = tp->next, i++) tp_bind(ts, tp, bestv[i]);
    }
    return best;
}

static void ool_registered_class(Ent *cls, OolDef *od);
static int try_ool_member(Ent *f);
static void define_ool_statics(Ent *cls);

static Ent *instantiate_class(Ent *tent, TArg **given, int ngiven, Node *at) {
    Tmpl *tm = tent->tmpl;
    TArg **args;
    int nargs;
    Scope2 *ts, *pts;
    TInst *in, *probe;
    TSpec *spec;
    Ent *cls;
    Type *t;
    Node *cn;
    InstState st;
    if (tm->kind != TK_CLASS) serr(at, "'%s' is not a class template", tent->name->s);
    pts = tscope_new(tm->scope);
    complete_args(tm, given, ngiven, pts, &args, &nargs, at);
    for (probe = tm->insts; probe; probe = probe->next)
        if (targs_equal(probe->args, probe->nargs, args, nargs)) return probe->ent;
    if (tdepth > 150) serr(at, "template instantiation depth exceeds the maximum (infinite recursion?)");
    ts = new_instance_scope(tm);
    spec = select_spec(tm, args, nargs, ts, at);
    if (spec) cn = spec->cn;
    else {
        int i;
        cn = tm->classnode;
        for (i = 0; i < tm->ntp; i++) tp_bind(ts, tm->tp[i], args[i]);
    }
    if (!cn) serr(at, "invalid use of the incomplete type '%s%s'", tent->name->s, targs_text(args, nargs));
    cls = ent_new(EK_CLASS, tent->name, 0, tm->scope);
    t = anew(sizeof(Type));
    t->kind = TY_CLASS;
    t->ent = cls;
    t->len = -1;
    cls->type = t;
    cls->cname = class_inst_cname(tm, tent->name);
    cls->inst_text = targs_text(args, nargs);
    if (cn->op == OPKW + KW_union) cls->flags |= EF_UNION;
    in = anew(sizeof(TInst));
    in->tmpl = tent;
    in->args = args;
    in->nargs = nargs;
    in->ent = cls;
    in->next = tm->insts;
    tm->insts = in;
    cls->inst = in;
    log_inst(in);
    if (nbuilding < 64) building[nbuilding++] = in;
    /* the injected class name */
    {
        Ent *inj = ent_new(EK_TYPEDEF, tent->name, t, ts);
        inj->flags |= EF_INJECTED;
        inj->tmpl_ent = tent;
        scope_add(ts, inj);
    }
    tdepth++;
    note_push(fmt_str("%s%s", tent->name->s, cls->inst_text), at);
    inst_begin(&st);
    emitf("%s;\n", class_tag(cls));
    scope2_cur = ts;
    define_class(cls, cn);
    define_ool_statics(cls);
    inst_end(&st);
    note_pop();
    tdepth--;
    if (nbuilding > 0 && building[nbuilding - 1] == in) nbuilding--;
    {
        /* virtual functions and destructors are needed with the class */
        ClassInfo *ci = cls->ci;
        int i;
        for (i = 0; i < ci->nmethods; i++) {
            Ent *f = ci->methods[i];
            if ((f->flags & (EF_VIRTUAL | EF_DTOR)) && !(f->flags & (EF_DEFINED | EF_DEFAULTED | EF_DELETED | EF_PURE | EF_IMPLICIT))) eager_add(f);
        }
        if (ci->dtor && ci->dtor->owner == cls && !(ci->dtor->flags & (EF_DEFINED | EF_IMPLICIT | EF_DEFAULTED | EF_DELETED))) eager_add(ci->dtor);
    }
    return cls;
}

/* ------------------------------------------------------------ alias templates */

static Ent *instantiate_alias(Ent *tent, TArg **given, int ngiven, Node *at) {
    Tmpl *tm = tent->tmpl;
    TArg **args;
    int nargs;
    Scope2 *ts = tscope_new(tm->scope), *saved = scope2_cur;
    Type *t;
    Ent *e;
    complete_args(tm, given, ngiven, ts, &args, &nargs, at);
    scope2_cur = ts;
    t = apply_declarator(decl_specs_type(tm->alias->a->a, 0), tm->alias->a->b, 0, 0);
    scope2_cur = saved;
    e = ent_new(EK_TYPEDEF, tent->name, t, tm->scope);
    return e;
}

/* ------------------------------------------------------------ function templates */

static Node *function_declarator_of(Node *d) {
    for (; d; d = d->a)
        if (d->kind == N_D_FUNC) return d;
    return 0;
}

static Node *tmpl_function_declr(Tmpl *tm) {
    Node *def = tm->decl;
    if (def->kind == N_FUNCDEF) return def->b;
    return def->b ? def->b->a : 0;
}

static void lazy_set(Ent *e, Node *fd, Node *fnode, Type *ft, Scope2 *parent) {
    Lazy *lz = anew(sizeof(Lazy));
    lz->ent = e;
    lz->fd = fd;
    lz->fnode = fnode;
    lz->ft = ft;
    lz->parent = parent;
    e->lazy = lz;
}

static Ent *instantiate_function_ex(Ent *tf, TArg **args, int nargs, Node *def, Node *at) {
    Tmpl *tm = tf->tmpl;
    Scope2 *ts = tscope_new(tm->scope);
    InstState st;
    Node *declr, *id = 0, *fn = 0, *specs;
    Type *base, *t;
    int dsflags = 0, eflags = 0, i;
    Type *conv = 0;
    Name *ename;
    Ent *e, *cls = tm->owner;
    TInst *in;
    int is_const;
    for (i = 0; i < tm->ntp; i++) tp_bind(ts, tm->tp[i], args[i]);
    if (tdepth > 150) serr(at, "template instantiation depth exceeds the maximum (infinite recursion?)");
    declr = def->kind == N_FUNCDEF ? def->b : (def->b ? def->b->a : 0);
    specs = def->a;
    tdepth++;
    inst_begin(&st);
    scope2_cur = ts;
    if (specs_empty(specs)) {
        base = T_VOID;
        dsflags = specs->flags;
    } else base = decl_specs_type(specs, &dsflags);
    t = apply_declarator(base, declr, &id, &fn);
    if (!id || t->kind != TY_FUNC || !fn) serr(def, "unsupported function template");
    ename = function_entity_name(def, id, cls, &eflags, &conv);
    if (conv) t = ty_func(conv, t->params, t->nparams, t->variadic);
    is_const = (fn->flags & FQ_CONST) != 0;
    e = ent_new(EK_FUNC, ename, t, ts);
    e->flags |= eflags | EF_TINST;
    if (cls) {
        e->flags |= EF_MEMBER;
        e->owner = cls;
        e->access = tf->access;
        if (is_const) e->flags |= EF_CONST_METHOD;
        if (dsflags & DS_EXPLICIT) e->flags |= EF_EXPLICIT;
        e->cname = function_cname(cls->ci->scope, cls, ename, eflags, t, is_const);
    } else if (strncmp(ename->s, "operator", 8) == 0) {
        e->cname = fmt_str("%sop_%s__%s", tm->scope->prefix, entity_c_word(ename), sig_text(t));
    } else e->cname = mangle_function_name(tm->scope, ename, t);
    e->cname = fmt_str("%s__t%d", e->cname, ++tinst_count);
    if (dsflags & DS_STATIC) e->flags |= EF_STATIC;
    if (fn->flags & FQ_DELETE) e->flags |= EF_DELETED;
    e->defaults = anew(sizeof(Node *) * (t->nparams + 1));
    {
        Node *p;
        for (p = fn->b, i = 0; p && i < t->nparams; p = p->next, i++)
            if (p->c) e->defaults[i] = p->c;
    }
    e->file = def->file;
    e->line = def->line;
    in = anew(sizeof(TInst));
    in->tmpl = tf;
    in->args = args;
    in->nargs = nargs;
    in->ent = e;
    in->fnode = fn;
    in->ft = t;
    in->next = tm->insts;
    tm->insts = in;
    e->inst = in;
    log_inst(in);
    if (nbuilding < 64) building[nbuilding++] = in;
    emit_prototype(e, fn, dsflags);
    if (def->kind == N_FUNCDEF && def->d) lazy_set(e, def, fn, t, ts);
    inst_end(&st);
    tdepth--;
    if (nbuilding > 0 && building[nbuilding - 1] == in) nbuilding--;
    return e;
}

static Ent *instantiate_function(Ent *tf, TArg **args, int nargs, Node *at) {
    Tmpl *tm = tf->tmpl;
    TInst *in;
    for (in = tm->insts; in; in = in->next)
        if (targs_equal(in->args, in->nargs, args, nargs)) return in->ent;
    return instantiate_function_ex(tf, args, nargs, tm->decl, at);
}

/* the arguments for all parameters from the deduced ones: defaults, empty packs; 0 if one cannot be determined */
static TArg **finish_deduction(Tmpl *tm, Binds *b, Node *at) {
    TArg **v = anew(sizeof(TArg *) * (tm->ntp + 1));
    Scope2 *ts = tscope_new(tm->scope);
    int i;
    for (i = 0; i < tm->ntp; i++) {
        Node *tp = tm->tp[i];
        TArg *a = b->v[i];
        if (!a) {
            if (tp->flags & TPF_PACK) a = ta_pack(0, 0);
            else if (tm->defaults[i]) {
                Node *df = tm->defaults[i];
                Scope2 *saved = scope2_cur;
                scope2_cur = ts;
                if (tp_is_type(tp)) a = ta_type(typeid_type(df));
                else {
                    Val *dv = expr(df);
                    if (!dv->is_const) serr(df, "a default template argument is not a constant expression");
                    a = ta_value(tp_value_type(ts, tp), dv->cv);
                }
                scope2_cur = saved;
            } else return 0;
        }
        v[i] = a;
        tp_bind(ts, tp, a);
    }
    (void)at;
    return v;
}

static Binds *binds_new(Tmpl *tm) {
    Binds *b = anew(sizeof(Binds));
    b->n = tm->ntp;
    b->tp = tm->tp;
    b->v = anew(sizeof(TArg *) * (tm->ntp + 1));
    b->fixed = anew(tm->ntp + 1);
    return b;
}

/* pre-binds the explicitly given arguments */
static int bind_explicit(Tmpl *tm, Binds *b, TArg **ex, int nex) {
    int i, gi = 0;
    for (i = 0; i < tm->ntp && gi < nex; i++) {
        Node *tp = tm->tp[i];
        if (tp->flags & TPF_PACK) {
            TArg **e = anew(sizeof(TArg *) * (nex - gi + 1));
            int k = 0;
            while (gi < nex) e[k++] = ex[gi++];
            b->v[i] = ta_pack(e, k);
            b->fixed[i] = 1;
            break;
        }
        if (tp_is_type(tp) != (ex[gi]->kind == TA_TYPE)) return 0;
        b->v[i] = ex[gi++];
        b->fixed[i] = 1;
    }
    return gi == nex;
}

/* deduces the template parameters from one argument of a call */
static int deduce_param(Binds *b, Node *p, Val *v) {
    Node *nodes[16];
    int k = declarator_chain(p->b, nodes, 16);
    Type *a = v->t;
    if (v->fn) {
        if (v->fn->overload || v->fn->kind != EK_FUNC) return 1;
        a = v->fn->type;
    }
    if (k > 0 && (nodes[k - 1]->kind == N_D_REF || nodes[k - 1]->kind == N_D_RREF)) {
        Node *d = nodes[k - 1];
        if (d->kind == N_D_RREF && k == 1 && v->lval && !v->fn && !(p->a->flags & (DS_CONST | DS_VOLATILE)) && p->a->a && p->a->a->kind == N_ID) {
            int idx;
            if (p->a->a->a && !p->a->a->a->next && !(p->a->a->a->flags & PART_ARGS)) {
                idx = binds_index(b, p->a->a->a->name);
                if (idx >= 0 && tp_is_type(b->tp[idx])) return bind_arg(b, idx, ta_type(ty_ref(a)));
            }
        }
        lenient_cv = 1;
        return match_chain(b, p->a, nodes, k - 1, a);
    }
    if (a->kind == TY_ARRAY) a = ty_ptr(a->base);
    else if (a->kind == TY_FUNC) a = ty_ptr(a);
    a = ty_unqual(a);
    lenient_cv = 1;
    return match_chain(b, p->a, nodes, k, a);
}

static Ent *deduce_function(Ent *tf, Val **args, int nargs, Type *objt) {
    Tmpl *tm = tf->tmpl;
    Binds *b = binds_new(tm);
    Node *declr = tmpl_function_declr(tm), *fn, *p;
    int ai = 0;
    Trial tr;
    Ent *r = 0;
    TArg **full;
    (void)objt;
    if (!declr) return 0;
    fn = function_declarator_of(declr);
    if (!fn) return 0;
    if (tf->ntargs && !bind_explicit(tm, b, tf->targs, tf->ntargs)) return 0;
    TRIAL_BEGIN(tr);
    if (setjmp(tr.jb) == 0) {
        Scope2 *call_scope = scope2_cur;
        scope2_cur = tm->scope;                  /* the names of the patterns are looked up where the template is defined */
        for (p = fn->b; p; p = p->next) {
            if (p->a->val == BT_VOID && !p->a->a && !p->b) break;
            if (declarator_has_pack(p->b)) {
                for (; ai < nargs; ai++)
                    if (!deduce_param(b, p, args[ai])) {
                        err_trap = tr.saved_trap;
                        scope2_cur = call_scope;
                        return 0;
                    }
                break;
            }
            if (ai >= nargs) break;
            if (!deduce_param(b, p, args[ai++])) {
                err_trap = tr.saved_trap;
                scope2_cur = call_scope;
                return 0;
            }
        }
        full = finish_deduction(tm, b, 0);
        if (!full) {
            err_trap = tr.saved_trap;
            scope2_cur = call_scope;
            return 0;
        }
        scope2_cur = call_scope;
        r = instantiate_function(tf, full, tm->ntp, 0);
        trial_end_ok(&tr);
        return r;
    }
    trial_failed(&tr);
    return 0;
}

/* the instance of a function template in the set whose type is ft (for a function name converted to a pointer to function) */
static Ent *deduce_function_for_type(Ent *set, Type *ft) {
    Ent *tf;
    for (tf = set; tf; tf = tf->overload) {
        Tmpl *tm;
        Binds *b;
        Node *fn, *p;
        int i = 0, ok = 1;
        Trial tr;
        TArg **full;
        Ent *r = 0;
        if (tf->kind == EK_FUNC) {
            if (ty_equal(tf->type, ft)) return tf;
            continue;
        }
        if (tf->kind != EK_FTEMPLATE) continue;
        tm = tf->tmpl;
        b = binds_new(tm);
        if (tf->ntargs && !bind_explicit(tm, b, tf->targs, tf->ntargs)) continue;
        fn = function_declarator_of(tmpl_function_declr(tm));
        if (!fn) continue;
        TRIAL_BEGIN(tr);
        if (setjmp(tr.jb) == 0) {
            for (p = fn->b; p && ok; p = p->next, i++) {
                Node *nodes[16];
                int k;
                if (p->a->val == BT_VOID && !p->a->a && !p->b) break;
                if (i >= ft->nparams || declarator_has_pack(p->b)) {
                    ok = 0;
                    break;
                }
                k = declarator_chain(p->b, nodes, 16);
                if (!match_chain(b, p->a, nodes, k, ft->params[i])) ok = 0;
            }
            if (ok && i != ft->nparams) ok = 0;
            if (ok) {
                full = finish_deduction(tm, b, 0);
                if (full) {
                    r = instantiate_function(tf, full, tm->ntp, 0);
                    if (r && !ty_equal(r->type, ft)) r = 0;
                }
            }
            trial_end_ok(&tr);
        } else {
            trial_failed(&tr);
            r = 0;
        }
        if (r) return r;
    }
    return 0;
}

/* a function template named with explicit arguments: the instance if the arguments determine it, else the template with the arguments */
static Ent *explicit_function_set(Ent *set, Node *p) {
    TArg **a;
    int n;
    Ent *head = 0, *tail = 0, *t;
    eval_targs(p->b, &a, &n);
    for (t = set; t; t = t->overload) {
        Ent *c = 0;
        Tmpl *tm;
        Binds *b;
        Trial tr;
        if (t->kind != EK_FTEMPLATE) continue;
        tm = t->tmpl;
        b = binds_new(tm);
        if (!bind_explicit(tm, b, a, n)) continue;
        {
            int i, all = 1;
            for (i = 0; i < tm->ntp; i++)
                if (!b->fixed[i]) all = 0;
            if (all) {
                TArg **full;
                TRIAL_BEGIN(tr);
                if (setjmp(tr.jb) == 0) {
                    full = finish_deduction(tm, b, p);
                    if (full) c = instantiate_function(t, full, tm->ntp, p);
                    trial_end_ok(&tr);
                } else {
                    trial_failed(&tr);
                    continue;
                }
            }
        }
        if (!c) {
            c = anew(sizeof(Ent));
            *c = *t;
            c->targs = a;
            c->ntargs = n;
        }
        if (c->kind == EK_FUNC) {
            Ent *cp = anew(sizeof(Ent));
            *cp = *c;
            cp->overload = 0;
            c = cp;
        }
        c->overload = 0;
        if (tail) tail->overload = c;
        else head = c;
        tail = c;
    }
    if (!head) serr(p, "no function template '%s' matches the template arguments", p->name ? p->name->s : "?");
    return head;
}

/* a variable template: one variable per set of arguments, in a scope of its own that gives the C name a unique prefix */
static Ent *instantiate_var(Ent *tent, TArg **given, int ngiven, Node *at) {
    Tmpl *tm = tent->tmpl;
    TArg **args;
    int nargs;
    Scope2 *ts = tscope_new(tm->scope);
    TInst *in;
    InstState st;
    Ent *v;
    Node *d = tm->decl;
    complete_args(tm, given, ngiven, ts, &args, &nargs, at);
    for (in = tm->insts; in; in = in->next)
        if (targs_equal(in->args, in->nargs, args, nargs)) return in->ent;
    ts->prefix = fmt_str("%s%s__V%d_", tm->scope->prefix, tent->name->s, ++tinst_count);
    inst_begin(&st);
    scope2_cur = ts;
    process_decl(d);
    inst_end(&st);
    v = sfind(ts, tent->name);
    if (!v) serr(at, "the variable template '%s' did not declare a variable", tent->name->s);
    in = anew(sizeof(TInst));
    in->tmpl = tent;
    in->args = args;
    in->nargs = nargs;
    in->ent = v;
    in->next = tm->insts;
    tm->insts = in;
    log_inst(in);
    return v;
}

static Ent *apply_part_args(Ent *e, Node *p) {
    TArg **a;
    int n;
    if ((e->flags & EF_INJECTED) && e->tmpl_ent) e = e->tmpl_ent;
    if (e->kind == EK_TEMPLATE) {
        eval_targs(p->b, &a, &n);
        if (e->tmpl->kind == TK_VAR) return instantiate_var(e, a, n, p);
        if (e->tmpl->kind == TK_ALIAS) return instantiate_alias(e, a, n, p);
        return instantiate_class(e, a, n, p);
    }
    if (e->kind == EK_FTEMPLATE) return explicit_function_set(e, p);
    serr(p, "'%s' is not a template", p->name ? p->name->s : "?");
    return 0;
}

/* when two candidates are equally good: a function that is not an instance wins, then the more specialized template */
static int template_tiebreak(Ent *a, Ent *b) {
    int ta = (a->flags & EF_TINST) != 0, tb = (b->flags & EF_TINST) != 0;
    if (ta && !tb) return 1;
    if (!ta && tb) return -1;
    if (ta && tb && a->inst && b->inst && a->inst->tmpl != b->inst->tmpl) {
        /* the more specialized template: the one whose parameters can be deduced from the other's parameter types */
        Tmpl *ma = a->inst->tmpl->tmpl, *mb = b->inst->tmpl->tmpl;
        int sa = pattern_score_node(tmpl_function_declr(ma)), sb2 = pattern_score_node(tmpl_function_declr(mb));
        if (sa > sb2) return -1;
        if (sb2 > sa) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------ bodies */

static void generate_lazy(Ent *f, Lazy *lz) {
    InstState st;
    note_push(ent_qualified_name(f), use_at);
    inst_begin(&st);
    scope2_cur = f->scope;
    body_parent = lz->parent;
    define_function(lz->fd, f, lz->fnode, lz->ft);
    inst_end(&st);
    note_pop();
}

static void use_function(Ent *f) {
    if (f->lazy) {
        Lazy *lz = f->lazy;
        if (lz->done) {
            if (lz->ent && lz->ent != f) f->type = lz->ent->type;
            return;
        }
        lz->done = 1;
        if (nlazy_log == cap_lazy_log) {
            cap_lazy_log = cap_lazy_log ? cap_lazy_log * 2 : 256;
            lazy_log = xrealloc(lazy_log, sizeof(Ent *) * cap_lazy_log);
        }
        lazy_log[nlazy_log++] = lz->ent ? lz->ent : f;
        generate_lazy(f, lz);
        return;
    }
    if (f->owner && f->owner->inst && (f->flags & EF_MEMBER)
        && !(f->flags & (EF_DEFINED | EF_IMPLICIT | EF_DELETED | EF_DEFAULTED | EF_PURE | EF_OOLTRIED | EF_TINST))) {
        f->flags |= EF_OOLTRIED;
        try_ool_member(f);
    }
}

/* ------------------------------------------------------------ out-of-line definitions of members */

static Node *ool_class_part(Node *id) {
    Node *p;
    for (p = id->a; p && p->next; p = p->next)
        if (!p->next->next) return p;
    return 0;
}

static Name *ool_entity_name(Node *id, Ent *cls) {
    Node *lp = id_last(id);
    int fl = 0;
    Type *conv;
    Name *n = function_entity_name(id, id, cls, &fl, &conv);
    (void)lp;
    return n;
}

/* binds the parameters of the out-of-line definition from the arguments of the class instance; 0 if they do not match */
static Scope2 *ool_bind(OolDef *od, Ent *cls, Node *cpart) {
    Binds b;
    Node *tp;
    int n = 0, i;
    Scope2 *ts;
    TInst *in = cls->inst;
    for (tp = od->tnode->a; tp; tp = tp->next) n++;
    b.n = n;
    b.tp = anew(sizeof(Node *) * (n + 1));
    b.v = anew(sizeof(TArg *) * (n + 1));
    b.fixed = anew(n + 1);
    for (tp = od->tnode->a, i = 0; tp; tp = tp->next, i++) b.tp[i] = tp;
    if (!cpart || !(cpart->flags & PART_ARGS)) return 0;
    if (!match_targs(&b, cpart->b, in->args, in->nargs)) return 0;
    ts = tscope_new(cls->ci->scope);          /* the names of the class, then the parameters of the definition */
    for (i = 0; i < n; i++) {
        if (!b.v[i]) {
            if (b.tp[i]->flags & TPF_PACK) b.v[i] = ta_pack(0, 0);
            else return 0;
        }
        tp_bind(ts, b.tp[i], b.v[i]);
    }
    return ts;
}

static Node *ool_declr(OolDef *od) {
    Node *def = od->def;
    if (def->kind == N_FUNCDEF) return def->b;
    return def->b ? def->b->a : 0;
}

static int ool_is_function(OolDef *od) {
    Node *d = ool_declr(od);
    return od->def->kind == N_FUNCDEF || function_declarator_of(d) != 0;
}

static int try_ool_member(Ent *f) {
    Ent *cls = f->owner;
    Tmpl *tm;
    OolDef *od;
    int found = 0;
    if (!cls || !cls->inst) return 0;
    tm = cls->inst->tmpl->tmpl;
    for (od = tm->ool; od && !found; od = od->next) {
        Node *declr, *id, *cpart, *fn = 0, *nid = 0;
        Scope2 *ts;
        Trial tr;
        Type *base, *t = 0;
        int dsflags = 0, eflags = 0, is_const;
        Name *en = 0;
        Ent *m = 0;
        InstState st;
        if (od->def->kind != N_FUNCDEF || !od->def->d) continue;
        declr = ool_declr(od);
        id = declarator_name_id_of(declr);
        if (!id) continue;
        cpart = ool_class_part(id);
        if (!cpart || cpart->name != cls->name) continue;
        {
            /* cheap name check */
            Node *lp = id_last(id);
            if ((lp->flags & 7) == PART_NAME && !(lp->name == cls->name && f->name == nm_ctor) && lp->name != f->name) continue;
            if ((lp->flags & 7) == PART_DTOR && f->name != nm_dtor) continue;
            if ((lp->flags & 7) == PART_NAME && f->name == nm_dtor) continue;
        }
        ts = ool_bind(od, cls, cpart);
        if (!ts) continue;
        inst_begin(&st);
        TRIAL_BEGIN(tr);
        if (setjmp(tr.jb) == 0) {
            scope2_cur = ts;
            if (specs_empty(od->def->a)) {
                base = T_VOID;
                dsflags = od->def->a->flags;
            } else base = decl_specs_type(od->def->a, &dsflags);
            t = apply_declarator(base, declr, &nid, &fn);
            en = ool_entity_name(nid, cls);
            {
                Type *conv = 0;
                int fl = 0;
                function_entity_name(nid, nid, cls, &fl, &conv);
                eflags = fl;
                if (conv) t = ty_func(conv, t->params, t->nparams, t->variadic);
            }
            is_const = fn && (fn->flags & FQ_CONST);
            m = find_member_overload(sfind(cls->ci->scope, en), t, is_const, eflags);
            trial_end_ok(&tr);
        } else {
            trial_failed(&tr);
            m = 0;
        }
        if (m == f) {
            found = 1;
            body_parent = ts;
            scope2_cur = ts;
            define_function(od->def, f, fn, t);
        }
        inst_end(&st);
    }
    return found;
}

/* the static data members of a class instance that are defined outside the class */
static void define_ool_statics(Ent *cls) {
    Tmpl *tm;
    OolDef *od;
    if (!cls->inst) return;
    tm = cls->inst->tmpl->tmpl;
    for (od = tm->ool; od; od = od->next) {
        Node *declr, *id, *cpart;
        Scope2 *ts;
        if (od->def->kind != N_DECL || ool_is_function(od) || !od->def->b) continue;
        declr = ool_declr(od);
        id = declarator_name_id_of(declr);
        if (!id) continue;
        cpart = ool_class_part(id);
        if (!cpart || cpart->name != cls->name) continue;
        ts = ool_bind(od, cls, cpart);
        if (!ts) continue;
        {
            Scope2 *saved = scope2_cur;
            Ent *old = sfind(cls->ci->scope, id_last(id)->name);
            if (!old || old->kind != EK_VAR || (old->flags & EF_DEFINED)) continue;
            scope2_cur = ts;
            process_decl(od->def);
            old->flags |= EF_DEFINED;
            scope2_cur = saved;
        }
    }
}

static void ool_registered_class(Ent *tent, OolDef *od) {
    TInst *in;
    Tmpl *tm = tent->tmpl;
    for (in = tm->insts; in; in = in->next) {
        Ent *cls = in->ent;
        ClassInfo *ci = cls->ci;
        int i;
        InstState st;
        if (!ci || !ci->complete) continue;
        inst_begin(&st);
        if (od->def->kind == N_DECL && !ool_is_function(od)) define_ool_statics(cls);
        else
            for (i = 0; i < ci->nmethods; i++) {
                Ent *f = ci->methods[i];
                if ((f->flags & (EF_VIRTUAL | EF_DTOR)) && !(f->flags & (EF_DEFINED | EF_DEFAULTED | EF_DELETED | EF_PURE | EF_IMPLICIT)) && !f->lazy) try_ool_member(f);
            }
        inst_end(&st);
    }
}

/* ------------------------------------------------------------ declarations of templates */

static int node_equal(Node *a, Node *b) {
    for (; a && b; a = a->next, b = b->next) {
        if (a->kind != b->kind || a->op != b->op || a->flags != b->flags || a->val != b->val || a->name != b->name) return 0;
        if (a->text && (!b->text || strcmp(a->text, b->text) != 0)) return 0;
        if (!a->text && b->text) return 0;
        if (!node_equal(a->a, b->a) || !node_equal(a->b, b->b)) return 0;
        if (a->kind != N_PARAM && !node_equal(a->c, b->c)) return 0;
    }
    return a == b;
}

static Tmpl *tmpl_new(int kind, Node *t, Scope2 *sc) {
    Tmpl *tm = anew(sizeof(Tmpl));
    Node *tp;
    int i = 0;
    tm->kind = kind;
    tm->tnode = t;
    tm->scope = sc;
    tm->tparams = t->a;
    for (tp = t->a; tp; tp = tp->next) tm->ntp++;
    tm->tp = anew(sizeof(Node *) * (tm->ntp + 1));
    tm->defaults = anew(sizeof(Node *) * (tm->ntp + 1));
    for (tp = t->a; tp; tp = tp->next, i++) {
        tm->tp[i] = tp;
        tm->defaults[i] = tparam_default(tp);
        if (tp->b) serr(tp, "template template parameters are not supported yet");
    }
    return tm;
}

static void tmpl_merge(Tmpl *tm, Node *t) {
    Node *tp;
    int i = 0, n = 0;
    for (tp = t->a; tp; tp = tp->next) n++;
    if (n != tm->ntp) serr(t, "redeclaration of the template with a different parameter list");
    tm->tparams = t->a;
    tm->tnode = t;
    for (tp = t->a; tp; tp = tp->next, i++) {
        Node *df = tparam_default(tp);
        tm->tp[i] = tp;
        if (df) {
            if (tm->defaults[i]) serr(tp, "redefinition of the default template argument");
            tm->defaults[i] = df;
        }
    }
}


static void register_class_template(Node *t, Node *cn, Ent *in_cls) {
    Node *lp = id_last(cn->a);
    Name *name = lp->name;
    Scope2 *sc = scope2_cur;
    Ent *e;
    int is_spec = (lp->flags & PART_ARGS) != 0;
    if (cn->a->a->next) serr(cn, "a qualified class template name is not supported yet");
    (void)in_cls;
    if (is_spec) {
        TSpec *sp;
        Tmpl *tm;
        e = lookup_unqual(scope2_cur, name);
        if (!e || e->kind != EK_TEMPLATE || e->tmpl->kind != TK_CLASS) serr(cn, "'%s' is not a class template", name->s);
        tm = e->tmpl;
        if (!(cn->flags & CL_DEFINITION)) return;
        sp = anew(sizeof(TSpec));
        sp->tnode = t;
        sp->cn = cn;
        sp->targs = lp->b;
        sp->scope = sc;
        if (t->flags & TP_SPEC) {
            sp->is_exact = 1;
            eval_targs(lp->b, &sp->exact, &sp->nexact);
            {
                TArg **full;
                int nf;
                Scope2 *ts = tscope_new(tm->scope);
                complete_args(tm, sp->exact, sp->nexact, ts, &full, &nf, cn);
                sp->exact = full;
                sp->nexact = nf;
            }
        } else sp->score = pattern_score_node(lp->b);
        sp->next = tm->specs;
        tm->specs = sp;
        return;
    }
    e = sfind(sc, name);
    if (e && e->kind != EK_TEMPLATE) serr(cn, "'%s' redeclared as a different kind of symbol", name->s);
    if (!e) {
        Tmpl *tm = tmpl_new(TK_CLASS, t, sc);
        e = ent_new(EK_TEMPLATE, name, 0, sc);
        e->tmpl = tm;
        tm->owner = in_cls;
        scope_add(sc, e);
    } else tmpl_merge(e->tmpl, t);
    if (cn->flags & CL_DEFINITION) {
        if (e->tmpl->classnode) serr(cn, "redefinition of the class template '%s'", name->s);
        e->tmpl->classnode = cn;
    }
}

static Ent *ool_target_template(Node *id) {
    Node *cp = ool_class_part(id);
    Ent *e;
    if (!cp || !(cp->flags & PART_ARGS)) return 0;
    e = lookup_unqual(scope2_cur, cp->name);
    if (e && (e->flags & EF_INJECTED) && e->tmpl_ent) e = e->tmpl_ent;
    if (e && e->kind == EK_TEMPLATE && e->tmpl->kind == TK_CLASS) return e;
    return 0;
}

static void register_function_template(Node *t, Node *d, Ent *in_cls) {
    Node *declr = d->kind == N_FUNCDEF ? d->b : (d->b ? d->b->a : 0);
    Node *id = declr ? declarator_name_id_of(declr) : 0;
    Node *fn;
    Scope2 *sc;
    Ent *e, *head, *cls = in_cls;
    Name *ename;
    int eflags = 0;
    Type *conv = 0;
    Tmpl *tm;
    if (!id) serr(d, "unsupported template declaration");
    fn = function_declarator_of(declr);
    if (id->a->next) {
        /* a member of a class template defined outside the class */
        Ent *tent = ool_target_template(id);
        if (tent) {
            OolDef *od = anew(sizeof(OolDef));
            od->tnode = t;
            od->def = d;
            od->scope = scope2_cur;
            od->next = 0;
            {
                OolDef **pp = &tent->tmpl->ool;
                while (*pp) pp = &(*pp)->next;
                *pp = od;
            }
            ool_registered_class(tent, od);
            return;
        }
    }
    if (!fn) {
        /* a variable template, or a static data member */
        Ent *tent = ool_target_template(id);
        if (tent) {
            OolDef *od = anew(sizeof(OolDef));
            od->tnode = t;
            od->def = d;
            od->scope = scope2_cur;
            {
                OolDef **pp = &tent->tmpl->ool;
                while (*pp) pp = &(*pp)->next;
                *pp = od;
            }
            ool_registered_class(tent, od);
            return;
        }
        {
            Node *lp = id_last(id);
            Tmpl *vt;
            Ent *ve;
            if (d->kind != N_DECL || !d->b || d->b->next || (lp->flags & PART_ARGS)) serr(d, "unsupported variable template");
            vt = tmpl_new(TK_VAR, t, scope2_cur);
            vt->decl = d;
            ve = ent_new(EK_TEMPLATE, lp->name, 0, scope2_cur);
            ve->tmpl = vt;
            scope_add(scope2_cur, ve);
            return;
        }
    }
    sc = in_cls ? in_cls->ci->scope : declaration_scope(id);
    ename = function_entity_name(d, id, cls, &eflags, &conv);
    if (t->flags & TP_SPEC) {
        /* an explicit specialization of a function template */
        Ent *tf;
        TArg **args = 0;
        int nargs = 0;
        Node *lp = id_last(id);
        Val *pv[32];
        int k = 0, i;
        Type *base, *ft;
        int dsflags = 0;
        Node *pn;
        head = sfind(sc, ename);
        if (!head) serr(d, "the specialization does not match a function template");
        if (lp->flags & PART_ARGS) eval_targs(lp->b, &args, &nargs);
        if (specs_empty(d->a)) base = T_VOID;
        else base = decl_specs_type(d->a, &dsflags);
        ft = apply_declarator(base, declr, 0, 0);
        for (tf = head; tf; tf = tf->overload) {
            Binds *b;
            Trial tr;
            TArg **full;
            if (tf->kind != EK_FTEMPLATE) continue;
            b = binds_new(tf->tmpl);
            if (nargs && !bind_explicit(tf->tmpl, b, args, nargs)) continue;
            k = 0;
            for (i = 0; i < ft->nparams && k < 32; i++) {
                Type *pt = ft->params[i];
                Val *v = val_new(pt, "");
                v->lval = 1;
                pv[k++] = v;
            }
            {
                Node *pfn = function_declarator_of(tmpl_function_declr(tf->tmpl));
                int ok = 1, ai = 0;
                if (!pfn) continue;
                TRIAL_BEGIN(tr);
                if (setjmp(tr.jb) == 0) {
                    for (pn = pfn->b; pn && ok; pn = pn->next) {
                        if (pn->a->val == BT_VOID && !pn->a->a && !pn->b) break;
                        if (ai >= k) break;
                        if (!deduce_param(b, pn, pv[ai++])) ok = 0;
                    }
                    trial_end_ok(&tr);
                } else {
                    trial_failed(&tr);
                    ok = 0;
                }
                if (!ok) continue;
            }
            full = finish_deduction(tf->tmpl, b, d);
            if (!full) continue;
            {
                Ent *inst;
                TInst *ti;
                for (ti = tf->tmpl->insts; ti; ti = ti->next)
                    if (targs_equal(ti->args, ti->nargs, full, tf->tmpl->ntp)) serr(d, "the function template is already instantiated for these arguments");
                inst = instantiate_function_ex(tf, full, tf->tmpl->ntp, d, d);
                (void)inst;
            }
            return;
        }
        serr(d, "the specialization does not match a function template");
    }
    head = sfind(sc, ename);
    if (head && head->kind != EK_FUNC && head->kind != EK_FTEMPLATE) serr(d, "'%s' redeclared as a different kind of symbol", ename->s);
    /* the same template declared before? */
    for (e = head; e; e = e->overload) {
        Node *odeclr;
        if (e->kind != EK_FTEMPLATE) continue;
        tm = e->tmpl;
        odeclr = tmpl_function_declr(tm);
        if (!odeclr || tm->ntp != 0) {
            Node *tp;
            int n = 0;
            for (tp = t->a; tp; tp = tp->next) n++;
            if (n != tm->ntp) continue;
        }
        {
            Node *of = function_declarator_of(odeclr);
            if (of && fn && node_equal(of->b, fn->b) && node_equal(tm->decl->a, d->a) && !!(of->flags & FQ_CONST) == !!(fn->flags & FQ_CONST)) {
                if (d->kind == N_FUNCDEF && d->d) {
                    if (tm->decl->kind == N_FUNCDEF && tm->decl->d) serr(d, "redefinition of the function template '%s'", ename->s);
                    tm->decl = d;
                    tmpl_merge(tm, t);
                    {
                        TInst *in;
                        for (in = tm->insts; in; in = in->next)
                            if (!in->ent->lazy && !(in->ent->flags & EF_DEFINED)) lazy_set(in->ent, d, in->fnode, in->ft, 0);
                    }
                } else {
                    Node *tp;
                    int i2 = 0;
                    for (tp = t->a; tp; tp = tp->next, i2++) {
                        Node *df = tparam_default(tp);
                        if (df && i2 < tm->ntp) {
                            if (tm->defaults[i2]) serr(tp, "redefinition of the default template argument");
                            tm->defaults[i2] = df;
                        }
                    }
                }
                return;
            }
        }
    }
    tm = tmpl_new(TK_FUNC, t, sc);
    tm->decl = d;
    tm->owner = in_cls;
    e = ent_new(EK_FTEMPLATE, ename, ty_func(T_VOID, 0, 0, 0), sc);
    e->flags |= eflags;
    e->cname = "";
    e->tmpl = tm;
    e->defaults = anew(sizeof(Node *) * 2);
    if (in_cls) {
        e->flags |= EF_MEMBER;
        e->owner = in_cls;
        e->access = tmpl_access;
    }
    scope_add(sc, e);
}

static void process_alias_template(Node *t, Node *d) {
    Tmpl *tm;
    Ent *e;
    tm = tmpl_new(TK_ALIAS, t, scope2_cur);
    tm->alias = d;
    e = ent_new(EK_TEMPLATE, d->name, 0, scope2_cur);
    e->tmpl = tm;
    scope_add(scope2_cur, e);
}

static void explicit_instantiation(Node *t) {
    Node *d = t->a;
    if (d && d->kind == N_DECL && d->a && d->a->a && d->a->a->kind == N_CLASS) {
        Node *cn = d->a->a;
        Ent *e = resolve_id(cn->a, 1);
        if (e->kind == EK_CLASS && e->ci) {
            int i;
            for (i = 0; i < e->ci->nmethods; i++) {
                Ent *f = e->ci->methods[i];
                if (!(f->flags & (EF_DELETED | EF_PURE))) use_function(f);
            }
        }
        return;
    }
    if (d && d->kind == N_DECL && d->b) {
        Node *id = declarator_name_id_of(d->b->a);
        Node *lp = id ? id_last(id) : 0;
        if (lp && (lp->flags & PART_ARGS)) {
            Ent *e = resolve_id(id, 1);
            for (; e; e = e->overload)
                if (e->kind == EK_FUNC) use_function(e);
            return;
        }
    }
}

static void process_template(Node *t, Ent *in_cls) {
    Node *d = t->b;
    if (t->kind == N_INSTANTIATION) {
        explicit_instantiation(t);
        return;
    }
    if (!d) return;
    if (d->kind == N_TEMPLATE) serr(t, "member templates of class templates defined outside the class are not supported yet");
    if (d->kind == N_DECL && d->a && (d->a->flags & DS_FRIEND)) {
        /* a friend template: every instance of a class template is a friend */
        if (in_cls && in_cls->ci && d->a->a && d->a->a->kind == N_CLASS && d->a->a->a && !d->b) {
            Ent *te = template_of_id(d->a->a->a);
            if (te) {
                ClassInfo *ci = in_cls->ci;
                ci->friend_tmpls = xrealloc(ci->friend_tmpls, sizeof(Ent *) * (ci->nfriend_tmpls + 1));
                ci->friend_tmpls[ci->nfriend_tmpls++] = te;
            }
        }
        return;
    }
    if (d->kind == N_FUNCDEF && d->a && (d->a->flags & DS_FRIEND)) return;
    if (d->kind == N_DECL && d->a && d->a->a && d->a->a->kind == N_CLASS && !d->b) {
        Node *cn = d->a->a;
        if (!cn->a) serr(t, "an anonymous class cannot be a template");
        register_class_template(t, cn, in_cls);
        return;
    }
    if (d->kind == N_FUNCDEF || d->kind == N_DECL) {
        register_function_template(t, d, in_cls);
        return;
    }
    if (d->kind == N_ALIAS) {
        process_alias_template(t, d);
        return;
    }
    if (d->kind == N_FRIEND) serr(t, "friend templates are not supported yet");
    serr(t, "unsupported template declaration");
}

/* the type std::initializer_list<E> for the elements of a braced list (auto x = {...}, for (x : {...})) */
static Type *braced_il_type(Val *bv, Node *at) {
    Ent *std_ns = lookup_in(scope2_global, intern_str("std"), 0), *te;
    Type *e;
    TArg *a[1];
    if (!bv->nbvals) serr(at, "cannot deduce the type of an empty braced list");
    te = std_ns && std_ns->kind == EK_NS ? lookup_in(std_ns->members, intern_str("initializer_list"), 0) : 0;
    if (!te || te->kind != EK_TEMPLATE) serr(at, "you have to #include <initializer_list> to use a braced list here");
    e = deduce_auto(auto_type(), bv->bvals[0], at);
    a[0] = ta_type(e);
    return instantiate_class(te, a, 1, at)->type;
}
