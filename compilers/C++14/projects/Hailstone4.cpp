/*
 * Hailstone4.cpp
 *
 * Multi-threaded search for the starting number up to a limit that has the
 * longest Hailstone (Collatz) sequence.  The range is split into one equal
 * work unit per processor core; the main thread does the first unit and a
 * thread does each of the others.  The sequence lengths of the values below
 * CACHE_SIZE are memoized in a heap array that all the threads share (two
 * threads may compute the same entry, but they store the same value).  Each
 * thread reports its best value under a mutex.
 * C++ version of documentation/projects/Hailstone4.asm; it uses <thread.h>.
 *
 * Usage:
 *     Hailstone4 <limit>
 *
 * Output:
 *     the number of cores, the work size, a line for each thread starting,
 *     finishing and finding a new high, then "Max <length> found at <i>".
 *
 * Author:   Richard Lesh
 */
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread.h>

static const long CACHE_SIZE = 3000000;     // number of memoized entries

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
static long gWorksize = 10000;      // starting values per thread
static long gLimit;
static mutex_t gMutex;
static HailstoneCache *gCache;

// handles the starting values first .. first + gWorksize - 1
static void worker(void *data)
{
    long first = (long)data, limit = first + gWorksize, wmax = 0, wimax = 0;
    if (limit > gLimit) limit = gLimit;
    long pid = get_pid();
    std::printf("Thread work unit %ld executing with PID %ld...\n", first, pid);
    for (long i = first; i < limit; i++) {
        long hs = gCache->length(i);
        if (hs > wmax) {
            wmax = hs;
            wimax = i;
        }
    }
    mutex_lock(&gMutex);
    // a longer sequence wins; of two equal ones the smaller starting value
    if (wmax > gMax || (wmax == gMax && wimax < gImax)) {
        std::printf("New high: %ld:%ld (%ld)\n", wimax, wmax, pid);
        gMax = wmax;
        gImax = wimax;
    }
    mutex_unlock(&gMutex);
    std::printf("Thread %ld finishing...\n", pid);
}

int main(int argc, char **argv)
{
    long cores = get_num_cores();
    std::printf("Number of cores: %ld\n", cores);
    if (argc < 2) {
        std::cout << "You must supply a positive integer argument." << std::endl;
        return 1;
    }
    gLimit = std::atol(argv[1]);
    if (gLimit < 2) {
        std::cout << "Argument must be > 2!" << std::endl;
        return 1;
    }
    long ws = (gLimit + cores - 1) / cores;      // ceil(limit / cores)
    std::printf("Worksize: %ld\n", ws);
    gWorksize = ws;
    mutex_init(&gMutex);
    // on the heap, which the threads share (the main thread's stack is not shared)
    gCache = new HailstoneCache;
    worker((void *)0);                      // the main thread does the first unit
    long j = ws;
    std::vector<long> pids(cores, 0);
    for (long i = 1; i < cores; i++) {
        pids[i] = thread_create(worker, (void *)j);
        j += ws;
    }
    for (long i = 1; i < cores; i++) {
        std::printf("Main is joining %ld...\n", pids[i]);
        thread_join(pids[i]);
    }
    std::printf("Max %ld found at %ld\n", gMax, gImax);
    delete gCache;
    return 0;
}
