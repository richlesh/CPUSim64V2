#ifndef MULTI_TLS_H
#define MULTI_TLS_H
#include <stddef.h>
extern _Thread_local int tls_counter;           /* defined in multi_tls_a.c */
extern _Thread_local int tls_table[4];          /* defined in multi_tls_a.c */
extern _Thread_local const char *tls_label;     /* defined in multi_tls_a.c */
int tls_bump(int n);                            /* adds n to tls_counter, returns it */
int tls_hidden_bump(void);                      /* has its own static _Thread_local */
int tls_other(void);                            /* main module's tls_own through a function */
void tls_run(void (*fn)(void *), void *jobs, int n, size_t size);   /* multi_tls_b.c: n threads, joined */
extern _Thread_local int tls_own;               /* defined in the main module */
#endif
