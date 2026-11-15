/*
 * Cubic.cpp
 *
 * Solves A*x^3 + B*x^2 + C*x + D = 0 with Cardano's method.  The equation is
 * reduced to the depressed cubic t^3 + p*t + q = 0 with x = t - b/3 (after
 * dividing by A); the sign of the discriminant (q/2)^2 + (p/3)^3 selects
 *   > 0:  one real root and two complex conjugate roots (Cardano),
 *   < 0:  three distinct real roots (the trigonometric method),
 *   = 0:  a multiple root.
 * C++ version of documentation/projects/Cubic.asm.
 *
 * Usage:
 *     Cubic A B C D
 *
 * Author:   Richard Lesh
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

static const double EPSILON = 0.000000001;
static const double PI = 3.14159265358979323846;

// the real cube root, also of negative numbers
static double real_cbrt(double x)
{
    double r = std::pow(std::fabs(x), 1.0 / 3);
    return x < 0 ? -r : r;
}

static void solve_cubic(double a, double b, double c, double d)
{
    b /= a;
    c /= a;
    d /= a;
    double shift = b / 3;
    double p = c - b * b / 3;
    double q = b * b * b * 2 / 27 - b * c / 3 + d;
    double disc = (q / 2) * (q / 2) + (p / 3) * (p / 3) * (p / 3);
    if (disc > EPSILON) {
        double s = std::sqrt(disc), h = -(q / 2);
        double u = real_cbrt(h + s), v = real_cbrt(h - s);
        double real = -((u + v) / 2) - shift;
        double imag = (u - v) * std::sqrt(3.0) / 2;
        std::printf("x1 = %.6f\n", u + v - shift);
        std::printf("x2 = %.6f + %.6fi\n", real, imag);
        std::printf("x3 = %.6f - %.6fi\n", real, imag);
    } else if (disc < -EPSILON) {
        double m = std::sqrt(-(p / 3)) * 2;
        double theta = std::acos(q * 3 / (p * 2) * std::sqrt(-(3 / p))) / 3;
        for (int k = 0; k < 3; k++)
            std::printf("x%d = %.6f\n", k + 1, std::cos(theta - PI * 2 * k / 3) * m - shift);
    } else {
        double u = real_cbrt(-(q / 2));
        std::printf("x1 = %.6f\n", u * 2 - shift);
        std::printf("x2 = %.6f\n", -u - shift);
        std::printf("x3 = %.6f\n", -u - shift);
    }
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        std::cout << "Syntax: Cubic A B C D   (solves A*x^3 + B*x^2 + C*x + D = 0)" << std::endl;
        return 1;
    }
    double a = std::atof(argv[1]);
    double b = std::atof(argv[2]);
    double c = std::atof(argv[3]);
    double d = std::atof(argv[4]);
    if (a == 0.0) {
        std::cout << "Error: coefficient A must be non-zero for a cubic equation." << std::endl;
        return 1;
    }
    std::printf("Solving %.6f*x^3 + %.6f*x^2 + %.6f*x + %.6f = 0\n", a, b, c, d);
    solve_cubic(a, b, c, d);
    return 0;
}
