/*
 * vbc.h - VB.NET -> CPUSim64 compiler: shared declarations.
 *
 * The compiler is written in the C subset of the CPUSim64 C11 compiler
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
 * The whole compiler is one translation unit: vbc.c includes the other .c
 * files.
 */
#ifndef VBC_H
#define VBC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define VBC_VERSION "1.0"

typedef unsigned long long u64;
typedef long long i64;

/* ------------------------------------------------------------ util.c */

void fatal(const char *msg);
void *xalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
const char *display_path(const char *p);

/* growable native string */
typedef struct {
    char *s;
    int len;
    int cap;
} StrBuf;

void sb_init(StrBuf *b);
void sb_putc(StrBuf *b, int c);          /* one char (ASCII or native unit) */
void sb_put_cp(StrBuf *b, int cp);       /* one codepoint */
void sb_puts(StrBuf *b, const char *s);
void sb_printf(StrBuf *b, const char *fmt, ...);
char *sb_take(StrBuf *b);                /* the string; resets the buffer */

void out_cp(FILE *f, int cp);            /* writes one codepoint */

/* source files */
typedef struct {
    const char *path;       /* as given on the command line */
    const char *disp;       /* for messages */
    int *text;              /* codepoints, 0-terminated */
    int len;
} SrcFile;

extern SrcFile *files;
extern int nfiles;
int load_file(const char *path);         /* returns the file index, -1 if unreadable */

/* errors: "file:line: error: message" on stderr, then exit(1) */
void error_at(int file, int line, const char *fmt, ...);
void verror_at(int file, int line, const char *fmt, va_list ap);

/* interned case-folded names (VB names are case-insensitive) */
typedef struct Name {
    const char *key;        /* folded spelling, the interned pointer */
    int kw;                 /* keyword token kind, or 0 */
    struct Name *next;
} Name;

Name *intern(const char *key);           /* key must already be folded */
Name *intern_word(const char *spelling); /* folds an ASCII spelling first */
int fold_cp(int cp);                     /* simple Unicode case folding */

/* ------------------------------------------------------------ lexer.c */

enum {
    T_EOF = 0,
    T_NEWLINE,          /* end of a logical line */
    T_COLON,            /* statement separator ':' */
    T_IDENT,
    T_INT,
    T_FLOAT,
    T_STRING,
    T_CHAR,
    T_INTERP,           /* $"..." : text holds the raw contents */
    /* punctuation */
    P_LPAREN, P_RPAREN, P_LBRACE, P_RBRACE, P_BANG, P_HASH, P_COMMA,
    P_DOT, P_QUESTION, P_QDOT, P_COLONEQ,
    P_AMP, P_STAR, P_PLUS, P_MINUS, P_SLASH, P_BACKSLASH, P_CARET,
    P_LT, P_EQ, P_GT, P_LE, P_GE, P_NE, P_SHL, P_SHR,
    P_PLUSEQ, P_MINUSEQ, P_STAREQ, P_SLASHEQ, P_BACKSLASHEQ, P_CARETEQ,
    P_AMPEQ, P_SHLEQ, P_SHREQ,
    P_LAST,
    /* keywords: K_AddHandler ... K_Xor */
#define KW(x) K_##x,
#include "keywords.inc"
#undef KW
    K_LAST
};

/* literal suffixes (integer and floating point) */
enum {
    LS_NONE = 0, LS_S, LS_US, LS_I, LS_UI, LS_L, LS_UL, LS_F, LS_R, LS_D
};

/* token flags */
#define TF_SPACE   1        /* white space (or a line start) before the token */
#define TF_ESCAPED 2        /* [escaped] identifier */

typedef struct {
    int kind;
    int file;
    int line;
    int col;
    int start, end;         /* codepoint range in the file's text */
    int flags;
    const char *text;       /* identifier spelling, string value, interp raw text */
    Name *name;             /* identifiers and keywords: interned folded name */
    int tchar;              /* identifier type character, or literal suffix LS_* */
    int base;               /* integer literals: 10, 16, 8 or 2 */
    u64 ival;               /* integer and char literal value */
    double dval;            /* floating point literal value */
} Token;

typedef struct {
    Token *v;
    int n;
    int cap;
} TokVec;

extern const char *tok_kind_names[];      /* indexed by kind < P_LAST */
const char *tok_spelling_of(int kind);    /* "(" or "AddHandler" */

void lex_init(void);
void lex_define(const char *name, const char *value);   /* -d NAME=value */
TokVec *lex_file(int file);
/* lexes codepoints [start, end) of a file (an interpolation hole) */
TokVec *lex_range(int file, int start, int end, int line, int col);
void dump_tokens(FILE *f, TokVec *tv);

/* ------------------------------------------------------------ ast.c */

enum {
#define NK(id, nm, la, lb, lc, ld, l1, l2, l3) N_##id,
#include "nodes.inc"
#undef NK
    N_LAST
};

/* declaration modifiers (Node.mods) */
#define M_PUBLIC          (1 << 0)
#define M_PRIVATE         (1 << 1)
#define M_PROTECTED       (1 << 2)
#define M_FRIEND          (1 << 3)
#define M_SHADOWS         (1 << 4)
#define M_SHARED          (1 << 5)
#define M_OVERRIDABLE     (1 << 6)
#define M_NOTOVERRIDABLE  (1 << 7)
#define M_MUSTOVERRIDE    (1 << 8)
#define M_OVERRIDES       (1 << 9)
#define M_OVERLOADS       (1 << 10)
#define M_PARTIAL         (1 << 11)
#define M_MUSTINHERIT     (1 << 12)
#define M_NOTINHERITABLE  (1 << 13)
#define M_READONLY        (1 << 14)
#define M_WRITEONLY       (1 << 15)
#define M_WITHEVENTS      (1 << 16)
#define M_DIM             (1 << 17)
#define M_CONST           (1 << 18)
#define M_STATIC          (1 << 19)
#define M_DEFAULT         (1 << 20)
#define M_WIDENING        (1 << 21)
#define M_NARROWING       (1 << 22)
#define M_ASYNC           (1 << 23)
#define M_ITERATOR        (1 << 24)
#define M_BYVAL           (1 << 25)
#define M_BYREF           (1 << 26)
#define M_OPTIONAL        (1 << 27)
#define M_PARAMARRAY      (1 << 28)
#define M_NBITS           29

/* node flags (Node.flags) */
#define F_NULLABLE    (1 << 0)    /* x? */
#define F_HASARGS     (1 << 1)    /* New T(...) has an argument list */
#define F_QDOT        (1 << 2)    /* ?. or ?( */
#define F_CALLKW      (1 << 3)    /* Call statement */
#define F_LINEIF      (1 << 4)    /* single-line If */
#define F_HASELSE     (1 << 5)
#define F_PRESERVE    (1 << 6)    /* ReDim Preserve */
#define F_ASNEW       (1 << 7)    /* Dim x As New T */
#define F_MULTILINE   (1 << 8)    /* multi-line lambda */
#define F_GLOBAL      (1 << 9)    /* Global.X */
#define F_KEY         (1 << 10)   /* Key field of an anonymous type */
#define F_DESC        (1 << 11)   /* Order By ... Descending */
#define F_ASC         (1 << 12)   /* Order By ... Ascending */
#define F_ELSE        (1 << 13)   /* Case Else */
#define F_AUTO        (1 << 14)   /* auto-implemented property */
#define F_CUSTOM      (1 << 15)   /* Custom Event */
#define F_HASFINALLY  (1 << 16)
#define F_ISNOT       (1 << 17)   /* TypeOf x IsNot T */
#define F_IN          (1 << 18)   /* In type parameter (variance) */
#define F_OUT         (1 << 19)   /* Out type parameter (variance) */
#define F_TOP         (1 << 20)   /* Do While/Until ... Loop */
#define F_BOTTOM      (1 << 21)   /* Do ... Loop While/Until */
#define F_UNTIL       (1 << 22)   /* Until condition */
#define F_WHILE       (1 << 23)   /* While condition (Do, Take/Skip While) */
#define F_PARENS      (1 << 24)   /* a method declared with a parameter list */
#define F_ESCAPED     (1 << 25)   /* [escaped] identifier */
#define F_EXPLICIT    (1 << 26)   /* Convert: an explicit conversion (CInt, CType, ...) */
#define F_COPYBACK    (1 << 27)   /* Arg: ByRef argument copied back after the call */
#define F_DIRECT      (1 << 28)   /* Me of MyBase / MyClass: calls are not virtual */
#define F_TRYCAST     (1 << 29)   /* Convert: TryCast (Nothing if the object is not of the type) */
#define F_ENUM        (1 << 30)   /* ForEach: with an enumerator (d MoveNext, b Current) */
#define F_NBITS       31

typedef struct Node Node;
typedef struct Type Type;
typedef struct Sym Sym;

typedef struct Scope Scope;

typedef struct NodeVec {
    Node **v;
    int n;
    int cap;
    Scope *scope;           /* the scope of a block (set by the semantic analysis) */
} NodeVec;

struct Node {
    int kind;
    int file, line, col;
    int op;                 /* operator or keyword token kind */
    int mods;               /* M_* */
    int flags;              /* F_* */
    Name *name;             /* folded name */
    const char *text;       /* spelling of the name, string value, ... */
    int tchar;              /* identifier type character or literal suffix */
    u64 ival;               /* integer value, array rank */
    double dval;
    int aux;                /* integer literals: the base (10, 16, 8, 2) */
    Node *parent;           /* a declared variable: its declarator (not dumped) */
    int seen;               /* walks of the tree that may meet a node twice (Exit, GoTo targets) */
    Scope *scope;           /* For, For Each: the scope of the loop variable */
    Node *a, *b, *c, *d;
    NodeVec *list, *list2, *list3;
    /* set by semantic analysis */
    Type *type;             /* expressions: the type of the value */
    Sym *sym;               /* the symbol a name, call or declaration refers to */
    int cls;                /* expressions: EC_* */
};

/* expression classification (Node.cls) */
enum {
    EC_NONE = 0,
    EC_VALUE,       /* a value */
    EC_VAR,         /* a variable: can be assigned and passed ByRef */
    EC_METHOD,      /* a method group: sym = the first overload, a = receiver */
    EC_TYPE,        /* a type or module name: type (or sym for a module) */
    EC_NS,          /* a namespace: sym */
    EC_VOID,        /* a call of a Sub */
    EC_PROP,        /* a property (N_PROPREF): read by its getter, set by its setter */
    EC_EVENT        /* an event: sym the event, a the object (AddHandler, RaiseEvent) */
};

Node *new_node(int kind, Token *t);
NodeVec *nv_new(void);
void nv_push(NodeVec *v, Node *n);
void dump_ast(FILE *f, Node *n);
extern const char *node_names[];

/* ------------------------------------------------------------ parser.c */

void parser_init(void);
Node *parse_file(TokVec *tv);

/* ------------------------------------------------------------ types.c */

enum {
    TY_VOID = 0,
    TY_BOOLEAN, TY_SBYTE, TY_BYTE, TY_SHORT, TY_USHORT, TY_INTEGER, TY_UINTEGER,
    TY_LONG, TY_ULONG, TY_DECIMAL, TY_SINGLE, TY_DOUBLE, TY_DATE, TY_CHAR, TY_STRING,
    TY_OBJECT,
    TY_NOTHING,     /* the type of the literal Nothing */
    TY_NPRIM,       /* number of the kinds above */
    TY_ARRAY = TY_NPRIM,
    TY_ENUM,        /* elem: the underlying type */
    TY_CLASS,
    TY_STRUCT,
    TY_INTERFACE
};

struct Type {
    int kind;
    const char *name;       /* VB name of a primitive type */
    const char *sysname;    /* .NET name: Int32, ... */
    Type *elem;             /* arrays */
    int rank;
    Type *arrays;           /* array types of this element type */
    Type *next_array;
    Sym *sym;               /* the System type of a primitive; the S_TYPE of a declared type */
};

int ty_user(Type *t);               /* an enum, class or structure */
Type *ty_under(Type *t);            /* enums: the underlying type; else t */

extern Type *ty[TY_NPRIM];          /* the primitive types by kind */
void types_init(void);
Type *ty_array(Type *elem, int rank);
const char *type_name(Type *t);
int ty_integral(Type *t);
int ty_signed(Type *t);             /* SByte, Short, Integer, Long */
int ty_unsigned(Type *t);           /* Byte, UShort, UInteger, ULong */
int ty_float(Type *t);              /* Single, Double */
int ty_numeric(Type *t);            /* integral, Decimal or floating point */
int ty_bits(Type *t);               /* integral types: 8 .. 64 */
int ty_ref(Type *t);                /* String, Object, arrays, Nothing */

enum { CV_NONE, CV_NARROW, CV_WIDEN, CV_IDENT };
int conv_class(Type *from, Type *to);
int implements(Sym *t, Sym *i);

/* ------------------------------------------------------------ sema.c */

typedef struct {
    Sym **v;
    int n;
    int cap;
} SymVec;

void sv_push(SymVec *v, Sym *s);

enum {
    S_NAMESPACE = 1, S_MODULE, S_TYPE, S_METHOD, S_FIELD, S_CONST, S_LOCAL, S_PARAM,
    S_RETVAL,       /* the return variable of a Function (its name) */
    S_PROP,         /* a property: getter, setter (or field: auto-implemented) */
    S_EVENT         /* an event: field (the delegate), type (the delegate type) */
};

/* Sym.sflags */
#define SF_CTOR     1       /* Sub New */
#define SF_INSTANCE 2       /* a member of an instance (not Shared) */
#define SF_AUTO     4       /* an auto-implemented property */
#define SF_ACCESSOR 8       /* a property's Get or Set */
#define SF_GENERIC  16      /* a generic type or method: a template, never analyzed itself */
#define SF_BOUND    32      /* methods: the body is analyzed */
#define SF_LIVE     64      /* methods and types: reachable from Main (code is generated) */
#define SF_DELEGATE 128     /* types: a delegate type (invoke: its Invoke method) */
#define SF_LAMBDA   256     /* methods: a lambda (Me is its environment) */
#define SF_CAPTURED 512     /* locals and parameters: used by a lambda (stored in an environment) */
#define SF_THUNK    1024    /* Shared methods: a thunk for delegates is needed */

struct Scope {
    Sym *syms;
    Scope *parent;
    int kind;
    Sym *owner;
    int materialized;       /* attached to a block or loop: an environment can be made for it */
    int nenv;               /* captured variables: the size of its environment (codegen) */
};

struct Sym {
    int kind;
    Name *name;
    const char *spelling;
    Type *type;             /* variables; methods: the return type (NULL for a Sub) */
    Node *decl;
    Sym *owner;             /* namespace, module, type or method */
    Sym *next;              /* next symbol in the same scope */
    Sym *overload;          /* methods: the next overload */
    Scope *members;         /* namespaces, modules and types */
    int mods;               /* M_* */
    int index;              /* locals and parameters: number in the method; fields: in the program */
    int builtin;            /* builtin members: B_* */
    const char *rt;         /* builtin members: the run-time function ("name" or
                               "name:K" with a constant last argument), "#N" a
                               floating point interrupt, "@..." expanded by the
                               code generator */
    int state;              /* constants: 0 unevaluated, 1 evaluating, 2 done */
    Node *cval;             /* constants: the value; parameters: the default value */
    Node *init;             /* fields: the bound initializer */
    int file;               /* the file of the declaration (its options) */
    /* methods */
    SymVec params;
    SymVec locals;
    Sym *retval;
    NodeVec *body;          /* the bound statements */
    /* modules and types */
    SymVec fields;          /* types: the instance fields, in layout order */
    SymVec methods;
    int sflags;             /* SF_* */
    int size;               /* types: the words of an instance */
    int desc;               /* types: the number of the type descriptor */
    Sym *getter, *setter;   /* properties */
    Sym *field;             /* auto-implemented properties: the backing field */
    NodeVec *init_body;     /* types: the instance field initializers (assignments) */
    Sym *initm;             /* types: the method that runs them */
    Sym *ctors;             /* types: the instance constructors (an overload chain) */
    Sym *sctor;             /* types: the Shared Sub New */
    Sym *base;              /* classes: the base class (NULL: Object) */
    SymVec ifaces;          /* types: the interfaces they implement (and interfaces: their bases) */
    int vslot;              /* methods: the vtable or interface slot, or -1 */
    int nvirt;              /* types: the vtable size */
    int own_fields;         /* types: the instance fields declared in the type */
    int laid_out;
    Sym *chain;             /* constructors: the base constructor called implicitly */
    Sym *overrides;         /* methods: the base method they override */
    Sym *invoke;            /* delegate types: the Invoke method */
    Sym *outer;             /* lambdas: the method that contains them */
    Sym *me_sym;            /* methods: Me as captured by lambdas */
    Scope *scope;           /* locals, parameters: the scope that declares them; methods: the method scope */
    int envslot;            /* captured variables: their element in the environment */
    Scope **envchain;       /* lambdas: the environments visible where they are created (outermost first) */
    int nenvchain;
    int phase;              /* types: the passes done (sema_main.c) */
    Sym *tmpl;              /* generic instances: the template */
    Type **targs;           /* generic instances: the type arguments */
    int ntargs;
    SymVec insts;           /* templates: the instances */
    Scope *tparams;         /* generic method instances: their type parameters */
    SymVec vtab;            /* classes, structures: the method of each vtable slot */
    SymVec all_ifaces;      /* classes, structures: every interface implemented (with the bases') */
    Sym ***itabs;           /* the methods of each of all_ifaces, by slot */
};

/* builtin members (Sym.builtin) */
enum {
    B_NONE = 0,
    B_CONSOLE_WRITE, B_CONSOLE_WRITELINE, B_CONSOLE_READLINE,
    B_STRING_LENGTH,
    B_ARRAY_LENGTH,
    B_LAST
};

extern SymVec prog_modules;         /* the modules of the program, in order */
extern SymVec prog_types;           /* the enums, classes and structures */
extern SymVec prog_fields;          /* all module fields (and Static locals) */
extern Sym *prog_main;              /* Sub Main or Function Main */
void sema_init(void);
void sema_program(Node **files, int nfiles, int need_main);
const char *sym_kind_name(Sym *s);

/* ------------------------------------------------------------ codegen */

void codegen(const char *out_path, const char *rt_dir);

#endif
