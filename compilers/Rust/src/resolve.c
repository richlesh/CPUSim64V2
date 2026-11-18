/*
 * resolve.c - scopes, symbols and name resolution (see sema.h).
 *
 * resolve_crate() works in two passes over the user crate and, when present,
 * the crate `core` (lib/core.rs):
 *
 *   1. collect: every item of every module (also modules nested in modules
 *      and in blocks) becomes a Sym in the type / value / macro namespace of
 *      its module; struct fields, enum variants and trait / impl members are
 *      collected into Sym.members; `use` declarations are queued.
 *   2. resolve: the queued imports are resolved to a fixpoint (globs, groups,
 *      renames, re-exports), then every path in every item and body is
 *      resolved with lexical scopes (blocks, patterns, closures, labels).
 *
 * What the resolver leaves for check.c
 *   - A path like `Type::assoc` or `T::Item` or `Self::new`: the leading
 *     segments that name a module / type / enum variant have PATH_SEG.sym set,
 *     the remaining segments have sym == 0.  PATH.sym is the sym of the last
 *     RESOLVED segment (the type), so a path is complete exactly when the last
 *     segment's sym is non-null.
 *   - A qualified path `<T as Trait>::x` (F_QUALIFIED): `a` (the type) and `b`
 *     (the trait path) are resolved, PATH.sym = the trait (0 for `<T>::x`),
 *     the segments after `>::` are left (sym == 0).
 *   - Method calls, field accesses and operators.
 *
 * Scope layout
 *   module members -> fallback scope (empty) -> prelude members (core's
 *   `prelude` module) -> primitive types.  Block scopes, generics scopes and
 *   pattern scopes chain to the enclosing scope; the chain ends at the module.
 *   Sym.members of types and traits are NOT part of any lexical chain.
 */
#include "sema.h"

/* ================================================================ scopes */

typedef struct SEnt SEnt;
struct SEnt {
    Name *name;
    Sym *sym;
    Sym *vis;            /* module the name is visible in (and below); 0 = public */
    int glob;            /* brought in by a glob import (shadowed by explicit names) */
    SEnt *next;
};

struct NsTab {
    SEnt **b;
    int cap, cnt;
};

struct Scope {
    Scope *parent;
    struct NsTab t[3];
    Sym **items;         /* members in definition order (scope_put calls) */
    int nitems, citems;
    int boundary;        /* an item scope inside a body: locals / generics above it are not visible */
};

Sym *crate_root = 0;
Sym *core_root = 0;

/* extra interface for collect.c / check.c (not declared in sema.h) */
Sym **resolve_impls = 0;          /* every impl block of both crates, in source order */
int resolve_nimpls = 0;
static int impls_cap = 0;

static int building_core = 0;
static Sym *cur_crate_root = 0;
static Scope *prim_scope = 0;
static Scope *fallback_scope = 0;
static int lang_file = 0;

static Name *n_self, *n_Self, *n_super, *n_crate, *n_dcrate, *n_core, *n_std, *n_prelude;
static Name *n_underscore, *n_lang, *n_main;

static Scope *scope_new_internal(Scope *parent) {
    Scope *s = (Scope *)xalloc(sizeof(Scope));
    s->parent = parent;
    return s;
}

Scope *scope_new(Scope *parent) {
    return scope_new_internal(parent);
}

static SEnt *ent_find(Scope *s, int ns, Name *name) {
    struct NsTab *t = &s->t[ns];
    SEnt *e;
    if (!t->cap || !name) return 0;
    for (e = t->b[name->hash & (t->cap - 1)]; e; e = e->next)
        if (e->name == name) return e;
    return 0;
}

static void tab_grow(struct NsTab *t) {
    int ncap = t->cap ? t->cap * 2 : 8, i;
    SEnt **nb = (SEnt **)xalloc(sizeof(SEnt *) * ncap);
    for (i = 0; i < t->cap; i++) {
        SEnt *e = t->b[i], *nx;
        while (e) {
            int k = (int)(e->name->hash & (ncap - 1));
            nx = e->next;
            e->next = nb[k];
            nb[k] = e;
            e = nx;
        }
    }
    t->b = nb;
    t->cap = ncap;
}

/* adds or replaces the entry; returns it */
static SEnt *ent_put(Scope *s, int ns, Name *name, Sym *sym) {
    struct NsTab *t = &s->t[ns];
    SEnt *e = ent_find(s, ns, name);
    int k;
    if (e) {
        e->sym = sym;
        return e;
    }
    if (t->cnt * 2 >= t->cap) tab_grow(t);
    e = (SEnt *)xalloc(sizeof(SEnt));
    e->name = name;
    e->sym = sym;
    k = (int)(name->hash & (t->cap - 1));
    e->next = t->b[k];
    t->b[k] = e;
    t->cnt++;
    return e;
}

Sym *scope_get(Scope *s, int ns, Name *name) {
    SEnt *e;
    if (!s) return 0;
    e = ent_find(s, ns, name);
    return e ? e->sym : 0;
}

Sym *scope_lookup(Scope *s, int ns, Name *name) {
    for (; s; s = s->parent) {
        SEnt *e = ent_find(s, ns, name);
        if (e) return e->sym;
    }
    return 0;
}

void scope_put(Scope *s, int ns, Name *name, Sym *sym) {
    if (s->nitems == 0 || s->items[s->nitems - 1] != sym) {
        if (s->nitems == s->citems) {
            s->citems = s->citems ? s->citems * 2 : 8;
            s->items = (Sym **)xrealloc(s->items, sizeof(Sym *) * s->citems);
        }
        s->items[s->nitems++] = sym;
    }
    if (name) ent_put(s, ns, name, sym);
}

/* ordered members of a type / module / trait / impl scope (resolve.c extension) */
int scope_member_count(Scope *s) {
    return s ? s->nitems : 0;
}

Sym *scope_member(Scope *s, int i) {
    return s && i >= 0 && i < s->nitems ? s->items[i] : 0;
}

/* a lexical binding (local, generic, Self): not a member */
static void ent_set(Scope *s, int ns, Name *name, Sym *sym) {
    SEnt *e = ent_put(s, ns, name, sym);
    e->vis = 0;
    e->glob = 0;
}

Sym *sym_new(int kind, Name *name, Node *node, Sym *parent) {
    Sym *s = (Sym *)xalloc(sizeof(Sym));
    s->kind = kind;
    s->name = name;
    s->node = node;
    s->parent = parent;
    if (building_core) s->flags |= SF_CORE;
    return s;
}

const char *sym_kind_name(int kind) {
    switch (kind) {
    case SYM_MOD: return "module";
    case SYM_STRUCT: return "struct";
    case SYM_UNION: return "union";
    case SYM_ENUM: return "enum";
    case SYM_VARIANT: return "variant";
    case SYM_FIELD: return "field";
    case SYM_TRAIT: return "trait";
    case SYM_FN: return "function";
    case SYM_CONST: return "constant";
    case SYM_STATIC: return "static";
    case SYM_TYPEALIAS: return "type alias";
    case SYM_ASSOC_TYPE: return "associated type";
    case SYM_IMPL: return "implementation";
    case SYM_GENERIC: return "type parameter";
    case SYM_CONST_GENERIC: return "const parameter";
    case SYM_LOCAL: return "local variable";
    case SYM_PRIM: return "builtin type";
    case SYM_MACRO: return "macro";
    case SYM_SELF_TY: return "self type";
    }
    return "item";
}

/* ---------------------------------------------------------------- sym_path */

static void put_name(StrBuf *out, Name *n) {
    sb_puts(out, n ? n->s : "_");
}

static void impl_text(Sym *impl, StrBuf *out) {
    Node *t = impl->node ? impl->node->b : 0;
    if (t && t->kind == N_T_PATH && t->a && t->a->list.n > 0) {
        put_name(out, t->a->list.v[t->a->list.n - 1]->name);
        return;
    }
    sb_puts(out, "<impl>");
}

void sym_path(Sym *s, StrBuf *out) {
    char tmp[32];
    if (!s) {
        sb_puts(out, "?");
        return;
    }
    switch (s->kind) {
    case SYM_LOCAL: case SYM_GENERIC: case SYM_CONST_GENERIC: case SYM_PRIM: case SYM_SELF_TY:
        put_name(out, s->name);
        return;
    case SYM_MOD:
        if (!s->parent) {
            sb_puts(out, s == core_root ? "core" : "crate");
            return;
        }
        break;
    case SYM_IMPL:
        impl_text(s, out);
        return;
    default:
        break;
    }
    if (s->parent) {
        if (s->parent->kind == SYM_IMPL) {
            impl_text(s->parent, out);
            sb_puts(out, "::");
        } else if (s->parent->kind == SYM_MOD && !s->parent->parent && s->parent != core_root) {
            /* the user crate root is not printed */
        } else {
            sym_path(s->parent, out);
            sb_puts(out, "::");
        }
    }
    if (s->name) put_name(out, s->name);
    else {
        sprintf(tmp, "%d", s->index);
        sb_puts(out, tmp);
    }
}

/* ---------------------------------------------------------------- lang items */

static Sym **lang_syms = 0;
static int nlang = 0, clang = 0;

static void set_lang(Node *n, Sym *s) {
    long i;
    for (i = 0; i < n->attrs.n; i++) {
        Node *a = n->attrs.v[i];
        if (a->kind == N_ATTR && a->name == n_lang && (a->flags & F_EQ) && a->a && a->a->kind == N_E_LIT_STR) {
            s->lang = name_intern(a->a->str, a->a->slen);
            if (nlang == clang) {
                clang = clang ? clang * 2 : 32;
                lang_syms = (Sym **)xrealloc(lang_syms, sizeof(Sym *) * clang);
            }
            lang_syms[nlang++] = s;
            return;
        }
    }
}

Sym *lang_item(const char *name) {
    int i;
    for (i = 0; i < nlang; i++)
        if (strcmp(lang_syms[i]->lang->s, name) == 0) return lang_syms[i];
    error_at(lang_file, 1, "missing lang item `%s`", name);
    return 0;
}

/* ================================================================ state */

static Sym *cur_mod = 0;          /* the module the code being resolved is in */
static Scope *sc = 0;             /* the current lexical scope */
static Sym *cur_fn = 0;
static int nlocals = 0;
static int in_body = 0;           /* > 0 while resolving a body / initializer */

static Name **labels = 0;
static int nlabels = 0, clabels = 0, label_base = 0;

static void label_push(Name *n) {
    if (nlabels == clabels) {
        clabels = clabels ? clabels * 2 : 16;
        labels = (Name **)xrealloc(labels, sizeof(Name *) * clabels);
    }
    labels[nlabels++] = n;
}

/* ================================================================ visibility */

static Sym *vis_of(Node *item, Sym *mod) {
    Node *v = item->vis;
    if (!v) return mod;
    if (v->flags & F_VIS_CRATE) return cur_crate_root;
    if (v->flags & F_VIS_SUPER) return mod->parent ? mod->parent : mod;
    if (v->flags & F_VIS_SELF) return mod;
    if (v->flags & F_VIS_IN) return cur_crate_root;
    return 0;
}

static int in_or_below(Sym *m, Sym *v) {
    for (; m; m = m->parent)
        if (m == v) return 1;
    return 0;
}

static int accessible(SEnt *e, Sym *from) {
    return e->vis == 0 || in_or_below(from, e->vis);
}

/* ================================================================ collection */

typedef struct Import Import;
struct Import {
    Node *leaf;          /* USE_TREE, for the error line */
    Node **segs;
    int nsegs;
    int global, glob, self_only;
    Name *name;          /* the name bound (leaf imports) */
    Scope *target;
    Sym *mod;
    Sym *vis;
    Sym *croot;
    int done[3];
};

static Import **imports = 0;
static int nimports = 0, cimports = 0;

static void queue_import(Import *im) {
    if (nimports == cimports) {
        cimports = cimports ? cimports * 2 : 32;
        imports = (Import **)xrealloc(imports, sizeof(Import *) * cimports);
    }
    imports[nimports++] = im;
}

static void flatten_use(Node *t, Node **pre, int npre, int global, Node *item, Scope *target, Sym *mod) {
    long nseg = t->a ? t->a->list.n : 0, i;
    int k = npre;
    Node **segs = (Node **)xalloc(sizeof(Node *) * (npre + nseg + 1));
    Import *im;
    for (i = 0; i < npre; i++) segs[i] = pre[i];
    if (t->a) {
        if (npre == 0 && (t->a->flags & F_GLOBAL)) global = 1;
        for (i = 0; i < nseg; i++) segs[k++] = t->a->list.v[i];
    }
    if (t->flags & F_GROUP) {
        for (i = 0; i < t->list.n; i++) flatten_use(t->list.v[i], segs, k, global, item, target, mod);
        return;
    }
    im = (Import *)xalloc(sizeof(Import));
    im->leaf = t;
    im->segs = segs;
    im->global = global;
    im->target = target;
    im->mod = mod;
    im->croot = cur_crate_root;
    im->vis = vis_of(item, mod);
    if (t->flags & F_GLOB) {
        im->glob = 1;
        im->nsegs = k;
        if (k == 0) error_at(t->file, t->line, "glob imports without a path are not supported");
    } else {
        if (k == 0) error_at(t->file, t->line, "invalid import");
        if (segs[k - 1]->name == n_self) {
            if (k == 1) error_at(t->file, t->line, "`self` imports are only allowed within a { } list");
            k--;
            im->self_only = 1;
        }
        im->nsegs = k;
        im->name = t->name2 ? t->name2 : segs[k - 1]->name;
    }
    queue_import(im);
}

/* defines a name in a scope, reporting duplicates */
static void define(Scope *s, int ns, Node *item, Name *name, Sym *sym, Sym *vis) {
    SEnt *e;
    if (!name || name == n_underscore) return;
    e = ent_find(s, ns, name);
    if (e && !e->glob)
        error_at(item->file, item->line, "the name `%s` is defined multiple times", name->s);
    scope_put(s, ns, name, sym);
    e = ent_find(s, ns, name);
    e->vis = vis;
    e->glob = 0;
}

static Sym *new_item_sym(int kind, Node *n, Sym *parent) {
    Sym *s = sym_new(kind, n->name, n, parent);
    if (n->vis) s->flags |= SF_PUB;
    n->sym = s;
    set_lang(n, s);
    return s;
}

static void add_impl(Sym *s) {
    if (resolve_nimpls == impls_cap) {
        impls_cap = impls_cap ? impls_cap * 2 : 64;
        resolve_impls = (Sym **)xrealloc(resolve_impls, sizeof(Sym *) * impls_cap);
    }
    resolve_impls[resolve_nimpls++] = s;
}

/* fields of a struct / union / variant */
static void collect_fields(Sym *owner, Node *n) {
    long i;
    owner->members = scope_new(0);
    for (i = 0; i < n->list.n; i++) {
        Node *f = n->list.v[i];
        Sym *fs = sym_new(SYM_FIELD, f->name, f, owner);
        fs->index = (int)i;
        if (f->vis) fs->flags |= SF_PUB;
        f->sym = fs;
        if (f->name && ent_find(owner->members, NS_VALUE, f->name))
            error_at(f->file, f->line, "field `%s` is already declared", f->name->s);
        scope_put(owner->members, NS_VALUE, f->name, fs);
    }
}

static void collect_assoc(Sym *owner, NodeList *items, int in_trait) {
    long i;
    int trait_impl = in_trait || (owner->node->c != 0);
    Scope *m = owner->members;
    for (i = 0; i < items->n; i++) {
        Node *it = items->v[i];
        Sym *s = 0;
        int ns = NS_VALUE;
        switch (it->kind) {
        case N_ITEM_FN:
            s = new_item_sym(SYM_FN, it, owner);
            if (it->list.n > 0 && it->list.v[0]->kind == N_SELF_PARAM) s->flags |= SF_METHOD;
            break;
        case N_ITEM_CONST:
            s = new_item_sym(SYM_CONST, it, owner);
            break;
        case N_ITEM_TYPE:
            s = new_item_sym(SYM_ASSOC_TYPE, it, owner);
            ns = NS_TYPE;
            break;
        default:
            break;
        }
        if (!s) continue;
        if (trait_impl) s->flags |= SF_PUB;
        if (it->name && it->name != n_underscore) {
            if (ent_find(m, ns, it->name)) {
                if (in_trait)
                    error_at(it->file, it->line, "the name `%s` is defined multiple times", it->name->s);
                else
                    error_at(it->file, it->line, "duplicate definitions with name `%s`", it->name->s);
            }
            scope_put(m, ns, it->name, s);
        }
    }
}

static void collect_items(NodeList *items, Scope *scope, Sym *mod);

static void collect_item(Node *n, Scope *scope, Sym *mod) {
    Sym *s, *vis = vis_of(n, mod);
    long i;
    switch (n->kind) {
    case N_ITEM_FN:
        s = new_item_sym(SYM_FN, n, mod);
        if (n->list.n > 0 && n->list.v[0]->kind == N_SELF_PARAM) s->flags |= SF_METHOD;
        define(scope, NS_VALUE, n, n->name, s, vis);
        break;
    case N_ITEM_STRUCT:
        s = new_item_sym(SYM_STRUCT, n, mod);
        collect_fields(s, n);
        define(scope, NS_TYPE, n, n->name, s, vis);
        if (!(n->flags & F_NAMED)) define(scope, NS_VALUE, n, n->name, s, vis);
        break;
    case N_ITEM_UNION:
        s = new_item_sym(SYM_UNION, n, mod);
        collect_fields(s, n);
        define(scope, NS_TYPE, n, n->name, s, vis);
        break;
    case N_ITEM_ENUM:
        s = new_item_sym(SYM_ENUM, n, mod);
        s->members = scope_new(0);
        for (i = 0; i < n->list.n; i++) {
            Node *v = n->list.v[i];
            Sym *vs = sym_new(SYM_VARIANT, v->name, v, s);
            vs->index = (int)i;
            vs->flags |= SF_PUB;
            v->sym = vs;
            set_lang(v, vs);
            collect_fields(vs, v);
            if (ent_find(s->members, NS_TYPE, v->name))
                error_at(v->file, v->line, "the name `%s` is defined multiple times", v->name->s);
            scope_put(s->members, NS_TYPE, v->name, vs);
            scope_put(s->members, NS_VALUE, v->name, vs);
        }
        define(scope, NS_TYPE, n, n->name, s, vis);
        break;
    case N_ITEM_TRAIT:
        s = new_item_sym(SYM_TRAIT, n, mod);
        s->members = scope_new(0);
        collect_assoc(s, &n->list, 1);
        define(scope, NS_TYPE, n, n->name, s, vis);
        break;
    case N_ITEM_IMPL:
        s = new_item_sym(SYM_IMPL, n, mod);
        s->members = scope_new(0);
        add_impl(s);
        collect_assoc(s, &n->list, 0);
        break;
    case N_ITEM_MOD: {
        Scope *parent = fallback_scope;
        if (!(n->flags & F_INLINE)) error_at(n->file, n->line, "file modules are not supported");
        s = new_item_sym(SYM_MOD, n, mod);
        if (building_core && mod == core_root && n->name == n_prelude) parent = prim_scope;
        s->members = scope_new(parent);
        if (parent == prim_scope) fallback_scope->parent = s->members;
        define(scope, NS_TYPE, n, n->name, s, vis);
        collect_items(&n->list, s->members, s);
        break;
    }
    case N_ITEM_USE:
        if (n->a) flatten_use(n->a, 0, 0, 0, n, scope, mod);
        break;
    case N_ITEM_CONST:
        s = new_item_sym(SYM_CONST, n, mod);
        define(scope, NS_VALUE, n, n->name, s, vis);
        break;
    case N_ITEM_STATIC:
        s = new_item_sym(SYM_STATIC, n, mod);
        if (n->flags & F_MUT) s->flags |= SF_MUT;
        define(scope, NS_VALUE, n, n->name, s, vis);
        break;
    case N_ITEM_TYPE:
        s = new_item_sym(SYM_TYPEALIAS, n, mod);
        define(scope, NS_TYPE, n, n->name, s, vis);
        break;
    case N_ITEM_EXTERN_CRATE: {
        Sym *target = 0;
        Name *as = n->name2 ? n->name2 : n->name;
        if (n->name == n_core || n->name == n_std) target = core_root;
        else if (n->name == n_self) target = cur_crate_root;
        if (!target) error_at(n->file, n->line, "can't find crate for `%s`", n->name->s);
        if (as != n_underscore) {
            SEnt *e = ent_find(scope, NS_TYPE, as);
            if (e && !e->glob) error_at(n->file, n->line, "the name `%s` is defined multiple times", as->s);
            e = ent_put(scope, NS_TYPE, as, target);
            e->vis = vis;
            e->glob = 0;
        }
        break;
    }
    case N_ITEM_EXTERN_BLOCK:
        for (i = 0; i < n->list.n; i++) {
            Node *it = n->list.v[i];
            Sym *vs = mod;
            Sym *evis = vis_of(it, mod);
            if (it->kind == N_ITEM_FN) {
                s = new_item_sym(SYM_FN, it, vs);
                s->flags |= SF_EXTERN;
                if (n->str && strcmp(n->str, "rust-intrinsic") == 0) s->flags |= SF_INTRINSIC;
                define(scope, NS_VALUE, it, it->name, s, evis);
            } else if (it->kind == N_ITEM_STATIC) {
                s = new_item_sym(SYM_STATIC, it, vs);
                s->flags |= SF_EXTERN;
                if (it->flags & F_MUT) s->flags |= SF_MUT;
                define(scope, NS_VALUE, it, it->name, s, evis);
            } else if (it->kind == N_ITEM_TYPE) {
                s = new_item_sym(SYM_TYPEALIAS, it, vs);
                s->flags |= SF_EXTERN;
                define(scope, NS_TYPE, it, it->name, s, evis);
            }
        }
        break;
    case N_ITEM_MACRO_RULES:
        s = new_item_sym(SYM_MACRO, n, mod);
        scope_put(scope, NS_MACRO, n->name, s);
        {
            SEnt *e = ent_find(scope, NS_MACRO, n->name);
            e->vis = 0;
        }
        break;
    default:
        break;
    }
}

static void collect_items(NodeList *items, Scope *scope, Sym *mod) {
    long i;
    Sym *save = cur_mod;
    cur_mod = mod;
    for (i = 0; i < items->n; i++) collect_item(items->v[i], scope, mod);
    cur_mod = save;
}

/* ================================================================ imports */

static long progress = 0;

static int is_container(Sym *s) {
    return s->kind == SYM_MOD || s->kind == SYM_ENUM;
}

/* the first segment of a use path found lexically (items only) */
static Sym *use_lookup(Scope *from, Name *name) {
    Sym *s = scope_lookup(from, NS_TYPE, name);
    if (s && (s->kind == SYM_LOCAL || s->kind == SYM_GENERIC || s->kind == SYM_SELF_TY || s->kind == SYM_CONST_GENERIC))
        s = 0;
    return s;
}

/* resolves segments [0, count) of an import path to a module or enum.
   Returns 0 and sets *fail to the failing segment; *priv is set when the
   segment exists but is private. */
static Sym *walk_use_prefix(Import *im, int count, int *fail, Sym **priv) {
    Sym *cur = 0;
    int i = 0;
    Name *nm;
    *priv = 0;
    *fail = 0;
    if (count == 0) return 0;
    nm = im->segs[0]->name;
    if (im->global) {
        if ((nm == n_core || nm == n_std) && core_root) cur = core_root;
        else return 0;
        i = 1;
    } else if (nm == n_crate || nm == n_dcrate) {
        cur = im->croot;
        i = 1;
    } else if (nm == n_self) {
        cur = im->mod;
        i = 1;
    } else if (nm == n_super) {
        cur = im->mod;
        while (i < count && im->segs[i]->name == n_super) {
            if (!cur->parent) {
                *fail = i;
                return 0;
            }
            cur = cur->parent;
            i++;
        }
    } else {
        cur = use_lookup(im->target, nm);
        if (!cur && (nm == n_core || nm == n_std) && core_root) cur = core_root;
        if (!cur) return 0;
        i = 1;
    }
    for (; i < count; i++) {
        SEnt *e;
        nm = im->segs[i]->name;
        if (!is_container(cur)) {
            *fail = i;
            return 0;
        }
        if (nm == n_super && cur->kind == SYM_MOD && cur->parent) {
            cur = cur->parent;
            continue;
        }
        e = ent_find(cur->members, NS_TYPE, nm);
        if (!e) {
            *fail = i;
            return 0;
        }
        if (cur->kind == SYM_MOD && !accessible(e, im->mod)) {
            *fail = i;
            *priv = e->sym;
            return 0;
        }
        cur = e->sym;
    }
    return cur;
}

/* binds sym under the import's name in namespace ns */
static void bind_import(Import *im, int ns, Sym *sym) {
    SEnt *e;
    Name *nm = im->name;
    static int anon = 0;
    if (nm == n_underscore) {
        char buf[32];
        sprintf(buf, "_#%d", anon++);
        nm = name_intern_str(buf);
    }
    e = ent_find(im->target, ns, nm);
    if (e && !e->glob)
        error_at(im->leaf->file, im->leaf->line, "the name `%s` is defined multiple times", nm->s);
    e = ent_put(im->target, ns, nm, sym);
    e->vis = im->vis;
    e->glob = 0;
    im->done[ns] = 1;
    progress++;
}

static void glob_import(Import *im, Sym *c) {
    int ns, b;
    if (im->target == c->members) return;
    for (ns = 0; ns < 3; ns++) {
        struct NsTab *t = &c->members->t[ns];
        for (b = 0; b < t->cap; b++) {
            SEnt *e;
            for (e = t->b[b]; e; e = e->next) {
                SEnt *ne;
                if (c->kind == SYM_MOD && !accessible(e, im->mod)) continue;
                if (ent_find(im->target, ns, e->name)) continue;
                ne = ent_put(im->target, ns, e->name, e->sym);
                ne->vis = im->vis;
                ne->glob = 1;
                progress++;
            }
        }
    }
}

static void try_import(Import *im) {
    int fail, nprefix, ns;
    Sym *priv, *c;
    Name *nm;
    nprefix = im->glob ? im->nsegs : im->nsegs - 1;
    if (nprefix == 0) {
        /* `use core;` / `use std;` */
        Sym *s = 0;
        nm = im->segs[0]->name;
        if (im->done[NS_TYPE]) return;
        if ((nm == n_core || nm == n_std) && core_root) s = core_root;
        if (s) bind_import(im, NS_TYPE, s);
        return;
    }
    c = walk_use_prefix(im, nprefix, &fail, &priv);
    if (!c || !is_container(c)) return;
    if (im->glob) {
        glob_import(im, c);
        return;
    }
    nm = im->segs[im->nsegs - 1]->name;
    for (ns = 0; ns < 3; ns++) {
        SEnt *e;
        if (im->done[ns]) continue;
        if (im->self_only && ns != NS_TYPE) continue;
        e = ent_find(c->members, ns, nm);
        if (!e) continue;
        if (c->kind == SYM_MOD && !accessible(e, im->mod)) continue;
        bind_import(im, ns, e->sym);
    }
}

static void path_text(StrBuf *sb, Import *im, int upto) {
    int i;
    if (im->global) sb_puts(sb, "::");
    for (i = 0; i <= upto && i < im->nsegs; i++) {
        if (i) sb_puts(sb, "::");
        sb_puts(sb, im->segs[i]->name->s);
    }
}

static void import_fail(Import *im, int upto, Sym *priv) {
    StrBuf sb;
    Node *t = im->leaf;
    if (priv) {
        error_at(t->file, t->line, "%s `%s` is private", sym_kind_name(priv->kind), priv->name->s);
    }
    sb_init(&sb);
    path_text(&sb, im, upto);
    error_at(t->file, t->line, "unresolved import `%s`", sb.s);
}

static void check_import(Import *im) {
    int fail = 0, nprefix, ns;
    Sym *priv = 0, *c;
    Name *nm;
    nprefix = im->glob ? im->nsegs : im->nsegs - 1;
    if (nprefix == 0) {
        if (!im->done[NS_TYPE]) import_fail(im, 0, 0);
        return;
    }
    c = walk_use_prefix(im, nprefix, &fail, &priv);
    if (!c) import_fail(im, fail, priv);
    if (!is_container(c)) import_fail(im, nprefix - 1, 0);
    if (im->glob) return;
    if (im->done[0] || im->done[1] || im->done[2]) return;
    nm = im->segs[im->nsegs - 1]->name;
    for (ns = 0; ns < 3; ns++) {
        SEnt *e = ent_find(c->members, ns, nm);
        if (e && !accessible(e, im->mod)) import_fail(im, im->nsegs - 1, e->sym);
    }
    import_fail(im, im->nsegs - 1, 0);
}

static void process_imports(int from) {
    int i;
    do {
        progress = 0;
        for (i = from; i < nimports; i++) try_import(imports[i]);
    } while (progress);
    for (i = from; i < nimports; i++) check_import(imports[i]);
}

/* ================================================================ resolution */

enum { W_VALUE = 0, W_FN, W_TYPE, W_TRAIT, W_STRUCT, W_TSTRUCT, W_UNIT };

static int what_ns(int w) {
    return (w == W_TYPE || w == W_TRAIT || w == W_STRUCT) ? NS_TYPE : NS_VALUE;
}

static const char *what_noun(int w) {
    switch (w) {
    case W_VALUE: return "value";
    case W_FN: return "function";
    case W_TYPE: return "type";
    case W_TRAIT: return "trait";
    case W_STRUCT: return "struct, variant or union type";
    case W_TSTRUCT: return "tuple struct or tuple variant";
    }
    return "unit struct, unit variant or constant";
}

static void resolve_type(Node *t);
static void resolve_expr(Node *e);
static void resolve_block(Node *b);
static void resolve_bounds(Node *bs);
static void resolve_item(Node *n);
static void resolve_fn(Node *fn, int nested);

/* looks a name up lexically.  quiet: for patterns, only items count (a local
   or a name behind an item boundary gives 0). */
static Sym *lookup_lex(int ns, Name *name, Node *at, int quiet) {
    Scope *s;
    int crossed = 0;
    for (s = sc; s; s = s->parent) {
        SEnt *e = ent_find(s, ns, name);
        if (e) {
            Sym *r = e->sym;
            if (quiet) {
                if (r->kind == SYM_LOCAL || crossed) return 0;
                return r;
            }
            if (crossed) {
                if (r->kind == SYM_LOCAL)
                    error_at(at->file, at->line, "can't capture dynamic environment in a fn item");
                if (r->kind == SYM_GENERIC || r->kind == SYM_CONST_GENERIC)
                    error_at(at->file, at->line, "can't use generic parameters from outer item");
                if (r->kind == SYM_SELF_TY)
                    error_at(at->file, at->line, "can't use `Self` from outer item");
            }
            return r;
        }
        if (s->boundary) crossed = 1;
    }
    return 0;
}

static int is_type_like(Sym *s) {
    switch (s->kind) {
    case SYM_STRUCT: case SYM_UNION: case SYM_TRAIT: case SYM_TYPEALIAS: case SYM_ASSOC_TYPE:
    case SYM_GENERIC: case SYM_SELF_TY: case SYM_PRIM:
        return 1;
    }
    return 0;
}

static void resolve_seg_args(Node *seg) {
    long i;
    for (i = 0; i < seg->list.n; i++) {
        Node *a = seg->list.v[i];
        switch (a->kind) {
        case N_LIFETIME:
            break;
        case N_ASSOC_BIND:
            if (a->a) resolve_type(a->a);
            if (a->b) resolve_bounds(a->b);
            break;
        default:
            if (a->kind >= N_T_PATH && a->kind <= N_T_INFER) resolve_type(a);
            else resolve_expr(a);
            break;
        }
    }
    if (seg->a) resolve_type(seg->a);
}

static void expected_found(int w, Sym *found, Node *seg) {
    const char *exp = w == W_TYPE ? "type" : w == W_TRAIT ? "trait" : what_noun(w);
    if (w == W_FN) exp = "function";
    error_at(seg->file, seg->line, "expected %s, found %s `%s`", exp, sym_kind_name(found->kind),
             found->name ? found->name->s : "_");
}

/* Resolves a path.  Returns the number of leading segments resolved; the
   rest (associated items) are left for check.c.  Sets PATH.sym and
   PATH_SEG.sym of the resolved segments. */
static int resolve_path(Node *path, int what) {
    int ns = what_ns(what);
    long n = path->list.n, i;
    Node **sg = path->list.v;
    Sym *cur = 0;
    Node *seg;
    Name *name;

    if (path->flags & F_QUALIFIED) {
        resolve_type(path->a);
        if (path->b) {
            resolve_path(path->b, W_TRAIT);
            if (path->b->sym && ((Sym *)path->b->sym)->kind != SYM_TRAIT)
                error_at(path->b->file, path->b->line, "expected trait, found %s `%s`",
                         sym_kind_name(((Sym *)path->b->sym)->kind), ((Sym *)path->b->sym)->name->s);
        }
        for (i = 0; i < n; i++) resolve_seg_args(sg[i]);
        path->sym = path->b ? path->b->sym : 0;
        return 0;
    }
    if (n == 0) return 0;
    seg = sg[0];
    name = seg->name;
    i = 0;

    if (path->flags & F_GLOBAL) {
        if ((name == n_core || name == n_std) && core_root) cur = core_root;
        else error_at(seg->file, seg->line, "failed to resolve: could not find `%s` in the list of imported crates", name->s);
        seg->sym = cur;
        i = 1;
    } else if (name == n_crate || name == n_dcrate) {
        cur = cur_crate_root;
        seg->sym = cur;
        i = 1;
    } else if (name == n_self && n > 1) {
        cur = cur_mod;
        seg->sym = cur;
        i = 1;
    } else if (name == n_super) {
        cur = cur_mod;
        while (i < n && sg[i]->name == n_super) {
            if (!cur->parent) error_at(sg[i]->file, sg[i]->line, "there are too many leading `super` keywords");
            cur = cur->parent;
            sg[i]->sym = cur;
            i++;
        }
    } else {
        int fns = n == 1 ? ns : NS_TYPE;
        Sym *s = lookup_lex(fns, name, seg, 0);
        if (!s && n == 1 && ns == NS_VALUE && name == n_Self) s = lookup_lex(NS_TYPE, name, seg, 0);
        if (!s && n > 1 && (name == n_core || name == n_std) && core_root) s = core_root;
        if (!s && n == 1 && ns == NS_TYPE) {
            Sym *v = lookup_lex(NS_VALUE, name, seg, 0);
            if (v && (v->kind == SYM_CONST_GENERIC || v->kind == SYM_CONST) && what == W_TYPE) s = v;
            else if (v) expected_found(what, v, seg);
        }
        if (!s && n == 1 && ns == NS_VALUE) {
            Sym *t = lookup_lex(NS_TYPE, name, seg, 0);
            if (t) expected_found(what, t, seg);
        }
        if (!s) {
            if (n > 1) {
                if (name == n_Self)
                    error_at(seg->file, seg->line,
                             "failed to resolve: `Self` is only available in impls, traits, and type definitions");
                error_at(seg->file, seg->line, "failed to resolve: use of undeclared crate or module `%s`", name->s);
            }
            error_at(seg->file, seg->line, "cannot find %s `%s` in this scope", what_noun(what), name->s);
        }
        seg->sym = s;
        cur = s;
        i = 1;
    }

    while (i < n) {
        int last = (i == n - 1);
        Name *nm = sg[i]->name;
        if (cur->kind == SYM_MOD) {
            SEnt *e;
            int lns = last ? ns : NS_TYPE;
            if (nm == n_super && cur->parent) {
                cur = cur->parent;
                sg[i]->sym = cur;
                i++;
                continue;
            }
            e = ent_find(cur->members, lns, nm);
            if (!e && last && what == W_TYPE) {
                /* a const used as a generic argument */
                e = ent_find(cur->members, NS_VALUE, nm);
                if (e && e->sym->kind != SYM_CONST && e->sym->kind != SYM_CONST_GENERIC) e = 0;
            }
            if (!e) {
                SEnt *o = last ? ent_find(cur->members, lns == NS_TYPE ? NS_VALUE : NS_TYPE, nm) : 0;
                StrBuf sb;
                if (o && accessible(o, cur_mod)) expected_found(what, o->sym, sg[i]);
                sb_init(&sb);
                if (cur == cur_crate_root || cur->parent == 0) sb_puts(&sb, "the crate root");
                else {
                    sb_puts(&sb, "module `");
                    sym_path(cur, &sb);
                    sb_puts(&sb, "`");
                }
                if (last) {
                    if (cur->parent == 0 && cur != cur_crate_root)
                        error_at(sg[i]->file, sg[i]->line, "cannot find %s `%s` in crate `%s`", what_noun(what), nm->s, cur->name->s);
                    error_at(sg[i]->file, sg[i]->line, "cannot find %s `%s` in %s", what_noun(what), nm->s, sb.s);
                }
                error_at(sg[i]->file, sg[i]->line, "failed to resolve: could not find `%s` in %s", nm->s, sb.s);
            }
            if (!accessible(e, cur_mod))
                error_at(sg[i]->file, sg[i]->line, "%s `%s` is private", sym_kind_name(e->sym->kind), nm->s);
            cur = e->sym;
            sg[i]->sym = cur;
            i++;
            continue;
        }
        if (cur->kind == SYM_ENUM) {
            SEnt *e = ent_find(cur->members, NS_TYPE, nm);
            if (e) {
                cur = e->sym;
                sg[i]->sym = cur;
                i++;
                continue;
            }
            break;
        }
        if (is_type_like(cur)) break;
        error_at(sg[i]->file, sg[i]->line, "failed to resolve: `%s` is a %s, not a module",
                 sg[i - 1]->name->s, sym_kind_name(cur->kind));
    }

    if (i == n && (what == W_VALUE || what == W_FN || what == W_TSTRUCT || what == W_UNIT)) {
        if (cur->kind == SYM_MOD)
            error_at(sg[n - 1]->file, sg[n - 1]->line, "expected %s, found module `%s`",
                     what == W_FN ? "function" : what_noun(what), sg[n - 1]->name->s);
        if ((what == W_VALUE || what == W_FN) && cur->kind == SYM_VARIANT && (cur->node->flags & F_NAMED))
            error_at(sg[n - 1]->file, sg[n - 1]->line, "expected %s, found struct variant `%s`",
                     what == W_FN ? "function" : "value", sg[n - 1]->name->s);
    }
    for (i = 0; i < n; i++) resolve_seg_args(sg[i]);
    path->sym = cur;
    {
        long k = 0;
        while (k < n && sg[k]->sym) k++;
        return (int)k;
    }
}

/* ---------------------------------------------------------------- types */

static void resolve_type(Node *t) {
    long i;
    if (!t) return;
    switch (t->kind) {
    case N_T_PATH:
        resolve_path(t->a, W_TYPE);
        t->sym = t->a->sym;
        break;
    case N_T_REF: case N_T_PTR: case N_T_SLICE:
        resolve_type(t->a);
        break;
    case N_T_ARRAY:
        resolve_type(t->a);
        in_body++;
        resolve_expr(t->b);
        in_body--;
        break;
    case N_T_TUPLE:
        for (i = 0; i < t->list.n; i++) resolve_type(t->list.v[i]);
        break;
    case N_T_FN:
        for (i = 0; i < t->list.n; i++) resolve_type(t->list.v[i]->b);
        resolve_type(t->b);
        break;
    case N_T_DYN: case N_T_IMPL:
        resolve_bounds(t->a);
        break;
    default:
        break;
    }
}

static void resolve_bounds(Node *bs) {
    long i;
    if (!bs) return;
    for (i = 0; i < bs->list.n; i++) {
        Node *b = bs->list.v[i];
        Node *p;
        if (b->kind != N_BOUND_TRAIT) continue;
        p = b->a;
        if ((b->flags & F_MAYBE) && p->list.n == 1 && !lookup_lex(NS_TYPE, p->list.v[0]->name, p, 0)) continue;
        resolve_path(p, W_TRAIT);
        if (p->sym && ((Sym *)p->sym)->kind != SYM_TRAIT)
            error_at(p->file, p->line, "expected trait, found %s `%s`", sym_kind_name(((Sym *)p->sym)->kind), ((Sym *)p->sym)->name->s);
        b->sym = p->sym;
    }
}

static void resolve_where(Node *w) {
    long i;
    if (!w) return;
    for (i = 0; i < w->list.n; i++) {
        Node *c = w->list.v[i];
        if (c->kind == N_WHERE_TYPE) {
            resolve_type(c->a);
            resolve_bounds(c->b);
        }
    }
}

/* declares the generic parameters of an owner in the current scope, then
   resolves their bounds and defaults */
static void resolve_generics(Node *g, Sym *owner) {
    long i;
    int idx = 0;
    if (!g) return;
    for (i = 0; i < g->list.n; i++) {
        Node *p = g->list.v[i];
        Sym *s;
        int ns;
        if (p->kind == N_GEN_TYPE) {
            s = sym_new(SYM_GENERIC, p->name, p, owner);
            ns = NS_TYPE;
        } else if (p->kind == N_GEN_CONST) {
            s = sym_new(SYM_CONST_GENERIC, p->name, p, owner);
            ns = NS_VALUE;
        } else continue;
        s->index = idx++;
        p->sym = s;
        if (ent_find(sc, ns, p->name))
            error_at(p->file, p->line,
                     "the name `%s` is already used for a generic parameter in this item's generic parameters",
                     p->name->s);
        ent_set(sc, ns, p->name, s);
    }
    for (i = 0; i < g->list.n; i++) {
        Node *p = g->list.v[i];
        if (p->kind == N_GEN_TYPE) {
            resolve_bounds(p->b);
            resolve_type(p->a);
        } else if (p->kind == N_GEN_CONST) {
            resolve_type(p->a);
            if (p->b) resolve_expr(p->b);
        }
    }
}

/* ---------------------------------------------------------------- patterns */

enum { PK_LET = 0, PK_MATCH, PK_FOR, PK_PARAM };

typedef struct {
    Name **names;
    Sym **syms;
    char *bound;         /* bound in the alternative being resolved */
    int n, cap;
    int later;           /* inside the 2nd or later alternative of an or-pattern */
    int kind;
} PatCtx;

static void pc_init(PatCtx *pc, int kind) {
    memset(pc, 0, sizeof(PatCtx));
    pc->kind = kind;
}

static const char *pk_descr(int kind) {
    switch (kind) {
    case PK_LET: return "let binding";
    case PK_MATCH: return "match binding";
    case PK_FOR: return "`for` loop binding";
    }
    return "function parameter";
}

static void bind_name(PatCtx *pc, Node *p) {
    int i;
    for (i = 0; i < pc->n; i++)
        if (pc->names[i] == p->name) break;
    if (i < pc->n) {
        if (pc->bound[i]) {
            if (pc->kind == PK_PARAM)
                error_at(p->file, p->line, "identifier `%s` is bound more than once in this parameter list", p->name->s);
            error_at(p->file, p->line, "identifier `%s` is bound more than once in the same pattern", p->name->s);
        }
        pc->bound[i] = 1;
        p->sym = pc->syms[i];
        return;
    }
    if (pc->n == pc->cap) {
        pc->cap = pc->cap ? pc->cap * 2 : 8;
        pc->names = (Name **)xrealloc(pc->names, sizeof(Name *) * pc->cap);
        pc->syms = (Sym **)xrealloc(pc->syms, sizeof(Sym *) * pc->cap);
        pc->bound = (char *)xrealloc(pc->bound, sizeof(char) * pc->cap);
    }
    {
        Sym *s = sym_new(SYM_LOCAL, p->name, p, cur_fn);
        if ((p->flags & F_MUT) && !(p->flags & F_REF)) s->flags |= SF_MUT;
        s->index = nlocals++;
        p->sym = s;
        pc->names[pc->n] = p->name;
        pc->syms[pc->n] = s;
        pc->bound[pc->n] = 1;
        pc->n++;
    }
}

static void pc_commit(PatCtx *pc, Scope *target) {
    int i;
    for (i = 0; i < pc->n; i++) ent_set(target, NS_VALUE, pc->names[i], pc->syms[i]);
}

static void resolve_pat(Node *p, PatCtx *pc) {
    long i;
    if (!p) return;
    switch (p->kind) {
    case N_P_IDENT:
        if (!p->a && !(p->flags & (F_REF | F_MUT))) {
            Sym *s = lookup_lex(NS_VALUE, p->name, p, 1);
            if (s) {
                if (s->kind == SYM_CONST ||
                    ((s->kind == SYM_STRUCT || s->kind == SYM_VARIANT) && !(s->node->flags & F_TUPLE))) {
                    p->sym = s;
                    return;
                }
                if (s->kind == SYM_STATIC)
                    error_at(p->file, p->line, "%ss cannot shadow statics", pk_descr(pc->kind));
                if (s->kind == SYM_STRUCT || s->kind == SYM_VARIANT)
                    error_at(p->file, p->line, "%ss cannot shadow tuple %s", pk_descr(pc->kind),
                             s->kind == SYM_STRUCT ? "structs" : "variants");
            }
        }
        bind_name(pc, p);
        if (p->a) resolve_pat(p->a, pc);
        break;
    case N_P_RANGE:
        resolve_pat(p->a, pc);
        resolve_pat(p->b, pc);
        break;
    case N_P_REF:
        resolve_pat(p->a, pc);
        break;
    case N_P_TUPLE: case N_P_SLICE:
        for (i = 0; i < p->list.n; i++) resolve_pat(p->list.v[i], pc);
        break;
    case N_P_PATH:
        resolve_path(p->a, W_UNIT);
        p->sym = p->a->sym;
        break;
    case N_P_TUPLE_STRUCT:
        resolve_path(p->a, W_TSTRUCT);
        p->sym = p->a->sym;
        for (i = 0; i < p->list.n; i++) resolve_pat(p->list.v[i], pc);
        break;
    case N_P_STRUCT:
        resolve_path(p->a, W_STRUCT);
        p->sym = p->a->sym;
        for (i = 0; i < p->list.n; i++) resolve_pat(p->list.v[i], pc);
        break;
    case N_P_FIELD:
        resolve_pat(p->a, pc);
        break;
    case N_P_OR: {
        int n0 = pc->n, n1 = 0, k, j;
        char *s0 = (char *)xalloc(n0 + 1), *b0 = 0;
        if (n0) memcpy(s0, pc->bound, n0);
        for (k = 0; k < p->list.n; k++) {
            Node *alt = p->list.v[k];
            if (k > 0) {
                if (n0) memcpy(pc->bound, s0, n0);
                for (j = n0; j < n1; j++) pc->bound[j] = 0;
                pc->later++;
            }
            resolve_pat(alt, pc);
            if (k == 0) {
                n1 = pc->n;
                b0 = (char *)xalloc(n1 + 1);
                if (n1) memcpy(b0, pc->bound, n1);
            } else {
                pc->later--;
                for (j = 0; j < n1; j++)
                    if (pc->bound[j] != b0[j])
                        error_at(alt->file, alt->line, "variable `%s` is not bound in all patterns", pc->names[j]->s);
                if (pc->n > n1)
                    error_at(alt->file, alt->line, "variable `%s` is not bound in all patterns", pc->names[n1]->s);
            }
        }
        if (n1) memcpy(pc->bound, b0, n1);
        break;
    }
    default:
        break;
    }
}

/* ---------------------------------------------------------------- expressions */

static void check_label(Node *e) {
    int i;
    if (!e->name) return;
    for (i = nlabels - 1; i >= label_base; i--)
        if (labels[i] == e->name) return;
    error_at(e->file, e->line, "use of undeclared label `%s`", e->name->s);
}

static void resolve_closure(Node *e) {
    Scope *s0 = sc, *cs = scope_new(sc);
    int sbase = label_base;
    PatCtx pc;
    long i;
    label_base = nlabels;
    pc_init(&pc, PK_PARAM);
    for (i = 0; i < e->list.n; i++) {
        Node *p = e->list.v[i];
        if (p->b) resolve_type(p->b);
        if (p->a) resolve_pat(p->a, &pc);
    }
    pc_commit(&pc, cs);
    sc = cs;
    if (e->a) resolve_type(e->a);
    resolve_expr(e->b);
    sc = s0;
    label_base = sbase;
}

static void resolve_expr(Node *e) {
    long i;
    if (!e) return;
    switch (e->kind) {
    case N_E_PATH:
        resolve_path(e->a, W_VALUE);
        e->sym = e->a->sym;
        break;
    case N_E_PAREN: case N_E_UNARY: case N_E_DEREF: case N_E_REF: case N_E_TRY: case N_E_AWAIT:
    case N_E_FIELD: case N_E_TUPLE_FIELD:
        resolve_expr(e->a);
        break;
    case N_E_BINARY: case N_E_ASSIGN: case N_E_ASSIGN_OP: case N_E_INDEX: case N_E_RANGE: case N_E_REPEAT:
        resolve_expr(e->a);
        resolve_expr(e->b);
        break;
    case N_E_CAST:
        resolve_expr(e->a);
        resolve_type(e->b);
        break;
    case N_E_CALL:
        if (e->a && e->a->kind == N_E_PATH) {
            resolve_path(e->a->a, W_FN);
            e->a->sym = e->a->a->sym;
        } else resolve_expr(e->a);
        for (i = 0; i < e->list.n; i++) resolve_expr(e->list.v[i]);
        break;
    case N_E_METHOD_CALL:
        resolve_expr(e->a);
        if (e->b) resolve_seg_args(e->b);
        for (i = 0; i < e->list.n; i++) resolve_expr(e->list.v[i]);
        break;
    case N_BLOCK:
        resolve_block(e);
        break;
    case N_E_IF: {
        Scope *s0 = sc;
        resolve_expr(e->a);
        resolve_block(e->b);
        sc = s0;
        resolve_expr(e->c);
        break;
    }
    case N_E_LET: {
        Scope *ns;
        PatCtx pc;
        resolve_expr(e->b);
        ns = scope_new(sc);
        pc_init(&pc, PK_LET);
        resolve_pat(e->a, &pc);
        pc_commit(&pc, ns);
        sc = ns;
        break;
    }
    case N_E_MATCH:
        resolve_expr(e->a);
        for (i = 0; i < e->list.n; i++) {
            Node *arm = e->list.v[i];
            Scope *s0 = sc, *as = scope_new(sc);
            PatCtx pc;
            pc_init(&pc, PK_MATCH);
            resolve_pat(arm->a, &pc);
            pc_commit(&pc, as);
            sc = as;
            resolve_expr(arm->b);
            resolve_expr(arm->c);
            sc = s0;
        }
        break;
    case N_E_WHILE: {
        Scope *s0 = sc;
        int sl = nlabels;
        if (e->name) label_push(e->name);
        resolve_expr(e->a);
        resolve_block(e->b);
        sc = s0;
        nlabels = sl;
        break;
    }
    case N_E_LOOP: {
        int sl = nlabels;
        if (e->name) label_push(e->name);
        resolve_block(e->a);
        nlabels = sl;
        break;
    }
    case N_E_FOR: {
        Scope *s0 = sc, *fs;
        int sl = nlabels;
        PatCtx pc;
        resolve_expr(e->b);
        fs = scope_new(sc);
        pc_init(&pc, PK_FOR);
        resolve_pat(e->a, &pc);
        pc_commit(&pc, fs);
        sc = fs;
        if (e->name) label_push(e->name);
        resolve_block(e->c);
        sc = s0;
        nlabels = sl;
        break;
    }
    case N_E_BREAK:
        check_label(e);
        resolve_expr(e->a);
        break;
    case N_E_CONTINUE:
        check_label(e);
        break;
    case N_E_RETURN:
        resolve_expr(e->a);
        break;
    case N_E_CLOSURE:
        resolve_closure(e);
        break;
    case N_E_STRUCT:
        resolve_path(e->a, W_STRUCT);
        e->sym = e->a->sym;
        for (i = 0; i < e->list.n; i++) resolve_expr(e->list.v[i]->a);
        resolve_expr(e->b);
        break;
    case N_E_TUPLE: case N_E_ARRAY:
        for (i = 0; i < e->list.n; i++) resolve_expr(e->list.v[i]);
        break;
    default:
        break;     /* literals, `_`, unexpanded macro calls */
    }
}

static int is_item_kind(int k) {
    switch (k) {
    case N_ITEM_FN: case N_ITEM_STRUCT: case N_ITEM_ENUM: case N_ITEM_UNION: case N_ITEM_TRAIT:
    case N_ITEM_IMPL: case N_ITEM_MOD: case N_ITEM_CONST: case N_ITEM_STATIC: case N_ITEM_TYPE:
    case N_ITEM_EXTERN_BLOCK:
        return 1;
    }
    return 0;
}

static void resolve_block(Node *b) {
    Scope *s0 = sc, *bs = scope_new(sc);
    int sl = nlabels, from = nimports;
    long i;
    sc = bs;
    in_body++;
    if (b->name) label_push(b->name);
    collect_items(&b->list, bs, cur_mod);
    if (nimports > from) process_imports(from);
    for (i = 0; i < b->list.n; i++) {
        Node *s = b->list.v[i];
        if (is_item_kind(s->kind)) {
            resolve_item(s);
            sc = bs;
        } else if (s->kind == N_STMT_LET) {
            PatCtx pc;
            resolve_type(s->b);
            resolve_expr(s->c);
            sc = bs;
            if (s->d) resolve_block(s->d);
            pc_init(&pc, PK_LET);
            resolve_pat(s->a, &pc);
            pc_commit(&pc, bs);
        } else if (s->kind == N_STMT_EXPR) {
            resolve_expr(s->a);
            sc = bs;
        }
    }
    resolve_expr(b->a);
    nlabels = sl;
    sc = s0;
    in_body--;
}

/* ---------------------------------------------------------------- items */

static void resolve_fn(Node *fn, int nested) {
    Sym *fs = (Sym *)fn->sym;
    Scope *s0 = sc, *g = scope_new(sc), *ps;
    Sym *sf = cur_fn;
    int snl = nlocals, sbase = label_base;
    PatCtx pc;
    long i;
    g->boundary = nested;
    sc = g;
    resolve_generics(fn->a, fs);
    resolve_where(fn->d);
    ps = scope_new(g);
    pc_init(&pc, PK_PARAM);
    cur_fn = fs;
    nlocals = 0;
    label_base = nlabels;
    for (i = 0; i < fn->list.n; i++) {
        Node *p = fn->list.v[i];
        if (p->kind == N_SELF_PARAM) {
            Sym *l = sym_new(SYM_LOCAL, n_self, p, fs);
            if ((p->flags & F_MUT) && !(p->flags & F_REF)) l->flags |= SF_MUT;
            l->index = nlocals++;
            p->sym = l;
            ent_set(ps, NS_VALUE, n_self, l);
            if (p->b) resolve_type(p->b);
        } else {
            resolve_type(p->b);
            if (p->a) resolve_pat(p->a, &pc);
        }
    }
    pc_commit(&pc, ps);
    resolve_type(fn->b);
    if (fn->c) {
        sc = ps;
        in_body++;
        resolve_block(fn->c);
        in_body--;
    }
    sc = s0;
    cur_fn = sf;
    nlocals = snl;
    label_base = sbase;
}

static void resolve_const(Node *n, int nested) {
    Scope *s0 = sc, *g = scope_new(sc);
    Sym *sf = cur_fn;
    int snl = nlocals, sbase = label_base;
    g->boundary = nested;
    sc = g;
    cur_fn = (Sym *)n->sym;
    nlocals = 0;
    label_base = nlabels;
    resolve_type(n->a);
    if (n->b) {
        in_body++;
        resolve_expr(n->b);
        in_body--;
    }
    sc = s0;
    cur_fn = sf;
    nlocals = snl;
    label_base = sbase;
}

static void resolve_alias(Node *n, int nested) {
    Scope *s0 = sc, *g = scope_new(sc);
    g->boundary = nested;
    sc = g;
    resolve_generics(n->a, (Sym *)n->sym);
    resolve_where(n->d);
    resolve_type(n->b);
    resolve_bounds(n->c);
    sc = s0;
}

static void resolve_fields(Node *owner) {
    long i;
    for (i = 0; i < owner->list.n; i++) resolve_type(owner->list.v[i]->a);
}

static void resolve_adt(Node *n, int nested) {
    Scope *s0 = sc, *g = scope_new(sc);
    long i;
    g->boundary = nested;
    sc = g;
    resolve_generics(n->a, (Sym *)n->sym);
    resolve_where(n->d);
    if (n->kind == N_ITEM_ENUM) {
        for (i = 0; i < n->list.n; i++) {
            Node *v = n->list.v[i];
            resolve_fields(v);
            if (v->a) {
                in_body++;
                resolve_expr(v->a);
                in_body--;
            }
        }
    } else resolve_fields(n);
    sc = s0;
}

static void resolve_assoc_items(Node *n) {
    long i;
    for (i = 0; i < n->list.n; i++) {
        Node *it = n->list.v[i];
        if (it->kind == N_ITEM_FN) resolve_fn(it, 0);
        else if (it->kind == N_ITEM_CONST) resolve_const(it, 0);
        else if (it->kind == N_ITEM_TYPE) resolve_alias(it, 0);
    }
}

static void resolve_trait(Node *n, int nested) {
    Scope *s0 = sc, *g = scope_new(sc);
    Sym *ts = (Sym *)n->sym;
    g->boundary = nested;
    sc = g;
    ent_set(g, NS_TYPE, n_Self, sym_new(SYM_SELF_TY, n_Self, n, ts));
    resolve_generics(n->a, ts);
    resolve_bounds(n->c);
    resolve_where(n->d);
    resolve_assoc_items(n);
    sc = s0;
}

static void resolve_impl(Node *n, int nested) {
    Scope *s0 = sc, *g = scope_new(sc);
    Sym *is = (Sym *)n->sym;
    g->boundary = nested;
    sc = g;
    ent_set(g, NS_TYPE, n_Self, sym_new(SYM_SELF_TY, n_Self, n, is));
    resolve_generics(n->a, is);
    resolve_type(n->b);
    if (n->c) {
        resolve_path(n->c, W_TRAIT);
        if (n->c->sym && ((Sym *)n->c->sym)->kind != SYM_TRAIT)
            error_at(n->c->file, n->c->line, "expected trait, found %s `%s`",
                     sym_kind_name(((Sym *)n->c->sym)->kind), ((Sym *)n->c->sym)->name->s);
        is->trait = n->c->sym;
    }
    resolve_where(n->d);
    resolve_assoc_items(n);
    sc = s0;
}

static void resolve_mod(Node *n) {
    Sym *m = (Sym *)n->sym;
    Scope *s0 = sc;
    Sym *sm = cur_mod;
    long i;
    cur_mod = m;
    sc = m->members;
    for (i = 0; i < n->list.n; i++) resolve_item(n->list.v[i]);
    sc = s0;
    cur_mod = sm;
}

static void resolve_item(Node *n) {
    int nested = in_body > 0;
    long i;
    switch (n->kind) {
    case N_ITEM_FN: resolve_fn(n, nested); break;
    case N_ITEM_STRUCT: case N_ITEM_UNION: case N_ITEM_ENUM: resolve_adt(n, nested); break;
    case N_ITEM_TRAIT: resolve_trait(n, nested); break;
    case N_ITEM_IMPL: resolve_impl(n, nested); break;
    case N_ITEM_MOD: resolve_mod(n); break;
    case N_ITEM_CONST: case N_ITEM_STATIC: resolve_const(n, nested); break;
    case N_ITEM_TYPE: resolve_alias(n, nested); break;
    case N_ITEM_EXTERN_BLOCK:
        for (i = 0; i < n->list.n; i++) {
            Node *it = n->list.v[i];
            if (it->kind == N_ITEM_FN) resolve_fn(it, nested);
            else if (it->kind == N_ITEM_STATIC) resolve_const(it, nested);
        }
        break;
    default:
        break;
    }
}

/* ================================================================ entry */

static void add_prim(const char *nm, int kind, int ik) {
    Name *n = name_intern_str(nm);
    Sym *s = sym_new(SYM_PRIM, n, 0, 0);
    s->prim = kind;
    s->ik = ik;
    s->flags |= SF_PUB;
    scope_put(prim_scope, NS_TYPE, n, s);
}

static void resolve_crate_items(Node *crate, Sym *root, int core) {
    long i;
    building_core = core;
    cur_crate_root = root;
    cur_mod = root;
    sc = root->members;
    cur_fn = 0;
    in_body = 0;
    for (i = 0; i < crate->list.n; i++) resolve_item(crate->list.v[i]);
    building_core = 0;
}

void resolve_crate(Node *crate, Node *core) {
    static const char *inames[] = {"i8", "i16", "i32", "i64", "i128", "isize",
                                   "u8", "u16", "u32", "u64", "u128", "usize"};
    int i;
    n_self = name_intern_str("self");
    n_Self = name_intern_str("Self");
    n_super = name_intern_str("super");
    n_crate = name_intern_str("crate");
    n_dcrate = name_intern_str("$crate");
    n_core = name_intern_str("core");
    n_std = name_intern_str("std");
    n_prelude = name_intern_str("prelude");
    n_underscore = name_intern_str("_");
    n_lang = name_intern_str("lang");
    n_main = name_intern_str("main");
    (void)n_main;

    prim_scope = scope_new(0);
    fallback_scope = scope_new(prim_scope);
    add_prim("bool", TY_BOOL, 0);
    add_prim("char", TY_CHAR, 0);
    add_prim("str", TY_STR, 0);
    for (i = 0; i < 12; i++) add_prim(inames[i], TY_INT, IK_I8 + i);
    add_prim("f32", TY_FLOAT, 32);
    add_prim("f64", TY_FLOAT, 64);

    lang_file = core ? core->file : crate->file;
    core_root = 0;
    if (core) {
        building_core = 1;
        core_root = sym_new(SYM_MOD, n_core, core, 0);
        core_root->flags |= SF_PUB;
        core_root->members = scope_new(fallback_scope);
        core->sym = core_root;
        cur_crate_root = core_root;
        collect_items(&core->list, core_root->members, core_root);
        building_core = 0;
    }
    crate_root = sym_new(SYM_MOD, n_crate, crate, 0);
    crate_root->flags |= SF_PUB;
    crate_root->members = scope_new(fallback_scope);
    crate->sym = crate_root;
    cur_crate_root = crate_root;
    collect_items(&crate->list, crate_root->members, crate_root);

    process_imports(0);

    if (core) resolve_crate_items(core, core_root, 1);
    resolve_crate_items(crate, crate_root, 0);
}
