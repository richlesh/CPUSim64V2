/*
 * Congruent.cpp
 *
 * Decides whether n is a congruent number (the area of a right triangle
 * with rational sides) with Tunnell's theorem, assuming the Birch and
 * Swinnerton-Dyer conjecture.  Let m be the squarefree kernel of n.
 *   m odd:   t1 = #{2x^2 + y^2 +  8z^2 = m},   t2 = #{2x^2 + y^2 + 32z^2 = m}
 *   m even:  t1 = #{8x^2 + 2y^2 + 16z^2 = m},  t2 = #{8x^2 + 2y^2 + 64z^2 = m}
 * m is congruent  <=>  t1 == 2 * t2.
 * C++ version of documentation/projects/Congruent.asm.
 *
 * Usage:
 *     Congruent <n>
 *
 * Author:   Richard Lesh
 */
#include <cmath>
#include <cstdlib>
#include <iostream>

// n divided by its largest square factor
static long squarefree_kernel(long n)
{
    long m = n, i = 2;
    while (i * i <= m) {
        if (m % (i * i) == 0) m /= i * i;
        else i++;
    }
    return m;
}

static long isqrt(long n)
{
    return (long)std::floor(std::sqrt((double)n));
}

// floor(sqrt(target / divisor))
static long compute_limit(long target, long divisor)
{
    return (long)std::floor(std::sqrt((double)target / (double)divisor));
}

// the number of integer solutions (x, y, z) of a*x^2 + b*y^2 + c*z^2 = target
static long count_solutions(long target, long a, long b, long c)
{
    long count = 0;
    long xlim = compute_limit(target, a);
    for (long x = -xlim; x <= xlim; x++) {
        long remX = a * x * x;
        if (remX > target) continue;
        remX = target - remX;
        long ylim = compute_limit(remX, b);
        for (long y = -ylim; y <= ylim; y++) {
            long remXY = b * y * y;
            if (remXY > remX) continue;
            remXY = remX - remXY;
            if (remXY % c == 0) {
                long zsq = remXY / c;
                long z = isqrt(zsq);
                if (z * z == zsq) count += z == 0 ? 1 : 2;      // z and -z
            }
        }
    }
    return count;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "Usage: Congruent <n>" << std::endl;
        return 1;
    }
    long n = std::atol(argv[1]);
    if (n <= 0) {
        std::cout << "n must be a positive integer" << std::endl;
        return 1;
    }
    long m = squarefree_kernel(n), t1, t2;
    if (m & 1) {
        t1 = count_solutions(m, 2, 1, 8);
        t2 = count_solutions(m, 2, 1, 32);
    } else {
        t1 = count_solutions(m, 8, 2, 16);
        t2 = count_solutions(m, 8, 2, 64);
    }
    std::cout << "n = " << n << " (squarefree kernel = " << m << ")\n";
    std::cout << "t1 = " << t1 << ", 2*t2 = " << 2 * t2 << '\n';
    if (t1 == 2 * t2) std::cout << n << " IS a congruent number (assuming BSD)\n";
    else std::cout << n << " is NOT a congruent number\n";
    return 0;
}
