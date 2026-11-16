/*
 * cppc.h - C++14 -> C11 translator: shared declarations.
 *
 * The translator is written in the C subset of the CPUSim64 C11 compiler
 * (compilers/C11): every scalar is one 64-bit word, a char holds one Unicode
 * codepoint, and there is no float, no bit-fields and no VLAs.  It must also
 * build with a host C compiler (build.sh --native) for development, where a
 * char is a byte.  So:
 *   - source files are read in binary mode and decoded from UTF-8 into
 *     codepoint arrays (int *), which the lexer works on;
 *   - names and string values are "native strings" (char *): one codepoint
 *     per char under C11, UTF-8 natively.  sb_put_cp() and out_cp() hide
 *     the difference;
 *   - no code relies on 32-bit int wraparound; 64-bit values use
 *     long long / unsigned long long.
 *
 * The whole translator is one translation unit: cppc.c includes the other
 * .c files.
 */
#ifndef CPPC_H
#define CPPC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>

#define CPPC_VERSION "1.0"

typedef unsigned long long u64;
typedef long long i64;

/* ------------------------------------------------------------ util.c */

void fatal(const char *msg);
void *xalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
void *anew(size_t n);                  /* zeroed memory from an arena; never freed */
int cp_encode(char *dst, int cp);      /* writes cp as a native character sequence; returns its length */
const char *display_path(const char *p);
const char *base_name(const char *p);

typedef struct {
    char *s;
    int len, cap;
} StrBuf;

void sb_init(StrBuf *b);
void sb_putc(StrBuf *b, int c);
void sb_put_cp(StrBuf *b, int cp);
void sb_puts(StrBuf *b, const char *s);
void sb_vprintf(StrBuf *b, const char *fmt, va_list ap);
void sb_printf(StrBuf *b, const char *fmt, ...);
char *sb_take(StrBuf *b);
void out_cp(FILE *f, int cp);

typedef struct {
    char *path;          /* as given */
    const char *disp;    /* for messages */
    int *text;           /* codepoints, NUL terminated */
    int len;
    int is_lib;          /* a file of the C++ library (lib/) */
} SrcFile;

extern SrcFile *files;
extern int nfiles;
int load_file(const char *path);
int add_file_name(const char *name);   /* a file name without text (#line) */
char *output_path(const char *src);

extern void (*err_context_hook)(void);
extern jmp_buf *err_trap;
extern char err_trap_msg[600];
void verror_at(int file, int line, const char *fmt, va_list ap);
void error_at(int file, int line, const char *fmt, ...);
void warning_at(int file, int line, const char *fmt, ...);

/* ------------------------------------------------------------ names */

typedef struct Name Name;
typedef struct Macro Macro;

/* An interned identifier: equal spellings are the same Name. */
struct Name {
    char *s;
    int len;
    int hash;
    Name *next;      /* hash chain */
    int kw;          /* KW_xxx or 0 */
    int alt;         /* P_xxx for the alternative tokens (and, or, ...) or 0 */
    int flags;       /* NF_xxx */
    Macro *macro;    /* the preprocessor macro with this name, or 0 */
};

#define NF_MEMBER_TEMPLATE 1    /* declared as a member template in some class */

Name *intern(const char *s, int len);
Name *intern_str(const char *s);

/* ------------------------------------------------------------ tokens */

enum {
    K_EOF = 0,
    K_IDENT,         /* identifier or keyword (id = KW_xxx) */
    K_NUM,           /* pp-number */
    K_CHAR,          /* character literal, spelling includes prefix and quotes */
    K_STR,           /* string literal, spelling includes prefix and quotes */
    K_PUNCT,         /* operator or punctuator (id = P_xxx) */
    K_OTHER,         /* any other character, or an unterminated quote */
    K_PLACE          /* placemarker (inside macro expansion only) */
};

enum {
    KW_NONE = 0,
#define KW(x) KW_##x,
#include "keywords.inc"
#undef KW
    KW_LAST
};

enum {
    P_NONE = 0,
#define P(id, s) P_##id,
#include "puncts.inc"
#undef P
    P_LAST
};

#define TF_BOL    1      /* first token of a logical line */
#define TF_SPACE  2      /* preceded by white space */
#define TF_NOEXP  4      /* identifier that must not be macro-expanded */

typedef struct Hide Hide;
struct Hide {            /* a set of macro names (linked list) */
    Name *name;
    Hide *next;
};

typedef struct Token Token;
struct Token {
    int kind;
    int id;              /* KW_xxx for a keyword, P_xxx for a punctuator */
    Name *name;          /* K_IDENT */
    const char *text;    /* spelling */
    int file, line;
    int flags;
    Hide *hide;
};

typedef struct {
    Token **v;
    int n, cap;
} TokVec;

void tv_push(TokVec *t, Token *x);
Token *tok_new(int kind, const char *text, int file, int line);
Token *tok_copy(const Token *t);

/* ------------------------------------------------------------ AST */

enum {
    N_NONE = 0,
#define N(x) N_##x,
#include "nodes.inc"
#undef N
    N_LAST
};

typedef struct Node Node;
typedef struct Scope Scope;
typedef struct Sym Sym;

struct Node {
    int kind;
    int file, line;
    int op;              /* an operator (P_xxx) or keyword (KW_xxx) */
    int flags;
    i64 val;
    Name *name;
    const char *text;
    Node *a, *b, *c, *d;
    Node *next;          /* the next node of a list */
    Scope *scope;        /* the scope that the node opens */
    Sym *sym;            /* the symbol that an N_ID names, if the parser found it */
};

/* flags of N_ID */
#define ID_GLOBAL    1   /* leading :: */
#define ID_TYPENAME  2   /* typename keyword */
#define ID_TEMPLATE  4   /* the last part was written with the template keyword */
/* flags of N_PART: kind of the name; op is the operator for PART_OPERATOR */
#define PART_NAME     0
#define PART_OPERATOR 1  /* operator+ : op = P_xxx, or op = KW_new / KW_delete (+ flags PART_ARRAY), text = "()" / "[]" for those */
#define PART_CONV     2  /* operator T: a = N_TYPEID */
#define PART_DTOR     3  /* ~Name */
#define PART_LITERAL  4  /* operator"" _x */
#define PART_ARGS     8  /* has template arguments (b may be empty) */
#define PART_ARRAY    16
/* decl-specifier flags (N_TYPE.flags) */
#define DS_TYPEDEF    0x1
#define DS_STATIC     0x2
#define DS_EXTERN     0x4
#define DS_INLINE     0x8
#define DS_VIRTUAL    0x10
#define DS_EXPLICIT   0x20
#define DS_FRIEND     0x40
#define DS_CONSTEXPR  0x80
#define DS_MUTABLE    0x100
#define DS_REGISTER   0x200
#define DS_THREAD     0x400
#define DS_CONST      0x800
#define DS_VOLATILE   0x1000
/* builtin type bits (N_TYPE.val) */
#define BT_VOID     0x1
#define BT_BOOL     0x2
#define BT_CHAR     0x4
#define BT_WCHAR    0x8
#define BT_CHAR16   0x10
#define BT_CHAR32   0x20
#define BT_SHORT    0x40
#define BT_INT      0x80
#define BT_LONG     0x100
#define BT_LONG2    0x200   /* long long */
#define BT_SIGNED   0x400
#define BT_UNSIGNED 0x800
#define BT_FLOAT    0x1000
#define BT_DOUBLE   0x2000
#define BT_AUTO     0x4000
/* N_D_FUNC flags */
#define FQ_CONST     0x1
#define FQ_VOLATILE  0x2
#define FQ_REF       0x4
#define FQ_RREF      0x8
#define FQ_VARIADIC  0x10   /* ... */
#define FQ_NOEXCEPT  0x20
#define FQ_PURE      0x40   /* = 0 */
#define FQ_OVERRIDE  0x80
#define FQ_FINAL     0x100
#define FQ_DEFAULT   0x200  /* = default */
#define FQ_DELETE    0x400  /* = delete */
#define FQ_THROW     0x800  /* throw() */
#define CL_FINAL     1
#define CL_DEFINITION 2     /* has a member list */
#define EN_CLASS     1
#define EN_OPAQUE    2
#define NS_INLINE    1
#define TPF_PACK     1
#define TP_SPEC      1
#define LF_MUTABLE   1
#define LF_NOEXCEPT  2
#define LF_PARAMS    4
#define IF_CONSTEXPR 1

/* Node.op: a punctuator is P_xxx, a keyword is OPKW + KW_xxx */
#define OPKW 1000

const char *node_kind_name(int kind);
Node *node_new(int kind, Token *at);
void dump_ast(FILE *f, Node *root);

/* ------------------------------------------------------------ parser.c */

Node *parse_file(int file);

/* ------------------------------------------------------------ lexer.c */

void lex_init(void);
void lex_text(int *text, int n, int file, int line, TokVec *out);
TokVec *lex_file(int file);
int *native_to_cps(const char *s, int *n);
const char *punct_spelling(int id);
const char *kw_spelling(int id);
const char *tok_kind_name(int kind);
void dump_tokens(FILE *f, TokVec *tv);

/* ------------------------------------------------------------ pp.c */

void pp_init(void);
void pp_add_include_dir(const char *dir);
void pp_set_lib_dir(const char *dir);
extern int pp_c_include_count;
extern const char **pp_c_includes;
void pp_define(const char *name, const char *value);
void pp_begin(int file);
Token *pp_next(void);
void dump_preprocessed(FILE *f, int file);

#endif
