/*
 * Hailstone3.c
 *
 * Finds the starting number up to a limit that has the longest Hailstone
 * (Collatz) sequence.  The lengths of starting values below CACHE_SIZE are
 * memoized in a heap array, so each value is computed once.  A progress line
 * is printed every 1000 candidates.
 * C11 version of documentation/projects/Hailstone3.asm.
 *
 * Usage:
 *     Hailstone3 <limit>
 *
 * Output:
 *     "<i>...<imax>:<max>" every 1000 candidates, then "<imax>: <max>".
 *
 * Author:   Richard Lesh
 * Original: 2009/03/20
 */
#include <stdio.h>
#include <stdlib.h>

#define CACHE_SIZE 3000000          /* number of memoized entries */

static long *cache;                 /* cache[n] = length for n, 0 = not computed yet */

static long compute_hailstone(long n)
{
    long len;
    if (!cache) {
        cache = calloc(CACHE_SIZE, sizeof(long));
        if (!cache) {
            printf("Can't allocate cache size %d\n", CACHE_SIZE);
            exit(1);
        }
        cache[1] = 1;               /* the sequence of 1 has length 1 */
    }
    if (n < CACHE_SIZE && cache[n] != 0) return cache[n];
    if ((n & 1) == 0) len = compute_hailstone(n / 2) + 1;
    else len = compute_hailstone(3 * n + 1) + 1;
    if (n < CACHE_SIZE) cache[n] = len;
    return len;
}

int main(int argc, char **argv)
{
    long i, limit, imax = 1, max = 1;
    if (argc < 2) {
        puts("You must supply a positive integer argument.");
        return 1;
    }
    limit = atol(argv[1]);
    for (i = 2; i <= limit; i++) {
        long len = compute_hailstone(i);
        if (len > max) {
            imax = i;
            max = len;
        }
        if (i % 1000 == 0) printf("%ld...%ld:%ld\n", i, imax, max);
    }
    printf("%ld: %ld\n", imax, max);
    return 0;
}
