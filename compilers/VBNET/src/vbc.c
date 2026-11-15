/*
 * vbc.c - VB.NET -> CPUSim64 compiler (main program).
 *
 * Usage:  vbc [options] prog.vb [more.vb ...]
 *
 * Compiles a VB.NET program and writes prog.asm next to the first source
 * file.  All the source files form one program.  On an error it prints
 * "file:line: error: message" to stderr and exits with 1.
 *
 * Options:
 *   --tokens              print the tokens of the first file and stop
 *   --syntax              print the syntax tree of the first file and stop
 *   --ast                 print the typed syntax tree of the first file
 *                         (after semantic analysis) and stop
 *   --parse-only          check the syntax of all the files and stop
 *   --define=NAME[=VALUE] a conditional compilation constant (default True;
 *                         VALUE may be True, False, a number or a string)
 *   --lib=DIR             the directory of the library sources (System.vb:
 *                         the exception classes; default: lib/ next to rt/)
 *   --gc-stress           the program collects garbage at every allocation
 *                         (to test the garbage collector)
 *   --rt=DIR              the directory of the run-time library vb_rt.asm
 *                         (default: rt/ in the plug-in directory, found from
 *                         the compiler's own path)
 *
 * The whole compiler is one translation unit (the parts are included
 * below), so that the CPUSim64 C11 compiler can compile it in one step.
 */
#include "vbc.h"

#include "util.c"
#include "lexer.c"
#include "ast.c"
#include "parser.c"
#include "types.c"
#include "sema_sym.c"
#include "sema_const.c"
#include "sema_ops.c"
#include "sema_names.c"
#include "sema_call.c"
#include "sema_expr.c"
#include "sema_stmt.c"
#include "sema_decl.c"
#include "sema_type.c"
#include "sema_generic.c"
#include "sema_lambda.c"
#include "sema_query.c"
#include "sema_main.c"
#include "codegen.c"
#include "codegen_stmt.c"

static int opt_tokens = 0;
static int opt_ast = 0;
static int opt_syntax = 0;
static const char *rt_dir = 0;
static const char *lib_dir = 0;
static int opt_parse_only = 0;

static void banner(int argc, char **argv) {
    int i;
    printf("================================================================================\n");
    printf("CPUSim64 VB.NET Compiler %s\n", VBC_VERSION);
    printf("©2026 Richard Lesh\n");   /* a literal: one codepoint for C11, UTF-8 bytes natively */
    printf("Compiles Visual Basic .NET .vb source files into CPUSim64 .asm files.\n");
    printf("================================================================================\n");
    printf("Arguments:");
    for (i = 1; i < argc; i++)
        printf(" %s", argv[i][0] == '-' ? argv[i] : display_path(argv[i]));
    printf("\n");
}

/* prog.vb -> prog.asm (any extension is replaced) */
static char *output_path(const char *in) {
    size_t n = strlen(in), i;
    char *out = xalloc(n + 5);
    strcpy(out, in);
    for (i = n; i > 0; i--) {
        if (out[i - 1] == '/') break;
        if (out[i - 1] == '.') { out[i - 1] = 0; break; }
    }
    strcat(out, ".asm");
    return out;
}

/* the run-time library directory: rt/ next to the obj/ directory of the
   compiler (argv[0] is .../VBNET/obj/vbc.o64), as an absolute path */
static char *find_rt(const char *self) {
    const char *cwd = getenv("PWD");
    size_t n = strlen(self) + (cwd ? strlen(cwd) : 0) + 20, i;
    char *p = xalloc(n);
    if (self[0] != '/' && cwd) {
        strcpy(p, cwd);
        strcat(p, "/");
    }
    strcat(p, self);
    for (i = strlen(p); i > 0 && p[i - 1] != '/'; i--) ;
    p[i] = 0;                                   /* .../VBNET/obj/ */
    if (i >= 5 && strcmp(p + i - 5, "/obj/") == 0) p[i - 4] = 0;
    else if (i == 4 && strcmp(p, "obj/") == 0) p[0] = 0;
    strcat(p, "rt");
    return p;
}

static void define_option(const char *arg) {
    char *s = xstrdup(arg), *eq = strchr(s, '=');
    if (eq) *eq = 0;
    if (!*s) {
        fprintf(stderr, "vbc: --define needs a name\n");
        exit(2);
    }
    lex_define(s, eq ? eq + 1 : "True");
}

int main(int argc, char **argv) {
    int i, nsrc = 0;
    const char **srcs = xalloc(sizeof(char *) * (argc + 1));
    TokVec **toks;
    Node **asts;
    char *out_name;
    lex_init();
    parser_init();
    sema_init();
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) opt_tokens = 1;
        else if (strcmp(argv[i], "--ast") == 0) opt_ast = 1;
        else if (strcmp(argv[i], "--syntax") == 0) opt_syntax = 1;
        else if (strcmp(argv[i], "--parse-only") == 0) opt_parse_only = 1;
        else if (strncmp(argv[i], "--define=", 9) == 0) define_option(argv[i] + 9);
        else if (strncmp(argv[i], "--rt=", 5) == 0) rt_dir = argv[i] + 5;
        else if (strncmp(argv[i], "--lib=", 6) == 0) lib_dir = argv[i] + 6;
        else if (strcmp(argv[i], "--gc-stress") == 0) opt_gc_stress = 1;
        else if (argv[i][0] == '-') {
            fprintf(stderr, "vbc: unknown option %s\n", argv[i]);
            return 2;
        } else srcs[nsrc++] = argv[i];
    }
    banner(argc, argv);
    if (nsrc == 0) {
        fprintf(stderr, "usage: vbc [--tokens|--syntax|--ast|--parse-only] [--define=NAME=VALUE] prog.vb [more.vb ...]\n");
        return 2;
    }
    for (i = 0; i < nsrc; i++) {
        if (load_file(srcs[i]) < 0) {
            fprintf(stderr, "vbc: can't open %s\n", display_path(srcs[i]));
            return 2;
        }
    }
    nuser_files = nfiles;
    /* the library sources (the exception classes) */
    if (!rt_dir) rt_dir = find_rt(argc > 0 ? argv[0] : "");
    if (!lib_dir) {
        char *p = xalloc(strlen(rt_dir) + 8);
        size_t n;
        strcpy(p, rt_dir);
        n = strlen(p);
        if (n >= 2 && strcmp(p + n - 2, "rt") == 0) p[n - 2] = 0;
        else strcat(p, "/../");
        strcat(p, "lib");
        lib_dir = p;
    }
    {
        static const char *lib_files[] = { "System.vb", "Collections.vb", "Linq.vb", "Threading.vb", 0 };
        int k;
        for (k = 0; lib_files[k]; k++) {
            char *p = xalloc(strlen(lib_dir) + 32);
            sprintf(p, "%s/%s", lib_dir, lib_files[k]);
            if (load_file(p) < 0) {
                fprintf(stderr, "vbc: can't open the library source %s\n", p);
                return 2;
            }
        }
    }
    toks = xalloc(sizeof(TokVec *) * nfiles);
    for (i = 0; i < nfiles; i++) toks[i] = lex_file(i);
    if (opt_tokens) {
        dump_tokens(stdout, toks[0]);
        return 0;
    }
    asts = xalloc(sizeof(Node *) * nfiles);
    for (i = 0; i < nfiles; i++) asts[i] = parse_file(toks[i]);
    if (opt_syntax) {
        dump_ast(stdout, asts[0]);
        return 0;
    }
    if (opt_parse_only) return 0;
    sema_program(asts, nfiles, !opt_ast);
    if (opt_ast) {
        dump_ast(stdout, asts[0]);
        return 0;
    }
    out_name = output_path(srcs[0]);
    codegen(out_name, rt_dir);
    printf("Compiler output: %s\n", display_path(out_name));
    return 0;
}
