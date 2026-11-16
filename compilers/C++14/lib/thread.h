// <thread.h>: the CPUSim64 threads, processes, shared memory, atomics and
// locks of the C library (compilers/C11/lib/_Thread.h), for C++ programs.
// The generated C includes the C header <_Thread.h>; these are its declarations.
#ifndef _CPPC_THREAD_H
#define _CPPC_THREAD_H
#pragma cppc c_include <_Thread.h>
#include <stddef.h>
extern "C" {
typedef void (*thread_func)(void *arg);
long thread_create(thread_func fn, void *arg);  // the new thread's id, or -1
void thread_join(long tid);                     // waits until the thread has ended
void thread_wake(long tid);                     // ends a sleep_ms() of the thread early
long fork(void);                    // 0 in the child, the child's id in the parent, -1 on failure
void wait_pid(long pid);            // waits until the child process has ended
void wait_all(void);                // waits for all child processes
void *alloc_shared(size_t words);   // memory shared with the child processes (never freed)
void sleep_ms(long ms);             // sleeps; a thread_wake() ends it early
long get_pid(void);                 // the id of the current thread or process
long get_num_cores(void);           // the number of processor cores of the host
int atomic_cas(volatile long *addr, long expected, long desired);   // 1 if *addr was expected and is now desired
long atomic_add(volatile long *addr, long n);                       // adds n; returns the new value
long atomic_increment(volatile long *addr);                         // returns the new value
long atomic_decrement(volatile long *addr);                         // returns the new value
typedef long spinlock_t;
void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
int spinlock_trylock(spinlock_t *lock);         // 1 if the lock was taken
void spinlock_unlock(spinlock_t *lock);
struct mutex_t {                    // (the C header's struct mutex_t)
    long owner;                     // the id of the owner, 0 when free
    long count;                     // how many times the owner has locked it
};
void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
int mutex_trylock(mutex_t *m);                  // 1 if the mutex was taken
void mutex_unlock(mutex_t *m);
}
#endif
