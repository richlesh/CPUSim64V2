/*
 * stdatomic.h - atomic operations (C11 7.17) for the CPUSim64 C compiler.
 *
 * The compiler has no _Atomic and no _Generic, but every scalar is one
 * 64-bit word and the simulator's memory is sequentially consistent, so:
 *
 *  - the atomic types are the plain integer types (declared volatile);
 *    an "atomic" object is any word that is only accessed through the
 *    atomic_* operations below,
 *  - the generic operations are macros that cast the object pointer to
 *    volatile long * and call word-sized functions with reserved names
 *    (__atomic_*_w, built on the simulator's compare-and-swap, see
 *    stdatomic.c); their result has type long, so assign it to a pointer
 *    with a cast: (int *)atomic_load(&p),
 *  - every memory_order argument is accepted and ignored: all operations
 *    are sequentially consistent.
 *
 * Plain x++, x += n or x = y on an atomic object are NOT atomic.  Pointer
 * objects work with atomic_fetch_add/sub, which scale n by the pointed-to
 * type as in C11.  Each macro evaluates the object and the value arguments
 * once, except that atomic_fetch_add/sub name the object a second time in
 * an unevaluated conditional branch.
 */
#ifndef _CPUSIM64_STDATOMIC_H
#define _CPUSIM64_STDATOMIC_H

#include <stddef.h>
#include <stdint.h>

#define ATOMIC_BOOL_LOCK_FREE      2
#define ATOMIC_CHAR_LOCK_FREE      2
#define ATOMIC_CHAR16_T_LOCK_FREE  2
#define ATOMIC_CHAR32_T_LOCK_FREE  2
#define ATOMIC_WCHAR_T_LOCK_FREE   2
#define ATOMIC_SHORT_LOCK_FREE     2
#define ATOMIC_INT_LOCK_FREE       2
#define ATOMIC_LONG_LOCK_FREE      2
#define ATOMIC_LLONG_LOCK_FREE     2
#define ATOMIC_POINTER_LOCK_FREE   2

#define ATOMIC_VAR_INIT(value) (value)
#define kill_dependency(y) (y)

typedef enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;

/* ---- types */
typedef volatile _Bool atomic_bool;
typedef volatile char atomic_char;
typedef volatile signed char atomic_schar;
typedef volatile unsigned char atomic_uchar;
typedef volatile short atomic_short;
typedef volatile unsigned short atomic_ushort;
typedef volatile int atomic_int;
typedef volatile unsigned int atomic_uint;
typedef volatile long atomic_long;
typedef volatile unsigned long atomic_ulong;
typedef volatile long long atomic_llong;
typedef volatile unsigned long long atomic_ullong;
typedef volatile long atomic_char16_t;          /* char16_t / char32_t: <uchar.h> does not exist */
typedef volatile long atomic_char32_t;
typedef volatile wchar_t atomic_wchar_t;

typedef volatile int_least8_t atomic_int_least8_t;
typedef volatile uint_least8_t atomic_uint_least8_t;
typedef volatile int_least16_t atomic_int_least16_t;
typedef volatile uint_least16_t atomic_uint_least16_t;
typedef volatile int_least32_t atomic_int_least32_t;
typedef volatile uint_least32_t atomic_uint_least32_t;
typedef volatile int_least64_t atomic_int_least64_t;
typedef volatile uint_least64_t atomic_uint_least64_t;
typedef volatile int_fast8_t atomic_int_fast8_t;
typedef volatile uint_fast8_t atomic_uint_fast8_t;
typedef volatile int_fast16_t atomic_int_fast16_t;
typedef volatile uint_fast16_t atomic_uint_fast16_t;
typedef volatile int_fast32_t atomic_int_fast32_t;
typedef volatile uint_fast32_t atomic_uint_fast32_t;
typedef volatile int_fast64_t atomic_int_fast64_t;
typedef volatile uint_fast64_t atomic_uint_fast64_t;
typedef volatile intptr_t atomic_intptr_t;
typedef volatile uintptr_t atomic_uintptr_t;
typedef volatile size_t atomic_size_t;
typedef volatile ptrdiff_t atomic_ptrdiff_t;
typedef volatile intmax_t atomic_intmax_t;
typedef volatile uintmax_t atomic_uintmax_t;

/* ---- atomic_flag */
typedef struct atomic_flag {
    volatile long __af_set;
} atomic_flag;
#define ATOMIC_FLAG_INIT {0}

_Bool atomic_flag_test_and_set(volatile atomic_flag *__sa_f);
_Bool atomic_flag_test_and_set_explicit(volatile atomic_flag *__sa_f, memory_order __sa_o);
void atomic_flag_clear(volatile atomic_flag *__sa_f);
void atomic_flag_clear_explicit(volatile atomic_flag *__sa_f, memory_order __sa_o);

/* ---- fences (sequentially consistent memory: nothing to do) */
void atomic_thread_fence(memory_order __sa_o);
void atomic_signal_fence(memory_order __sa_o);

/* ---- word operations behind the generic macros (stdatomic.c) */
long __atomic_load_w(volatile long *__sa_p);
void __atomic_store_w(volatile long *__sa_p, long __sa_v);
long __atomic_exchange_w(volatile long *__sa_p, long __sa_v);
_Bool __atomic_cmpxchg_w(volatile long *__sa_p, long *__sa_e, long __sa_d);   /* on failure *__sa_e = current value */
long __atomic_fetch_add_w(volatile long *__sa_p, long __sa_v);
long __atomic_fetch_and_w(volatile long *__sa_p, long __sa_v);
long __atomic_fetch_or_w(volatile long *__sa_p, long __sa_v);
long __atomic_fetch_xor_w(volatile long *__sa_p, long __sa_v);

/* ---- generic operations */
#define atomic_is_lock_free(obj) (sizeof(*(obj)) > 0)
#define atomic_init(obj, value) __atomic_store_w((volatile long *)(obj), (long)(value))

#define atomic_store(obj, desired) __atomic_store_w((volatile long *)(obj), (long)(desired))
#define atomic_store_explicit(obj, desired, order) atomic_store(obj, desired)
#define atomic_load(obj) __atomic_load_w((volatile long *)(obj))
#define atomic_load_explicit(obj, order) atomic_load(obj)
#define atomic_exchange(obj, desired) __atomic_exchange_w((volatile long *)(obj), (long)(desired))
#define atomic_exchange_explicit(obj, desired, order) atomic_exchange(obj, desired)

#define atomic_compare_exchange_strong(obj, expected, desired) \
    __atomic_cmpxchg_w((volatile long *)(obj), (long *)(expected), (long)(desired))
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail) \
    atomic_compare_exchange_strong(obj, expected, desired)
#define atomic_compare_exchange_weak(obj, expected, desired) \
    atomic_compare_exchange_strong(obj, expected, desired)
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail) \
    atomic_compare_exchange_strong(obj, expected, desired)

/* (1 ? 0 : *(obj)) has the type of the object without reading it: for a pointer
   object the addition scales n by the size of the pointed-to type, as C11 requires */
#define atomic_fetch_add(obj, n) \
    __atomic_fetch_add_w((volatile long *)(obj), (long)((1 ? 0 : *(obj)) + (n)))
#define atomic_fetch_add_explicit(obj, n, order) atomic_fetch_add(obj, n)
#define atomic_fetch_sub(obj, n) \
    __atomic_fetch_add_w((volatile long *)(obj), (long)((1 ? 0 : *(obj)) - (n)))
#define atomic_fetch_sub_explicit(obj, n, order) atomic_fetch_sub(obj, n)
#define atomic_fetch_and(obj, v) __atomic_fetch_and_w((volatile long *)(obj), (long)(v))
#define atomic_fetch_and_explicit(obj, v, order) atomic_fetch_and(obj, v)
#define atomic_fetch_or(obj, v) __atomic_fetch_or_w((volatile long *)(obj), (long)(v))
#define atomic_fetch_or_explicit(obj, v, order) atomic_fetch_or(obj, v)
#define atomic_fetch_xor(obj, v) __atomic_fetch_xor_w((volatile long *)(obj), (long)(v))
#define atomic_fetch_xor_explicit(obj, v, order) atomic_fetch_xor(obj, v)

#endif
