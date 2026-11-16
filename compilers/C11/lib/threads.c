/*
 * threads.c - C implementations for lib/threads.h (C11 7.26).
 *
 * Appended to the program by the compiler; only used functions are
 * compiled.  All private names start with __t11_ because this file shares
 * the program's macro namespace.
 *
 * Threads: thrd_create makes a record (function, argument, result, jmp_buf
 * for thrd_exit, state) kept in a global list, and starts __t11_start with
 * thread_create.  The list is protected by one spin lock.  The creator
 * stores the thread id in the record while it holds the lock, and the new
 * thread takes the lock first, so the record always has its id before the
 * thread runs.  A joined thread's record is freed by thrd_join, a detached
 * thread's by itself when it ends.
 *
 * Mutexes wrap the recursive mutex_t of lib/_Thread.h.  Condition variables
 * keep a queue of waiters (malloc'ed nodes) under a spin lock: signal
 * removes a node, marks it and wakes its thread with thread_wake; a waiter
 * sleeps in pauses of up to 16 ms, so even a wake that arrives before the
 * sleep starts costs at most one pause.  Thread-specific storage is a list
 * of (key, thread id, value) nodes.
 */
#include <threads.h>
#include <_Thread.h>
#include <setjmp.h>
#include <stdlib.h>

/* ---- the lock that protects the lists below */
static spinlock_t __t11_lk;

static void __t11_lock(void) { spinlock_lock(&__t11_lk); }
static void __t11_unlock(void) { spinlock_unlock(&__t11_lk); }

/* sleep like sleep_ms but at least until a stale thread_wake is ignored: callers loop */
static void __t11_pause(long *__t11_w) {
    sleep_ms(*__t11_w);
    if (*__t11_w < 16) *__t11_w = *__t11_w * 2;
}

/* milliseconds from now until the absolute time ts (rounded up), 0 if it has passed */
static long __t11_ms_until(const struct timespec *__t11_ts) {
    struct timespec __t11_now;
    long __t11_ns;
    timespec_get(&__t11_now, TIME_UTC);
    if (__t11_ts->tv_sec - __t11_now.tv_sec > 1000000) return 1000000000;   /* far away: avoid overflow */
    __t11_ns = (__t11_ts->tv_sec - __t11_now.tv_sec) * 1000000000 + (__t11_ts->tv_nsec - __t11_now.tv_nsec);
    if (__t11_ns <= 0) return 0;
    return (__t11_ns + 999999) / 1000000;
}

/* ================================================================ call_once */
void call_once(once_flag *__t11_f, void (*__t11_fn)(void)) {
    long __t11_w = 1;
    if (__t11_f->__state == 2) return;
    if (atomic_cas(&__t11_f->__state, 0, 1)) {
        __t11_fn();
        __t11_f->__state = 2;
        return;
    }
    while (__t11_f->__state != 2) __t11_pause(&__t11_w);
}

/* ================================================================ mutexes */
int mtx_init(mtx_t *__t11_m, int __t11_type) {
    if (__t11_type < 0 || __t11_type > 3) return thrd_error;
    mutex_init((mutex_t *)__t11_m);
    __t11_m->__type = __t11_type;
    return thrd_success;
}

void mtx_destroy(mtx_t *__t11_m) { (void)__t11_m; }

int mtx_trylock(mtx_t *__t11_m) {
    /* a mutex that is not recursive is busy even for its owner */
    if (!(__t11_m->__type & mtx_recursive) && __t11_m->__owner == get_pid()) return thrd_busy;
    return mutex_trylock((mutex_t *)__t11_m) ? thrd_success : thrd_busy;
}

int mtx_lock(mtx_t *__t11_m) {
    mutex_lock((mutex_t *)__t11_m);
    return thrd_success;
}

int mtx_timedlock(mtx_t *__t11_m, const struct timespec *__t11_ts) {
    long __t11_w = 1, __t11_ms;
    for (;;) {
        if (mtx_trylock(__t11_m) == thrd_success) return thrd_success;
        __t11_ms = __t11_ms_until(__t11_ts);
        if (__t11_ms == 0) return thrd_timedout;
        if (__t11_w > __t11_ms) __t11_w = __t11_ms;
        sleep_ms(__t11_w);
        if (__t11_w < 16) __t11_w = __t11_w * 2;
    }
}

int mtx_unlock(mtx_t *__t11_m) {
    if (__t11_m->__owner != get_pid()) return thrd_error;
    mutex_unlock((mutex_t *)__t11_m);
    return thrd_success;
}

/* ================================================================ threads */
struct __t11_rec {
    struct __t11_rec *next;
    long tid;                   /* set by the creator, under the lock */
    thrd_start_t fn;
    void *arg;
    int res;
    long detached;
    long finished;
    long claimed;               /* a thrd_join is under way */
    jmp_buf jb;                 /* thrd_exit jumps here */
};

static struct __t11_rec *__t11_threads;

/* the record of thread tid (the lock must be held), or NULL */
static struct __t11_rec *__t11_find(long __t11_tid) {
    struct __t11_rec *__t11_r = __t11_threads;
    while (__t11_r) {
        if (__t11_r->tid == __t11_tid) return __t11_r;
        __t11_r = __t11_r->next;
    }
    return NULL;
}

static void __t11_unlink(struct __t11_rec *__t11_r) {
    struct __t11_rec **__t11_p = &__t11_threads;
    while (*__t11_p) {
        if (*__t11_p == __t11_r) {
            *__t11_p = __t11_r->next;
            return;
        }
        __t11_p = &(*__t11_p)->next;
    }
}

/* ---- thread-specific storage */
struct __t11_key {
    struct __t11_key *next;
    long id;
    tss_dtor_t dtor;
};

struct __t11_val {
    struct __t11_val *next;
    long key;
    long tid;
    void *val;
    long round;             /* the destructor round that last handled it, -1 for none */
};

static struct __t11_key *__t11_keys;
static struct __t11_val *__t11_vals;
static long __t11_nextkey;

/* runs the destructors of the values of the calling thread, then frees them */
static void __t11_tss_cleanup(void) {
    long __t11_me = get_pid(), __t11_round, __t11_again;
    struct __t11_val *__t11_v, **__t11_p;
    struct __t11_key *__t11_k;
    tss_dtor_t __t11_d;
    void *__t11_x;
    for (__t11_round = 0; __t11_round < TSS_DTOR_ITERATIONS; __t11_round++) {
        __t11_again = 0;
        __t11_lock();
        __t11_v = __t11_vals;
        while (__t11_v) {
            if (__t11_v->tid == __t11_me && __t11_v->val != NULL && __t11_v->round != __t11_round) {
                __t11_d = NULL;
                __t11_k = __t11_keys;
                while (__t11_k) {
                    if (__t11_k->id == __t11_v->key) __t11_d = __t11_k->dtor;
                    __t11_k = __t11_k->next;
                }
                if (__t11_d) {
                    /* handled once per round: a value the destructor sets again
                       waits for the next round (at most TSS_DTOR_ITERATIONS) */
                    __t11_v->round = __t11_round;
                    __t11_x = __t11_v->val;
                    __t11_v->val = NULL;
                    __t11_unlock();
                    __t11_d(__t11_x);
                    __t11_again = 1;
                    __t11_lock();
                    __t11_v = __t11_vals;       /* the list may have changed: start over */
                    continue;
                }
            }
            __t11_v = __t11_v->next;
        }
        __t11_unlock();
        if (!__t11_again) break;
    }
    __t11_lock();
    __t11_p = &__t11_vals;
    while (*__t11_p) {
        __t11_v = *__t11_p;
        if (__t11_v->tid == __t11_me) {
            *__t11_p = __t11_v->next;
            free(__t11_v);
        } else {
            __t11_p = &__t11_v->next;
        }
    }
    __t11_unlock();
}

static void __t11_start(void *__t11_arg) {
    struct __t11_rec *__t11_r = (struct __t11_rec *)__t11_arg;
    __t11_lock();                       /* the creator has stored our id when it releases the lock */
    __t11_unlock();
    if (setjmp(__t11_r->jb) == 0) {
        thrd_start_t __t11_fn = __t11_r->fn;
        void *__t11_a = __t11_r->arg;
        __t11_r->res = __t11_fn(__t11_a);
    }
    __t11_tss_cleanup();
    __t11_lock();
    __t11_r->finished = 1;
    if (__t11_r->detached) {
        __t11_unlink(__t11_r);
        free(__t11_r);
    }
    __t11_unlock();
}

int thrd_create(thrd_t *__t11_thr, thrd_start_t __t11_fn, void *__t11_arg) {
    struct __t11_rec *__t11_r = (struct __t11_rec *)calloc(1, sizeof(struct __t11_rec));
    long __t11_tid;
    if (!__t11_r) return thrd_nomem;
    __t11_r->fn = __t11_fn;
    __t11_r->arg = __t11_arg;
    __t11_lock();
    __t11_r->next = __t11_threads;
    __t11_threads = __t11_r;
    __t11_tid = thread_create(__t11_start, __t11_r);
    if (__t11_tid < 0) {
        __t11_unlink(__t11_r);
        __t11_unlock();
        free(__t11_r);
        return thrd_error;
    }
    __t11_r->tid = __t11_tid;
    __t11_unlock();
    *__t11_thr = __t11_tid;             /* the record may be gone by now: do not touch it */
    return thrd_success;
}

thrd_t thrd_current(void) { return get_pid(); }

int thrd_equal(thrd_t __t11_a, thrd_t __t11_b) { return __t11_a == __t11_b; }

int thrd_detach(thrd_t __t11_t) {
    struct __t11_rec *__t11_r;
    int __t11_ret = thrd_success;
    __t11_lock();
    __t11_r = __t11_find(__t11_t);
    if (!__t11_r || __t11_r->detached || __t11_r->claimed) {
        __t11_ret = thrd_error;
    } else if (__t11_r->finished) {
        __t11_unlink(__t11_r);
        free(__t11_r);
    } else {
        __t11_r->detached = 1;
    }
    __t11_unlock();
    return __t11_ret;
}

int thrd_join(thrd_t __t11_t, int *__t11_res) {
    struct __t11_rec *__t11_r;
    __t11_lock();
    __t11_r = __t11_find(__t11_t);
    if (!__t11_r || __t11_r->detached || __t11_r->claimed || __t11_t == get_pid()) {
        __t11_unlock();
        return thrd_error;
    }
    __t11_r->claimed = 1;
    __t11_unlock();
    thread_join(__t11_t);
    __t11_lock();
    if (__t11_res) *__t11_res = __t11_r->res;
    __t11_unlink(__t11_r);
    __t11_unlock();
    free(__t11_r);
    return thrd_success;
}

/* thrd_exit in the main thread (or a thread not made by thrd_create): wait for the
   other threads, then end the program with status res */
static void __t11_main_exit(int __t11_res) {
    struct __t11_rec *__t11_r;
    long __t11_w = 1, __t11_tid;
    for (;;) {
        __t11_lock();
        __t11_r = __t11_threads;
        while (__t11_r && (__t11_r->detached || __t11_r->claimed)) __t11_r = __t11_r->next;
        if (!__t11_r) break;
        __t11_tid = __t11_r->tid;
        __t11_unlock();
        thrd_join(__t11_tid, NULL);
    }
    while (__t11_threads) {             /* detached threads that are still running */
        __t11_unlock();
        __t11_pause(&__t11_w);
        __t11_lock();
    }
    __t11_unlock();
    __t11_tss_cleanup();
    exit(__t11_res);
}

void thrd_exit(int __t11_res) {
    struct __t11_rec *__t11_r;
    __t11_lock();
    __t11_r = __t11_find(get_pid());
    __t11_unlock();
    if (!__t11_r) __t11_main_exit(__t11_res);
    __t11_r->res = __t11_res;
    longjmp(__t11_r->jb, 1);
}

int thrd_sleep(const struct timespec *__t11_d, struct timespec *__t11_rem) {
    struct timespec __t11_end;
    long __t11_ms;
    if (__t11_d->tv_sec < 0 || __t11_d->tv_nsec < 0 || __t11_d->tv_nsec > 999999999) return -2;
    timespec_get(&__t11_end, TIME_UTC);
    __t11_end.tv_sec += __t11_d->tv_sec;
    __t11_end.tv_nsec += __t11_d->tv_nsec;
    if (__t11_end.tv_nsec >= 1000000000) {
        __t11_end.tv_sec++;
        __t11_end.tv_nsec -= 1000000000;
    }
    for (;;) {                          /* a stray thread_wake must not shorten the sleep */
        __t11_ms = __t11_ms_until(&__t11_end);
        if (__t11_ms == 0) break;
        sleep_ms(__t11_ms);
    }
    if (__t11_rem) {
        __t11_rem->tv_sec = 0;
        __t11_rem->tv_nsec = 0;
    }
    return 0;
}

void thrd_yield(void) { sleep_ms(0); }

/* ================================================================ thread-specific storage */
int tss_create(tss_t *__t11_key, tss_dtor_t __t11_dtor) {
    struct __t11_key *__t11_k = (struct __t11_key *)malloc(sizeof(struct __t11_key));
    if (!__t11_k) return thrd_error;
    __t11_lock();
    __t11_k->id = ++__t11_nextkey;
    __t11_k->dtor = __t11_dtor;
    __t11_k->next = __t11_keys;
    __t11_keys = __t11_k;
    __t11_unlock();
    *__t11_key = __t11_k->id;
    return thrd_success;
}

void tss_delete(tss_t __t11_key) {
    struct __t11_key **__t11_kp;
    struct __t11_key *__t11_k;
    struct __t11_val **__t11_vp;
    struct __t11_val *__t11_v;
    __t11_lock();
    __t11_kp = &__t11_keys;
    while (*__t11_kp) {
        if ((*__t11_kp)->id == __t11_key) {
            __t11_k = *__t11_kp;
            *__t11_kp = __t11_k->next;
            free(__t11_k);
            break;
        }
        __t11_kp = &(*__t11_kp)->next;
    }
    __t11_vp = &__t11_vals;
    while (*__t11_vp) {
        __t11_v = *__t11_vp;
        if (__t11_v->key == __t11_key) {
            *__t11_vp = __t11_v->next;
            free(__t11_v);
        } else {
            __t11_vp = &__t11_v->next;
        }
    }
    __t11_unlock();
}

void *tss_get(tss_t __t11_key) {
    long __t11_me = get_pid();
    struct __t11_val *__t11_v;
    void *__t11_x = NULL;
    __t11_lock();
    __t11_v = __t11_vals;
    while (__t11_v) {
        if (__t11_v->key == __t11_key && __t11_v->tid == __t11_me) {
            __t11_x = __t11_v->val;
            break;
        }
        __t11_v = __t11_v->next;
    }
    __t11_unlock();
    return __t11_x;
}

int tss_set(tss_t __t11_key, void *__t11_val) {
    long __t11_me = get_pid();
    struct __t11_key *__t11_k;
    struct __t11_val *__t11_v;
    __t11_lock();
    __t11_k = __t11_keys;
    while (__t11_k && __t11_k->id != __t11_key) __t11_k = __t11_k->next;
    if (!__t11_k) {
        __t11_unlock();
        return thrd_error;
    }
    __t11_v = __t11_vals;
    while (__t11_v) {
        if (__t11_v->key == __t11_key && __t11_v->tid == __t11_me) break;
        __t11_v = __t11_v->next;
    }
    if (!__t11_v) {
        __t11_v = (struct __t11_val *)malloc(sizeof(struct __t11_val));
        if (!__t11_v) {
            __t11_unlock();
            return thrd_nomem;
        }
        __t11_v->key = __t11_key;
        __t11_v->tid = __t11_me;
        __t11_v->round = -1;
        __t11_v->next = __t11_vals;
        __t11_vals = __t11_v;
    }
    __t11_v->val = __t11_val;
    __t11_unlock();
    return thrd_success;
}

/* ================================================================ condition variables */
struct __t11_waiter {
    struct __t11_waiter *next;
    long tid;
    long signaled;
};

int cnd_init(cnd_t *__t11_c) {
    __t11_c->__lock = 0;
    __t11_c->__head = 0;
    __t11_c->__tail = 0;
    return thrd_success;
}

void cnd_destroy(cnd_t *__t11_c) { (void)__t11_c; }

/* removes the first waiter (cond lock held) and marks it; returns its thread id, or 0 */
static long __t11_wake_one(cnd_t *__t11_c) {
    struct __t11_waiter *__t11_n = (struct __t11_waiter *)__t11_c->__head;
    long __t11_tid;
    if (!__t11_n) return 0;
    __t11_c->__head = (long)__t11_n->next;
    if (!__t11_c->__head) __t11_c->__tail = 0;
    __t11_tid = __t11_n->tid;           /* the waiter may free the node once it is marked */
    __t11_n->signaled = 1;
    return __t11_tid;
}

int cnd_signal(cnd_t *__t11_c) {
    long __t11_tid;
    spinlock_lock((spinlock_t *)&__t11_c->__lock);
    __t11_tid = __t11_wake_one(__t11_c);
    spinlock_unlock((spinlock_t *)&__t11_c->__lock);
    if (__t11_tid) thread_wake(__t11_tid);
    return thrd_success;
}

int cnd_broadcast(cnd_t *__t11_c) {
    long __t11_tid;
    for (;;) {
        spinlock_lock((spinlock_t *)&__t11_c->__lock);
        __t11_tid = __t11_wake_one(__t11_c);
        spinlock_unlock((spinlock_t *)&__t11_c->__lock);
        if (!__t11_tid) break;
        thread_wake(__t11_tid);
    }
    return thrd_success;
}

/* the common part of cnd_wait and cnd_timedwait: ts is NULL for no timeout */
static int __t11_cnd_wait(cnd_t *__t11_c, mtx_t *__t11_m, const struct timespec *__t11_ts) {
    struct __t11_waiter *__t11_n = (struct __t11_waiter *)malloc(sizeof(struct __t11_waiter));
    struct __t11_waiter *__t11_q, *__t11_prev;
    long __t11_w = 1, __t11_ms;
    int __t11_ret = thrd_success;
    if (!__t11_n) return thrd_nomem;
    __t11_n->next = NULL;
    __t11_n->tid = get_pid();
    __t11_n->signaled = 0;
    spinlock_lock((spinlock_t *)&__t11_c->__lock);
    if (__t11_c->__tail) ((struct __t11_waiter *)__t11_c->__tail)->next = __t11_n;
    else __t11_c->__head = (long)__t11_n;
    __t11_c->__tail = (long)__t11_n;
    spinlock_unlock((spinlock_t *)&__t11_c->__lock);
    mtx_unlock(__t11_m);
    while (!__t11_n->signaled) {
        if (__t11_ts) {
            __t11_ms = __t11_ms_until(__t11_ts);
            if (__t11_ms == 0) {
                /* timed out, unless a signal got in first: decide under the lock */
                spinlock_lock((spinlock_t *)&__t11_c->__lock);
                if (!__t11_n->signaled) {
                    __t11_prev = NULL;
                    __t11_q = (struct __t11_waiter *)__t11_c->__head;
                    while (__t11_q && __t11_q != __t11_n) {
                        __t11_prev = __t11_q;
                        __t11_q = __t11_q->next;
                    }
                    if (__t11_q) {
                        if (__t11_prev) __t11_prev->next = __t11_n->next;
                        else __t11_c->__head = (long)__t11_n->next;
                        if ((struct __t11_waiter *)__t11_c->__tail == __t11_n) __t11_c->__tail = (long)__t11_prev;
                    }
                    __t11_ret = thrd_timedout;
                }
                spinlock_unlock((spinlock_t *)&__t11_c->__lock);
                break;
            }
            if (__t11_w > __t11_ms) __t11_w = __t11_ms;
        }
        sleep_ms(__t11_w);
        if (__t11_w < 16) __t11_w = __t11_w * 2;
    }
    free(__t11_n);
    mtx_lock(__t11_m);
    return __t11_ret;
}

int cnd_wait(cnd_t *__t11_c, mtx_t *__t11_m) {
    return __t11_cnd_wait(__t11_c, __t11_m, NULL);
}

int cnd_timedwait(cnd_t *__t11_c, mtx_t *__t11_m, const struct timespec *__t11_ts) {
    return __t11_cnd_wait(__t11_c, __t11_m, __t11_ts);
}
