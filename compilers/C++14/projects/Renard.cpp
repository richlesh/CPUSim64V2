/*
 * Renard.cpp
 *
 * Computes and prints a Renard series of preferred numbers.  The R5, R10,
 * R20 and R40 series divide a decade into 5, 10, 20 or 40 equal logarithmic
 * steps; term n of the series S is (10 ^ (1/S)) ^ n.
 * C++ version of documentation/projects/Renard.asm.
 *
 * Usage:
 *     Renard <series>
 *
 * Example:
 *     > Renard 10
 *     1.00 1.26 1.58 2.00 2.51 3.16 3.98 5.01 6.31 7.94
 *
 * Author:   Richard Lesh
 * Original: 2022/11/28
 */
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "You must supply a series value 5, 10, 20 or 40" << std::endl;
        return 1;
    }
    long series = std::atol(argv[1]);
    if (series < 2) {
        std::cout << "You must supply a series value > 2" << std::endl;
        return 2;
    }
    double factor = std::pow(10.0, 1.0 / series);          // the common ratio
    for (long i = 0; i < series; i++) std::printf("%.2f ", std::pow(factor, (double)i));
    std::cout << '\n';
    return 0;
}
