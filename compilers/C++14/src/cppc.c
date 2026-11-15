/*
 * cppc.c - C++14 -> C11 translator for CPUSim64 (main program).
 *
 * Usage:  cppc [options] prog.cpp
 *
 * Translates a C++ program to C that the CPUSim64 C11 compiler (compilers/C11)
 * compiles.  The IDE runs this program (File > New Compiler > cpp) and then
 * the C11 compiler on its output: prog.cpp is translated to prog.gen.c, which
 * contains #line directives that name prog.cpp, so the debugger shows the C++
 * source.  On an error it prints "file:line: error: message" to stderr and
 * exits with 1.
 *
 * The translator is written in the C subset of the CPUSim64 C11 compiler (see
 * cppc.h) and is one translation unit: this file includes the others.
 *
 * Options:
 *   --tokens         print the tokens of the file and stop
 *   --preprocess     print the preprocessed tokens and stop
 *   --ast            print the syntax tree and stop
 *   --parse-only     only check the syntax
 *   -IDIR            add an include directory (searched before lib/)
 *   -DNAME[=VALUE]   define a macro (default value 1)
 *   --unit           the file is a unit of a program whose main() is in another
 *                    file: it gets a function __cppc_init_NAME instead of main()
 *   --lib=DIR        the directory of the C++ library headers (default: lib/
 *                    next to the obj/ directory of the translator)
 *
 * Without an option the translator writes prog.gen.c.  More source files after
 * the first are the other units of the program (translated separately with
 * --unit): main() calls their initialization.
 */
#include "cppc.h"

#include "util.c"
#include "lexer.c"
#include "pp.c"
#include "ast.c"
#include "parser.c"
#include "types.c"
#include "sema.c"
#include "gen.c"
#include "braced.c"
#include "tmpl.c"
#include "lambda.c"
#include "constexpr.c"
#include "exc.c"
#include "translate.c"

static int opt_tokens = 0, opt_preprocess = 0, opt_ast = 0, opt_parse_only = 0;
static const char *lib_dir = 0;

static void usage(void) {
    fprintf(stderr, "usage: cppc [--tokens|--preprocess] [-IDIR] [-DNAME[=VALUE]] prog.cpp\n");
}

static void banner(int argc, char **argv) {
    int i;
    printf("================================================================================\n");
    printf("CPUSim64 C++ Translator %s\n", CPPC_VERSION);
    printf("©2026 Richard Lesh\n");   /* a literal: one codepoint for C11, UTF-8 bytes natively */
    printf("Translates C++14 source files into C for the CPUSim64 C11 compiler.\n");
    printf("================================================================================\n");
    printf("Arguments:");
    for (i = 1; i < argc; i++)
        printf(" %s", argv[i][0] == '-' ? argv[i] : display_path(argv[i]));
    printf("\n");
}

/* the library directory: lib/ next to the obj/ directory of the translator
   (argv[0] is .../cpp/obj/cppc.o64), as an absolute path */
static char *find_lib(const char *self) {
    const char *cwd = getenv("PWD");
    size_t n = strlen(self) + (cwd ? strlen(cwd) : 0) + 20, i;
    char *p = xalloc(n);
    if (self[0] != '/' && cwd) {
        strcpy(p, cwd);
        strcat(p, "/");
    }
    strcat(p, self);
    for (i = strlen(p); i > 0 && p[i - 1] != '/'; i--) ;
    p[i] = 0;                                   /* .../cpp/obj/ */
    if (i >= 5 && strcmp(p + i - 5, "/obj/") == 0) p[i - 4] = 0;
    else if (i == 4 && strcmp(p, "obj/") == 0) p[0] = 0;
    strcat(p, "lib");
    return p;
}

static void define_option(const char *arg) {
    char *s = xstrdup(arg), *eq = strchr(s, '=');
    if (eq) *eq = 0;
    if (!*s) {
        fprintf(stderr, "cppc: -D needs a name\n");
        exit(2);
    }
    pp_define(s, eq ? eq + 1 : "1");
}

int main(int argc, char **argv) {
    int i;
    const char *src = 0;
    char *out;
    FILE *f;
    pp_init();
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) opt_tokens = 1;
        else if (strcmp(argv[i], "--preprocess") == 0) opt_preprocess = 1;
        else if (strcmp(argv[i], "--ast") == 0) opt_ast = 1;
        else if (strcmp(argv[i], "--parse-only") == 0) opt_parse_only = 1;
        else if (strcmp(argv[i], "--unit") == 0) {
            unit_secondary = 1;
            multi_unit = 1;
            pp_define("_CPPC_NO_STREAM_OBJECTS", "1");
        }
        else if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2]) pp_add_include_dir(argv[i] + 2);
        else if (strncmp(argv[i], "-D", 2) == 0 && argv[i][2]) define_option(argv[i] + 2);
        else if (strncmp(argv[i], "--lib=", 6) == 0) lib_dir = argv[i] + 6;
        else if (argv[i][0] == '-') {
            fprintf(stderr, "cppc: unknown option %s\n", argv[i]);
            return 2;
        } else if (src) {
            /* another unit of the program: only its name matters here (its initialization is called from main) */
            unit_names = xrealloc(unit_names, sizeof(char *) * (nunit_names + 1));
            unit_names[nunit_names++] = argv[i];
            multi_unit = 1;
        } else src = argv[i];
    }
    banner(argc, argv);
    if (!src) {
        usage();
        return 2;
    }
    if (!lib_dir) lib_dir = find_lib(argc > 0 ? argv[0] : "");
    pp_set_lib_dir(lib_dir);
    if (load_file(src) < 0) {
        fprintf(stderr, "cppc: can't open %s\n", display_path(src));
        return 2;
    }
    if (opt_tokens) {
        dump_tokens(stdout, lex_file(0));
        return 0;
    }
    if (opt_preprocess) {
        dump_preprocessed(stdout, 0);
        return 0;
    }
    if (opt_ast || opt_parse_only) {
        Node *tree = parse_file(0);
        if (opt_ast) dump_ast(stdout, tree);
        return 0;
    }
    {
        Node *tree = parse_file(0);
        out = output_path(src);
        f = fopen(out, "w");          /* text mode: CPUSim64 writes the codepoints of the chars as UTF-8 */
        if (!f) {
            fprintf(stderr, "cppc: can't write %s\n", display_path(out));
            return 2;
        }
        translate(tree, out, f);
        fclose(f);
    }
    printf("Compiler output: %s\n", display_path(out));
    return 0;
}
