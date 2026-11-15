/*
 * Hailstone2.c
 *
 * Finds the starting number up to a limit that has the longest Hailstone
 * (Collatz) sequence.  Each length is computed recursively with no
 * memoization (see Hailstone3 for the cached version).
 * C11 version of documentation/projects/Hailstone2.asm.
 *
 * Usage:
 *     Hailstone2 <limit>
 *
 * Output:
 *     "<imax>: <max>", the best starting number and its sequence length.
 *
 * Author:   Richard Lesh
 * Original: 2009/03/20
 */
#include <stdio.h>
#include <stdlib.h>

static long compute_hailstone(long n)
{
    if (n <= 1) return 1;
    if ((n & 1) == 0) return compute_hailstone(n / 2) + 1;
    return compute_hailstone(3 * n + 1) + 1;
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
    }
    printf("%ld: %ld\n", imax, max);
    return 0;
}
