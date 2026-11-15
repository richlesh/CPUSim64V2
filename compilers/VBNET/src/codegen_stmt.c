/*
 * codegen_stmt.c - code generation: statements, methods and the program.
 *
 * The program is one .asm file: it includes the run-time library (a linker
 * module), defines vb_program() (called by the library's main()), which
 * runs the field initializers and Main, then the methods, the error
 * routines, the constants and string literals, and the #global variables
 * (the module fields, Static locals and their "initialized" flags).
 */

static int ntrys;            /* the Try statements around the code (see "exceptions") */
static void set_depth(int label, int depth);
static void cg_branch(int label);
static int cg_nglobals;
int opt_gc_stress = 0;      /* --gc-stress: a collection at every allocation */     /* global words: prog_fields, then Static flags */
static int cg_src_file = -1, cg_src_line = -1;

static int method_label(Sym *m) {
    static int next = 0;
    if (!m->index) m->index = ++next;
    return m->index;
}

/* ------------------------------------------------------------ .source lines */

static const char *dir_of(const char *p) {
    StrBuf b;
    const char *s = p, *last = 0;
    for (; *s; s++) if (*s == '/') last = s;
    sb_init(&b);
    if (last) for (s = p; s < last; s++) sb_putc(&b, *s);
    return b.s;
}

static const char *abs_path(const char *p) {
    const char *cwd = getenv("PWD");
    StrBuf b;
    if (p[0] == '/' || !cwd) return p;
    sb_init(&b);
    sb_puts(&b, cwd);
    sb_putc(&b, '/');
    sb_puts(&b, p);
    return b.s;
}

/* the name of a source file for a .source directive: relative to the .asm
   file when it is in the same directory, else absolute */
static const char *source_name(int f) {
    const char *p = files[f].path, *s, *base = p;
    for (s = p; *s; s++) if (*s == '/') base = s + 1;
    if (strcmp(dir_of(abs_path(p)), cg_asm_dir) == 0) return base;
    return abs_path(p);
}

static void cg_source(Node *s) {
    if (s->file >= nuser_files) return;     /* library code keeps its assembly lines */
    if (s->file == cg_src_file && s->line == cg_src_line) return;
    cg_src_file = s->file;
    cg_src_line = s->line;
    cg_pend_src = source_name(s->file);
    cg_pend_line = s->line;
}

/* ------------------------------------------------------------ assignments */

/* stores r0 / f0 into a variable expression (a name or an array element) */
static void store_lv(Node *v) {
    if (v->kind == N_NAME) {
        store_var(v->sym);
        return;
    }
    push_val(v->type);
    cg_addr(v);
    emit("move    r1, r0");
    emit(is_f(v->type) ? "pop     f0" : "pop     r0");
    emit(is_f(v->type) ? "store   f0, r1[0]" : "store   r0, r1[0]");
}

/* target = value; the value may use N_TARGET for the target's value */
static void cg_assign(Node *target, Node *value) {
    int sv_mode = tg_mode, sv_slot = tg_slot;
    Sym *sv_var = tg_var;
    if (target->kind == N_NAME) {
        tg_mode = TG_VAR;
        tg_var = target->sym;
        cg_value(value);
        store_var(target->sym);
    } else {
        int slot = alloc_temp();
        cg_addr(target);
        emit("store   r0, sf[%d]", -slot);
        tg_mode = TG_ADDR;
        tg_slot = slot;
        cg_value(value);
        emit("load    r1, sf[%d]", -slot);
        emit(is_f(target->type) ? "store   f0, r1[0]" : "store   r0, r1[0]");
        free_temp();
    }
    tg_mode = sv_mode;
    tg_slot = sv_slot;
    tg_var = sv_var;
}

static void cg_block(NodeVec *v);

/* ------------------------------------------------------------ declarations */

static void cg_local(Node *st) {
    int i, j;
    for (i = 0; i < st->list->n; i++) {
        Node *d = st->list->v[i];
        for (j = 0; j < d->list->n; j++) {
            Node *vi = d->list->v[j];
            Sym *s = vi->sym;
            int skip = 0;
            if (!s || s->kind == S_CONST || (!vi->b && !vi->list)) continue;
            if (s->mods & M_STATIC) {
                /* initialized the first time only */
                int flag = cg_nglobals++;
                skip = new_label();
                emit("load    r0, __vbG%d+1", flag);
                emit("test    r0");
                emit("jump    nz, __vbL%d", skip);
                emit("move    r0, -1");
                emit("store   r0, __vbG%d+1", flag);
            }
            if (vi->b) cg_value(vi->b);
            else cg_array(s->type, vi->list, 0);
            store_var(s);
            if (skip) emit_label(skip);
        }
    }
}

/* ------------------------------------------------------------ loops */

/* jumps to target when r0 (f0) compared with r1 (f1) by op is true */
static void jump_if(int op, Type *t, int target) {
    compare(op, t);
    emit("test    r0");
    emit("jump    nz, __vbL%d", target);
}

static int const_negative(Node *c) {
    if (c->kind == N_FLOAT) return c->dval < 0;
    if (c->kind == N_INT) return ty_signed(c->type) && (i64)c->ival < 0;
    return 0;
}

static void load_slot(int slot, Type *t, int reg) {
    emit(is_f(t) ? "load    f%d, sf[%d]" : "load    r%d, sf[%d]", reg, -slot);
}

static void cg_for1(Node *st);

static void cg_for(Node *st) {
    int env = st->scope && st->scope->nenv;
    if (env) env_enter(st->scope, &cg_captured);
    cg_for1(st);
    if (env) env_leave();
}

static void cg_for1(Node *st) {
    Node *var = st->a;
    Type *t = var->type;
    int to = alloc_temp(), step = -1, top = new_label(), cont = new_label(), end = new_label();
    int cstep = !st->d || is_const(st->d), neg = st->d && cstep && const_negative(st->d);
    st->aux = end;
    st->ival = cont;
    set_depth(end, ntrys);
    set_depth(cont, ntrys);
    cg_expr(st->b);
    store_lv(var);
    cg_expr(st->c);
    emit(is_f(t) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -to);
    if (st->d && !cstep) {
        step = alloc_temp();
        cg_expr(st->d);
        emit(is_f(t) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -step);
    }
    emit_label(top);
    cg_expr(var);
    load_slot(to, t, 1);
    if (cstep) jump_if(neg ? P_LT : P_GT, t, end);
    else {
        int l_neg = new_label(), l_body = new_label();
        /* the direction of a variable step */
        if (is_f(t)) {
            emit("push    f0");
            load_slot(step, t, 2);
            emit("move    r2, 0");
            emit("move    f3, r2");
            emit("cmp     f2, f3");
            emit("pop     f0");
        } else {
            emit("push    r0");
            load_slot(step, t, 2);
            emit("test    r2");
            emit("pop     r0");
        }
        emit("jump    n, __vbL%d", l_neg);
        jump_if(P_GT, t, end);
        emit("jump    __vbL%d", l_body);
        emit_label(l_neg);
        cg_expr(var);
        load_slot(to, t, 1);
        jump_if(P_LT, t, end);
        emit_label(l_body);
    }
    cg_block(st->list);
    emit_label(cont);
    cg_source(st);
    cg_expr(var);
    if (step >= 0) load_slot(step, t, 1);
    else if (is_f(t)) {
        emit("push    f0");
        if (st->d) load_double(0, st->d->dval);
        else load_double(0, 1.0);
        emit("move    f1, f0");
        emit("pop     f0");
    } else load_int(1, st->d ? (i64)st->d->ival : 1);
    if (is_f(t)) {
        emit("add     f0, f1");
        if (t->kind == TY_SINGLE) to_single();
    } else if (t->kind == TY_ULONG) ulong_arith(P_PLUS);
    else int_arith(P_PLUS, t);
    store_lv(var);
    emit("jump    __vbL%d", top);
    emit_label(end);
    if (step >= 0) free_temp();
    free_temp();
}

/* For Each with an enumerator: MoveNext (d), Current (b) */
static void cg_foreach_enum(Node *st) {
    int el = alloc_temp(), top = new_label(), end = new_label();
    int sv_mode = tg_mode, sv_slot = tg_slot;
    st->aux = end;
    st->ival = top;
    set_depth(end, ntrys);
    set_depth(top, ntrys);
    emit_label(top);
    cg_expr(st->d);
    emit("test    r0");
    emit("jump    z, __vbL%d", end);
    cg_expr(st->b);
    emit(is_f(st->b->type) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -el);
    tg_mode = TG_SLOT;
    tg_slot = el;
    cg_expr(st->c);
    if (st->a->type->kind == TY_STRUCT) {
        emit("push    r0");
        emit("call    __c_vbrt_struct_copy");
        emit("add     sp, 1");
    }
    tg_mode = sv_mode;
    tg_slot = sv_slot;
    store_lv(st->a);
    cg_block(st->list);
    emit("jump    __vbL%d", top);
    emit_label(end);
    free_temp();
}

static void cg_foreach1(Node *st);

static void cg_foreach(Node *st) {
    int env = st->scope && st->scope->nenv;
    if (env) env_enter(st->scope, &cg_captured);
    cg_foreach1(st);
    if (env) env_leave();
}

static void cg_foreach1(Node *st) {
    Type *ct = st->b->type, *et = st->c->a ? st->c->a->type : 0;
    if (st->flags & F_ENUM) {
        cg_foreach_enum(st);
        return;
    }
    int arr = alloc_temp(), idx = alloc_temp(), el = alloc_temp();
    int top = new_label(), cont = new_label(), end = new_label();
    int base = ct->kind == TY_STRING ? 1 : 2 + ct->rank;
    int sv_mode = tg_mode, sv_slot = tg_slot;
    (void)et;
    st->aux = end;
    st->ival = cont;
    set_depth(end, ntrys);
    set_depth(cont, ntrys);
    cg_expr(st->b);
    emit("store   r0, sf[%d]", -arr);
    emit("test    r0");
    rt_throw_jump("z", "null");
    emit("move    r0, 0");
    emit("store   r0, sf[%d]", -idx);
    emit_label(top);
    emit("load    r0, sf[%d]", -idx);
    emit("load    r1, sf[%d]", -arr);
    emit("load    r2, r1[0]");
    emit("cmp     r0, r2");
    emit("jump    ge, __vbL%d", end);
    emit("add     r1, r1, r0");
    {
        Type *elem = ct->kind == TY_STRING ? ty[TY_CHAR] : ct->elem;
        emit(is_f(elem) ? "load    f0, r1[%d]" : "load    r0, r1[%d]", base);
        emit(is_f(elem) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -el);
    }
    tg_mode = TG_SLOT;
    tg_slot = el;
    cg_expr(st->c);
    if (st->a->type->kind == TY_STRUCT) {
        emit("push    r0");
        emit("call    __c_vbrt_struct_copy");
        emit("add     sp, 1");
    }
    tg_mode = sv_mode;
    tg_slot = sv_slot;
    store_lv(st->a);
    cg_block(st->list);
    emit_label(cont);
    emit("load    r0, sf[%d]", -idx);
    emit("add     r0, 1");
    emit("store   r0, sf[%d]", -idx);
    emit("jump    __vbL%d", top);
    emit_label(end);
    free_temp();
    free_temp();
    free_temp();
}

static void cg_select(Node *st) {
    int slot = alloc_temp(), end = new_label(), i, j, sv = sel_slot;
    st->aux = end;
    set_depth(end, ntrys);
    cg_expr(st->a);
    emit(is_f(st->a->type) ? "store   f0, sf[%d]" : "store   r0, sf[%d]", -slot);
    for (i = 0; i < st->list->n; i++) {
        Node *cs = st->list->v[i];
        if (cs->flags & F_ELSE) {
            cg_block(cs->list2);
            break;
        } else {
            int body = new_label(), next = new_label();
            sel_slot = slot;
            for (j = 0; j < cs->list->n; j++) {
                Node *cl = cs->list->v[j];
                cg_source(cl);
                cg_expr(cl->a);
                emit("test    r0");
                emit("jump    nz, __vbL%d", body);
            }
            sel_slot = sv;
            emit("jump    __vbL%d", next);
            emit_label(body);
            cg_block(cs->list2);
            emit("jump    __vbL%d", end);
            emit_label(next);
        }
    }
    emit_label(end);
    sel_slot = sv;
    free_temp();
}

static int label_of(Node *l) {
    if (!l->aux) l->aux = new_label();
    return l->aux;
}

/* ------------------------------------------------------------ exceptions */

/* A Try pushes a handler record of 4 words onto the chain __vb_handler:
   rec[0] the address of its Catch code, rec[1] SP, rec[2] SF, rec[3] the
   previous record.  __vb_throw (r0 = the exception) pops the innermost
   record, restores SP and SF, and jumps to its Catch code.  Leaving a Try
   (Exit, Continue, Return, GoTo) pops the records it crosses and runs the
   Finally blocks: the jump sets the Try's reason (2, 3, ...: one for each
   target), the Finally block ends with a dispatch on it (1: rethrow the
   pending exception). */

enum { TS_BODY, TS_CATCH, TS_FINALLY };

typedef struct {
    Node *node;
    int rec;            /* the first of the 4 temporaries of the record */
    int has_finally;
    int state;          /* TS_BODY: the record is active; TS_CATCH: the catch-region record if has_finally */
    int reason, pend, exc;
    int fin_label;
    int nexits;
    int exit_label[64], exit_depth[64];
} TryLv;

static TryLv trys[64];
static int ntrys;
static int *label_depth;    /* the Try depth of each label */
static int label_depth_cap;
static int cur_exc = -1;    /* the slot of the exception of the innermost Catch */

static void set_depth(int label, int depth) {
    if (label >= label_depth_cap) {
        int n = label_depth_cap ? label_depth_cap : 256;
        while (n <= label) n *= 2;
        label_depth = xrealloc(label_depth, sizeof(int) * n);
        memset(label_depth + label_depth_cap, 0, sizeof(int) * (n - label_depth_cap));
        label_depth_cap = n;
    }
    label_depth[label] = depth;
}

static int depth_of(int label) { return label < label_depth_cap ? label_depth[label] : 0; }

static void push_record(TryLv *lv, int catch_label) {
    int k = lv->rec;
    emit("move    r1, __vbL%d", catch_label);
    emit("store   r1, sf[%d]", -(k + 3));
    emit("store   sp, sf[%d]", -(k + 2));
    emit("store   sf, sf[%d]", -(k + 1));
    emit("load    r1, __vb_handler+1");
    emit("store   r1, sf[%d]", -k);
    emit("add     r1, sf, %d", -(k + 3));
    emit("store   r1, __vb_handler+1");
}

static void pop_record(TryLv *lv) {
    emit("load    r1, sf[%d]", -lv->rec);
    emit("store   r1, __vb_handler+1");
}

/* a jump to the label from inside Try statements: through their Finally blocks */
static void cg_branch(int label) {
    int target = depth_of(label), i;
    for (i = ntrys - 1; i >= target; i--) {
        TryLv *lv = &trys[i];
        if (lv->state == TS_BODY || (lv->state == TS_CATCH && lv->has_finally)) pop_record(lv);
        if (lv->has_finally && lv->state != TS_FINALLY) {
            int k;
            for (k = 0; k < lv->nexits && lv->exit_label[k] != label; k++) ;
            if (k == lv->nexits) {
                if (k == 64) cg_fail(lv->node, "More than 64 jumps out of one 'Try' are");
                lv->exit_label[k] = label;
                lv->nexits++;
            }
            emit("move    r0, %d", 2 + k);
            emit("store   r0, sf[%d]", -lv->reason);
            emit("jump    __vbL%d", lv->fin_label);
            return;
        }
    }
    emit("jump    __vbL%d", label);
}

static void cg_try(Node *st) {
    TryLv *lv;
    int l_catch = new_label(), l_fault = new_label(), l_end = new_label(), i, k, sv_exc = cur_exc;
    int depth = ntrys;
    if (ntrys == 64) cg_fail(st, "'Try' statements nested more than 64 deep are");
    lv = &trys[ntrys++];
    memset(lv, 0, sizeof *lv);
    lv->node = st;
    lv->rec = alloc_temp();
    alloc_temp();
    alloc_temp();
    alloc_temp();
    lv->rec = cg_method->locals.n + cg_ntemps - 4;
    lv->exc = alloc_temp();
    lv->has_finally = st->list3 != 0;
    if (lv->has_finally) {
        lv->reason = alloc_temp();
        lv->pend = alloc_temp();
        lv->fin_label = new_label();
        emit("move    r0, 0");
        emit("store   r0, sf[%d]", -lv->reason);
    }
    st->aux = l_end;                /* Exit Try */
    set_depth(l_end, depth);
    lv->state = TS_BODY;
    push_record(lv, l_catch);
    cg_block(st->list);
    pop_record(lv);
    emit("jump    __vbL%d", lv->has_finally ? lv->fin_label : l_end);
    /* the Catch blocks: r0 is the exception */
    emit_label(l_catch);
    emit("store   r0, sf[%d]", -lv->exc);
    lv->state = TS_CATCH;
    for (i = 0; i < st->list2->n; i++) {
        Node *cn = st->list2->v[i];
        int next = new_label();
        cg_source(cn);
        if (cn->type->sym->owner != ns_path("System") || cn->type->sym->name != intern_word("exception")) {
            emit("load    r0, sf[%d]", -lv->exc);
            emit("push    r0");
            emit("move    r0, __vbD%d+1", cn->type->sym->desc);
            emit("push    r0");
            rt_call("isinst", 2);
            emit("test    r0");
            emit("jump    z, __vbL%d", next);
        }
        if (cn->sym) {
            emit("load    r0, sf[%d]", -lv->exc);
            store_var(cn->sym);
        }
        if (cn->b) {
            cg_expr(cn->b);
            emit("test    r0");
            emit("jump    z, __vbL%d", next);
        }
        if (lv->has_finally) push_record(lv, l_fault);
        cur_exc = lv->exc;
        cg_block(cn->list);
        cur_exc = sv_exc;
        if (lv->has_finally) pop_record(lv);
        emit("jump    __vbL%d", lv->has_finally ? lv->fin_label : l_end);
        emit_label(next);
    }
    /* no Catch for it: on to the next handler (after the Finally) */
    emit("load    r0, sf[%d]", -lv->exc);
    if (lv->has_finally) {
        emit("store   r0, sf[%d]", -lv->pend);
        emit("move    r0, 1");
        emit("store   r0, sf[%d]", -lv->reason);
        emit("jump    __vbL%d", lv->fin_label);
        /* an exception in a Catch block */
        emit_label(l_fault);
        emit("store   r0, sf[%d]", -lv->pend);
        emit("move    r0, 1");
        emit("store   r0, sf[%d]", -lv->reason);
        /* the Finally block, then the dispatch on the reason */
        emit_label(lv->fin_label);
        lv->state = TS_FINALLY;
        cg_block(st->list3);
        emit("load    r0, sf[%d]", -lv->reason);
        emit("cmp     r0, 1");
        emit("jump    ne, __vbL%d", l_fault = new_label());
        emit("load    r0, sf[%d]", -lv->pend);
        emit("jump    __vb_throw");
        emit_label(l_fault);
        {
            int nx = lv->nexits, labels[64];
            for (k = 0; k < nx; k++) labels[k] = lv->exit_label[k];
            ntrys--;                /* the jumps continue outside this Try */
            for (k = 0; k < nx; k++) {
                int skip = new_label();
                emit("load    r0, sf[%d]", -lv->reason);
                emit("cmp     r0, %d", 2 + k);
                emit("jump    ne, __vbL%d", skip);
                cg_branch(labels[k]);
                emit_label(skip);
            }
            ntrys++;
        }
    } else emit("jump    __vb_throw");
    emit_label(l_end);
    ntrys--;
    cg_ntemps -= lv->has_finally ? 7 : 5;
}

static void cg_throw(Node *st) {
    if (st->a) {
        cg_expr(st->a);
        emit("test    r0");
        rt_throw_jump("z", "null");
    } else emit("load    r0, sf[%d]", -cur_exc);
    emit("jump    __vb_throw");
}

/* the Try depth of the labels of a method (GoTo targets) */
static void label_depths(NodeVec *v, int depth) {
    int i, j;
    if (!v) return;
    for (i = 0; i < v->n; i++) {
        Node *s = v->v[i];
        switch (s->kind) {
        case N_LABEL: set_depth(label_of(s), depth); break;
        case N_TRY:
            label_depths(s->list, depth + 1);
            for (j = 0; j < s->list2->n; j++) label_depths(s->list2->v[j]->list, depth + 1);
            label_depths(s->list3, depth + 1);
            break;
        case N_IF:
            label_depths(s->list, depth);
            if (s->list2) for (j = 0; j < s->list2->n; j++) label_depths(s->list2->v[j]->list, depth);
            label_depths(s->list3, depth);
            break;
        case N_SELECT:
            for (j = 0; j < s->list->n; j++) label_depths(s->list->v[j]->list2, depth);
            break;
        case N_WHILE: case N_DO: case N_FOR: case N_FOREACH: case N_BLOCK:
            label_depths(s->list, depth);
            break;
        }
    }
}

/* ------------------------------------------------------------ statements */


static void cg_cond_jump(Node *c, int if_false) {
    cg_expr(c);
    emit("test    r0");
    emit("jump    z, __vbL%d", if_false);
}

static void cg_stmt(Node *s) {
    int i;
    if (s->kind != N_LABEL) cg_source(s);
    switch (s->kind) {
    case N_LOCAL: cg_local(s); return;
    case N_ASSIGN:
        if (s->flags & F_DIRECT) {
            /* With on a structure variable: the same box */
            cg_expr(s->b);
            store_lv(s->a);
            return;
        }
        cg_assign(s->a, s->b);
        return;
    case N_CALLSTMT: cg_expr(s->a); return;
    case N_IF: {
        int end = new_label(), next = new_label();
        cg_cond_jump(s->a, next);
        cg_block(s->list);
        emit("jump    __vbL%d", end);
        emit_label(next);
        if (s->list2)
            for (i = 0; i < s->list2->n; i++) {
                Node *ei = s->list2->v[i];
                next = new_label();
                cg_source(ei);
                cg_cond_jump(ei->a, next);
                cg_block(ei->list);
                emit("jump    __vbL%d", end);
                emit_label(next);
            }
        if (s->list3) cg_block(s->list3);
        emit_label(end);
        return;
    }
    case N_SELECT: cg_select(s); return;
    case N_WHILE: {
        int top = new_label(), end = new_label();
        s->aux = end;
        s->ival = top;
        set_depth(end, ntrys);
        set_depth(top, ntrys);
        emit_label(top);
        cg_cond_jump(s->a, end);
        cg_block(s->list);
        emit("jump    __vbL%d", top);
        emit_label(end);
        return;
    }
    case N_DO: {
        int top = new_label(), cont = new_label(), end = new_label();
        s->aux = end;
        s->ival = cont;
        set_depth(end, ntrys);
        set_depth(cont, ntrys);
        emit_label(top);
        if (s->flags & F_TOP) {
            cg_expr(s->a);
            emit("test    r0");
            emit("jump    %s, __vbL%d", (s->flags & F_WHILE) ? "z" : "nz", end);
        }
        cg_block(s->list);
        emit_label(cont);
        if (s->flags & F_BOTTOM) {
            cg_source(s->a);
            cg_expr(s->a);
            emit("test    r0");
            emit("jump    %s, __vbL%d", (s->flags & F_WHILE) ? "nz" : "z", top);
        } else emit("jump    __vbL%d", top);
        emit_label(end);
        return;
    }
    case N_FOR: cg_for(s); return;
    case N_FOREACH: cg_foreach(s); return;
    case N_EXIT:
        if (s->op == K_Sub || s->op == K_Function || s->op == K_Property) cg_branch(cg_epilogue);
        else cg_branch((int)s->c->aux);
        return;
    case N_CONTINUE:
        cg_branch((int)s->c->ival);
        return;
    case N_RETURN:
        if (s->a) {
            cg_value(s->a);
            store_var(cg_method->retval);
        }
        cg_branch(cg_epilogue);
        return;
    case N_GOTO:
        cg_branch(label_of(s->c));
        return;
    case N_TRY: cg_try(s); return;
    case N_BLOCK: cg_block(s->list); return;
    case N_THROW: cg_throw(s); return;
    case N_LABEL:
        emit_label(label_of(s));
        return;
    case N_END: case N_STOP:
        emit("call    __c_vbrt_end");
        return;
    case N_REDIM:
        for (i = 0; i < s->list->n; i++) {
            Node *c = s->list->v[i];
            int k, rank = c->type->rank;
            if (s->flags & F_PRESERVE) cg_expr(c->a);
            else emit("move    r0, 0");
            emit("push    r0");
            emit("move    r0, %d", (s->flags & F_PRESERVE) ? 1 : 0);
            emit("push    r0");
            emit("move    r0, %d", array_tag(c->type));
            emit("push    r0");
            emit("move    r0, %d", rank);
            emit("push    r0");
            for (k = 0; k < rank; k++) {
                cg_expr(c->list->v[k]);
                emit("push    r0");
            }
            rt_call("redim", 4 + rank);
            if (c->type->elem->kind == TY_STRUCT) {
                emit("push    r0");
                emit("move    r1, __vbD%d+1", c->type->elem->sym->desc);
                emit("push    r1");
                emit("load    r1, sp[2]");
                emit("push    r1");
                emit("call    __c_vbrt_array_fill");
                emit("add     sp, 2");
                emit("pop     r0");
            }
            store_lv(c->a);
        }
        return;
    case N_ERASE:
        for (i = 0; i < s->list->n; i++) {
            emit("move    r0, 0");
            store_lv(s->list->v[i]);
        }
        return;
    }
    cg_fail(s, node_names[s->kind]);
}

static void cg_block(NodeVec *v) {
    int i, env = v && v->scope && v->scope->nenv;
    if (!v) return;
    if (env) env_enter(v->scope, &cg_captured);
    for (i = 0; i < v->n; i++) cg_stmt(v->v[i]);
    if (env) env_leave();
}

/* the environment slots of the variables that lambdas capture */
static void env_layout(Sym *m) {
    int i;
    SymVec all;
    memset(&all, 0, sizeof all);
    cg_captured.n = 0;
    for (i = 0; i < m->params.n; i++) sv_push(&all, m->params.v[i]);
    for (i = 0; i < m->locals.n; i++) sv_push(&all, m->locals.v[i]);
    if (m->me_sym) sv_push(&all, m->me_sym);
    for (i = 0; i < all.n; i++) {
        Sym *s = all.v[i];
        if (!(s->sflags & SF_CAPTURED)) continue;
        if (!s->scope || !s->scope->materialized) s->scope = m->scope;
        s->envslot = ++s->scope->nenv;
        sv_push(&cg_captured, s);
    }
    free(all.v);
}

/* ------------------------------------------------------------ methods */

static void begin_method(Sym *m) {
    cg_method = m;
    cg_ntemps = cg_maxtemps = 0;
    sb_init(&cg_code);
    cg_epilogue = new_label();
    set_depth(cg_epilogue, 0);
    ntrys = 0;
    label_depths(m->body, 0);
    cg_src_file = cg_src_line = -1;
}

/* writes the method: label, prologue (frame, zeroed), code, epilogue */
static void end_method(const char *label, const char *comment, Node *decl) {
    int frame = cg_method->locals.n + cg_maxtemps;
    fprintf(cg_out, "\n// %s\n%s:\n", comment, label);
    if (decl && decl->file >= nuser_files) decl = 0;
    if (decl && frame) fprintf(cg_out, ".source \"%s\", %d\n", source_name(decl->file), decl->line);
    /* the locals and temporaries, zeroed (iMEMCLEAR only clears the heap) */
    if (frame > 0 && frame <= 8) {
        int i;
        for (i = 0; i < frame; i++) fprintf(cg_out, "    push    0\n");
    } else if (frame > 8) {
        int l = new_label();
        fprintf(cg_out, "    move    r1, %d\n__vbL%d:\n    push    0\n    sub     r1, 1\n    jump    nz, __vbL%d\n",
                frame, l, l);
    }
    {
        /* the locals of structure types: their zero values */
        int i;
        for (i = 0; i < cg_method->locals.n; i++) {
            Sym *l = cg_method->locals.v[i];
            if (l->type && l->type->kind == TY_STRUCT && !(l->mods & M_STATIC)) {
                fprintf(cg_out, "    move    r0, __vbD%d+1\n    push    r0\n    call    __c_vbrt_struct_new\n", l->type->sym->desc);
                fprintf(cg_out, "    add     sp, 1\n    store   r0, sf[%d]\n", -l->index);
            }
        }
    }
    cg_pend_src = 0;
    fputs(cg_code.s, cg_out);
    fprintf(cg_out, "__vbL%d:\n", cg_epilogue);
    if (cg_method->retval) {
        Sym *r = cg_method->retval;
        fprintf(cg_out, "    load    %s, sf[%d]\n", is_f(r->type) ? "f0" : "r0", -r->index);
    }
    fprintf(cg_out, "    return\n");
    if (decl) fprintf(cg_out, ".source_end\n");
    free(cg_code.s);
}

static void gen_method(Sym *m) {
    char label[40], comment[200];
    int menv;
    begin_method(m);
    cg_nenvs = 0;
    env_layout(m);
    menv = m->scope && m->scope->nenv;
    if (menv) env_enter(m->scope, &cg_captured);
    if ((m->sflags & SF_CTOR) && (m->sflags & SF_INSTANCE)) {
        /* MyBase.New / Me.New (explicit or implicit), then the field initializers */
        Node *first = (m->body && m->body->n) ? m->body->v[0] : 0;
        int skip = 0, init = 1;
        if (first && first->kind == N_CALLSTMT && (first->a->flags & F_DIRECT) && first->a->kind == N_NOTHING) skip = 1;
        else if (first && first->kind == N_CALLSTMT && first->a->kind == N_INVOKE && first->a->sym &&
                 (first->a->sym->sflags & SF_CTOR)) {
            cg_stmt(first);
            skip = 1;
            if (first->a->sym->owner == m->owner) init = 0;     /* Me.New ran them */
        } else if (m->cval) {
            cg_expr(m->cval);
        }
        if (init && (m->owner->initm->sflags & SF_LIVE)) {
            load_me();
            emit("push    r0");
            emit("call    __vbM%d", method_label(m->owner->initm));
            emit("add     sp, 1");
        }
        if (skip) {
            int k;
            for (k = 1; k < m->body->n; k++) cg_stmt(m->body->v[k]);
        } else cg_block(m->body);
    } else cg_block(m->body);
    if (menv) env_leave();
    snprintf(label, sizeof label, "__vbM%d", method_label(m));
    snprintf(comment, sizeof comment, "%s.%s", qual_name(m->owner), m->spelling);
    end_method(label, comment, (m->decl && m->decl->kind != N_CLASS && m->decl->kind != N_STRUCTURE) ? m->decl : 0);
}

/* ------------------------------------------------------------ type descriptors */

/* the number of fields of structure types */
static int nested_structs(Sym *t) {
    int k, n = 0;
    for (k = 0; k < t->fields.n; k++) if (t->fields.v[k]->type->kind == TY_STRUCT) n++;
    return n;
}

/* the instance fields that hold references (for the garbage collector),
   with those of the base classes; their slots into out (or just counted) */
static int is_gc_ref(Type *t) {
    return ty_ref(t) || t->kind == TY_STRUCT;
}

static int ref_fields(Sym *t, int *out) {
    int n = 0, k;
    Sym *c;
    for (c = t; c; c = c->base)
        for (k = 0; k < c->fields.n; k++)
            if (is_gc_ref(c->fields.v[k]->type)) {
                if (out) out[n] = c->fields.v[k]->index;
                n++;
            }
    return n;
}

/* the words of a descriptor (see rt/vb_rt.c) */
static int desc_words(Sym *t) {
    int k, n;
    if (t->type->kind == TY_ENUM) return 6 + 2 * t->fields.n;
    if (t->type->kind == TY_INTERFACE) return 11;
    n = 11 + 2 * t->all_ifaces.n + 2 * nested_structs(t) + t->vtab.n + ref_fields(t, 0);
    for (k = 0; k < t->all_ifaces.n; k++) n += t->all_ifaces.v[k]->nvirt;
    return n;
}

static void dword(int d, int w, const char *fmt, ...) {
    char buf[100];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    emit("move    r0, %s", buf);
    emit("store   r0, __vbD%d+%d", d, 1 + w);
}

static int has_code(Sym *m) {
    return m && !(m->mods & M_MUSTOVERRIDE) && (m->sflags & SF_LIVE);
}

/* ------------------------------------------------------------ reachability */

/* Code is generated only for the methods that can run: those reachable
   from Main, the field initializers and the run-time library's callback,
   and the virtual and interface methods of the types that are created
   (their constructors are reachable). */

static SymVec live_work;

static void live_type(Sym *t);

static void live_method(Sym *m) {
    if (!m || (m->sflags & SF_LIVE) || m->rt) return;
    m->sflags |= SF_LIVE;
    sv_push(&live_work, m);
    if ((m->sflags & SF_CTOR) && m->owner && m->owner->kind == S_TYPE) live_type(m->owner);
}

static void live_type(Sym *t) {
    int k, j;
    if (!t || (t->sflags & SF_LIVE)) return;
    t->sflags |= SF_LIVE;
    if (t->base) live_type(t->base);
    if (t->initm && t->init_body && t->init_body->n) live_method(t->initm);
    if (t->sctor) live_method(t->sctor);
    for (k = 0; k < t->vtab.n; k++) live_method(t->vtab.v[k]);
    for (k = 0; k < t->all_ifaces.n; k++)
        for (j = 0; j < t->all_ifaces.v[k]->nvirt; j++) live_method(t->itabs[k][j]);
}

static void live_tree(Node *n);

static void live_vec(NodeVec *v) {
    int i;
    if (v) for (i = 0; i < v->n; i++) live_tree(v->v[i]);
}

static void live_tree(Node *n) {
    if (!n || n->seen) return;
    n->seen = 1;
    if ((n->kind == N_INVOKE || n->kind == N_NEW || n->kind == N_DLGNEW) && n->sym && n->sym->kind == S_METHOD)
        live_method(n->sym);
    if (n->type && n->type->kind == TY_STRUCT) live_type(n->type->sym);
    if (n->kind == N_DLGNEW) live_type(n->type->sym);     /* delegates are created without a constructor */
    live_tree(n->a);
    live_tree(n->b);
    live_tree(n->c);
    live_tree(n->d);
    live_vec(n->list);
    live_vec(n->list2);
    live_vec(n->list3);
}

static void mark_live(void) {
    int i;
    Sym *h = scope_find(ns_path("System")->members, intern_word("vbruntimehelpers"));
    live_method(prog_main);
    if (h) live_method(scope_find(h->members, intern_word("makeexception")));
    for (i = 0; i < prog_fields.n; i++) live_tree(prog_fields.v[i]->init);
    for (i = 0; i < prog_types.n; i++) {
        Sym *t = prog_types.v[i];
        /* structures (their zero values need no constructor) */
        if (t->type->kind == TY_STRUCT) live_type(t);
    }
    while (live_work.n) {
        Sym *m = live_work.v[--live_work.n];
        live_vec(m->body);
        live_tree(m->cval);     /* the implicit MyBase.New() */
        live_vec(m->init_body);
        if (m->decl && m->kind == S_METHOD) {
            int k;
            for (k = 0; k < m->locals.n; k++) {
                Sym *l = m->locals.v[k];
                if (l->type && l->type->kind == TY_STRUCT) live_type(l->type->sym);
            }
        }
    }
}

/* the .NET name of a type: System.Int32, Stack`1[System.Int32], Int32[] */
static void net_name_to(StrBuf *b, Type *t);

static void net_sym_name(StrBuf *b, Sym *s) {
    int i;
    if (!s->tmpl) {
        sb_puts(b, qual_name(s));
        return;
    }
    if (s->owner && s->owner != ns_global && s->owner->kind != S_METHOD) {
        net_sym_name(b, s->owner);
        sb_putc(b, (s->owner->kind == S_TYPE || s->owner->kind == S_MODULE) ? '+' : '.');
    }
    sb_puts(b, s->tmpl->spelling);
    sb_printf(b, "`%d[", s->ntargs);
    for (i = 0; i < s->ntargs; i++) {
        if (i) sb_putc(b, ',');
        net_name_to(b, s->targs[i]);
    }
    sb_putc(b, ']');
}

static void net_name_to(StrBuf *b, Type *t) {
    if (t->kind == TY_ARRAY) {
        int i;
        net_name_to(b, t->elem);
        sb_putc(b, '[');
        for (i = 1; i < t->rank; i++) sb_putc(b, ',');
        sb_putc(b, ']');
    } else if (ty_user(t)) net_sym_name(b, t->sym);
    else sb_printf(b, "System.%s", t->sysname);
}

static const char *net_name(Sym *s) {
    StrBuf b;
    sb_init(&b);
    net_sym_name(&b, s);
    return b.s;
}

static void gen_descriptor(Sym *t) {
    int d = t->desc, k, kind, w, ni, nst;
    switch (t->type->kind) {
    case TY_ENUM: kind = 3; break;
    case TY_STRUCT: kind = 2; break;
    case TY_INTERFACE: kind = 4; break;
    default: kind = 1;
    }
    dword(d, 0, "%d", kind);
    dword(d, 1, "__vbS%d", string_literal(net_name(t)));
    dword(d, 2, "%d", t->size);
    if (t->base) dword(d, 3, "__vbD%d+1", t->base->desc);
    if (kind == 3) {
        dword(d, 4, "%d", t->type->elem->kind);
        dword(d, 5, "%d", t->fields.n);
        for (k = 0; k < t->fields.n; k++) {
            Sym *c = t->fields.v[k];
            load_int(0, (i64)c->cval->ival);
            emit("store   r0, __vbD%d+%d", d, 1 + 6 + 2 * k);
            dword(d, 7 + 2 * k, "__vbS%d", string_literal(c->spelling));
        }
        return;
    }
    if (kind == 4) return;
    if (!(t->sflags & SF_LIVE)) return;     /* never created: no vtable, interfaces or reference slots */
    ni = t->all_ifaces.n;
    nst = nested_structs(t);
    w = 11 + 2 * ni;
    {
        /* the reference fields */
        int nr = ref_fields(t, 0), *slots = xalloc(sizeof(int) * (nr + 1));
        ref_fields(t, slots);
        dword(d, 9, "%d", nr);
        dword(d, 10, "__vbD%d+%d", d, 1 + w);
        for (k = 0; k < nr; k++) dword(d, w++, "%d", slots[k]);
        free(slots);
    }
    /* the fields of structure types */
    dword(d, 4, "%d", nst);
    dword(d, 6, "__vbD%d+%d", d, 1 + w);
    for (k = 0; k < t->fields.n; k++) {
        Sym *f = t->fields.v[k];
        if (f->type->kind != TY_STRUCT) continue;
        dword(d, w++, "%d", f->index);
        dword(d, w++, "__vbD%d+1", f->type->sym->desc);
    }
    /* the vtable */
    dword(d, 5, "%d", t->vtab.n);
    dword(d, 7, "__vbD%d+%d", d, 1 + w);
    for (k = 0; k < t->vtab.n; k++, w++)
        if (has_code(t->vtab.v[k])) dword(d, w, "__vbM%d", method_label(t->vtab.v[k]));
    /* the interfaces and their itables */
    dword(d, 8, "%d", ni);
    for (k = 0; k < ni; k++) {
        Sym *it = t->all_ifaces.v[k];
        int s;
        dword(d, 11 + 2 * k, "__vbD%d+1", it->desc);
        dword(d, 12 + 2 * k, "__vbD%d+%d", d, 1 + w);
        for (s = 0; s < it->nvirt; s++, w++)
            if (has_code(t->itabs[k][s])) dword(d, w, "__vbM%d", method_label(t->itabs[k][s]));
    }
}

/* the field initializers, in declaration order */
static void gen_init(void) {
    Sym fake;
    int i;
    memset(&fake, 0, sizeof fake);
    begin_method(&fake);
    /* the type descriptors (see rt/vb_rt.c) */
    for (i = 0; i < prog_types.n; i++) gen_descriptor(prog_types.v[i]);
    for (i = 0; i < prog_fields.n; i++) {
        Sym *f = prog_fields.v[i];
        Node *vi = f->decl;
        if (f->kind != S_FIELD) continue;
        set_file(f->file);
        if (f->init) {
            cg_source(vi);
            cg_value(f->init);
            store_var(f);
        } else if (vi->list) {
            cg_source(vi);
            cg_array(f->type, vi->list, 0);
            store_var(f);
        }
    }
    end_method("__vb_init", "field initializers", 0);
}

void codegen(const char *out_path, const char *rt_dir) {
    int i, j;
    Sym *mn = prog_main;
    cg_out = fopen(out_path, "w");
    if (!cg_out) {
        fprintf(stderr, "vbc: can't write %s\n", out_path);
        exit(2);
    }
    cg_asm_dir = dir_of(abs_path(out_path));
    mark_live();
    sb_init(&cg_data);
    sb_init(&cg_globals);
    cg_nglobals = prog_fields.n;
    fprintf(cg_out, "// Generated by the CPUSim64 VB.NET compiler %s from %s\n", VBC_VERSION, files[0].disp);
    fprintf(cg_out, "#include \"%s/vb_rt.asm\"\n", rt_dir);
    fprintf(cg_out, "\n// vb_program(): the field initializers, then Main\n");
    fprintf(cg_out, "__c_vb_program:\n");
    /* the garbage collector: the program's globals and the top of the stack */
    fprintf(cg_out, "    push    sf\n    move    r0, __vb_gend+1\n    push    r0\n    move    r0, __vb_gstart\n    push    r0\n");
    fprintf(cg_out, "    call    __c_vbrt_gc_init\n    add     sp, 3\n");
    if (opt_gc_stress) fprintf(cg_out, "    move    r0, 1\n    push    r0\n    call    __c_vbrt_gc_stress\n    add     sp, 1\n");
    fprintf(cg_out, "    call    __vb_init\n");
    for (i = 0; i < prog_types.n; i++)
        if (prog_types.v[i]->sctor) fprintf(cg_out, "    call    __vbM%d\n", method_label(prog_types.v[i]->sctor));
    if (mn->params.n) {
        fprintf(cg_out, "    call    __c_vbrt_args\n");
        fprintf(cg_out, "    push    r0\n");
    }
    fprintf(cg_out, "    call    __vbM%d\n", method_label(mn));
    if (mn->params.n) fprintf(cg_out, "    add     sp, 1\n");
    if (!mn->type) fprintf(cg_out, "    move    r0, 0\n");
    fprintf(cg_out, "    return\n");
    gen_init();
    for (i = 0; i < prog_modules.n; i++) {
        Sym *mod = prog_modules.v[i];
        for (j = 0; j < mod->methods.n; j++) {
            if (!(mod->methods.v[j]->sflags & SF_LIVE)) continue;
            set_file(mod->methods.v[j]->file);
            gen_method(mod->methods.v[j]);
        }
    }
    for (i = 0; i < prog_types.n; i++) {
        Sym *t = prog_types.v[i];
        if (t->type->kind == TY_ENUM || t->type->kind == TY_INTERFACE) continue;
        set_file(t->file);
        if (t->initm->sflags & SF_LIVE) gen_method(t->initm);
        for (j = 0; j < t->methods.n; j++)
            if (t->methods.v[j]->kind == S_METHOD && has_code(t->methods.v[j])) {
                set_file(t->methods.v[j]->file);
                gen_method(t->methods.v[j]);
            }
    }
    {
        /* exceptions: __vb_throw (r0 = the exception), and the functions the
           run-time library calls: vb_throw_obj(ex), vb_make_exception(code, msg) */
        Sym *h = scope_find(ns_path("System")->members, intern_word("vbruntimehelpers"));
        Sym *mk = h ? scope_find(h->members, intern_word("makeexception")) : 0;
        if (!mk) serr(0, "lib/System.vb has no VBRuntimeHelpers.MakeException");
        fprintf(cg_out, "\n// exceptions\n");
        fprintf(cg_out, "__vb_throw:\n    load    r1, __vb_handler+1\n    test    r1\n    jump    z, __vb_unhandled\n");
        fprintf(cg_out, "    load    r2, r1[3]\n    store   r2, __vb_handler+1\n    load    r2, r1[0]\n");
        fprintf(cg_out, "    load    sp, r1[1]\n    load    sf, r1[2]\n    jump    r2\n");
        fprintf(cg_out, "__vb_unhandled:\n    push    r0\n    call    __c_vbrt_unhandled\n");
        fprintf(cg_out, "__c_vb_throw_obj:\n    load    r0, sf[3]\n    jump    __vb_throw\n");
        fprintf(cg_out, "__c_vb_make_exception:\n    jump    __vbM%d\n", method_label(mk));
    }
    /* the thunks of Shared methods for delegates: called with the target
       (Nothing) and the arguments, they call the method with the arguments */
    for (i = 0; i < prog_modules.n + prog_types.n; i++) {
        Sym *c = i < prog_modules.n ? prog_modules.v[i] : prog_types.v[i - prog_modules.n];
        for (j = 0; j < c->methods.n; j++) {
            Sym *m = c->methods.v[j];
            int k, n = m->params.n;
            if (m->kind != S_METHOD || !(m->sflags & SF_THUNK) || !(m->sflags & SF_LIVE)) continue;
            fprintf(cg_out, "\n// thunk of %s.%s\n__vbT%d:\n", qual_name(c), m->spelling, method_label(m));
            for (k = 0; k < n; k++) fprintf(cg_out, "    load    r0, sf[%d]\n    push    r0\n", 3 + n - 1 - k);
            fprintf(cg_out, "    call    __vbM%d\n", method_label(m));
            if (n) fprintf(cg_out, "    add     sp, %d\n", n);
            fprintf(cg_out, "    return\n");
        }
    }
    fprintf(cg_out, "\n// run-time errors\n");
    fprintf(cg_out, "__vb_ovf:\n    move    r0, 1\n    push    r0\n    call    __c_vbrt_throw\n");
    fprintf(cg_out, "__vb_divz:\n    move    r0, 2\n    push    r0\n    call    __c_vbrt_throw\n");
    fprintf(cg_out, "__vb_index:\n    move    r0, 3\n    push    r0\n    call    __c_vbrt_throw\n");
    fprintf(cg_out, "__vb_null:\n    move    r0, 4\n    push    r0\n    call    __c_vbrt_throw\n");
    fprintf(cg_out, "\n// constants and string literals\n");
    fprintf(cg_out, "__vb_lmin: .dci 0x8000000000000000\n");
    fputs(cg_data.s, cg_out);
    fprintf(cg_out, "\n// fields, Static locals (__vb_gstart .. __vb_gend: the roots of the garbage collector)\n");
    fprintf(cg_out, "#global __vb_gstart: .dci 0\n");
    fprintf(cg_out, "#global __vb_handler: .dci 0\n#global .dci 0\n");
    /* two words each, as the C11 compiler does: the label word, then the value at label+1 */
    for (i = 0; i < cg_nglobals; i++) fprintf(cg_out, "#global __vbG%d: .dci 0\n#global .dci 0\n", i);
    fprintf(cg_out, "\n// type descriptors (filled in by __vb_init)\n");
    for (i = 0; i < prog_types.n; i++) {
        Sym *t = prog_types.v[i];
        int k, words = desc_words(t);
        fprintf(cg_out, "#global __vbD%d: .dci 0\n", t->desc);
        for (k = 0; k < words; k++) fprintf(cg_out, "#global .dci 0\n");
    }
    fprintf(cg_out, "#global __vb_gend: .dci 0\n#global .dci 0\n");
    fclose(cg_out);
}
