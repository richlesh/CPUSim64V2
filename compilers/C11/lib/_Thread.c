/*
 * _Thread.c - the C part of _Thread.h: atomic arithmetic, spin locks and mutexes,
 * built on atomic_cas.
 */
#include <_Thread.h>

long atomic_add(volatile long *__thr_a, long __thr_n) {
    long __thr_old;
    do {
        __thr_old = *__thr_a;
    } while (!atomic_cas(__thr_a, __thr_old, __thr_old + __thr_n));
    return __thr_old + __thr_n;
}

long atomic_increment(volatile long *__thr_a) { return atomic_add(__thr_a, 1); }

long atomic_decrement(volatile long *__thr_a) { return atomic_add(__thr_a, -1); }

void spinlock_init(spinlock_t *__thr_l) { *__thr_l = 0; }

void spinlock_lock(spinlock_t *__thr_l) {
    while (!atomic_cas(__thr_l, 0, 1)) {}
}

int spinlock_trylock(spinlock_t *__thr_l) { return atomic_cas(__thr_l, 0, 1); }

void spinlock_unlock(spinlock_t *__thr_l) { *__thr_l = 0; }

void mutex_init(mutex_t *__thr_m) {
    __thr_m->owner = 0;
    __thr_m->count = 0;
}

int mutex_trylock(mutex_t *__thr_m) {
    long __thr_me = get_pid();
    if (__thr_m->owner == __thr_me) {
        __thr_m->count++;
        return 1;
    }
    if (!atomic_cas(&__thr_m->owner, 0, __thr_me)) return 0;
    __thr_m->count = 1;
    return 1;
}

void mutex_lock(mutex_t *__thr_m) {
    long __thr_wait = 1;
    while (!mutex_trylock(__thr_m)) {
        sleep_ms(__thr_wait);
        if (__thr_wait < 16) __thr_wait = __thr_wait * 2;      /* the pause grows to 16 ms */
    }
}

void mutex_unlock(mutex_t *__thr_m) {
    if (__thr_m->owner != get_pid()) return;
    if (--__thr_m->count == 0) __thr_m->owner = 0;
}
