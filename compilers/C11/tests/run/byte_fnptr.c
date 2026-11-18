/* the intrinsics also work through function pointers and as arguments */
#include <stdio.h>

long g[2] = {0x4142434445464748, 0x494A4B4C4D4E4F50};

static long twice(long (*f)(long), long a) { return f(a) * 2; }

int main(void) {
    long (*load)(long) = __ldb;
    void (*store)(long, long) = __stb;
    long base = (long)g * 8;
    printf("%lx\n", twice(load, base + 1));
    store(base + 9, 0x7A);
    printf("%lx\n", g[1]);
    printf("%lx\n", (long)__ldb(base + __ldb(base + 7) - 0x40));
    return 0;
}
