/*
 * Pi.cpp
 *
 * Computes pi with the Bailey-Borwein-Plouffe series
 *     pi = sum over k of 1/16^k * (4/(8k+1) - 2/(8k+4) - 1/(8k+5) - 1/(8k+6))
 * C++ version of documentation/projects/Pi.asm.
 *
 * Usage:
 *     Pi <terms>      the number of series terms, 1-11
 *
 * Example:
 *     > Pi 11
 *     PI(11): 3.1415926535897930
 *
 * Author:   Richard Lesh
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

static double compute_pi(long terms)
{
    double sum = 0.0;
    for (long k = 0; k < terms; k++) {
        double t = 4.0 / (8 * k + 1) - 2.0 / (8 * k + 4) - 1.0 / (8 * k + 5) - 1.0 / (8 * k + 6);
        sum += t / std::pow(16.0, (double)k);
    }
    return sum;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "You must supply the number of terms 1-11" << std::endl;
        return 1;
    }
    long terms = std::atol(argv[1]);
    if (!(0 < terms && terms <= 11)) {
        std::cout << "You must supply the number of terms in the range [1-11]" << std::endl;
        return 1;
    }
    std::printf("PI(%ld): %.16f\n", terms, compute_pi(terms));
    return 0;
}
