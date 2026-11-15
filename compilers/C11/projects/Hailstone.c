/*
 * Hailstone.c
 *
 * Computes the length of the Hailstone (Collatz) sequence of a starting
 * number:  f(n) = n / 2 if n is even, 3n + 1 if n is odd, until n = 1.
 * The length counts every term, including the start and the final 1.
 * This simplest version recurses without memoization.  Without an argument
 * it prints a table of the lengths for 1 ... 100.
 * C11 version of documentation/projects/Hailstone.asm.
 *
 * Usage:
 *     Hailstone <n>     the sequence length for n
 *     Hailstone         a table for n = 1 ... 100
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
    long i;
    if (argc >= 2) {
        printf("%ld\n", compute_hailstone(atol(argv[1])));
        return 0;
    }
    for (i = 1; i <= 100; i++) printf("%ld: %ld\n", i, compute_hailstone(i));
    return 1;
}
