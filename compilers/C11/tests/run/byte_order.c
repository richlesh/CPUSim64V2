/* __ldb: byte order within a word (index 0 = most significant byte) */
#include <stdio.h>

int main(void) {
    long a[2];
    a[0] = 0x0102030405060708;
    a[1] = 0xFFEEDDCCBBAA9988;      /* high bit set: bytes must not sign-extend */
    long base = (long)a * 8;        /* byte address of a[0] */
    for (int w = 0; w < 2; w++) {
        for (int i = 0; i < 8; i++)
            printf("%02x ", (int)__ldb(base + w * 8 + i));
        printf("\n");
    }
    /* the result is an int in 0..255 */
    int b = __ldb(base + 8);
    printf("%d %d\n", b, b == 0xFF);
    printf("%d\n", (int)__ldb(base + 15) + 1);
    return 0;
}
