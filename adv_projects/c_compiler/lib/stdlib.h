/*
 * stdlib.h - general utilities for the CPUSim64 C compiler.
 *
 * Functions marked [rt] are provided by the assembly runtime; all others
 * are implemented in C in lib/stdlib.c and compiled only when used.
 * Sizes are in 64-bit words (sizeof(char) == 1 word).
 */
#ifndef _CPUSIM64_STDLIB_H
#define _CPUSIM64_STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#define RAND_MAX        2147483647
#define MB_CUR_MAX      1

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

/* ---- memory management [rt] */
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

/* ---- program termination */
void exit(int status);              /* runs atexit handlers, then _Exit */
void _Exit(int status);             /* [rt] */
void abort(void);                   /* [rt] result 134 */
int atexit(void (*func)(void));     /* up to 32 handlers */
int system(__pstr command);         /* [rt] runs a host command */
char *getenv(const char *name);     /* value of an environment variable, or NULL;
                                           the same variable returns the same string */

/* ---- integer arithmetic */
int abs(int n);
long labs(long n);
long long llabs(long long n);
div_t div(int num, int den);
ldiv_t ldiv(long num, long den);
lldiv_t lldiv(long long num, long long den);

/* ---- numeric conversion */
int atoi(const char *s);
long atol(const char *s);
long long atoll(const char *s);
double atof(const char *s);
long strtol(const char *s, char **end, int base);
long long strtoll(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
unsigned long long strtoull(const char *s, char **end, int base);
double strtod(const char *s, char **end);

/* ---- pseudo-random numbers (deterministic; srand(1) by default;
        srand(time(NULL)) gives a different sequence on every run) */
int rand(void);
void srand(unsigned seed);

/* ---- searching and sorting */
void qsort(void *base, size_t count, size_t size,
           int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t count, size_t size,
              int (*compar)(const void *, const void *));

#endif
