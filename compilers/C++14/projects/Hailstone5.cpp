/*
 * Hailstone5.cpp
 *
 * Multi-threaded search for the starting number up to a limit that has the
 * longest Hailstone (Collatz) sequence.  Unlike Hailstone4 (one fixed share
 * per thread), the range is cut into work units of WORKSIZE values and every
 * thread, one per processor core, takes the next unit with an atomic counter
 * until none are left, so the threads stay busy to the end.  The memo cache
 * (CACHE_SIZE entries) is filled in by the main thread before the threads
 * start and is shared by all of them.
 * C++ version of documentation/projects/Hailstone5.asm; it uses <thread.h>.
 *
 * Usage:
 *     Hailstone5 <limit>
 *
 * Output:
 *     the number of cores, a line for each work unit and new high, then
 *     "Max <length> found at <i>".
 *
 * Author:   Richard Lesh
 */
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread.h>

static const long CACHE_SIZE = 8000000;     // number of memoized entries

// memoizes the length of the hailstone sequence of every starting value below CACHE_SIZE
class HailstoneCache {
public:
    HailstoneCache() : cache(CACHE_SIZE, 0)
    {
        cache[0] = 1;
        cache[1] = 1;
        cache[2] = 2;
        cache[3] = 8;
        cache[4] = 3;
        cache[5] = 6;
    }

    long length(long n)
    {
        if (n < CACHE_SIZE && cache[n] != 0) return cache[n];
        long len;
        if ((n & 1) == 0) len = length(n / 2) + 1;
        else len = length(3 * n + 1) + 1;
        if (n < CACHE_SIZE) cache[n] = len;
        return len;
    }

private:
    std::vector<long> cache;
};

static long gMax, gImax;            // the best length so far, and its starting value
static long gWorksize = 100000;     // starting values per work unit
static long gNextUnit = -1;         // the last work unit taken
static long gLimit;
static mutex_t gMutex;
static HailstoneCache *gCache;

static void worker(void *)
{
    long pid = get_pid();
    for (;;) {
        long d = atomic_increment(&gNextUnit) * gWorksize;      // the next work unit
        if (d >= gLimit) break;
        long limit = d + gWorksize, wmax = 0, wimax = 0;
        if (limit > gLimit) limit = gLimit;
        std::printf("Thread work unit %ld executing with PID %ld...\n", d, pid);
        for (long i = d; i < limit; i++) {
            long hs = gCache->length(i);
            if (hs > wmax) {
                wmax = hs;
                wimax = i;
            }
        }
        mutex_lock(&gMutex);
        if (wmax > gMax || (wmax == gMax && wimax < gImax)) {
            std::printf("New high: %ld:%ld (%ld)\n", wimax, wmax, pid);
            gMax = wmax;
            gImax = wimax;
        }
        mutex_unlock(&gMutex);
    }
    std::printf("Thread %ld finishing...\n", pid);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "You must supply a positive integer argument." << std::endl;
        return 1;
    }
    gLimit = std::atol(argv[1]);
    if (gLimit < 2) {
        std::cout << "Argument must be >= 2!" << std::endl;
        return 1;
    }
    mutex_init(&gMutex);
    // on the heap, which the threads share (the main thread's stack is not shared); built before they start
    gCache = new HailstoneCache;
    long cores = get_num_cores();
    std::printf("Number of cores: %ld\n", cores);
    std::vector<long> pids(cores, 0);
    for (long i = 0; i < cores; i++) pids[i] = thread_create(worker, nullptr);
    for (long i = 0; i < cores; i++) {
        std::printf("Main is joining %ld...\n", pids[i]);
        thread_join(pids[i]);
    }
    std::printf("Max %ld found at %ld\n", gMax, gImax);
    delete gCache;
    return 0;
}
