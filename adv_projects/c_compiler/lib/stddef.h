/*
 * stddef.h - common definitions for the CPUSim64 C compiler.
 *
 * Every scalar is one 64-bit word, so sizes (sizeof, size_t) count words.
 */
#ifndef _CPUSIM64_STDDEF_H
#define _CPUSIM64_STDDEF_H

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

#ifndef __WCHAR_T_DEFINED
#define __WCHAR_T_DEFINED
typedef int wchar_t;
#endif

typedef long ptrdiff_t;
typedef long max_align_t;

#define offsetof(type, member) ((size_t)&((type *)0)->member)

#endif
