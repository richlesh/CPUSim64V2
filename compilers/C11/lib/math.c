/*
 * math.c - C implementations for lib/math.h.
 *
 * The compiler appends this file after the program and compiles only the
 * functions that are used.  Because it shares the program's macro
 * namespace, every local name starts with __m_.
 *
 * Notes on the CPUSim64 simulator:
 *  - comparisons with NaN do not behave as in IEEE 754 (NaN compares equal),
 *    so NaN and infinity are recognized from the bit pattern;
 *  - a register move between integer and double converts the value, so bit
 *    patterns are read and written through a union in memory.
 */
#include <math.h>

union __m_bits { double __m_d; unsigned long __m_u; };

static unsigned long __m_tobits(double __m_x) {
    union __m_bits __m_b;
    __m_b.__m_d = __m_x;
    return __m_b.__m_u;
}

static double __m_frombits(unsigned long __m_u) {
    union __m_bits __m_b;
    __m_b.__m_u = __m_u;
    return __m_b.__m_d;
}

#define __M_SIGN 0x8000000000000000UL
#define __M_EXP  0x7ff0000000000000UL
#define __M_MANT 0x000fffffffffffffUL

/* ---- classification */
int isnan(double __m_x) {
    unsigned long __m_u = __m_tobits(__m_x);
    return (__m_u & __M_EXP) == __M_EXP && (__m_u & __M_MANT) != 0;
}

int isinf(double __m_x) {
    unsigned long __m_u = __m_tobits(__m_x);
    return (__m_u & ~__M_SIGN) == __M_EXP;
}

int isfinite(double __m_x) {
    return (__m_tobits(__m_x) & __M_EXP) != __M_EXP;
}

int signbit(double __m_x) {
    return (__m_tobits(__m_x) & __M_SIGN) != 0;
}

double copysign(double __m_x, double __m_y) {
    unsigned long __m_u = __m_tobits(__m_x) & ~__M_SIGN;
    return __m_frombits(__m_u | (__m_tobits(__m_y) & __M_SIGN));
}

/* ---- rounding */
double trunc(double __m_x) {
    if (!isfinite(__m_x)) return __m_x;
    /* |x| >= 2^52 is already an integer (and may not fit a long) */
    if (fabs(__m_x) >= 4503599627370496.0) return __m_x;
    if (__m_x == 0.0) return __m_x;
    double __m_r = (double)(long)__m_x;
    if (__m_r == 0.0) return copysign(0.0, __m_x);
    return __m_r;
}

double rint(double __m_x) {
    if (!isfinite(__m_x)) return __m_x;
    if (fabs(__m_x) >= 4503599627370496.0) return __m_x;
    double __m_f = floor(__m_x);
    double __m_d = __m_x - __m_f;
    double __m_r;
    if (__m_d > 0.5) __m_r = __m_f + 1.0;
    else if (__m_d < 0.5) __m_r = __m_f;
    else if (fmod(__m_f, 2.0) == 0.0) __m_r = __m_f;   /* tie: to even */
    else __m_r = __m_f + 1.0;
    if (__m_r == 0.0) return copysign(0.0, __m_x);
    return __m_r;
}

double nearbyint(double __m_x) { return rint(__m_x); }

long lround(double __m_x) { return (long)round(__m_x); }
long long llround(double __m_x) { return (long long)round(__m_x); }
long lrint(double __m_x) { return (long)rint(__m_x); }

double fmod(double __m_x, double __m_y) {
    if (isnan(__m_x) || isnan(__m_y) || isinf(__m_x) || __m_y == 0.0)
        return NAN;
    if (isinf(__m_y)) return __m_x;
    /* the IEEE remainder is exact; adjust it to have the sign of x */
    double __m_ay = fabs(__m_y);
    double __m_r = remainder(fabs(__m_x), __m_ay);
    if (__m_r < 0.0) __m_r = __m_r + __m_ay;
    return copysign(__m_r, __m_x);
}

double modf(double __m_x, double *__m_ip) {
    double __m_i = trunc(__m_x);
    *__m_ip = __m_i;
    if (isinf(__m_x)) return copysign(0.0, __m_x);
    return copysign(__m_x - __m_i, __m_x);
}

/* ---- exponent manipulation */
double ldexp(double __m_x, int __m_e) {
    if (__m_x == 0.0 || !isfinite(__m_x)) return __m_x;
    /* apply the scale in steps that each stay representable */
    while (__m_e > 1000) { __m_x = __m_x * 1.0715086071862673e301; __m_e = __m_e - 1000; }
    while (__m_e < -1000) { __m_x = __m_x * 9.332636185032189e-302; __m_e = __m_e + 1000; }
    return __m_x * pow(2.0, (double)__m_e);
}

double scalbn(double __m_x, int __m_n) { return ldexp(__m_x, __m_n); }

double frexp(double __m_x, int *__m_ep) {
    if (__m_x == 0.0 || !isfinite(__m_x)) { *__m_ep = 0; return __m_x; }
    int __m_adj = 0;
    unsigned long __m_u = __m_tobits(__m_x);
    if ((__m_u & __M_EXP) == 0) {
        /* subnormal: scale by 2^54 first */
        __m_x = __m_x * 18014398509481984.0;
        __m_adj = -54;
        __m_u = __m_tobits(__m_x);
    }
    int __m_e = (int)((__m_u >> 52) & 0x7ff) - 1022;
    *__m_ep = __m_e + __m_adj;
    /* replace the exponent so that 0.5 <= |m| < 1 */
    __m_u = (__m_u & ~__M_EXP) | 0x3fe0000000000000UL;
    return __m_frombits(__m_u);
}

/* ---- exponential and logarithmic */
double exp2(double __m_x) { return pow(2.0, __m_x); }

double expm1(double __m_x) {
    if (fabs(__m_x) < 1e-5)
        return __m_x + __m_x * __m_x / 2.0 + __m_x * __m_x * __m_x / 6.0;
    return exp(__m_x) - 1.0;
}

double log1p(double __m_x) {
    if (fabs(__m_x) < 1e-4) {
        double __m_x2 = __m_x * __m_x;
        return __m_x - __m_x2 / 2.0 + __m_x2 * __m_x / 3.0 - __m_x2 * __m_x2 / 4.0;
    }
    return log(1.0 + __m_x);
}

double log10(double __m_x) {
    double __m_r = log(__m_x) / 2.302585092994045684;
    /* make exact powers of ten exact */
    double __m_n = round(__m_r);
    if (fabs(__m_r - __m_n) < 1e-9 && pow(10.0, __m_n) == __m_x) return __m_n;
    return __m_r;
}

double log2(double __m_x) {
    double __m_r = log(__m_x) / 0.6931471805599453094;
    double __m_n = round(__m_r);
    if (fabs(__m_r - __m_n) < 1e-9 && pow(2.0, __m_n) == __m_x) return __m_n;
    return __m_r;
}

double cbrt(double __m_x) {
    if (__m_x == 0.0 || !isfinite(__m_x)) return __m_x;
    double __m_a = fabs(__m_x);
    double __m_r = pow(__m_a, 1.0 / 3.0);
    /* one Newton step: r = r - (r^3 - a) / (3 r^2) */
    __m_r = __m_r - (__m_r * __m_r * __m_r - __m_a) / (3.0 * __m_r * __m_r);
    return __m_x < 0.0 ? -__m_r : __m_r;
}

double hypot(double __m_x, double __m_y) {
    if (isinf(__m_x) || isinf(__m_y)) return INFINITY;
    double __m_a = fabs(__m_x);
    double __m_b = fabs(__m_y);
    if (__m_a < __m_b) { double __m_t = __m_a; __m_a = __m_b; __m_b = __m_t; }
    if (__m_a == 0.0) return 0.0;
    /* scale to avoid overflow / underflow of the squares */
    double __m_q = __m_b / __m_a;
    return __m_a * sqrt(1.0 + __m_q * __m_q);
}

/* ---- minimum, maximum, difference, fused multiply-add */
double fmin(double __m_x, double __m_y) {
    if (isnan(__m_x)) return __m_y;
    if (isnan(__m_y)) return __m_x;
    return __m_x < __m_y ? __m_x : __m_y;
}

double fmax(double __m_x, double __m_y) {
    if (isnan(__m_x)) return __m_y;
    if (isnan(__m_y)) return __m_x;
    return __m_x > __m_y ? __m_x : __m_y;
}

double fdim(double __m_x, double __m_y) {
    if (isnan(__m_x) || isnan(__m_y)) return NAN;
    return __m_x > __m_y ? __m_x - __m_y : 0.0;
}

double fma(double __m_x, double __m_y, double __m_z) {
    return __m_x * __m_y + __m_z;
}
