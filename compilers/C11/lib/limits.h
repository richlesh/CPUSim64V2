/*
 * limits.h - sizes of integer types for the CPUSim64 C compiler.
 *
 * Every integer type is one 64-bit word: char holds a Unicode codepoint
 * (CHAR_BIT is 64) and short, int, long and long long are all 64 bits.
 * The signed char, unsigned char and short limits are the nominal C values;
 * variables of those types are still 64-bit words and do not wrap.
 */
#ifndef _CPUSIM64_LIMITS_H
#define _CPUSIM64_LIMITS_H

#define CHAR_BIT    64
#define CHAR_MIN    0
#define CHAR_MAX    1114111
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define SHRT_MIN    (-32767-1)
#define SHRT_MAX    32767
#define USHRT_MAX   65535
#define INT_MAX     0x7fffffffffffffff
#define INT_MIN     (-INT_MAX-1)
#define UINT_MAX    0xffffffffffffffffU
#define LONG_MAX    INT_MAX
#define LONG_MIN    INT_MIN
#define ULONG_MAX   UINT_MAX
#define LLONG_MAX   INT_MAX
#define LLONG_MIN   INT_MIN
#define ULLONG_MAX  UINT_MAX
#define MB_LEN_MAX  4

#endif
