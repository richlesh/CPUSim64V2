// float.h - the C library's float.h (float is double on CPUSim64).
#ifndef _CPPC_FLOAT_H
#define _CPPC_FLOAT_H
#pragma cppc c_include <float.h>
#define FLT_RADIX 2
#define DECIMAL_DIG 17
#define DBL_MANT_DIG 53
#define DBL_DIG 15
#define DBL_MIN_EXP (-1021)
#define DBL_MIN_10_EXP (-307)
#define DBL_MAX_EXP 1024
#define DBL_MAX_10_EXP 308
#define DBL_MAX 1.7976931348623157e308
#define DBL_MIN 2.2250738585072014e-308
#define DBL_EPSILON 2.220446049250313e-16
#define FLT_MANT_DIG DBL_MANT_DIG
#define FLT_DIG DBL_DIG
#define FLT_MAX DBL_MAX
#define FLT_MIN DBL_MIN
#define FLT_EPSILON DBL_EPSILON
#endif
