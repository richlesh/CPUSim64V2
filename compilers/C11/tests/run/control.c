#include <stdio.h>
int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
long fact(int n) { if (n <= 1) return 1; return n * fact(n - 1); }
int gcd(int a, int b) { while (b != 0) { int t = a % b; a = b; b = t; } return a; }
int classify(int x) {
    switch (x) {
    case 0: return 100;
    case 1:
    case 2: return 200;
    case -5: return 300;
    default: return 400;
    }
}
int main(void) {
    int i, sum = 0, n = 0;
    for (i = 0; i < 10; i++) {
        if (i == 3) continue;
        if (i == 8) break;
        sum += i;
    }
    printf("sum=%d\n", sum);
    i = 0;
    do { n += 2; i++; } while (i < 5);
    printf("n=%d\n", n);
    while (n > 3) n -= 3;
    printf("n=%d\n", n);
    printf("fib(20)=%d fact(20)=%ld gcd=%d\n", fib(20), fact(20), gcd(1071, 462));
    printf("%d %d %d %d %d\n", classify(0), classify(2), classify(-5), classify(7), classify(1));
    int k = 0;
    for (int j = 0; j < 3; j++)
        for (int m = 0; m < 3; m++) {
            if (m == j) continue;
            k += j * 10 + m;
        }
    printf("k=%d\n", k);
    int x = 5, y;
    y = x++ + ++x;
    printf("x=%d y=%d\n", x, y);
    y = x-- - --x;
    printf("x=%d y=%d\n", x, y);
    x = 10; x *= 3; x -= 4; x /= 2; x %= 7; x <<= 2; x |= 1; x ^= 3; x &= 0xff; x >>= 1;
    printf("x=%d\n", x);
    i = 0;
loop:
    i++;
    if (i < 5) goto loop;
    printf("goto i=%d\n", i);
    int c = 3;
    switch (c) { case 1: c = 10; case 3: c += 1; case 4: c += 2; break; case 5: c = 0; }
    printf("c=%d\n", c);
    printf("%d\n", (1, 2, 3));
    printf("%d %d\n", 5 > 3 ? 10 : 20, 0 ? 1 : 0 ? 2 : 3);
    return 0;
}
