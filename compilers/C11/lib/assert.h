/*
 * assert.h - diagnostics for the CPUSim64 C compiler.
 *
 * As in C, this header has no include guard: assert is redefined each time
 * it is included, according to whether NDEBUG is defined at that point.
 * A failed assertion prints "file:line: Assertion `expr' failed." to stderr
 * and ends the program with result 134.  __assert_fail is provided by the
 * assembly runtime.
 */
void __assert_fail(__pstr expr, __pstr file, int line);

#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#define assert(e) ((e) ? (void)0 : __assert_fail(#e, __FILE__, __LINE__))
#endif

#ifndef static_assert
#define static_assert _Static_assert
#endif
