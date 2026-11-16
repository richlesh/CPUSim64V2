/*
 * threads.h - C11 threads (C11 7.26) for the CPUSim64 C compiler.
 *
 * Built on the thread primitives of lib/_Thread.h (thread_create,
 * thread_join, thread_wake, sleep_ms, get_pid, atomic_cas, mutex_t), which
 * are used only in lib/threads.c, so the CPUSim64 names stay out of the
 * program's namespace.  Everything is implemented in C in lib/threads.c.
 *
 * thrd_t is the CPUSim64 thread id, so thrd_current() in a thread equals
 * the value thrd_create() stored for it (also in the main thread).  Threads
 * share the heap and the global variables; give a thread an argument that
 * is global or allocated with malloc, never the address of a local variable
 * of a function that may return first.
 *
 * Time arguments (thrd_sleep, mtx_timedlock, cnd_timedwait) are struct
 * timespec from <time.h>; the absolute times of mtx_timedlock and
 * cnd_timedwait are TIME_UTC times as returned by timespec_get().
 */
#ifndef _CPUSIM64_THREADS_H
#define _CPUSIM64_THREADS_H

#include <stddef.h>
#include <time.h>               /* struct timespec, timespec_get, TIME_UTC */

#define thread_local            _Thread_local
#define ONCE_FLAG_INIT          {0}
#define TSS_DTOR_ITERATIONS     4

/* the fields are private */
typedef struct { long __owner; long __count; long __type; } mtx_t;
typedef struct { long __lock; long __head; long __tail; } cnd_t;
typedef struct { long __state; } once_flag;
typedef long thrd_t;
typedef long tss_t;
typedef void (*tss_dtor_t)(void *);
typedef int (*thrd_start_t)(void *);

enum { mtx_plain = 0, mtx_recursive = 1, mtx_timed = 2 };

enum {
    thrd_success = 0,
    thrd_busy = 1,
    thrd_error = 2,
    thrd_nomem = 3,
    thrd_timedout = 4
};

/* ---- initialization functions */
void call_once(once_flag *flag, void (*func)(void));

/* ---- condition variable functions */
int cnd_broadcast(cnd_t *cond);
void cnd_destroy(cnd_t *cond);
int cnd_init(cnd_t *cond);
int cnd_signal(cnd_t *cond);
int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);
int cnd_wait(cnd_t *cond, mtx_t *mtx);

/* ---- mutex functions */
void mtx_destroy(mtx_t *mtx);
int mtx_init(mtx_t *mtx, int type);
int mtx_lock(mtx_t *mtx);
int mtx_timedlock(mtx_t *mtx, const struct timespec *ts);
int mtx_trylock(mtx_t *mtx);
int mtx_unlock(mtx_t *mtx);

/* ---- thread functions */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
thrd_t thrd_current(void);
int thrd_detach(thrd_t thr);
int thrd_equal(thrd_t thr0, thrd_t thr1);
void thrd_exit(int res);
int thrd_join(thrd_t thr, int *res);
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void thrd_yield(void);

/* ---- thread-specific storage functions */
int tss_create(tss_t *key, tss_dtor_t dtor);
void tss_delete(tss_t key);
void *tss_get(tss_t key);
int tss_set(tss_t key, void *val);

#endif
