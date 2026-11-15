/*
 * Hailstone3.cpp
 *
 * Finds the starting number up to a limit that has the longest Hailstone
 * (Collatz) sequence.  The lengths of starting values below CACHE_SIZE are
 * memoized in a heap array, so each value is computed once.  A progress line
 * is printed every 1000 candidates.
 * C++ version of documentation/projects/Hailstone3.asm.
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
#include <cstdlib>
#include <iostream>
#include <vector>

static const long CACHE_SIZE = 3000000;     // number of memoized entries

// cache[n] = length for n, 0 = not computed yet
static std::vector<long> cache;

static long compute_hailstone(long n)
{
    if (cache.empty()) {
        cache.assign(CACHE_SIZE, 0);
        cache[1] = 1;               // the sequence of 1 has length 1
    }
    if (n < CACHE_SIZE && cache[n] != 0) return cache[n];
    long len;
    if ((n & 1) == 0) len = compute_hailstone(n / 2) + 1;
    else len = compute_hailstone(3 * n + 1) + 1;
    if (n < CACHE_SIZE) cache[n] = len;
    return len;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "You must supply a positive integer argument." << std::endl;
        return 1;
    }
    long limit = std::atol(argv[1]), imax = 1, max = 1;
    for (long i = 2; i <= limit; i++) {
        long len = compute_hailstone(i);
        if (len > max) {
            imax = i;
            max = len;
        }
        if (i % 1000 == 0) std::cout << i << "..." << imax << ":" << max << '\n';
    }
    std::cout << imax << ": " << max << '\n';
    return 0;
}
