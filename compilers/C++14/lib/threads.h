// threads.h - the C11 library's threads.h (C11 7.26) as the C++ translator sees it.
// The mutex and condition variable objects are passed as void *: <mutex>,
// <condition_variable> and <thread> keep their storage (three words each).
#ifndef _CPPC_THREADS_H
#define _CPPC_THREADS_H
#pragma cppc c_include <threads.h>
#include <stddef.h>
#include <time.h>
enum { mtx_plain = 0, mtx_recursive = 1, mtx_timed = 2 };
enum { thrd_success = 0, thrd_busy = 1, thrd_error = 2, thrd_nomem = 3, thrd_timedout = 4 };
extern "C" {
int thrd_create(long *thr, int (*func)(void *), void *arg);
long thrd_current(void);
int thrd_detach(long thr);
int thrd_equal(long thr0, long thr1);
int thrd_join(long thr, int *res);
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void thrd_yield(void);
int mtx_init(void *mtx, int type);          // mtx: 3 words; type: 0 plain, 1 recursive, 2 timed
void mtx_destroy(void *mtx);
int mtx_lock(void *mtx);
int mtx_timedlock(void *mtx, const struct timespec *ts);
int mtx_trylock(void *mtx);
int mtx_unlock(void *mtx);
int cnd_init(void *cond);                   // cond: 3 words
void cnd_destroy(void *cond);
int cnd_signal(void *cond);
int cnd_broadcast(void *cond);
int cnd_wait(void *cond, void *mtx);
int cnd_timedwait(void *cond, void *mtx, const struct timespec *ts);
}
#endif
