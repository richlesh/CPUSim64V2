/*
 * ast.h - the Rust AST (shared by the parser, the resolver, the type checker
 * and code generation) and its text dump.
 *
 * One generic Node struct, as in compilers/C++14.  The kinds are listed in
 * nodes.inc, with a comment per kind saying what a, b, c, d, list, name, str,
 * ival, flags and toks mean.  Nodes are allocated from an arena (ast_alloc) and
 * never freed.  Included after rustc.h; ast.c is included by rustc.c.
 *
 * Conventions
 *   - A child pointer that is not used is 0.  List fields use NodeList.
 *   - name / name2 are interned (Name *): equal spellings are the same pointer,
 *     so names compare with ==.  The text is a "native string" (see rustc.h).
 *   - str / slen are a BYTE string: slen bytes, one byte (0..255) per char
 *     element, whatever the host char width.  String literals hold the UTF-8
 *     bytes of the decoded value (so slen is Rust's len()).  str is also
 *     NUL terminated.  Use node_set_str() to fill it.
 *   - toks / ntoks: a token tree (macro arguments, attribute arguments, macro_rules
 *     parts) as a copy of the lexer's tokens, without the outer delimiters.
 *     Only `struct Token;` is declared here (token.h defines it).
 *   - line / col / file are those of the first token of the construct.
 *   - mk() stamps file with ast_cur_file; the parser sets it per source file.
 *   - ty (type checker) and sym (resolver) start at 0 and are owned by those stages.
 *   - Flags are one namespace (F_xxx below).  The comment in nodes.inc for a kind
 *     says which flags it uses.  Operators are OP_xxx.
 *
 * Dump format (ast_dump), stable, used by the parser tests
 *   One node per line, children indented two more spaces.  A node is
 *
 *       [label](KIND [name] [as:name2] ["str"] [#ival] [op:SYM] [{flag,flag}] [toks:N]
 *
 *   followed by its children, one per line, and the closing ')' is appended to the
 *   last line of the node (a leaf is one line, `(P_WILD)`).  Parts are printed
 *   only when set, always in the order above:
 *     name      the interned text, bare.
 *     as:name2  name2 when set.
 *     "str"     the byte string in double quotes; \\ \" \n \r \t, and any byte
 *               outside 0x20..0x7E as \xHH.  Printed when str is non-null.
 *     #ival     decimal, unsigned.  Printed when nonzero, and always for
 *               E_LIT_INT, E_LIT_CHAR, E_LIT_BYTE, E_LIT_BOOL, E_TUPLE_FIELD.
 *               (fval is not printed: floats show their spelling in str.)
 *     op:SYM    the operator spelling when op != 0, e.g. op:+ op:&&
 *     {flags}   the names of the set flags in bit order, lower case, comma
 *               separated, e.g. {mut,unsafe}.
 *     toks:N    N = ntoks, when toks is non-null or ntoks != 0.  If the hook
 *               ast_tok_text is set, toks:N is followed by a space and the text it
 *               prints; the hook gets the tokens and the count.
 *   Children, in this order, each on its own line with a label before '(':
 *       vis=   the visibility node
 *       @      each attribute (attrs list), label "@"
 *       a= b= c= d=   the fixed children that are non-null
 *       (none) each element of list
 *   The list normally comes after a= b= c= d=; for BLOCK and E_CLOSURE it comes
 *   before them (statements before the tail, parameters before return type and body).
 *   A label is the text directly before the '(' with no space, e.g. `a=(E_PATH ...`,
 *   `@(ATTR derive ...`.  List elements have no label.  Line numbers are not printed.
 *
 *   Example:
 *       (CRATE
 *         (ITEM_FN main
 *           b=(T_PATH
 *             a=(PATH
 *               (PATH_SEG i32)))
 *           c=(BLOCK
 *             a=(E_LIT_INT "1" #1))))
 */
#ifndef AST_H
#define AST_H

#include "rustc.h"

struct Token;

/* ---------------------------------------------------------------- memory */

void *ast_alloc(size_t n);             /* zeroed memory from an arena; never freed */

/* ---------------------------------------------------------------- names */

typedef struct Name Name;
struct Name {
    char *s;                           /* native string, NUL terminated */
    long len;                          /* number of chars in s */
    long hash;
    Name *next;                        /* hash chain */
};

Name *name_intern(const char *s, long len);
Name *name_intern_str(const char *s);

/* ---------------------------------------------------------------- kinds */

enum {
    N_NONE = 0,
#define N(x) N_##x,
#include "nodes.inc"
#undef N
    N__COUNT
};

const char *node_kind_name(int kind);  /* "ITEM_FN" for N_ITEM_FN, "" if out of range */

/* ---------------------------------------------------------------- flags */

#define F_MUT          ((long)1 << 0)
#define F_REF          ((long)1 << 1)
#define F_CONST        ((long)1 << 2)
#define F_ASYNC        ((long)1 << 3)
#define F_UNSAFE       ((long)1 << 4)
#define F_EXTERN       ((long)1 << 5)
#define F_VARIADIC     ((long)1 << 6)
#define F_MOVE         ((long)1 << 7)
#define F_GLOBAL       ((long)1 << 8)
#define F_QUALIFIED    ((long)1 << 9)
#define F_TURBOFISH    ((long)1 << 10)
#define F_PARENS       ((long)1 << 11)
#define F_ANGLE        ((long)1 << 12)
#define F_NAMED        ((long)1 << 13)
#define F_TUPLE        ((long)1 << 14)
#define F_NEG          ((long)1 << 15)
#define F_INCLUSIVE    ((long)1 << 16)
#define F_MAYBE        ((long)1 << 17)
#define F_REST         ((long)1 << 18)
#define F_SHORTHAND    ((long)1 << 19)
#define F_TYPED        ((long)1 << 20)
#define F_GLOB         ((long)1 << 21)
#define F_GROUP        ((long)1 << 22)
#define F_INLINE       ((long)1 << 23)
#define F_AUTO         ((long)1 << 24)
#define F_SEMI         ((long)1 << 25)
#define F_DYN          ((long)1 << 26)
#define F_INNER        ((long)1 << 27)
#define F_DOC          ((long)1 << 28)
#define F_EQ           ((long)1 << 29)
#define F_RAW          ((long)1 << 30)
#define F_PAREN        ((long)1 << 31)
#define F_DELIM_PAREN  ((long)1 << 32)
#define F_DELIM_BRACKET ((long)1 << 33)
#define F_DELIM_BRACE  ((long)1 << 34)
#define F_VIS_CRATE    ((long)1 << 35)
#define F_VIS_SUPER    ((long)1 << 36)
#define F_VIS_SELF     ((long)1 << 37)
#define F_VIS_IN       ((long)1 << 38)

/* ---------------------------------------------------------------- operators */

enum {
    OP_NONE = 0,
#define OP(n, s) OP_##n,
#include "nodes.inc"
#undef OP
    OP__COUNT
};

const char *op_spelling(int op);       /* "+" for OP_ADD, "" if out of range */

/* ---------------------------------------------------------------- the node */

typedef struct Node Node;

typedef struct {
    Node **v;
    long n, cap;
} NodeList;

struct Node {
    int kind;                          /* N_xxx */
    int op;                            /* OP_xxx, or 0 */
    int line, col;                     /* 1-based line, 1-based column of the first token */
    int file;                          /* index into files[] */

    Node *a, *b, *c, *d;               /* fixed children, per kind (see nodes.inc) */
    NodeList list;                     /* variable-length children */
    NodeList attrs;                    /* N_ATTR nodes, in source order (outer then inner) */
    Node *vis;                         /* N_VIS or 0 (items, fields, variants) */

    Name *name;                        /* identifier, label, method, suffix, or 0 */
    Name *name2;                       /* alias (use / extern crate) or 0 */
    char *str;                         /* byte string value / ABI / spelling, or 0 */
    long slen;                         /* number of bytes in str */
    long long ival;                    /* integer / char / bool value, tuple index */
    double fval;                       /* float literal value */
    long flags;                        /* F_xxx */

    struct Token *toks;                /* token tree (macros, attributes) or 0 */
    long ntoks;

    void *ty;                          /* the type checker's slot */
    void *sym;                         /* the resolver's slot */
};

extern int ast_cur_file;               /* the file mk() stamps into new nodes */

Node *mk(int kind, int line, int col);
void list_push(NodeList *l, Node *n);  /* appends; a null n is ignored */
void node_set_str(Node *n, const char *bytes, long len);   /* copies len bytes into the arena */
/* To store a token tree the parser copies the tokens itself (struct Token is complete there):
 *   n->toks = ast_alloc(sizeof(struct Token) * cnt); memcpy(...); n->ntoks = cnt; */

/* ---------------------------------------------------------------- dump */

/* Prints root and its descendants, in the format described at the top, ending
 * with a newline.  A null root prints nothing. */
void ast_dump(FILE *f, Node *root);

/* optional hook: prints the spelling of ntoks tokens (set by the code that knows struct Token) */
extern void (*ast_tok_text)(FILE *f, const struct Token *toks, long ntoks);

#endif
