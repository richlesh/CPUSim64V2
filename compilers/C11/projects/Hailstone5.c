/*
 * Hailstone5.c
 *
 * Multi-threaded search for the starting number up to a limit that has the
 * longest Hailstone (Collatz) sequence.  Unlike Hailstone4 (one fixed share
 * per thread), the range is cut into work units of WORKSIZE values and every
 * thread, one per processor core, takes the next unit with an atomic counter
 * until none are left, so the threads stay busy to the end.  The memo cache
 * (CACHE_SIZE entries) is filled in by the main thread before the threads
 * start and is shared by all of them.
 * C11 version of documentation/projects/Hailstone5.asm; it uses <threads.h> and <stdatomic.h>.
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
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>
#include <unistd.h>

#define CACHE_SIZE 8000000          /* number of memoized entries */

static long gMax, gImax;            /* the best length so far, and its starting value */
static long gWorksize = 100000;     /* starting values per work unit */
static atomic_long gNextUnit = -1;        /* the last work unit taken */
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

static int worker(void *unused)
{
    long pid = thrd_current();
    (void)unused;
    for (;;) {
        long d = (atomic_fetch_add(&gNextUnit, 1) + 1) * gWorksize;      /* the next work unit */
        long limit, i, wmax = 0, wimax = 0;
        if (d >= gLimit) break;
        limit = d + gWorksize;
        if (limit > gLimit) limit = gLimit;
        printf("Thread work unit %ld executing with PID %ld...\n", d, pid);
        for (i = d; i < limit; i++) {
            long hs = compute_hailstone(i);
            if (hs > wmax) {
                wmax = hs;
                wimax = i;
            }
        }
        mtx_lock(&gMutex);
        if (wmax > gMax || (wmax == gMax && wimax < gImax)) {
            printf("New high: %ld:%ld (%ld)\n", wimax, wmax, pid);
            gMax = wmax;
            gImax = wimax;
        }
        mtx_unlock(&gMutex);
    }
    printf("Thread %ld finishing...\n", pid);
    return 0;
}

int main(int argc, char **argv)
{
    long i, cores;
    thrd_t *pids;
    if (argc < 2) {
        puts("You must supply a positive integer argument.");
        return 1;
    }
    gLimit = atol(argv[1]);
    if (gLimit < 2) {
        puts("Argument must be >= 2!");
        return 1;
    }
    mtx_init(&gMutex, mtx_plain);
    compute_hailstone(1);                   /* allocates the cache before the threads start */
    cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of cores: %ld\n", cores);
    pids = malloc(cores * sizeof(thrd_t));
    if (!pids) {
        printf("Can't allocate pids array!\n");
        exit(1);
    }
    for (i = 0; i < cores; i++) thrd_create(&pids[i], worker, NULL);
    for (i = 0; i < cores; i++) {
        printf("Main is joining %ld...\n", pids[i]);
        thrd_join(pids[i], NULL);
    }
    printf("Max %ld found at %ld\n", gMax, gImax);
    free(pids);
    return 0;
}
