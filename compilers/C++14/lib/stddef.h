// stddef.h - the C library's stddef.h as the C++ translator sees it.
// Declarations inside extern "C" in the library are not written to the generated C:
// the generated program includes the C11 compiler's own header instead.
#ifndef _CPPC_STDDEF_H
#define _CPPC_STDDEF_H
#pragma cppc c_include <stddef.h>
#define NULL 0
extern "C" {
typedef unsigned long size_t;
typedef long ptrdiff_t;
}
#endif
