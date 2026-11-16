// stdatomic.h - the word operations behind the C11 library's stdatomic.h (the
// generic atomic_* operations of C are macros over these), for <atomic>.
#ifndef _CPPC_STDATOMIC_H
#define _CPPC_STDATOMIC_H
#pragma cppc c_include <stdatomic.h>
extern "C" {
long __atomic_load_w(volatile long *p);
void __atomic_store_w(volatile long *p, long v);
long __atomic_exchange_w(volatile long *p, long v);
bool __atomic_cmpxchg_w(volatile long *p, long *expected, long desired);   // on failure *expected = the current value
long __atomic_fetch_add_w(volatile long *p, long v);        // these return the old value
long __atomic_fetch_and_w(volatile long *p, long v);
long __atomic_fetch_or_w(volatile long *p, long v);
long __atomic_fetch_xor_w(volatile long *p, long v);
void atomic_thread_fence(int order);
void atomic_signal_fence(int order);
}
#endif
