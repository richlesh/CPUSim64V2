#include <stdio.h>
#include <math.h>
#include <stdlib.h>
double square(double x) { return x * x; }
double avg(int n, double *v) { double s = 0; for (int i = 0; i < n; i++) s += v[i]; return s / n; }
double mix(int a, double b, int c) { return a + b * c; }
double gd = 2.5;
double garr[3] = { 1.0, 0.5, -3 };
int main(void) {
    double a = 1.5, b = -2.25;
    printf("%f %f %f %f\n", a + b, a - b, a * b, a / b);
    printf("%.3f %.1f %g\n", square(a), sqrt(2.0) * 10, 0.5);
    double v[4] = { 1, 2, 3, 4.5 };
    printf("avg=%.4f\n", avg(4, v));
    int i = 7;
    double d = i / 2;
    double e = i / 2.0;
    int t = (int)(e * 3);
    printf("%f %f %d %d\n", d, e, t, (int)-3.9);
    printf("%d %d %d %d\n", a < b, a > b, a == 1.5, b != b);
    printf("mix=%f\n", mix(2, 0.5, 3));
    printf("%.4f %.4f %.4f\n", sin(M_PI / 2), cos(0.0), pow(2.0, 10.0));
    printf("%.2f %.2f %.2f %.2f\n", floor(-2.5), ceil(2.1), fabs(-3.25), fmod(7.5, 2.0));
    gd *= 2; garr[1] += gd;
    printf("%f %f %f\n", gd, garr[1], garr[2]);
    double x = 1.0;
    x++; ++x; x--;
    x += 1; x -= 0.5; x *= 4; x /= 2;
    printf("x=%f\n", x);
    double r1, r2, qa = 1, qb = -3, qc = 2;
    double disc = qb * qb - 4 * qa * qc;
    r1 = (-qb + sqrt(disc)) / (2 * qa);
    r2 = (-qb - sqrt(disc)) / (2 * qa);
    printf("roots %.2f %.2f\n", r1, r2);
    double sum = 0; int steps = 1000;
    for (int k = 0; k < steps; k++) { double xm = (k + 0.5) / steps; sum += 4.0 / (1 + xm * xm); }
    printf("pi~%.6f\n", sum / steps);
    printf("%.3f %d\n", atof("3.25e2"), !0.0);
    double big = 1e300; big = big * 1e10;
    printf("%d %f\n", big > 1e308, -0.0);
    return 0;
}
