/*
 * cg.h - the code generator: Rust (checked AST) -> C for the C11 compiler.
 * The representation and conventions are in design-codegen.md; read it first.
 *
 * Files:
 *   cg_types.c  cg_crate() (the driver), the output sections, C types and
 *               layouts, monomorphization (the instance worklist), trait
 *               method resolution for an instance, vtables, drop glue,
 *               statics and consts, the C main
 *   cg_expr.c   cg_fn_body(): function and closure bodies: statements,
 *               expressions, places, patterns and match, operators with
 *               overflow checks, casts, calls, intrinsics, drops at scope end
 *   lib/rust_rt.c  the C runtime copied into every output (rs_* functions)
 */
#ifndef CG_H
#define CG_H

#include "sema.h"

/* ================================================================ output */

/* The output file is assembled from sections, in this order:
 *   CG_RT      lib/rust_rt.c (copied once)
 *   CG_TYPES   struct definitions, in dependency order
 *   CG_PROTOS  prototypes of every emitted function (so bodies can call
 *              each other in any order) and extern declarations of vtables
 *   CG_DATA    string literals, vtables, statics
 *   CG_FUNCS   function bodies (cg_types.c emits drop glue here too)
 *   CG_MAIN    the C main */
enum { CG_RT, CG_TYPES, CG_PROTOS, CG_DATA, CG_FUNCS, CG_MAIN, CG_NSECT };
extern StrBuf cg_sect[CG_NSECT];
void cg_printf(int sect, const char *fmt, ...);
void cg_line(Node *n);                 /* emits `#line N "file"` into CG_FUNCS if the line changed */
int cg_new_id(void);                   /* fresh number for names (F12, S7, t3, ...) */

/* the instance being generated: generic args of the current fn (and impl) */
extern Subst *cg_subst;
/* the current fn's Sym (0 in a closure body: see cg_cur_closure) */
extern Sym *cg_cur_fn;

/* substitutes cg_subst, normalizes projections, resolves inference links:
 * every type the code generator looks at goes through this first */
Type *cg_mono(Type *t);

/* ================================================================ types (cg_types.c) */

/* C type spelling of a (mono) Rust type: "long", "double", "struct S12",
 * "struct rs_slice", "struct rs_dyn".  Emits the definition on first use. */
const char *cg_ctype(Type *t);

/* Classification used by cg_expr.c */
int cg_is_zst(Type *t);                /* unit, never, zero-sized struct/tuple, fn item, capture-less closure */
int cg_is_u8(Type *t);                 /* u8: packed in arrays/slices/str, byte-addressed references */
int cg_is_unsized(Type *t);            /* str, [T], dyn Tr */
int cg_ref_kind(Type *pointee);        /* how a reference/pointer/Box to pointee is represented: */
enum { REF_WORD = 0, REF_BYTE = 1, REF_SLICE = 2, REF_DYN = 3 };
int cg_words(Type *t);                 /* size in words (sizeof of the C type) */
int cg_needs_drop(Type *t);
int cg_is_box(Type *t);                /* TY_ADT of the owned_box lang item */
Type *cg_box_inner(Type *t);           /* Box<T> -> T */

/* the C member names */
const char *cg_field(Sym *field);      /* "_x" for named fields, "f0" for tuple fields */
/* an enum value's variant payload: "u.v2" (append "." + cg_field(...) for a field) */
const char *cg_variant_member(Sym *variant);
int cg_variant_index(Sym *variant);    /* the tag value */
long cg_discriminant(Sym *variant);    /* the `as` value (explicit discriminant or index) */
/* the index of a tuple field in a TY_TUPLE: always "f<i>" */

/* string literal data: emits `static const long L<n>[] = {...}` (packed UTF-8 /
 * bytes, big-endian, no length word) into CG_DATA once per distinct literal
 * and returns its name.  The slice is { (long)L<n> * 8, len }. */
const char *cg_str_literal(const char *bytes, long len);

/* ================================================================ instances (cg_types.c) */

/* The C function for fn `fn` instantiated with generic args `args` (in the
 * order of fn->gparams: impl/trait params first).  Queues the body for
 * generation on first use and returns its C name ("F34").  If fn is a trait
 * method and `self_ty` is given (the instantiated Self), the impl method (or the
 * trait's default body) is chosen with find_impl; an `extern "rust-intrinsic"`
 * fn never gets a C function (cg_expr.c lowers the call inline: see
 * cg_intrinsic). */
const char *cg_fn_instance(Sym *fn, Type **args, int nargs, Type *self_ty);

/* Resolves a callee from check_callee(): the fn actually called for the
 * current instance (trait method -> impl method or default), with its full
 * instantiated generic args, and its signature.  out_args/out_n get the
 * args, *out_sig the TY_FN_DEF with params/ret substituted. */
Sym *cg_resolve_callee(Callee *c, Type ***out_args, int *out_n, Type **out_sig);
/* the same for a path to a fn (E_PATH with sym = SYM_FN) given its node->ty
 * (TY_FN_DEF with generic args) */
Sym *cg_resolve_fn_type(Type *fnty, Type ***out_args, int *out_n, Type **out_sig);

/* A closure body: the C function for the E_CLOSURE node in the current
 * instance; cg_types.c queues it and calls cg_closure_body() later. */
const char *cg_closure_instance(Node *closure);

/* The vtable of `concrete` as `dyn trait` (with trait args): returns "VT5";
 * emitted into CG_DATA on first use.  cg_vtable_slot gives the index of a
 * trait method in it (2 + position). */
const char *cg_vtable(Type *concrete, Type *dyn_ty);
int cg_vtable_slot(Type *dyn_ty, Sym *method);

/* Drop glue for t: "D12" (static long D12(long addr)), 0 if !cg_needs_drop(t) */
const char *cg_drop_glue(Type *t);

/* statics and consts: the C global of a static ("G7"); a const is an inline
 * expression: cg_expr.c lowers its initializer at each use (consts are pure). */
const char *cg_static(Sym *s);

/* ================================================================ bodies (cg_expr.c) */

/* Generates the body of a fn instance into CG_FUNCS: the C header
 * `static R name(params) {`, the body, `}`.  cg_subst is already set. */
void cg_fn_body(Sym *fn, const char *cname);
/* the same for a closure: `static R name(long env, args..)` */
void cg_closure_body(Node *closure, const char *cname);
/* lowers a static's initializer into the C main's init section (CG_MAIN) as
 * `G7 = <value>;`; the code generator writes it into the current buffer */
void cg_static_init(Sym *s, const char *gname);

/* ================================================================ entry */

/* Generates the whole program: the user crate's `fn main` and everything it
 * reaches.  out = the open .gen.c file; src_name = the file name for #line and
 * panic messages (as given on the command line). */
void cg_crate(FILE *out, const char *src_name, const char *rt_path);

#endif
