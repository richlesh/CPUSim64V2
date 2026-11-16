/*
 * Hailstone4.c
 *
 * Multi-threaded search for the starting number up to a limit that has the
 * longest Hailstone (Collatz) sequence.  The range is split into one equal
 * work unit per processor core; the main thread does the first unit and a
 * thread does each of the others.  The sequence lengths of the values below
 * CACHE_SIZE are memoized in a heap array that all the threads share (two
 * threads may compute the same entry, but they store the same value).  Each
 * thread reports its best value under a mutex.
 * C11 version of documentation/projects/Hailstone4.asm; it uses <threads.h>.
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
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

#define CACHE_SIZE 3000000          /* number of memoized entries */

static long gMax, gImax;            /* the best length so far, and its starting value */
static long gWorksize = 10000;      /* starting values per thread */
static long gLimit;
static mtx_t gMutex;
static long *cache;

static long compute_hailstone(long n)
{
    long len;
    if (!cache) {
        cache = calloc(CACHE_SIZE, sizeof(long));
        if (!cache) {
            printf("Can't allocate cache size %d\n", CACHE_SIZE);
            exit(1);
        }
        cache[0] = 1;
        cache[1] = 1;
        cache[2] = 2;
        cache[3] = 8;
        cache[4] = 3;
        cache[5] = 6;
    }
    if (n < CACHE_SIZE && cache[n] != 0) return cache[n];
    if ((n & 1) == 0) len = compute_hailstone(n / 2) + 1;
    else len = compute_hailstone(3 * n + 1) + 1;
    if (n < CACHE_SIZE) cache[n] = len;
    return len;
}

/* handles the starting values first .. first + gWorksize - 1 */
static int worker(void *data)
{
    long first = (long)data, limit = first + gWorksize, i, pid, wmax = 0, wimax = 0;
    if (limit > gLimit) limit = gLimit;
    pid = thrd_current();
    printf("Thread work unit %ld executing with PID %ld...\n", first, pid);
    for (i = first; i < limit; i++) {
        long hs = compute_hailstone(i);
        if (hs > wmax) {
            wmax = hs;
            wimax = i;
        }
    }
    mtx_lock(&gMutex);
    /* a longer sequence wins; of two equal ones the smaller starting value */
    if (wmax > gMax || (wmax == gMax && wimax < gImax)) {
        printf("New high: %ld:%ld (%ld)\n", wimax, wmax, pid);
        gMax = wmax;
        gImax = wimax;
    }
    mtx_unlock(&gMutex);
    printf("Thread %ld finishing...\n", pid);
    return 0;
}

int main(int argc, char **argv)
{
    long i, j, cores, ws;
    thrd_t *pids;
    cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of cores: %ld\n", cores);
    if (argc < 2) {
        puts("You must supply a positive integer argument.");
        return 1;
    }
    gLimit = atol(argv[1]);
    if (gLimit < 2) {
        puts("Argument must be > 2!");
        return 1;
    }
    ws = (gLimit + cores - 1) / cores;      /* ceil(limit / cores) */
    printf("Worksize: %ld\n", ws);
    gWorksize = ws;
    mtx_init(&gMutex, mtx_plain);
    worker((void *)0);                      /* the main thread does the first unit */
    j = ws;
    pids = malloc(cores * sizeof(thrd_t));
    if (!pids) {
        printf("Can't allocate pids array!\n");
        exit(1);
    }
    for (i = 1; i < cores; i++) {
        thrd_create(&pids[i], worker, (void *)j);
        j += ws;
    }
    for (i = 1; i < cores; i++) {
        printf("Main is joining %ld...\n", pids[i]);
        thrd_join(pids[i], NULL);
    }
    printf("Max %ld found at %ld\n", gMax, gImax);
    free(pids);
    return 0;
}
