/*
 * parser.h - the Rust parser: token cursor and the entry points of the
 * parser files.  The parser is hand-written recursive descent following
 * grammar/RustParser.g4 (see grammar/README.md for the known gaps).
 *
 * Files (all included by rustc.c, one translation unit):
 *   parse_core.c   token cursor, errors, splitting of joint punctuation
 *   parse_item.c   crate, items, attributes, visibility, generics, where
 *   parse_type.c   types, paths, generic arguments, bounds
 *   parse_expr.c   expressions, statements, blocks
 *   parse_pat.c    patterns
 *   parse_macro.c  macro calls (token trees), macro_rules!, re-parsing a
 *                  macro's tokens as expressions/types/items
 *
 * Conventions:
 *   - Every parse_xxx() starts at the current token and leaves the cursor
 *     after what it parsed.  On a syntax error it calls perr(), which
 *     reports file:line: error: message and exits (no recovery).
 *   - Nodes are made with mk(kind, tok->line, tok->col) using the first
 *     token of the construct.  See nodes.inc for each kind's fields.
 *   - Doc comments (TK_DOC) are turned into N_ATTR nodes with F_DOC by
 *     parse_outer_attrs()/parse_inner_attrs(); everywhere else the cursor
 *     skips them (parse_core.c removes the ones that are not in an attribute
 *     position before parsing starts, see p_begin()).
 */
#ifndef PARSER_H
#define PARSER_H

#include "rustc.h"
#include "ast.h"
#include "token.h"

/* ---------------------------------------------------------------- cursor (parse_core.c) */

typedef struct {
    Token *toks;        /* the token array, ends with TK_EOF */
    int n;              /* number of tokens including TK_EOF */
    int pos;            /* index of the current token */
    int file;
} PState;

extern PState P;

void p_begin(Token *toks, int n, int file);   /* starts parsing these tokens */
PState p_save(void);                          /* saves the cursor (for re-parsing / backtracking) */
void p_restore(PState s);

Token *cur(void);               /* the current token */
Token *peek(int k);             /* the token k ahead (0 = current); TK_EOF past the end */
int at(int kind);               /* the current token is of this kind */
int at2(int k0, int k1);        /* the current token is k0 and the next is k1 */
Token *next(void);              /* returns the current token and advances (never past EOF) */
int accept(int kind);           /* if at(kind): advance and return 1, else 0 */
Token *expect(int kind, const char *what);   /* advance over kind, else perr "expected <what>" */
Name *expect_ident(const char *what);        /* an identifier (not a keyword), returns its name */
int at_ident(void);             /* TK_IDENT */

/* Joint punctuation: `>>` closes two generic lists, `||` is an empty
 * closure parameter list, `&&` is two borrows, `..=`/`<-`... at_prefix(k)
 * is true when the current token is k or starts with k's spelling (for
 * example at_prefix(TK_GT) on `>>=`); accept_prefix(k) then consumes just
 * k, leaving the rest of the token as the current token. */
int at_prefix(int kind);
int accept_prefix(int kind);
void expect_prefix(int kind, const char *what);

/* errors */
const char *tok_desc(Token *t);              /* "identifier `x`", "number", "`+`", "end of file" */
void perr(Token *t, const char *fmt, ...);   /* file:line of t: error: ...; exits */

/* ---------------------------------------------------------------- items (parse_item.c) */

Node *parse_crate(void);                     /* N_CRATE: inner attrs + items until EOF */
Node *parse_item(void);                      /* an item with its outer attrs and visibility, or 0
                                                if the current token cannot start an item */
int at_item_start(void);                     /* the current tokens start an item (used by statements) */
void parse_outer_attrs(NodeList *out);       /* #[...] and outer doc comments */
void parse_inner_attrs(NodeList *out);       /* #![...] and inner doc comments */
Node *parse_vis(void);                       /* N_VIS or 0 */
Node *parse_generics(void);                  /* `<...>` after an item name: N_GENERICS or 0 */
Node *parse_where(void);                     /* `where ...`: N_WHERE or 0 */
Node *parse_fn_sig_and_body(Node *fn);       /* params, ret type, where, body or `;` (used by traits/impls/extern) */

/* ---------------------------------------------------------------- types (parse_type.c) */

Node *parse_type(void);                      /* any type incl. `dyn A + B`, `impl A + B` */
Node *parse_type_no_bounds(void);            /* a type that cannot be followed by `+ Bound` (after `&`, `as`) */

/* Paths.  mode PATH_EXPR: generic args need turbofish `::<`; PATH_TYPE: `<`
 * directly after a segment starts generic args, `Fn(A) -> B` sugar allowed;
 * PATH_SIMPLE: no generic args (use, visibility, attributes, macros). */
enum { PATH_EXPR = 0, PATH_TYPE = 1, PATH_SIMPLE = 2 };
Node *parse_path(int mode);                  /* N_PATH, also `<T as Trait>::x` (F_QUALIFIED) */
int at_path_start(void);                     /* ident, `::`, self, super, crate, Self, `<` (qualified) */
void parse_generic_args(NodeList *out);      /* after `<` was seen (cursor AT `<`); fills LIFETIME / types / consts / ASSOC_BIND */
Node *parse_bounds(void);                    /* N_BOUNDS: Trait + 'a + ?Sized + for<'a> Fn(&'a T) */
Node *parse_lifetime(void);                  /* N_LIFETIME from a TK_LIFETIME token */

/* ---------------------------------------------------------------- expressions (parse_expr.c) */

Node *parse_expr(void);                      /* full expression incl. assignment and ranges */
Node *parse_expr_no_struct(void);            /* `if`/`while`/`match`/`for` heads: no struct literal at top level */
Node *parse_block(void);                     /* `{ stmts tail }`: N_BLOCK (cursor at `{`) */
Node *parse_stmt(void);                      /* one statement inside a block (used by macro re-parsing too) */
Node *parse_literal(void);                   /* E_LIT_* from the current literal token (also used by patterns) */

/* ---------------------------------------------------------------- patterns (parse_pat.c) */

Node *parse_pattern(void);                   /* top level: or-patterns `A | B`, leading `|` allowed */
Node *parse_pattern_no_alt(void);            /* a single pattern (closure parameters, fn params) */

/* ---------------------------------------------------------------- macros (parse_macro.c) */

/* After the path and `!` have been consumed: parses the delimited token tree
 * and returns N_MACRO_CALL (a = the path, toks = tokens inside the delimiters,
 * flag F_DELIM_PAREN/BRACKET/BRACE).  For an item/statement macro with ( ) or
 * [ ] the caller consumes the trailing `;`. */
Node *parse_macro_call(Node *path, Token *start);
Node *parse_macro_rules(void);               /* `macro_rules! name { rules }`: N_ITEM_MACRO_RULES (cursor at `macro_rules`) */

/* Re-parsing a macro call's tokens (for println!, vec!, assert! etc. and
 * for the expansion of macro_rules!): each runs the normal parser over
 * toks[0..ntoks) and checks that everything was consumed. */
Node *reparse_expr(Token *toks, long ntoks);
void reparse_expr_list(Token *toks, long ntoks, NodeList *out);   /* comma-separated, trailing comma ok */
Node *reparse_type(Token *toks, long ntoks);
void reparse_items(Token *toks, long ntoks, NodeList *out);
void reparse_stmts(Token *toks, long ntoks, NodeList *out);

#endif
