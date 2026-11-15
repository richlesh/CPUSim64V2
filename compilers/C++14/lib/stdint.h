// stdint.h - the C library's stdint.h as the C++ translator sees it (every integer is a 64-bit word).
#ifndef _CPPC_STDINT_H
#define _CPPC_STDINT_H
#pragma cppc c_include <stdint.h>
extern "C" {
typedef long int8_t;
typedef long int16_t;
typedef long int32_t;
typedef long int64_t;
typedef unsigned long uint8_t;
typedef unsigned long uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long uint64_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef long intmax_t;
typedef unsigned long uintmax_t;
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
}
#define INT8_MIN (-128)
#define INT8_MAX 127
#define UINT8_MAX 255
#define INT16_MIN (-32768)
#define INT16_MAX 32767
#define UINT16_MAX 65535
#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647
#define UINT32_MAX 4294967295u
#define INT64_MAX 0x7fffffffffffffff
#define INT64_MIN (-INT64_MAX - 1)
#define UINT64_MAX 0xffffffffffffffffu
#define SIZE_MAX UINT64_MAX
#endif
