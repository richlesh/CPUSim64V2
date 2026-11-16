/*
 * CAS_printf_stress.cpp
 *
 * Stress test of the atomic operations and of printf with several threads
 * (one per processor core).
 *
 * Test 1: every thread adds CAS_ITERS to a counter with fetch_add()
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
 * std::thread, std::mutex and std::atomic.
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
#include <thread>
#include <mutex>
#include <atomic>

static const long CAS_ITERS = 100000;
static const long PRINT_ITERS = 1000;
static const long PAUSE_MS = 10000;

static std::atomic<long> gLibCounter(0);    // incremented with fetch_add()
static std::atomic<long> gRawCounter(0);    // incremented with a compare_exchange loop
#ifdef USE_PRINT_MUTEX
static std::mutex gPrintMutex;
#endif

// waits ms milliseconds.  The C library has no sleep function, so this busy-waits on clock();
// with the environment variable NO_DELAY set it returns at once (for the tests).
static void pause_ms(long ms)
{
    if (std::getenv("NO_DELAY")) return;
    std::clock_t end = std::clock() + (std::clock_t)(ms * (CLOCKS_PER_SEC / 1000));
    while (std::clock() < end) {}
}

static void cas_worker(long)
{
    for (long i = 0; i < CAS_ITERS; i++) gLibCounter.fetch_add(1);
    for (long i = 0; i < CAS_ITERS; i++) {
        long old = gRawCounter.load();
        while (!gRawCounter.compare_exchange_weak(old, old + 1)) {
        }
    }
}

static void print_worker(long t)
{
    for (long i = 0; i < PRINT_ITERS; i++) {
        long key = t * 10000 + i;
#ifdef USE_PRINT_MUTEX
        std::lock_guard<std::mutex> lock(gPrintMutex);
#endif
        std::printf("T%ld L%ld K%ld abcdefghijklmnopqrstuvwxyz END\n", t, i, key);
    }
}

// runs count threads of fn, with the thread number as their argument; returns how many started
static long run_threads(void (*fn)(long), long count)
{
    std::vector<std::thread> threads;
    for (long i = 0; i < count; i++) threads.push_back(std::thread(fn, i));
    for (long i = 0; i < count; i++) threads[i].join();
    return count;
}

int main()
{
    long cores = (long)std::thread::hardware_concurrency(), failures = 0;
    if (cores < 1) cores = 4;
    std::printf("Threads: %ld\n", cores);
    std::puts("=== CAS TEST BEGIN ===");
    long started = run_threads(cas_worker, cores);
    long expected = CAS_ITERS * started;
    long actual = gLibCounter.load();
    if (actual == expected) {
        std::printf("PASS atomic_increment: %ld\n", actual);
    } else {
        std::printf("FAIL atomic_increment: expected %ld, got %ld (lost %ld)\n", expected, actual, expected - actual);
        failures++;
    }
    actual = gRawCounter.load();
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
    std::puts("(printf serialized with PRINT_MUTEX)");
#endif
    expected = PRINT_ITERS * cores;
    std::printf("=== PRINTF TEST BEGIN (expect %ld lines) ===\n", expected);
    started = run_threads(print_worker, cores);
    std::printf("=== PRINTF TEST END (%ld threads started) ===\n", started);
    return (int)failures;
}
