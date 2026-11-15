/*
 * Hailstone.cpp
 *
 * Computes the length of the Hailstone (Collatz) sequence of a starting
 * number:  f(n) = n / 2 if n is even, 3n + 1 if n is odd, until n = 1.
 * The length counts every term, including the start and the final 1.
 * This simplest version recurses without memoization.  Without an argument
 * it prints a table of the lengths for 1 ... 100.
 * C++ version of documentation/projects/Hailstone.asm.
 *
 * Usage:
 *     Hailstone <n>     the sequence length for n
 *     Hailstone         a table for n = 1 ... 100
 *
 * Author:   Richard Lesh
 * Original: 2009/03/20
 */
#include <cstdlib>
#include <iostream>

static long compute_hailstone(long n)
{
    if (n <= 1) return 1;
    if ((n & 1) == 0) return compute_hailstone(n / 2) + 1;
    return compute_hailstone(3 * n + 1) + 1;
}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        std::cout << compute_hailstone(std::atol(argv[1])) << '\n';
        return 0;
    }
    for (long i = 1; i <= 100; i++) std::cout << i << ": " << compute_hailstone(i) << '\n';
    return 1;
}
