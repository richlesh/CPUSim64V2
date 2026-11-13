/*
 * stdbool.h - boolean type for the CPUSim64 C compiler.
 *
 * bool, true and false are also keywords in this compiler (as in C23), so
 * they work without this header; the macros below give the C99 definitions.
 */
#ifndef _CPUSIM64_STDBOOL_H
#define _CPUSIM64_STDBOOL_H

#define bool    _Bool
#define true    1
#define false   0
#define __bool_true_false_are_defined 1

#endif
