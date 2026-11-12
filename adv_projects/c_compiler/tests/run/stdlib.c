#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
int sum_ints(int n, ...) {
    va_list ap; int s = 0;
    va_start(ap, n);
    for (int i = 0; i < n; i++) s += va_arg(ap, int);
    va_end(ap);
    return s;
}
double avg_d(int n, ...) {
    va_list ap; double s = 0;
    va_start(ap, n);
    for (int i = 0; i < n; i++) s += va_arg(ap, double);
    va_end(ap);
    return s / n;
}
int depth(int n) { return n == 0 ? 0 : 1 + depth(n - 1); }
struct S { int a; double b; char c[3]; };
int main(void) {
    int *a = calloc(10, sizeof(int));
    int zeros = 0;
    for (int i = 0; i < 10; i++) zeros += a[i] == 0;
    a = realloc(a, 20 * sizeof(int));
    a[19] = 5;
    printf("calloc zeros=%d a[19]=%d\n", zeros, a[19]);
    free(a);
    char buf[16];
    memset(buf, 'x', 5); buf[5] = 0;
    char cpy[16];
    memcpy(cpy, buf, 6);
    memmove(cpy + 1, cpy, 4);
    printf("%s %s\n", buf, cpy);
    printf("%d %d %d\n", abs(-5), toupper('a'), tolower('Q'));
    printf("%ld %.2f %.2f\n", strtol("-0x1A", 0, 0), strtod("2.5e1xyz", 0), atof("  -0.125"));
    printf("sum=%d avg=%.2f\n", sum_ints(4, 1, 2, 3, 4), avg_d(3, 1.0, 2.0, 4.5));
    int r = rand();
    printf("rand ok=%d\n", r >= 0 && r <= RAND_MAX);
    assert(1 + 1 == 2);
    printf("depth=%d\n", depth(20000));
    printf("%d %d\n", (int)offsetof(struct S, c), (int)sizeof(struct S));
    printf("%.3f %.3f %.3f\n", log10(1000.0), hypot(3, 4), exp(1.0));
    char text[] = "Hello World";
    int up = 0;
    for (char *p = text; *p; p++) if (isupper(*p)) up++;
    printf("upper=%d\n", up);
    printf("%s\n", strrchr("a/b/c", '/'));
    int k = INT_MAX;
    printf("%d\n", k == 0x7fffffffffffffff);
    exit(7);
}
