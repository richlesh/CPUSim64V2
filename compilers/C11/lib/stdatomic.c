/*
 * stdatomic.c - C implementations for lib/stdatomic.h.
 *
 * Appended to the program by the compiler; only used functions are
 * compiled.  Names are reserved (__atomic_*, __sa_*) because this file
 * shares the program's macro namespace.  The one non-reserved name is the
 * runtime routine atomic_cas (the simulator's compare-and-swap), declared
 * here with the same prototype as in <_Thread.h>.
 */
#include <stdatomic.h>

int atomic_cas(volatile long *__sa_a, long __sa_e, long __sa_d);   /* [rt] */

long __atomic_load_w(volatile long *__sa_p) { return *__sa_p; }

void __atomic_store_w(volatile long *__sa_p, long __sa_v) { *__sa_p = __sa_v; }

long __atomic_exchange_w(volatile long *__sa_p, long __sa_v) {
    long __sa_old;
    do {
        __sa_old = *__sa_p;
    } while (!atomic_cas(__sa_p, __sa_old, __sa_v));
    return __sa_old;
}

_Bool __atomic_cmpxchg_w(volatile long *__sa_p, long *__sa_e, long __sa_d) {
    long __sa_exp = *__sa_e;
    if (atomic_cas(__sa_p, __sa_exp, __sa_d)) return 1;
    *__sa_e = *__sa_p;
    return 0;
}

long __atomic_fetch_add_w(volatile long *__sa_p, long __sa_v) {
    long __sa_old;
    do {
        __sa_old = *__sa_p;
    } while (!atomic_cas(__sa_p, __sa_old, __sa_old + __sa_v));
    return __sa_old;
}

long __atomic_fetch_and_w(volatile long *__sa_p, long __sa_v) {
    long __sa_old;
    do {
        __sa_old = *__sa_p;
    } while (!atomic_cas(__sa_p, __sa_old, __sa_old & __sa_v));
    return __sa_old;
}

long __atomic_fetch_or_w(volatile long *__sa_p, long __sa_v) {
    long __sa_old;
    do {
        __sa_old = *__sa_p;
    } while (!atomic_cas(__sa_p, __sa_old, __sa_old | __sa_v));
    return __sa_old;
}

long __atomic_fetch_xor_w(volatile long *__sa_p, long __sa_v) {
    long __sa_old;
    do {
        __sa_old = *__sa_p;
    } while (!atomic_cas(__sa_p, __sa_old, __sa_old ^ __sa_v));
    return __sa_old;
}

_Bool atomic_flag_test_and_set(volatile atomic_flag *__sa_f) {
    return __atomic_exchange_w(&__sa_f->__af_set, 1) != 0;
}

_Bool atomic_flag_test_and_set_explicit(volatile atomic_flag *__sa_f, memory_order __sa_o) {
    (void)__sa_o;
    return __atomic_exchange_w(&__sa_f->__af_set, 1) != 0;
}

void atomic_flag_clear(volatile atomic_flag *__sa_f) { __sa_f->__af_set = 0; }

void atomic_flag_clear_explicit(volatile atomic_flag *__sa_f, memory_order __sa_o) {
    (void)__sa_o;
    __sa_f->__af_set = 0;
}

void atomic_thread_fence(memory_order __sa_o) { (void)__sa_o; }

void atomic_signal_fence(memory_order __sa_o) { (void)__sa_o; }
