#include <cstdio>

int main()
{
    int a = 17, b = 5;
    printf("%d %d %d %d %d\n", a + b, a - b, a * b, a / b, a % b);
    printf("%d %d %d\n", -a / b, -a % b, a / -b);
    unsigned u = 7, v = 3;
    printf("%u %u %u\n", u * v, u / v, u % v);
    long big = 1000000L * 1000000L;
    printf("%ld\n", big);
    double d = 7.0 / 2;
    printf("%.2f %.2f %d\n", d, 2.5 * 2, (int)d);
    bool t = a > b, f = a < b;
    printf("%d %d %d %d\n", t, f, t && f, t || f);
    int x = 5;
    printf("%d ", x++);
    printf("%d ", x);
    printf("%d ", ++x);
    printf("%d\n", x--);
    x += 10; x -= 3; x *= 2; x /= 3; x %= 5;
    printf("%d\n", x);
    int bits = 0xF0;
    printf("%d %d %d %d %d %d\n", bits & 0x3C, bits | 0x0F, bits ^ 0xFF, ~bits & 0xFF, bits << 2, bits >> 3);
    char c = 'a';
    c += 2;
    printf("%c %d\n", c, c - 'a');
    printf("%d\n", a > b ? a : b);
    printf("%d %d\n", (a, b), (a < b, a));
    double e = 1e3, g = 2.5e-1;
    printf("%.3f %.3f\n", e, g);
    int i = 3;
    i = i * 2 + 1;
    printf("%d %d\n", i, i << 1 >> 1);
    short s = 100;
    s = s * 2;
    printf("%d\n", s);
    printf("%d %d %d\n", 10 == 10, 3 != 3, !0);
    return 0;
}
