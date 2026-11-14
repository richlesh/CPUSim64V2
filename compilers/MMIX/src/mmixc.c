/*
 * mmixc.c - MMIXAL -> CPUSim64 translator (main program).
 *
 * Usage:  mmixc [options] prog.mms
 *
 * Reads an MMIXAL program, assembles it exactly as mmixal does, and writes
 * prog.asm next to it: a CPUSim64 program that runs the MMIX program.
 *   --dump    print the assembled MMIX image (rG, global registers and every
 *             nonzero tetrabyte) instead of translating; used by the tests
 *   --rt=DIR  directory of the runtime library mmix_rt.asm (default: rt/ in
 *             the plug-in directory, found from the translator's own path)
 *   --data=N, --pool=N, --stack=N
 *             bytes of memory for the data, pool and stack segments beyond
 *             the program's own data (K and M suffixes; defaults 1M, 512K,
 *             512K).  The stack segment holds the register stack.
 *
 * The whole translator is one translation unit (the other parts are
 * included below), so that the CPUSim64 C11 compiler can compile it.
 */
#include "mmixc.h"

int opt_dump = 0;
const char *src_name = "";
const char *rt_dir = 0;
const char *out_name = 0;

int err_count = 0;
int line_no = 0;

void err(const char *msg) {
    fprintf(stderr, "\"%s\", line %d: %s!\n", src_name, line_no, msg);
    err_count++;
}

void errs(const char *fmt, const char *s) {
    char buf[300];
    snprintf(buf, sizeof buf, fmt, s);
    err(buf);
}

void warn(const char *msg) {
    fprintf(stderr, "\"%s\", line %d warning: %s\n", src_name, line_no, msg);
}

void fatal(const char *msg) {
    fprintf(stderr, "\"%s\", line %d fatal error: %s!\n", src_name, line_no, msg);
    exit(2);
}

void *xalloc(size_t n) {
    void *p = calloc(1, n ? n : 1);
    if (!p) fatal("Capacity exceeded: out of memory");
    return p;
}

char *xstrdup(const char *s) {
    char *p = xalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

#include "mmixal.c"
#include "gen.c"

/* ---- paths for messages: relative to the working directory when shorter */

static const char *display_path(const char *p) {
    const char *cwd = getenv("PWD");
    size_t n;
    if (!cwd || p[0] != '/') return p;
    n = strlen(cwd);
    while (n > 1 && cwd[n - 1] == '/') n--;
    if (n > 1 && strncmp(p, cwd, n) == 0 && p[n] == '/') return p + n + 1;
    return p;
}

static void banner(int argc, char **argv) {
    int i;
    printf("================================================================================\n");
    printf("CPUSim64 MMIX Translator 1.0\n");
    printf("©2026 Richard Lesh\n");   /* a literal: one codepoint for C11, UTF-8 bytes natively */
    printf("Translates MMIXAL .mms source files into CPUSim64 .asm files.\n");
    printf("================================================================================\n");
    printf("Arguments:");
    for (i = 1; i < argc; i++)
        printf(" %s", argv[i][0] == '-' ? argv[i] : display_path(argv[i]));
    printf("\n");
}

/* the run-time library directory: rt/ next to the obj/ directory of the
   translator (argv[0] is .../MMIX/obj/mmixc.o64), as an absolute path */
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
    p[i] = 0;                                   /* .../MMIX/obj/ */
    if (i >= 5 && strcmp(p + i - 5, "/obj/") == 0) p[i - 4] = 0;
    else if (i == 4 && strcmp(p, "obj/") == 0) p[0] = 0;
    strcat(p, "rt");
    return p;
}

/* a size option: a number with an optional K or M suffix */
static octa size_arg(const char *s) {
    octa v = 0;
    while (*s >= '0' && *s <= '9') v = 10 * v + (octa)(*s++ - '0');
    if (*s == 'K' || *s == 'k') v <<= 10, s++;
    else if (*s == 'M' || *s == 'm') v <<= 20, s++;
    if (*s) {
        fprintf(stderr, "mmixc: bad size %s\n", s);
        exit(2);
    }
    return v;
}

/* prog.mms -> prog.asm (any extension is replaced) */
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

int main(int argc, char **argv) {
    int i;
    FILE *f;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump") == 0) opt_dump = 1;
        else if (strncmp(argv[i], "--rt=", 5) == 0) rt_dir = argv[i] + 5;
        else if (strncmp(argv[i], "--data=", 7) == 0) opt_data = size_arg(argv[i] + 7);
        else if (strncmp(argv[i], "--pool=", 7) == 0) opt_pool = size_arg(argv[i] + 7);
        else if (strncmp(argv[i], "--stack=", 8) == 0) opt_stack = size_arg(argv[i] + 8);
        else if (argv[i][0] == '-') {
            fprintf(stderr, "mmixc: unknown option %s\n", argv[i]);
            return 2;
        } else src_name = argv[i];
    }
    if (!opt_dump) banner(argc, argv);
    if (!src_name[0]) {
        fprintf(stderr, "usage: mmixc [--dump] [--rt=DIR] prog.mms\n");
        return 2;
    }
    f = fopen(src_name, "rb");
    if (!f) {
        fprintf(stderr, "mmixc: can't open %s\n", src_name);
        return 2;
    }
    assemble_file(f);
    fclose(f);
    if (err_count) {
        fprintf(stderr, "(%d error%s found)\n", err_count, err_count == 1 ? " was" : "s were");
        return 1;
    }
    if (opt_dump) {
        dump_image(stdout);
        return 0;
    }
    out_name = output_path(src_name);
    if (!rt_dir) rt_dir = find_rt(argc > 0 ? argv[0] : "");
    if (generate(out_name, rt_dir)) return 1;
    printf("Compiler output: %s\n", display_path(out_name));
    return 0;
}
