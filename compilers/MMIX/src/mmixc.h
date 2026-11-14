/*
 * mmixc.h - declarations shared by the parts of the MMIXAL -> CPUSim64
 * translator.
 *
 * Portable C: it is compiled natively for development and by the CPUSim64
 * C11 compiler for the plug-in.  Every value is held in a 64-bit integer, no
 * code relies on 8-bit chars or 32-bit int wraparound, and source files are
 * read in binary mode, one byte per character.
 */
#ifndef MMIXC_H
#define MMIXC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef unsigned long long octa;
typedef long long soct;

#define M32 0xffffffffULL

/* ---- options */
extern int opt_dump;            /* --dump: print the assembled image and stop */
extern const char *src_name;    /* source file as given on the command line */

/* ---- errors (mmixal style: "file", line N: message!) */
extern int err_count;
extern int line_no;
void err(const char *msg);
void errs(const char *fmt, const char *s);
void warn(const char *msg);
void fatal(const char *msg);

/* ---- utilities */
void *xalloc(size_t n);
char *xstrdup(const char *s);

/* ---- assembled image: tetrabytes (xor-loaded, as the MMIX loader does) */
typedef struct {
    octa addr;          /* tetra address (multiple of 4) */
    octa val;           /* 32-bit value */
    int line;           /* source line that assembled it (0 if unknown) */
    int is_inst;        /* 1 if assembled by an MMIX instruction */
} Tetra;

extern Tetra *tetras;
extern int ntetras;
Tetra *tetra_at(octa addr, int create);

/* ---- assembler results */
extern int greg;                    /* rG: first global register */
extern octa greg_val[256];          /* initial values of $greg..$255 */
extern int lreg;                    /* highest register declared LOCAL */
extern octa main_addr;              /* address of :Main */
extern int main_defined;

int assemble_file(FILE *f);         /* returns err_count */
void dump_image(FILE *out);

#endif
