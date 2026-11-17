/*
 * rustc.c - Rust -> C11 translator for CPUSim64 (main program).
 *
 * Usage:  rustc prog.rs
 *
 * Translates a Rust program to C that the CPUSim64 C11 compiler (compilers/C11)
 * compiles.  The IDE runs this program (File > New Compiler > Rust) and then
 * the C11 compiler on its output: prog.rs is translated to prog.gen.c, which
 * contains #line directives that name prog.rs, so the debugger shows the Rust
 * source.  On an error it prints "file:line: error: message" to stderr and
 * exits with 1.
 *
 * The translator is written in the C subset of the CPUSim64 C11 compiler (see
 * rustc.h) and is one translation unit: this file includes the others.
 *
 * Pipeline: lex, parse (parse_*.c), lib/core.rs as the crate `core`, macro
 * expansion (expand.c), name resolution (resolve.c), item collection
 * (collect.c), type checking (check.c), move checking (moves.c), and C code
 * generation (cg_types.c, cg_expr.c) with lib/rust_rt.c as the runtime.
 * Debug dumps: --tokens, --ast, --expand, --resolve, --types.
 */
#include "rustc.h"
#include "ast.h"
#include "token.h"
#include "parser.h"
#include "sema.h"

#include "util.c"
#include "ast.c"
#include "lexer.c"
#include "parse_core.c"
#include "parse_item.c"
#include "parse_type.c"
#include "parse_expr.c"
#include "parse_pat.c"
#include "parse_macro.c"
#include "expand.c"
#include "resolve.c"
#include "types.c"
#include "collect.c"
#include "check.c"
#include "moves.c"
#include "cg.h"
#include "cg_types.c"
#include "cg_expr.c"

static const char *lib_dir = 0;

/* the library directory: lib/ next to the obj/ directory of the translator
   (argv[0] is .../Rust/obj/rustc.o64), as an absolute path */
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
    p[i] = 0;
    if (i >= 5 && strcmp(p + i - 5, "/obj/") == 0) p[i - 4] = 0;
    else if (i == 4 && strcmp(p, "obj/") == 0) p[0] = 0;
    strcat(p, "lib");
    return p;
}

static void usage(void) {
    fprintf(stderr, "usage: rustc [--tokens | --tokens-flags | --ast | --expand | --resolve | --types] [--lib=DIR] [--no-core] prog.rs\n");
}

static void banner(int argc, char **argv) {
    int i;
    printf("================================================================================\n");
    printf("CPUSim64 Rust Translator %s\n", RUSTC_VERSION);
    printf("©2026 Richard Lesh\n");   /* a literal: one codepoint for C11, UTF-8 bytes natively */
    printf("Translates Rust source files into C for the CPUSim64 C11 compiler.\n");
    printf("================================================================================\n");
    printf("Arguments:");
    for (i = 1; i < argc; i++)
        printf(" %s", argv[i][0] == '-' ? argv[i] : display_path(argv[i]));
    printf("\n");
}

static Token *sl_toks;

int main(int argc, char **argv) {
    int i, ntok, tokens = 0, ast = 0, stage = 0, no_core = 0;
    const char *src = 0;
    char *out;
    FILE *f;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) tokens |= 1;
        else if (strcmp(argv[i], "--tokens-flags") == 0) tokens |= 3;
        else if (strcmp(argv[i], "--ast") == 0) ast = 1;
        else if (strcmp(argv[i], "--expand") == 0) stage = 1;
        else if (strcmp(argv[i], "--resolve") == 0) stage = 2;
        else if (strcmp(argv[i], "--types") == 0) stage = 3;
        else if (strcmp(argv[i], "--no-core") == 0) no_core = 1;
        else if (strncmp(argv[i], "--lib=", 6) == 0) lib_dir = argv[i] + 6;
        else if (argv[i][0] == '-') {
            fprintf(stderr, "rustc: unknown option %s\n", argv[i]);
            return 2;
        } else if (!src) src = argv[i];
        else {
            fprintf(stderr, "rustc: only one source file is supported yet\n");
            return 2;
        }
    }
    banner(argc, argv);
    if (!src) {
        usage();
        return 2;
    }
    if (load_file(src) < 0) {
        fprintf(stderr, "rustc: can't open %s\n", display_path(src));
        return 2;
    }
    ast_tok_text = tok_text_list;
    sl_toks = lex_all(0, &ntok);
    if (tokens) {
        for (i = 0; i < ntok; i++) tok_print_line(stdout, &sl_toks[i], tokens & 2 ? TOKP_FLAGS : 0);
        return 0;
    }
    {
        /* the full parser checks the syntax; it splits joint tokens in place,
           so it works on a copy */
        Token *pt = xalloc(sizeof(Token) * ntok);
        Node *crate, *core = 0;
        memcpy(pt, sl_toks, sizeof(Token) * ntok);
        p_begin(pt, ntok, 0);
        crate = parse_crate();
        if (ast) {
            ast_dump(stdout, crate);
            return 0;
        }
        if (!no_core) {
            /* lib/core.rs: the prelude, file 1 */
            char *cp;
            int cf, cn;
            Token *ct;
            if (!lib_dir) lib_dir = find_lib(argc > 0 ? argv[0] : "");
            cp = xalloc(strlen(lib_dir) + 16);
            strcpy(cp, lib_dir);
            strcat(cp, "/core.rs");
            cf = load_file(cp);
            if (cf < 0) {
                fprintf(stderr, "rustc: can't open %s\n", display_path(cp));
                return 2;
            }
            ct = lex_all(cf, &cn);
            p_begin(ct, cn, cf);
            core = parse_crate();
            expand_crate(core);
        }
        expand_crate(crate);
        if (stage == 1) {
            ast_dump(stdout, crate);
            return 0;
        }
        resolve_crate(crate, core);
        if (stage == 2) {
            ast_node_hook = sema_dump_hook;
            ast_dump(stdout, crate);
            return 0;
        }
        collect_crate();
        check_crate();
        moves_crate();
        if (stage == 3) {
            ast_node_hook = sema_dump_hook;
            ast_dump(stdout, crate);
            return 0;
        }
        {
            char *rtp;
            if (!lib_dir) lib_dir = find_lib(argc > 0 ? argv[0] : "");
            rtp = xalloc(strlen(lib_dir) + 16);
            strcpy(rtp, lib_dir);
            strcat(rtp, "/rust_rt.c");
            out = output_path(src);
            f = fopen(out, "w");      /* text mode: CPUSim64 writes the codepoints of the chars as UTF-8 */
            if (!f) {
                fprintf(stderr, "rustc: can't write %s\n", display_path(out));
                return 2;
            }
            cg_crate(f, src, rtp);
            fclose(f);
        }
    }
    printf("Compiler output: %s\n", display_path(out));
    return 0;
}
