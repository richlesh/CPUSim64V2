/*
 * sema_main.c - the passes of the semantic analysis, and Main.
 */

/* ------------------------------------------------------------ pass 6: Main */

static int main_ok(Sym *m) {
    if (m->type && m->type->kind != TY_INTEGER) return 0;
    if (m->params.n == 0) return 1;
    return m->params.n == 1 && m->params.v[0]->type == ty_array(ty[TY_STRING], 1) &&
           !(m->params.v[0]->mods & (M_BYREF | M_OPTIONAL));
}

static void find_main(void) {
    int i;
    Name *nm = intern_word("main");
    const char *base = files[0].path, *p;
    for (p = base; *p; p++) if (*p == '/') base = p + 1;
    for (i = 0; i < prog_modules.n + prog_types.n; i++) {
        Sym *c = i < prog_modules.n ? prog_modules.v[i] : prog_types.v[i - prog_modules.n];
        Sym *s = scope_find(c->members, nm);
        for (; s && s->kind == S_METHOD; s = s->overload) {
            if (c->kind == S_TYPE && !(s->mods & M_SHARED)) continue;
            if (s->sflags & SF_GENERIC) continue;
            if (!main_ok(s)) continue;
            if (prog_main) serr(s->decl, "'Main' is declared more than once: '%s' and '%s'.", qual_name(prog_main->owner),
                                qual_name(s->owner));
            prog_main = s;
        }
    }
    if (!prog_main) {
        StrBuf b;
        sb_init(&b);
        for (p = base; *p && *p != '.'; p++) sb_putc(&b, *p);
        error_at(-1, 0, "No accessible 'Main' method with an appropriate signature was found in '%s'.", b.s);
    }
}

/* the passes of a class, structure or interface, up to pass p (2 bases,
   3 layout, 4 signatures, 5 vtables, 6 interfaces, 7 fields); generic
   instances created late catch up with them */
static void advance(Sym *t, int p) {
    int kind = t->type->kind;
    while (t->phase < p) {
        int k = ++t->phase, i;
        if (kind == TY_ENUM) continue;
        switch (k) {
        case 2:
            implicit_ctor(t);
            bind_base(t);
            break;
        case 3:
            if (t->base) advance(t->base, 3);
            if (kind == TY_CLASS || kind == TY_STRUCT) layout(t);
            break;
        case 4:
            bind_type_signatures(t);
            check_overloads(t);
            break;
        case 5:
            if (t->base) advance(t->base, 5);
            for (i = 0; i < t->ifaces.n; i++) advance(t->ifaces.v[i], 5);
            if (kind == TY_INTERFACE) iface_slots(t);
            else {
                enter(t);
                assign_vslots(t);
            }
            break;
        case 6:
            if (kind != TY_INTERFACE) {
                enter(t);
                check_implements(t);
            }
            break;
        case 7:
            if (kind != TY_INTERFACE) bind_type_fields(t);
            break;
        }
    }
}

static void advance_all(int p) {
    int i;
    sema_phase = p;
    for (i = 0; i < prog_types.n; i++) advance(prog_types.v[i], p);   /* the list can grow */
}

/* binds the bodies not yet bound; 1 if there were any */
static int bind_bodies(Sym *c) {
    int j, any = 0;
    for (j = 0; j < c->methods.n; j++) {
        Sym *m = c->methods.v[j];
        if (m->kind != S_METHOD || (m->sflags & SF_BOUND)) continue;
        if (m->rt) {
            m->sflags |= SF_BOUND;      /* Invoke of a delegate type */
            continue;
        }
        m->sflags |= SF_BOUND;
        any = 1;
        if (m->mods & M_MUSTOVERRIDE) continue;
        if (m->decl && m->decl->kind != N_CLASS && m->decl->kind != N_STRUCTURE) bind_method(m);
        else if (m->sflags & SF_CTOR) {
            /* the implicit Sub New: the implicit MyBase.New() */
            enter(m);
            cur_method = m;
            ctor_chain(m, 0);
            cur_method = 0;
        }
    }
    return any;
}

void sema_program(Node **fs, int n, int need_main) {
    int i, j, more;
    file_opts = xalloc(sizeof(Opts) * (n + 1));
    for (i = 0; i < n; i++) declare_members(fs[i]->list2, ns_global);
    for (i = 0; i < n; i++) bind_options(fs[i], i);
    for (i = 0; i < prog_types.n; i++) {
        Sym *t = prog_types.v[i];
        if (t->type->kind != TY_ENUM) continue;
        enter(t);
        t->type->elem = t->decl->a ? bind_type(t->decl->a) : ty[TY_INTEGER];
        if (!ty_integral(t->type->elem)) serr(t->decl->a, "Enums must be declared as an integral type.");
    }
    for (i = 0; i < prog_types.n; i++) {
        Sym *t = prog_types.v[i];
        if (t->type->kind == TY_ENUM)
            for (j = 0; j < t->fields.n; j++) eval_const(t->fields.v[j], t->decl);
    }
    advance_all(2);
    advance_all(3);
    sema_phase = 4;
    for (i = 0; i < prog_modules.n; i++) {
        Sym *mod = prog_modules.v[i];
        Sym *s;
        for (s = mod->members->syms; s; s = s->next) if (s->kind == S_EVENT) bind_event_type(s);
        for (j = 0; j < mod->methods.n; j++)
            if (mod->methods.v[j]->kind == S_METHOD) bind_signature(mod->methods.v[j]);
        check_overloads(mod);
    }
    advance_all(4);
    advance_all(5);
    advance_all(6);
    sema_phase = 7;
    for (i = 0; i < prog_modules.n; i++) {
        Sym *mod = prog_modules.v[i], *s;
        for (s = mod->members->syms; s; s = s->next)
            if (s->kind == S_CONST) eval_const(s, s->decl);
        for (j = 0; j < mod->fields.n; j++) bind_field(mod->fields.v[j]);
        for (j = 0; j < mod->methods.n; j++)
            if (mod->methods.v[j]->kind == S_FIELD) {
                Sym *f = mod->methods.v[j];
                f->index = prog_fields.n;
                sv_push(&prog_fields, f);
            }
    }
    advance_all(7);
    sema_phase = 8;
    do {
        more = 0;
        for (i = 0; i < prog_modules.n; i++) more |= bind_bodies(prog_modules.v[i]);
        for (i = 0; i < prog_types.n; i++) {
            Sym *t = prog_types.v[i];
            if (t->type->kind == TY_ENUM || t->type->kind == TY_INTERFACE) continue;
            more |= bind_bodies(t);
        }
    } while (more);
    if (need_main) find_main();
}
