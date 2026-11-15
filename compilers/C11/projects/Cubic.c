/*
 * Cubic.c
 *
 * Solves A*x^3 + B*x^2 + C*x + D = 0 with Cardano's method.  The equation is
 * reduced to the depressed cubic t^3 + p*t + q = 0 with x = t - b/3 (after
 * dividing by A); the sign of the discriminant (q/2)^2 + (p/3)^3 selects
 *   > 0:  one real root and two complex conjugate roots (Cardano),
 *   < 0:  three distinct real roots (the trigonometric method),
 *   = 0:  a multiple root.
 * C11 version of documentation/projects/Cubic.asm.
 *
 * Usage:
 *     Cubic A B C D
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define EPSILON 0.000000001

/* the real cube root, also of negative numbers */
static double real_cbrt(double x)
{
    double r = pow(fabs(x), 1.0 / 3);
    return x < 0 ? -r : r;
}

static void solve_cubic(double a, double b, double c, double d)
{
    double p, q, disc, shift;
    b /= a;
    c /= a;
    d /= a;
    shift = b / 3;
    p = c - b * b / 3;
    q = b * b * b * 2 / 27 - b * c / 3 + d;
    disc = (q / 2) * (q / 2) + (p / 3) * (p / 3) * (p / 3);
    if (disc > EPSILON) {
        double s = sqrt(disc), h = -(q / 2);
        double u = real_cbrt(h + s), v = real_cbrt(h - s);
        double real = -((u + v) / 2) - shift;
        double imag = (u - v) * sqrt(3.0) / 2;
        printf("x1 = %.6f\n", u + v - shift);
        printf("x2 = %.6f + %.6fi\n", real, imag);
        printf("x3 = %.6f - %.6fi\n", real, imag);
    } else if (disc < -EPSILON) {
        double m = sqrt(-(p / 3)) * 2;
        double theta = acos(q * 3 / (p * 2) * sqrt(-(3 / p))) / 3;
        int k;
        for (k = 0; k < 3; k++) printf("x%d = %.6f\n", k + 1, cos(theta - M_PI * 2 * k / 3) * m - shift);
    } else {
        double u = real_cbrt(-(q / 2));
        printf("x1 = %.6f\n", u * 2 - shift);
        printf("x2 = %.6f\n", -u - shift);
        printf("x3 = %.6f\n", -u - shift);
    }
}

int main(int argc, char **argv)
{
    double a, b, c, d;
    if (argc != 5) {
        puts("Syntax: Cubic A B C D   (solves A*x^3 + B*x^2 + C*x + D = 0)");
        return 1;
    }
    a = atof(argv[1]);
    b = atof(argv[2]);
    c = atof(argv[3]);
    d = atof(argv[4]);
    if (a == 0.0) {
        puts("Error: coefficient A must be non-zero for a cubic equation.");
        return 1;
    }
    printf("Solving %.6f*x^3 + %.6f*x^2 + %.6f*x + %.6f = 0\n", a, b, c, d);
    solve_cubic(a, b, c, d);
    return 0;
}
