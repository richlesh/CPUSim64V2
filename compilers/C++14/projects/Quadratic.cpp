/*
 * Quadratic.cpp
 *
 * Solves A*x^2 + B*x + C = 0 with the quadratic formula and prints the two
 * real roots, the repeated root, or the two complex roots.
 * C++ version of documentation/projects/Quadratic.asm.
 *
 * Usage:
 *     Quadratic A B C
 *
 * Example:
 *     > Quadratic 1 -1 -2
 *     Two real roots: x1 = 2.000000, x2 = -1.000000
 *
 * Author:   Richard Lesh
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

static void solve_quadratic(double a, double b, double c)
{
    if (a == 0.0) {
        std::cout << "Not a quadratic equation: coefficient A must be non-zero." << std::endl;
        return;
    }
    double disc = b * b - a * c * 4;
    double twoA = a * 2;
    if (disc > 0.0) {
        double sqrtD = std::sqrt(disc);
        std::printf("Two real roots: x1 = %.6f, x2 = %.6f\n", (-b + sqrtD) / twoA, (-b - sqrtD) / twoA);
    } else if (disc == 0.0) {
        std::printf("One repeated (double) root: x = %.6f\n", -b / twoA);
    } else {
        double imag = std::fabs(std::sqrt(-disc) / twoA);
        double real = -b / twoA;
        std::printf("Two complex roots: x1 = %.6f + %.6fi, x2 = %.6f - %.6fi\n", real, imag, real, imag);
    }
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        std::cout << "Syntax: Quadratic A B C   (solves A*x^2 + B*x + C = 0)" << std::endl;
        return 1;
    }
    solve_quadratic(std::atof(argv[1]), std::atof(argv[2]), std::atof(argv[3]));
    return 0;
}
