#include <stdio.h>
#include <math.h>

/* Prints results with fixed precision so the output can be compared with a
   native build (cc -lm). */
static void p(const char *name, double v) { printf("%-10s %.6f\n", name, v); }
static void pi(const char *name, long v) { printf("%-10s %ld\n", name, v); }

int main(void) {
    /* trigonometric and hyperbolic */
    p("sin", sin(0.5));
    p("cos", cos(0.5));
    p("tan", tan(0.5));
    p("asin", asin(0.5));
    p("acos", acos(0.5));
    p("atan", atan(0.5));
    p("atan2", atan2(1.0, -2.0));
    p("atan2b", atan2(-3.0, 4.0));
    p("sinh", sinh(0.75));
    p("cosh", cosh(0.75));
    p("tanh", tanh(0.75));
    p("asinh", asinh(1.5));
    p("acosh", acosh(1.5));
    p("atanh", atanh(0.5));

    /* exponential, logarithmic and power */
    p("exp", exp(1.25));
    p("log", log(10.0));
    p("pow", pow(2.0, 0.5));
    p("sqrt", sqrt(2.0));
    p("exp2", exp2(3.5));
    p("expm1", expm1(0.5));
    p("expm1s", expm1(1e-7) * 1e7);
    p("log10", log10(1000.0));
    p("log10b", log10(2.0));
    p("log2", log2(8.0));
    p("log2b", log2(10.0));
    p("log1p", log1p(0.5));
    p("log1ps", log1p(1e-7) * 1e7);
    p("cbrt", cbrt(27.0));
    p("cbrtn", cbrt(-2.0));
    p("hypot", hypot(3.0, 4.0));
    p("hypotb", hypot(1e200, 1e200) / 1e200);

    /* rounding and remainders */
    p("fabs", fabs(-2.5));
    p("floor", floor(-2.5));
    p("ceil", ceil(-2.5));
    p("round", round(2.5));
    p("roundn", round(-2.5));
    p("trunc", trunc(-2.7));
    p("truncb", trunc(1e300) / 1e300);
    p("rint", rint(2.5));
    p("rintb", rint(3.5));
    p("rintc", rint(-2.5));
    p("rintd", rint(2.6));
    p("nearby", nearbyint(1.5));
    pi("lround", lround(2.5));
    pi("lroundn", lround(-2.5));
    pi("llround", (long)llround(7.49));
    pi("lrint", lrint(2.5));
    p("fmod", fmod(7.5, 2.0));
    p("fmodn", fmod(-7.5, 2.0));
    p("fmodb", fmod(10.0, 3.0));
    p("fmodc", fmod(5.0, -3.0));
    p("remainder", remainder(7.5, 2.0));
    p("remndrb", remainder(10.0, 3.0));
    double ip;
    double fr = modf(-3.25, &ip);
    p("modf", fr);
    p("modf_i", ip);

    /* floating-point manipulation */
    int e;
    double m = frexp(48.0, &e);
    p("frexp", m);
    pi("frexp_e", e);
    m = frexp(-0.15625, &e);
    p("frexpn", m);
    pi("frexpn_e", e);
    p("ldexp", ldexp(0.75, 6));
    p("ldexpb", ldexp(1.0, 1020) / ldexp(1.0, 1019));
    p("ldexpc", ldexp(ldexp(3.0, 1010), -1010));
    p("ldexpd", ldexp(ldexp(3.0, -1010), 1010));
    m = frexp(ldexp(0.6875, 37), &e);
    p("roundtrip", m);
    pi("rt_e", e);
    p("scalbn", scalbn(3.0, -2));
    p("copysign", copysign(3.0, -0.5));
    p("copysignb", copysign(-3.0, 2.0));

    /* min, max, difference, fma */
    p("fmin", fmin(2.0, -1.0));
    p("fmax", fmax(2.0, -1.0));
    p("fminnan", fmin(NAN, 4.0));
    p("fmaxnan", fmax(4.0, NAN));
    p("fdim", fdim(5.0, 3.0));
    p("fdimb", fdim(3.0, 5.0));
    p("fma", fma(2.0, 3.0, 4.0));

    /* classification */
    double inf = INFINITY, nan = NAN;
    pi("isnan", isnan(nan));
    pi("isnan1", isnan(1.0));
    pi("isnanI", isnan(inf));
    pi("isinf", isinf(inf));
    pi("isinfn", isinf(-inf));
    pi("isinf1", isinf(1.0));
    pi("isinfN", isinf(nan));
    pi("isfin", isfinite(1.0));
    pi("isfinI", isfinite(inf));
    pi("isfinN", isfinite(nan));
    pi("signbit", signbit(-2.0) != 0);
    pi("signbit0", signbit(-0.0) != 0);
    pi("signbitp", signbit(2.0) != 0);
    pi("huge", HUGE_VAL == inf);
    pi("fmodnan", isnan(fmod(1.0, 0.0)));

    /* constants */
    p("M_PI", M_PI);
    p("M_E", M_E);
    p("M_SQRT2", M_SQRT2);
    p("M_SQRT1_2", M_SQRT1_2);
    p("M_LN2", M_LN2);
    p("M_LN10", M_LN10);
    p("M_LOG2E", M_LOG2E);
    p("M_LOG10E", M_LOG10E);
    p("M_PI_2", M_PI_2);
    p("M_PI_4", M_PI_4);
    p("M_1_PI", M_1_PI);
    p("M_2_PI", M_2_PI);
    return 0;
}
