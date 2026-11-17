/* byte access to a malloc'ed block: a pointer is a word address */
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *p = malloc(3);
    p[0] = 0; p[1] = 0; p[2] = 0;
    long addr = (long)p * 8;
    for (int i = 0; i < 24; i++) __stb(addr + i, i + 1);
    printf("%lx %lx %lx\n", (long)p[0], (long)p[1], (long)p[2]);
    p[1] = 0x7060504030201000;
    for (int i = 8; i < 16; i++) printf("%x ", (int)__ldb(addr + i));
    printf("\n");
    __stb(addr + 12, 0);
    printf("%lx\n", (long)p[1]);
    /* a negative value in the word reads back as unsigned bytes */
    p[2] = -2;
    printf("%d %d\n", (int)__ldb(addr + 16), (int)__ldb(addr + 23));
    /* a global array */
    free(p);
    return 0;
}
