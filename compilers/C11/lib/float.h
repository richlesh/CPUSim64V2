/*
 * float.h - floating-point limits for the CPUSim64 C compiler.
 *
 * double is IEEE 754 binary64; float and long double are not supported.
 */
#ifndef _CPUSIM64_FLOAT_H
#define _CPUSIM64_FLOAT_H

#define FLT_RADIX       2
#define FLT_ROUNDS      1
#define FLT_EVAL_METHOD 0
#define DECIMAL_DIG     17
#define DBL_MANT_DIG    53
#define DBL_DIG         15
#define DBL_DECIMAL_DIG 17
#define DBL_MIN_EXP     (-1021)
#define DBL_MIN_10_EXP  (-307)
#define DBL_MAX_EXP     1024
#define DBL_MAX_10_EXP  308
#define DBL_MAX         1.7976931348623157e308
#define DBL_MIN         2.2250738585072014e-308
#define DBL_EPSILON     2.220446049250313e-16
#define DBL_TRUE_MIN    4.9406564584124654e-324

#endif
