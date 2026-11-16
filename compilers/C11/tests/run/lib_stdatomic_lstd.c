/* <stdatomic.h> and <_Thread.h> linked as library modules (-lstd) */
#include <stdio.h>
#include <stdatomic.h>
#include <_Thread.h>

#define THREADS 4
#define STEPS 250

static atomic_long counter;
static atomic_long bits;
static atomic_long maxseen;
static atomic_long ticket;
static atomic_flag lock = ATOMIC_FLAG_INIT;
static long plain_total;            /* guarded by the flag lock */
static long owners[THREADS + 1];
static atomic_long done;
static long ids[THREADS];           /* thread data must not be a local */

static void worker(void *arg)
{
    long id = *(long *)arg;
    long i, cur;
    for (i = 0; i < STEPS; i++) {
        atomic_fetch_add(&counter, 1);
        atomic_fetch_sub(&counter, -1);                 /* +1 again */
        atomic_fetch_or(&bits, 1L << (id * 8 + i % 8));
        /* compare-exchange loop: record the maximum */
        cur = atomic_load(&maxseen);
        while (cur < id * 1000 + i &&
               !atomic_compare_exchange_weak(&maxseen, &cur, id * 1000 + i)) {}
        /* spin lock made from an atomic_flag */
        while (atomic_flag_test_and_set_explicit(&lock, memory_order_acquire)) {}
        plain_total += id;
        owners[id]++;
        atomic_flag_clear_explicit(&lock, memory_order_release);
        atomic_fetch_add(&ticket, 1);
    }
    atomic_fetch_add(&done, 1);
}

int main(void)
{
    long tids[THREADS], i;
    for (i = 0; i < THREADS; i++) {
        ids[i] = i + 1;
        tids[i] = thread_create(worker, &ids[i]);
        if (tids[i] < 0) printf("thread_create failed\n");
    }
    for (i = 0; i < THREADS; i++) thread_join(tids[i]);
    printf("counter=%ld\n", atomic_load(&counter));
    printf("ticket=%ld\n", atomic_load(&ticket));
    printf("done=%ld\n", atomic_load(&done));
    printf("plain_total=%ld\n", plain_total);
    for (i = 1; i <= THREADS; i++) printf("owners[%ld]=%ld\n", i, owners[i]);
    printf("maxseen=%ld\n", atomic_load(&maxseen));
    printf("bits=%ld\n", atomic_load(&bits));
    return 0;
}
