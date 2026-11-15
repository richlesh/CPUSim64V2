/*
 * CAS_printf_stress.cpp
 *
 * Stress test of the atomic operations and of printf with several threads
 * (one per processor core).
 *
 * Test 1: every thread adds CAS_ITERS to a counter with atomic_increment()
 *         and CAS_ITERS to a second counter with its own compare-and-swap
 *         loop.  No increment may be lost, so each counter must end at
 *         CAS_ITERS * threads.
 * Test 2: after a pause every thread prints PRINT_ITERS lines.  The
 *         simulator writes a line character by character, so without a lock
 *         the lines of the threads are mixed on the Java simulator (the
 *         assembly version serializes them only with USE_PRINT_MUTEX).  Here
 *         a mutex serializes them unless NO_PRINT_MUTEX is defined (put
 *         "#define NO_PRINT_MUTEX" before the includes to see the mixing).
 * C++ version of documentation/projects/CAS_printf_stress.asm; it uses
 * <thread.h>.
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
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <thread.h>

static const long CAS_ITERS = 100000;
static const long PRINT_ITERS = 1000;
static const long PAUSE_MS = 10000;

static long gLibCounter;            // incremented with atomic_increment()
static long gRawCounter;            // incremented with a CAS loop
#ifdef USE_PRINT_MUTEX
static mutex_t gPrintMutex;

// holds a mutex for its lifetime
class MutexLock {
public:
    explicit MutexLock(mutex_t &m) : m(m) { mutex_lock(&m); }
    ~MutexLock() { mutex_unlock(&m); }
    MutexLock(const MutexLock &) = delete;
    MutexLock &operator=(const MutexLock &) = delete;

private:
    mutex_t &m;
};
#endif

// waits ms milliseconds.  The C library has no sleep function, so this busy-waits on clock();
// with the environment variable NO_DELAY set it returns at once (for the tests).
static void pause_ms(long ms)
{
    if (std::getenv("NO_DELAY")) return;
    std::clock_t end = std::clock() + (std::clock_t)(ms * (CLOCKS_PER_SEC / 1000));
    while (std::clock() < end) {}
}

static void cas_worker(void *)
{
    for (long i = 0; i < CAS_ITERS; i++) atomic_increment(&gLibCounter);
    for (long i = 0; i < CAS_ITERS; i++) {
        long old;
        do {
            old = gRawCounter;
        } while (!atomic_cas(&gRawCounter, old, old + 1));
    }
}

static void print_worker(void *arg)
{
    long t = (long)arg;
    for (long i = 0; i < PRINT_ITERS; i++) {
        long key = t * 10000 + i;
#ifdef USE_PRINT_MUTEX
        MutexLock lock(gPrintMutex);
#endif
        std::printf("T%ld L%ld K%ld abcdefghijklmnopqrstuvwxyz END\n", t, i, key);
    }
}

// runs count threads of fn, with the thread number as their argument; returns how many started
static long run_threads(thread_func fn, std::vector<long> &pids, long count)
{
    long started = 0;
    for (long i = 0; i < count; i++) {
        long pid = thread_create(fn, (void *)i);
        if (pid > 0) {
            pids[i] = pid;
            started++;
        } else {
            std::printf("Error: thread %ld failed to start!\n", i);
            pids[i] = 0;
        }
    }
    for (long i = 0; i < count; i++)
        if (pids[i] > 0) thread_join(pids[i]);
    return started;
}

int main()
{
    long cores = get_num_cores(), failures = 0;
    std::printf("Threads: %ld\n", cores);
    std::vector<long> pids(cores, 0);
    std::puts("=== CAS TEST BEGIN ===");
    long started = run_threads(cas_worker, pids, cores);
    long expected = CAS_ITERS * started;
    long actual = gLibCounter;
    if (actual == expected) {
        std::printf("PASS atomic_increment: %ld\n", actual);
    } else {
        std::printf("FAIL atomic_increment: expected %ld, got %ld (lost %ld)\n", expected, actual, expected - actual);
        failures++;
    }
    actual = gRawCounter;
    if (actual == expected) {
        std::printf("PASS raw CAS loop: %ld\n", actual);
    } else {
        std::printf("FAIL raw CAS loop: expected %ld, got %ld (lost %ld)\n", expected, actual, expected - actual);
        failures++;
    }
    std::puts("=== CAS TEST END ===");
    std::puts("Pausing 10 seconds...");
    pause_ms(PAUSE_MS);
#ifdef USE_PRINT_MUTEX
    mutex_init(&gPrintMutex);
    std::puts("(printf serialized with PRINT_MUTEX)");
#endif
    expected = PRINT_ITERS * cores;
    std::printf("=== PRINTF TEST BEGIN (expect %ld lines) ===\n", expected);
    started = run_threads(print_worker, pids, cores);
    std::printf("=== PRINTF TEST END (%ld threads started) ===\n", started);
    return (int)failures;
}
