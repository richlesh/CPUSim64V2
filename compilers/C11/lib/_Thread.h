/*
 * _Thread.h - threads, processes and atomic operations for the CPUSim64 C
 * compiler (not a standard header: CPUSim64's own, like system/thread.asm).
 *
 * Functions marked [rt] are provided by the assembly runtime (simulator
 * interrupts); the others are implemented in C in lib/_Thread.c.
 *
 * Threads share the heap and the global variables of the program, but each
 * thread has its own stack: pass a thread data that is global or allocated
 * with malloc, never the address of a local variable.  A thread function
 * returns void; it ends when it returns.
 *
 * A process made by fork() gets a copy of the whole memory; only memory
 * from alloc_shared() is shared between processes (it is never freed).
 * Mutexes and spin locks used by several processes must be in shared memory.
 */
#ifndef _CPUSIM64__THREAD_H
#define _CPUSIM64__THREAD_H

#include <stddef.h>

typedef void (*thread_func)(void *arg);

/* ---- threads [rt] */
long thread_create(thread_func fn, void *arg);  /* the new thread's id, or -1 */
void thread_join(long tid);                     /* waits until the thread has ended */
void thread_wake(long tid);                     /* ends a sleep_ms() of the thread early */

/* ---- processes [rt] */
long fork(void);                    /* 0 in the child, the child's id in the parent, -1 on failure */
void wait_pid(long pid);            /* waits until the child process has ended */
void wait_all(void);                /* waits for all child processes */
void *alloc_shared(size_t words);   /* memory shared with the child processes (never freed) */

/* ---- misc [rt] */
void sleep_ms(long ms);             /* sleeps; a thread_wake() ends it early */
long get_pid(void);                 /* the id of the current thread or process */
long get_num_cores(void);           /* the number of processor cores of the host */

/* ---- atomic operations */
int atomic_cas(volatile long *addr, long expected, long desired);   /* [rt] 1 if *addr was expected and is now desired */
long atomic_add(volatile long *addr, long n);                       /* adds n; returns the new value */
long atomic_increment(volatile long *addr);                         /* returns the new value */
long atomic_decrement(volatile long *addr);                         /* returns the new value */

/* ---- spin locks: a long that is 0 when free */
typedef long spinlock_t;
void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
int spinlock_trylock(spinlock_t *lock);         /* 1 if the lock was taken */
void spinlock_unlock(spinlock_t *lock);

/* ---- mutexes: recursive (the owner may lock again); a waiting thread sleeps with growing pauses */
typedef struct mutex_t {
    long owner;                     /* the id of the owner, 0 when free */
    long count;                     /* how many times the owner has locked it */
} mutex_t;
void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
int mutex_trylock(mutex_t *m);                  /* 1 if the mutex was taken */
void mutex_unlock(mutex_t *m);

#endif
