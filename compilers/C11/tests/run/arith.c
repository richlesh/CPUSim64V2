#include <stdio.h>
int main(void) {
    int a = 17, b = 5;
    printf("%d %d %d %d %d\n", a + b, a - b, a * b, a / b, a % b);
    printf("%d %d %d\n", -a / b, -a % b, a / -b);
    printf("%d %d %d %d %d\n", a & b, a | b, a ^ b, a << 3, a >> 2);
    printf("%d %d %d\n", ~a, -a >> 1, !a);
    printf("%d %d %d %d %d %d\n", a < b, a > b, a <= 17, a >= 18, a == 17, a != 17);
    printf("%d %d %d %d\n", a && b, a && 0, 0 || b, 0 || 0);
    printf("%d\n", 2 + 3 * 4 - (10 - 4) / 3);
    unsigned u = -1;
    printf("%d %d\n", u > 5, (int)(u >> 60));
    long big = 0x7fffffffffffffff;
    long small = -big - 1;
    printf("%d %d %d\n", small < 5, big > -5, small < big);
    printf("%x %d\n", 0x123456789abcdefL, 1000000000000000L);
    return 2 + 3 * 4;
}
