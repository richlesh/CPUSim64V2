#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>
#define NDEBUG
#include <assert.h>
#include <stdio.h>

struct S { int a; int b; };

int vsum(int n, ...) {
    va_list ap, cp;
    int s = 0;
    va_start(ap, n);
    va_copy(cp, ap);
    for (int i = 0; i < n; i++) s += va_arg(ap, int);
    va_end(ap);
    for (int i = 0; i < n; i++) s += va_arg(cp, int);
    va_end(cp);
    return s;
}

int main(void) {
    bool b = true;
    size_t n = sizeof(struct S);
    uint64_t u = UINT64_MAX;
    int32_t i = INT32_MIN;
    assert(0);   /* disabled by NDEBUG */
    printf("%d %d %d\n", b, (int)n, (int)offsetof(struct S, b));
    printf("%d %d\n", NULL == (void *)0, (int)(u == ULONG_MAX));
    printf("%ld %d %d\n", (long)i, INT_MAX == LONG_MAX, CHAR_BIT);
    printf("%d %d\n", DBL_DIG, DBL_MANT_DIG);
    printf("%d %d %d\n", isalpha('x'), isdigit('x'), toupper('x') == 'X');
    printf("%d\n", vsum(3, 1, 2, 3));
    return 0;
}
