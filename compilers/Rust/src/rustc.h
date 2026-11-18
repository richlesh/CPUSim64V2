/*
 * rustc.h - Rust -> C11 translator: shared declarations.
 *
 * The translator is written in the C subset of the CPUSim64 C11 compiler
 * (compilers/C11): every scalar is one 64-bit word, a char holds one Unicode
 * codepoint, and there is no float, no bit-fields and no VLAs.  It must also
 * build with a host C compiler (build.sh --native) for development, where a
 * char is a byte.  So:
 *   - source files are read in binary mode and decoded from UTF-8 into
 *     codepoint arrays (int *), which the lexer works on;
 *   - names are "native strings" (char *): one codepoint
 *     per char under C11, UTF-8 natively.  sb_put_cp() and out_cp() hide
 *     the difference.  String literal values are not native strings: the
 *     tokens and the AST hold them as UTF-8 bytes, one byte per char element.
 *
 * The whole translator is one translation unit: rustc.c includes the other
 * .c files.
 */
#ifndef RUSTC_H
#define RUSTC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define RUSTC_VERSION "1.0"

/* ------------------------------------------------------------ util.c */

void fatal(const char *msg);
void *xalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
int cp_encode(char *dst, int cp);      /* writes cp as a native character sequence; returns its length */
const char *display_path(const char *p);

typedef struct {
    char *s;
    int len, cap;
} StrBuf;

void sb_init(StrBuf *b);
void sb_putc(StrBuf *b, int c);
void sb_put_cp(StrBuf *b, int cp);
void sb_puts(StrBuf *b, const char *s);
char *sb_take(StrBuf *b);
void out_cp(FILE *f, int cp);

typedef struct {
    char *path;          /* as given */
    const char *disp;    /* for messages */
    int *text;           /* codepoints, NUL terminated */
    int len;
} SrcFile;

extern SrcFile *files;
extern int nfiles;
int load_file(const char *path);
char *output_path(const char *src);

/* prints "file:line: error: message" to stderr and exits with 1 */
void error_at(int file, int line, const char *fmt, ...);

/* The lexer's interface (Token, lex_all, ...) is in token.h, which needs the
 * Name type of ast.h; rustc.c includes rustc.h, ast.h and token.h in this
 * order. */

#endif
