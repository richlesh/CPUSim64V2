#ifndef _CPPC_MATH_H
#define _CPPC_MATH_H
#pragma cppc c_include <math.h>
#define M_PI 3.14159265358979323846
#define M_E 2.7182818284590452354
#define HUGE_VAL (1.0 / 0.0)
#define INFINITY (1.0 / 0.0)
#define NAN (0.0 / 0.0)
extern "C" {
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);
double asinh(double x);
double acosh(double x);
double atanh(double x);
double exp(double x);
double log(double x);
double pow(double x, double y);
double sqrt(double x);
double exp2(double x);
double expm1(double x);
double log10(double x);
double log2(double x);
double log1p(double x);
double cbrt(double x);
double hypot(double x, double y);
double fabs(double x);
double floor(double x);
double ceil(double x);
double round(double x);
double remainder(double x, double y);
double trunc(double x);
double rint(double x);
double nearbyint(double x);
long lround(double x);
long lrint(double x);
double fmod(double x, double y);
double modf(double x, double *ip);
double frexp(double x, int *e);
double ldexp(double x, int e);
double scalbn(double x, int e);
double copysign(double x, double y);
double fmin(double x, double y);
double fmax(double x, double y);
double fdim(double x, double y);
double fma(double x, double y, double z);
int isnan(double x);
int isinf(double x);
int isfinite(double x);
int signbit(double x);
}
#endif
