/*
 * CAS_printf_stress.c
 *
 * Stress test of the atomic operations and of printf with several threads
 * (one per processor core).
 *
 * Test 1: every thread adds CAS_ITERS to a counter with atomic_fetch_add()
 *         and CAS_ITERS to a second counter with its own compare-and-swap
 *         loop.  No increment may be lost, so each counter must end at
 *         CAS_ITERS * threads.
 * Test 2: after a pause every thread prints PRINT_ITERS lines.  The
 *         simulator writes a line character by character, so without a lock
 *         the lines of the threads are mixed on the Java simulator (the
 *         assembly version serializes them only with USE_PRINT_MUTEX).  Here
 *         a mutex serializes them unless NO_PRINT_MUTEX is defined (put
 *         "#define NO_PRINT_MUTEX" before the includes to see the mixing).
 * C11 version of documentation/projects/CAS_printf_stress.asm; it uses
 * <threads.h> and <stdatomic.h>.
 *
 * Usage:
 *     CAS_printf_stress
 *
 * Output:
 *     "PASS ..." or "FAIL ..." for each counter, then the printed lines.
 *     The result is the number of failed counters.
 *
 * Author:   Richard Lesh
 */
#ifndef NO_PRINT_MUTEX
#define USE_PRINT_MUTEX
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <threads.h>
#include <stdatomic.h>
#include <unistd.h>

#define CAS_ITERS 100000
#define PRINT_ITERS 1000
#define PAUSE_MS 10000

static atomic_long gLibCounter;     /* incremented with atomic_fetch_add() */
static atomic_long gRawCounter;     /* incremented with a CAS loop */
#ifdef USE_PRINT_MUTEX
static mtx_t gPrintMutex;
#endif

/* waits ms milliseconds.  The C library has no sleep function, so this busy-waits on clock();
   with the environment variable NO_DELAY set it returns at once (for the tests). */
static void pause_ms(long ms)
{
    clock_t end;
    if (getenv("NO_DELAY")) return;
    end = clock() + (clock_t)(ms * (CLOCKS_PER_SEC / 1000));
    while (clock() < end) {}
}

static int cas_worker(void *arg)
{
    long i, old;
    (void)arg;
    for (i = 0; i < CAS_ITERS; i++) atomic_fetch_add(&gLibCounter, 1);
    for (i = 0; i < CAS_ITERS; i++) {
        old = atomic_load(&gRawCounter);
        while (!atomic_compare_exchange_strong(&gRawCounter, &old, old + 1)) {}
    }
    return 0;
}

static int print_worker(void *arg)
{
    long t = (long)arg, i;
    for (i = 0; i < PRINT_ITERS; i++) {
        long key = t * 10000 + i;
#ifdef USE_PRINT_MUTEX
        mtx_lock(&gPrintMutex);
#endif
        printf("T%ld L%ld K%ld abcdefghijklmnopqrstuvwxyz END\n", t, i, key);
#ifdef USE_PRINT_MUTEX
        mtx_unlock(&gPrintMutex);
#endif
    }
    return 0;
}

/* runs count threads of fn, with the thread number as their argument; returns how many started */
static long run_threads(thrd_start_t fn, thrd_t *pids, long count)
{
    long i, started = 0;
    for (i = 0; i < count; i++) {
        if (thrd_create(&pids[i], fn, (void *)i) == thrd_success) {
            started++;
        } else {
            printf("Error: thread %ld failed to start!\n", i);
            pids[i] = 0;
        }
    }
    for (i = 0; i < count; i++)
        if (pids[i] > 0) thrd_join(pids[i], NULL);
    return started;
}

int main(void)
{
    long cores = sysconf(_SC_NPROCESSORS_ONLN), expected, actual, started, failures = 0;
    thrd_t *pids;
    printf("Threads: %ld\n", cores);
    pids = malloc(cores * sizeof(thrd_t));
    if (!pids) {
        puts("Error: PID array allocation failed!");
        return 1;
    }
    puts("=== CAS TEST BEGIN ===");
    started = run_threads(cas_worker, pids, cores);
    expected = CAS_ITERS * started;
    actual = gLibCounter;
    if (actual == expected) {
        printf("PASS atomic_increment: %ld\n", actual);
    } else {
        printf("FAIL atomic_increment: expected %ld, got %ld (lost %ld)\n", expected, actual, expected - actual);
        failures++;
    }
    actual = gRawCounter;
    if (actual == expected) {
        printf("PASS raw CAS loop: %ld\n", actual);
    } else {
        printf("FAIL raw CAS loop: expected %ld, got %ld (lost %ld)\n", expected, actual, expected - actual);
        failures++;
    }
    puts("=== CAS TEST END ===");
    puts("Pausing 10 seconds...");
    pause_ms(PAUSE_MS);
#ifdef USE_PRINT_MUTEX
    mtx_init(&gPrintMutex, mtx_plain);
    puts("(printf serialized with PRINT_MUTEX)");
#endif
    expected = PRINT_ITERS * cores;
    printf("=== PRINTF TEST BEGIN (expect %ld lines) ===\n", expected);
    started = run_threads(print_worker, pids, cores);
    printf("=== PRINTF TEST END (%ld threads started) ===\n", started);
    free(pids);
    return (int)failures;
}
