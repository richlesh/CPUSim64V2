/*
 * Hailstone5bad.c
 *
 * The first version of Hailstone5: the work units are not taken with an atomic
 * counter but popped from a shared stack (a vector of starting values) under
 * a mutex, so every unit costs two lock operations and the threads queue up
 * on the one lock.  It finds the same answer as Hailstone5; the work queue is
 * the "bad" part.  The main thread does the first unit before the threads
 * start, which also fills the memo cache (CACHE_SIZE entries).
 * C11 version of documentation/projects/Hailstone5bad.asm; it uses <thread.h>.
 *
 * Usage:
 *     Hailstone5bad <limit>
 *
 * Output:
 *     the number of cores, a line for each work unit and new high, then
 *     "Max <length> found at <i>".
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <thread.h>

#define CACHE_SIZE 8000000          /* number of memoized entries */

static long gMax, gImax;            /* the best length so far, and its starting value */
static long gWorksize = 100000;     /* starting values per work unit */
static long gLimit;
static mutex_t gMutex;              /* protects gMax and gImax */
static mutex_t gQueueMutex;         /* protects the queue */
static long *gQueue;                /* the starting values of the work units not yet taken */
static long gQueueSize;

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

/* pops the next work unit, or -1 when the queue is empty */
static long pop_unit(void)
{
    long d = -1;
    mutex_lock(&gQueueMutex);
    if (gQueueSize > 0) d = gQueue[--gQueueSize];
    mutex_unlock(&gQueueMutex);
    return d;
}

static void worker(void *unused)
{
    long pid = get_pid(), d;
    (void)unused;
    for (d = pop_unit(); d != -1; d = pop_unit()) {
        long limit = d + gWorksize, i, wmax = 0, wimax = 0;
        if (limit > gLimit) limit = gLimit;
        printf("Thread work unit %ld executing with PID %ld...\n", d, pid);
        for (i = d; i < limit; i++) {
            long hs = compute_hailstone(i);
            if (hs > wmax) {
                wmax = hs;
                wimax = i;
            }
        }
        mutex_lock(&gMutex);
        if (wmax > gMax || (wmax == gMax && wimax < gImax)) {
            printf("New high: %ld:%ld (%ld)\n", wimax, wmax, pid);
            gMax = wmax;
            gImax = wimax;
        }
        mutex_unlock(&gMutex);
    }
    printf("Thread %ld finishing...\n", pid);
}

int main(int argc, char **argv)
{
    long i, cores, units, first;
    long *pids;
    if (argc < 2) {
        puts("You must supply a positive integer argument.");
        return 1;
    }
    gLimit = atol(argv[1]);
    if (gLimit < 2) {
        puts("Argument must be > 2!");
        return 1;
    }
    mutex_init(&gMutex);
    mutex_init(&gQueueMutex);
    units = (gLimit + gWorksize - 1) / gWorksize;
    gQueue = malloc(units * sizeof(long));
    if (!gQueue) {
        printf("Can't allocate the work queue!\n");
        exit(1);
    }
    gQueue[0] = 0;
    gQueueSize = 1;
    worker(NULL);                           /* the main thread does the first unit, and sets up the cache */
    /* push the other units, the last first, so the lowest is popped first */
    for (first = (units - 1) * gWorksize; first > 0; first -= gWorksize) gQueue[gQueueSize++] = first;
    cores = get_num_cores();
    printf("Number of cores: %ld\n", cores);
    pids = malloc(cores * sizeof(long));
    if (!pids) {
        printf("Can't allocate pids array!\n");
        exit(1);
    }
    for (i = 0; i < cores; i++) pids[i] = thread_create(worker, NULL);
    for (i = 0; i < cores; i++) {
        printf("Main is joining %ld...\n", pids[i]);
        thread_join(pids[i]);
    }
    printf("Max %ld found at %ld\n", gMax, gImax);
    free(pids);
    free(gQueue);
    return 0;
}
