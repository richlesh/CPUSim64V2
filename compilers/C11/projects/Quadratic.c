/*
 * Quadratic.c
 *
 * Solves A*x^2 + B*x + C = 0 with the quadratic formula and prints the two
 * real roots, the repeated root, or the two complex roots.
 * C11 version of documentation/projects/Quadratic.asm.
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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static void solve_quadratic(double a, double b, double c)
{
    double disc, twoA;
    if (a == 0.0) {
        puts("Not a quadratic equation: coefficient A must be non-zero.");
        return;
    }
    disc = b * b - a * c * 4;
    twoA = a * 2;
    if (disc > 0.0) {
        double sqrtD = sqrt(disc);
        printf("Two real roots: x1 = %.6f, x2 = %.6f\n", (-b + sqrtD) / twoA, (-b - sqrtD) / twoA);
    } else if (disc == 0.0) {
        printf("One repeated (double) root: x = %.6f\n", -b / twoA);
    } else {
        double imag = fabs(sqrt(-disc) / twoA);
        double real = -b / twoA;
        printf("Two complex roots: x1 = %.6f + %.6fi, x2 = %.6f - %.6fi\n", real, imag, real, imag);
    }
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        puts("Syntax: Quadratic A B C   (solves A*x^2 + B*x + C = 0)");
        return 1;
    }
    solve_quadratic(atof(argv[1]), atof(argv[2]), atof(argv[3]));
    return 0;
}
