/*
 * stdarg.h - variable arguments for the CPUSim64 C compiler.
 *
 * Arguments are pushed right to left, so the callee finds them at increasing
 * addresses after the last named parameter.  va_arg advances by sizeof(T)
 * words: 1 for every scalar, the size in words for a struct.
 */
#ifndef _CPUSIM64_STDARG_H
#define _CPUSIM64_STDARG_H

typedef char *va_list;

#define va_start(ap, last)  ((ap) = (char *)(&(last) + 1))
#define va_arg(ap, type)    (*(type *)(((ap) = (ap) + sizeof(type)) - sizeof(type)))
#define va_end(ap)          ((void)0)
#define va_copy(dst, src)   ((dst) = (src))

#endif
