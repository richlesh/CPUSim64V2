/* copying a packed buffer with __ldb / __stb */
#include <stdio.h>
#include <stdlib.h>

static void bcopy(long dst, long src, int n) {
    for (int i = 0; i < n; i++) __stb(dst + i, __ldb(src + i));
}

int main(void) {
    const char *msg = "Hello, byte world!";
    int n = 18;
    long *src = malloc(4);
    long *dst = malloc(4);
    long sb = (long)src * 8, db = (long)dst * 8;
    for (int i = 0; i < 4; i++) { src[i] = 0; dst[i] = 0; }
    /* pack the characters one per byte */
    for (int i = 0; i < n; i++) __stb(sb + i, msg[i]);
    bcopy(db + 1, sb, n);           /* misaligned copy */
    for (int i = 0; i < n; i++)
        putchar((int)__ldb(db + 1 + i));
    putchar('\n');
    printf("%d %d\n", (int)__ldb(db), (int)__ldb(db + 1 + n));
    long sum = 0;
    for (int i = 0; i < 32; i++) sum += __ldb(db + i);
    printf("%ld\n", sum);
    free(src);
    free(dst);
    return 0;
}
