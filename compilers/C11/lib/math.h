/*
 * math.h - C99 double-precision math library for the CPUSim64 C compiler.
 *
 * Functions marked [rt] are thin runtime shims over CPUSim64 math
 * interrupts; all others are implemented in C in lib/math.c, which the
 * compiler appends to the translation unit and compiles only the functions
 * a program actually uses.
 *
 * All values are 64-bit words: double is IEEE 754 binary64 and long is a
 * 64-bit signed integer.
 */
#ifndef _CPUSIM64_MATH_H
#define _CPUSIM64_MATH_H

#define M_E         2.7182818284590452354
#define M_LOG2E     1.4426950408889634074
#define M_LOG10E    0.43429448190325182765
#define M_LN2       0.69314718055994530942
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440

#define INFINITY    (1.0 / 0.0)
#define NAN         (0.0 / 0.0)
#define HUGE_VAL    INFINITY

/* ---- trigonometric and hyperbolic [rt] */
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

/* ---- exponential, logarithmic and power */
double exp(double x);                   /* [rt] */
double log(double x);                   /* [rt] */
double pow(double x, double y);         /* [rt] */
double sqrt(double x);                  /* [rt] */
double exp2(double x);
double expm1(double x);
double log10(double x);
double log2(double x);
double log1p(double x);
double cbrt(double x);
double hypot(double x, double y);

/* ---- rounding and remainders */
double fabs(double x);                  /* [rt] */
double floor(double x);                 /* [rt] */
double ceil(double x);                  /* [rt] */
double round(double x);                 /* [rt] halfway cases away from 0 */
double remainder(double x, double y);   /* [rt] IEEE remainder */
double trunc(double x);
double rint(double x);                  /* halfway cases to even */
double nearbyint(double x);
long lround(double x);
long long llround(double x);
long lrint(double x);
double fmod(double x, double y);
double modf(double x, double *iptr);

/* ---- floating-point manipulation */
double frexp(double x, int *exp);
double ldexp(double x, int exp);
double scalbn(double x, int n);
double copysign(double x, double y);

/* ---- minimum, maximum, difference, fused multiply-add */
double fmin(double x, double y);
double fmax(double x, double y);
double fdim(double x, double y);
double fma(double x, double y, double z);

/* ---- classification (functions here; macros in C99) */
int isnan(double x);
int isinf(double x);
int isfinite(double x);
int signbit(double x);

#endif
