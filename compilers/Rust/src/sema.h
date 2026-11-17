/*
 * sema.h - semantic analysis of the Rust translator: macro expansion, name
 * resolution, types, item collection, type checking and move checking.
 *
 * Pipeline (rustc.c calls these in order, each stops at the first error with
 * error_at(): file:line: error: message):
 *
 *   1. expand.c   expand_crate(crate)   macro_rules! and the built-in macros
 *                                       (println!, format!, vec!, assert!, ...)
 *                                       are replaced by ordinary AST.
 *   2. resolve.c  resolve_crate(crate, core) builds the module tree, processes
 *                                       `use`, and sets node->sym on every
 *                                       path (E_PATH, T_PATH, P_PATH, P_IDENT
 *                                       that names a const/unit variant,
 *                                       E_STRUCT, P_STRUCT, P_TUPLE_STRUCT,
 *                                       BOUND_TRAIT, ITEM_IMPL trait) and on
 *                                       every binding (P_IDENT -> SYM_LOCAL).
 *   3. collect.c  collect_crate()       item signatures (Sym.ty), generics,
 *                                       struct fields, enum variants, impls
 *                                       and trait impls, lang items.
 *   4. check.c    check_crate()         type inference and checking of every
 *                                       body: sets node->ty on every
 *                                       expression and pattern, node->sym on
 *                                       method calls / fields / operators
 *                                       (the impl fn chosen), and records
 *                                       implicit adjustments (auto-ref,
 *                                       deref, unsizing) in Node.adj.
 *   5. moves.c    moves_crate()         use after move, assignment to an
 *                                       immutable binding, &mut of an
 *                                       immutable place.  (No lifetime or
 *                                       aliasing borrow checking.)
 *
 * The prelude is Rust source, lib/core.rs, parsed as the crate `core` and
 * analysed together with the user crate (rustc.c parses it first).  Its
 * items are visible in every module through the prelude scope; language
 * items are marked with #[lang = "name"] and found with lang_item().
 * Compiler built-ins are declared in lib/core.rs as
 *     extern "rust-intrinsic" { fn name(...) -> T; }
 * and code generation (Stage 4) implements them.
 */
#ifndef SEMA_H
#define SEMA_H

#include "parser.h"

typedef struct Sym Sym;
typedef struct Scope Scope;
typedef struct Type Type;

/* ================================================================ symbols */

enum {
    SYM_MOD = 1,        /* module (also the crate roots); members */
    SYM_STRUCT,         /* node = ITEM_STRUCT; members = fields (SYM_FIELD) */
    SYM_UNION,
    SYM_ENUM,           /* node = ITEM_ENUM; members = variants */
    SYM_VARIANT,        /* node = VARIANT; parent = the enum; index = variant number;
                           members = fields */
    SYM_FIELD,          /* node = FIELD; parent = struct/variant; index = position */
    SYM_TRAIT,          /* node = ITEM_TRAIT; members = assoc items */
    SYM_FN,             /* node = ITEM_FN; parent = module, impl or trait */
    SYM_CONST,
    SYM_STATIC,
    SYM_TYPEALIAS,      /* node = ITEM_TYPE */
    SYM_ASSOC_TYPE,     /* `type X;` in a trait or `type X = T;` in an impl */
    SYM_IMPL,           /* node = ITEM_IMPL; members = assoc items */
    SYM_GENERIC,        /* type parameter; index = position in its owner's generics */
    SYM_CONST_GENERIC,  /* const N: usize */
    SYM_LOCAL,          /* a binding: let, parameter, match arm, closure parameter */
    SYM_PRIM,           /* primitive type name: i32, bool, str, ... ; prim = TY_xxx kind */
    SYM_MACRO,          /* macro_rules! definition (node = ITEM_MACRO_RULES) */
    SYM_SELF_TY         /* `Self` inside an impl or trait (parent = the impl/trait) */
};

/* Sym.flags */
#define SF_PUB        1
#define SF_MUT        2      /* static mut, mutable binding */
#define SF_METHOD     4      /* fn with a self parameter */
#define SF_EXTERN     8      /* declared in an extern block (incl. intrinsics) */
#define SF_INTRINSIC 16      /* extern "rust-intrinsic" */
#define SF_COPY      32      /* the type implements Copy (set by collect) */
#define SF_CORE      64      /* defined in lib/core.rs */

struct Sym {
    int kind;                /* SYM_xxx */
    Name *name;
    Node *node;              /* the declaring node (item, field, variant, binding pattern) */
    Sym *parent;             /* enclosing module / impl / trait / enum / struct */
    Scope *members;          /* SYM_MOD, STRUCT, UNION, ENUM, VARIANT, TRAIT, IMPL */
    long flags;              /* SF_xxx */
    int index;               /* variant / field / generic / local number */
    int prim;                /* SYM_PRIM: the TY_xxx kind; for ints also ik */
    int ik;

    /* filled by collect.c */
    Type *ty;                /* fn: TY_FN_DEF signature; const/static/field/local: its type;
                                struct/enum/union: the self type TY_ADT with its own params;
                                alias / assoc type: the aliased type (0 in a trait) */
    Sym **gparams;           /* SYM_GENERIC / SYM_CONST_GENERIC in order (incl. the impl's or
                                trait's params first for associated fns) */
    int ngparams;
    struct Bound *bounds;    /* where-clause and inline bounds on the generics */
    int nbounds;
    Sym *trait;              /* SYM_IMPL: the trait or 0 (inherent); assoc item of a trait impl:
                                the trait's item it implements (in trait_item) */
    Sym *trait_item;
    Type *impl_self;         /* SYM_IMPL: the self type */
    Type **trait_args;       /* SYM_IMPL: the trait's generic arguments */
    int ntrait_args;
    Name *lang;              /* #[lang = "..."] */

    /* filled by later stages */
    void *cg;                /* code generator's data */
    Sym *next_impl;          /* collect.c: chain of impls on the same type constructor / trait */
};

typedef struct Bound {
    Type *ty;                /* the bounded type (usually TY_PARAM) */
    Sym *trait;
    Type **args;             /* trait generic args */
    int nargs;
    Name *assoc_name;        /* `T: Iterator<Item = X>`: Item ... */
    Type *assoc_ty;          /* ... = X */
} Bound;

/* A scope maps names to symbols in three namespaces. */
enum { NS_TYPE = 0, NS_VALUE = 1, NS_MACRO = 2 };

Scope *scope_new(Scope *parent);
Sym *scope_get(Scope *s, int ns, Name *name);         /* this scope only */
Sym *scope_lookup(Scope *s, int ns, Name *name);      /* this scope and its parents */
void scope_put(Scope *s, int ns, Name *name, Sym *sym);   /* error on duplicate is the caller's job */
Sym *sym_new(int kind, Name *name, Node *node, Sym *parent);
const char *sym_kind_name(int kind);
void sym_path(Sym *s, StrBuf *out);                   /* "core::option::Option" for messages */

extern Sym *crate_root;      /* the user crate (SYM_MOD) */
extern Sym *core_root;       /* lib/core.rs (SYM_MOD), 0 if not loaded */
Sym *lang_item(const char *name);    /* #[lang = name] item, or error "missing lang item" */
extern Sym **resolve_impls;          /* every impl, both crates, in source order */
extern int resolve_nimpls;
int scope_member_count(Scope *s);    /* members in definition order */
Sym *scope_member(Scope *s, int i);

/* ================================================================ types */

enum {
    TY_ERR = 0,
    TY_INFER,       /* inference variable: link = binding or 0; ik = INFER_INT / INFER_FLOAT / 0 */
    TY_BOOL, TY_CHAR, TY_INT, TY_FLOAT, TY_STR, TY_NEVER,
    TY_TUPLE,       /* args[0..nargs) ; unit = 0 args */
    TY_ARRAY,       /* elem; len (or len_param for [T; N]) */
    TY_SLICE,       /* elem */
    TY_REF,         /* elem; mut */
    TY_PTR,         /* elem; mut */
    TY_ADT,         /* def = struct/enum/union Sym; args = generic args */
    TY_FN_DEF,      /* def = fn Sym; args = generic args; params / ret = the instantiated signature */
    TY_FN_PTR,      /* params[0..nparams), ret */
    TY_CLOSURE,     /* def = 0; node = E_CLOSURE; params, ret; captures in the node */
    TY_PARAM,       /* def = SYM_GENERIC (index = its position) */
    TY_DYN,         /* def = the trait; args = its generic args */
    TY_PROJ,        /* <args[0] as def>::name (associated type not yet normalized) */
    TY_SELF         /* Self in a trait (before substitution) */
};

/* integer kinds (Type.ik) */
enum { IK_I8 = 1, IK_I16, IK_I32, IK_I64, IK_I128, IK_ISIZE,
       IK_U8, IK_U16, IK_U32, IK_U64, IK_U128, IK_USIZE };
#define INFER_INT   100      /* TY_INFER.ik: an integer literal's variable */
#define INFER_FLOAT 101

struct Type {
    int kind;
    int ik;                  /* TY_INT kind; TY_FLOAT: 32 or 64; TY_INFER: INFER_xxx */
    int mut;                 /* TY_REF / TY_PTR */
    long len;                /* TY_ARRAY */
    Sym *len_param;          /* TY_ARRAY with a const generic length */
    Type *elem;              /* REF, PTR, ARRAY, SLICE */
    Type **args;  int nargs; /* TUPLE, ADT, FN_DEF (generic args), DYN, PROJ */
    Type **params; int nparams;   /* FN_DEF, FN_PTR, CLOSURE */
    Type *ret;
    Sym *def;
    Name *name;              /* TY_PROJ: the associated type name */
    Node *node;              /* TY_CLOSURE */
    Type *link;              /* TY_INFER: the binding */
    int id;                  /* TY_INFER: number for messages ("{integer}", "_") */
};

/* constructors (types.c) */
Type *ty_prim(int kind);                 /* bool, char, str, never, err */
Type *ty_int(int ik);
Type *ty_float(int bits);
Type *ty_unit(void);
Type *ty_tuple(Type **args, int n);
Type *ty_ref(Type *elem, int mut);
Type *ty_ptr(Type *elem, int mut);
Type *ty_array(Type *elem, long len);
Type *ty_slice(Type *elem);
Type *ty_adt(Sym *def, Type **args, int n);
Type *ty_param(Sym *gen);
Type *ty_infer(int ik);                  /* a fresh inference variable */
Type *ty_fn_ptr(Type **params, int n, Type *ret);

Type *ty_resolve(Type *t);               /* follows TY_INFER links; never returns a bound var */
int ty_equal(Type *a, Type *b);          /* structural, after resolving */
int ty_is_unit(Type *t);
int ty_is_integral(Type *t);             /* TY_INT or an integer inference var */
int ty_is_numeric(Type *t);
int ty_has_infer(Type *t);

/* substitution of TY_PARAM (by Sym index) and TY_SELF */
typedef struct {
    Sym **params; Type **args; int n;
    Type *self;
} Subst;
Type *ty_subst(Type *t, Subst *s);

/* unification: binds inference variables; returns 1 on success.  Integer
 * variables only bind to TY_INT (or other integer variables), float ones to
 * TY_FLOAT.  TY_NEVER unifies with anything (coerces). */
int unify(Type *a, Type *b);

/* "i32", "&mut Vec<String>", "{integer}", "fn(i32) -> bool", "[u8; 4]" */
void ty_print(Type *t, StrBuf *out);
const char *ty_str(Type *t);             /* ty_print into a fresh string */

/* lowering of AST types after resolution (types.c); generics in scope come
 * from the T_PATH's resolved syms.  `_` gives a fresh inference variable. */
Type *ty_from_ast(Node *tnode);

/* defaulting after a body is checked: {integer} -> i32, {float} -> f64 */
void ty_default(Type *t);

/* ================================================================ adjustments */

/* Node.adj (an int field added to Node for sema use: see ast.h) holds the
 * implicit conversions check.c applied to an expression, in this order: */
#define ADJ_DEREF_SHIFT 0     /* bits 0..7: number of auto-derefs (built-in or Deref) */
#define ADJ_AUTOREF     256   /* then & taken */
#define ADJ_AUTOREF_MUT 512   /* then &mut taken */
#define ADJ_UNSIZE      1024  /* &[T; N] -> &[T], &T -> &dyn Trait, Box<T> -> Box<dyn Trait> */
#define ADJ_NEVER       2048  /* ! coerced to the expected type */
#define ADJ_REBORROW    4096  /* &mut T passed as &T */

/* ================================================================ entry points */

void expand_crate(Node *crate);          /* expand.c */
void resolve_crate(Node *crate, Node *core);   /* resolve.c: core may be 0 */
void collect_crate(void);                /* collect.c */
void check_crate(void);                  /* check.c */
void moves_crate(void);                  /* moves.c */

/* method and trait queries (collect.c), used by check.c and codegen */
Sym *find_impl(Sym *trait, Type *self, Type **trait_args, int ntrait_args, Subst *out);
                                         /* the impl of trait for self (generic impls are
                                            matched by unification; out gets the impl's
                                            generic args), or 0 */
int implements(Type *t, Sym *trait);     /* incl. bounds of TY_PARAM in the current fn, Copy for
                                            primitives, &T, raw pointers, tuples/arrays of Copy */
int is_copy(Type *t);
Sym *assoc_item(Sym *impl_or_trait, int ns, Name *name);
Type *normalize(Type *t);                /* resolves TY_PROJ through impls where possible */

/* ================================================================ debug dumps */

/* rustc --resolve / --types print the AST dump with ` ->path` (the node's
 * sym) and ` :type` (the node's ty) appended to each node line; ast.c
 * calls this hook for every node when it is set. */
void sema_dump_hook(FILE *f, Node *n);

#endif
