/*
 * stdint.h - integer types for the CPUSim64 C compiler.
 *
 * Every integer is one 64-bit word.  The "exact-width" names int8_t ...
 * uint32_t are provided for portability but are 64-bit types: their *_MAX /
 * *_MIN macros have the nominal C values, yet a variable does not wrap at
 * 8, 16 or 32 bits.  The least, fast, pointer and max types are 64-bit and
 * their limits are the real 64-bit limits.
 */
#ifndef _CPUSIM64_STDINT_H
#define _CPUSIM64_STDINT_H

typedef long int8_t;
typedef long int16_t;
typedef long int32_t;
typedef long int64_t;
typedef unsigned long uint8_t;
typedef unsigned long uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long uint64_t;

typedef long int_least8_t;
typedef long int_least16_t;
typedef long int_least32_t;
typedef long int_least64_t;
typedef unsigned long uint_least8_t;
typedef unsigned long uint_least16_t;
typedef unsigned long uint_least32_t;
typedef unsigned long uint_least64_t;

typedef long int_fast8_t;
typedef long int_fast16_t;
typedef long int_fast32_t;
typedef long int_fast64_t;
typedef unsigned long uint_fast8_t;
typedef unsigned long uint_fast16_t;
typedef unsigned long uint_fast32_t;
typedef unsigned long uint_fast64_t;

typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef long intmax_t;
typedef unsigned long uintmax_t;

#define __STDINT_MAX    0x7fffffffffffffff
#define __STDINT_MIN    (-__STDINT_MAX-1)
#define __STDINT_UMAX   0xffffffffffffffffU

#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255
#define INT16_MIN   (-32767-1)
#define INT16_MAX   32767
#define UINT16_MAX  65535
#define INT32_MIN   (-2147483647-1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295U
#define INT64_MIN   __STDINT_MIN
#define INT64_MAX   __STDINT_MAX
#define UINT64_MAX  __STDINT_UMAX

#define INT_LEAST8_MIN   __STDINT_MIN
#define INT_LEAST8_MAX   __STDINT_MAX
#define UINT_LEAST8_MAX  __STDINT_UMAX
#define INT_LEAST16_MIN  __STDINT_MIN
#define INT_LEAST16_MAX  __STDINT_MAX
#define UINT_LEAST16_MAX __STDINT_UMAX
#define INT_LEAST32_MIN  __STDINT_MIN
#define INT_LEAST32_MAX  __STDINT_MAX
#define UINT_LEAST32_MAX __STDINT_UMAX
#define INT_LEAST64_MIN  __STDINT_MIN
#define INT_LEAST64_MAX  __STDINT_MAX
#define UINT_LEAST64_MAX __STDINT_UMAX

#define INT_FAST8_MIN    __STDINT_MIN
#define INT_FAST8_MAX    __STDINT_MAX
#define UINT_FAST8_MAX   __STDINT_UMAX
#define INT_FAST16_MIN   __STDINT_MIN
#define INT_FAST16_MAX   __STDINT_MAX
#define UINT_FAST16_MAX  __STDINT_UMAX
#define INT_FAST32_MIN   __STDINT_MIN
#define INT_FAST32_MAX   __STDINT_MAX
#define UINT_FAST32_MAX  __STDINT_UMAX
#define INT_FAST64_MIN   __STDINT_MIN
#define INT_FAST64_MAX   __STDINT_MAX
#define UINT_FAST64_MAX  __STDINT_UMAX

#define INTPTR_MIN   __STDINT_MIN
#define INTPTR_MAX   __STDINT_MAX
#define UINTPTR_MAX  __STDINT_UMAX
#define INTMAX_MIN   __STDINT_MIN
#define INTMAX_MAX   __STDINT_MAX
#define UINTMAX_MAX  __STDINT_UMAX
#define PTRDIFF_MIN  __STDINT_MIN
#define PTRDIFF_MAX  __STDINT_MAX
#define SIZE_MAX     __STDINT_UMAX
#define WCHAR_MIN    0
#define WCHAR_MAX    1114111

#define INT8_C(v)    (v)
#define INT16_C(v)   (v)
#define INT32_C(v)   (v)
#define INT64_C(v)   (v ## L)
#define UINT8_C(v)   (v)
#define UINT16_C(v)  (v)
#define UINT32_C(v)  (v ## U)
#define UINT64_C(v)  (v ## UL)
#define INTMAX_C(v)  (v ## L)
#define UINTMAX_C(v) (v ## UL)

#endif
