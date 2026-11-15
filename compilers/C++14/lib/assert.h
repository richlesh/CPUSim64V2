// assert.h - the C library's assert as the C++ translator sees it.
#pragma cppc c_include <assert.h>
extern "C" {
void __assert_fail(const char *expr, const char *file, int line);
}
#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#define assert(e) ((e) ? (void)0 : __assert_fail(#e, __FILE__, __LINE__))
#endif
