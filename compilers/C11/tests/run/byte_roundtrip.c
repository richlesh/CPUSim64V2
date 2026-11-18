/* __stb / __ldb round trip; stores touch one byte and keep the others */
#include <stdio.h>

int main(void) {
    long w = 0;
    long base = (long)&w * 8;
    for (int i = 0; i < 8; i++) __stb(base + i, 0x10 + i);
    printf("%lx\n", w);
    __stb(base + 3, 0xAA);
    printf("%lx\n", w);
    __stb(base + 0, 0x1FF);         /* only the low 8 bits are stored */
    printf("%lx\n", w);
    __stb(base + 7, -1);
    printf("%lx\n", w);
    __stb(base + 5, 256);           /* low 8 bits are 0 */
    printf("%lx\n", w);
    for (int i = 0; i < 8; i++) printf("%d ", (int)__ldb(base + i));
    printf("\n");
    /* expressions as the address and the value */
    int k = 4;
    __stb(base + k - 4, k * 16 + 1);
    printf("%lx\n", w);
    return 0;
}
