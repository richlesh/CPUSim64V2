#include <stdio.h>

/* Unsigned 64-bit division and remainder, at run time and in constant
   expressions.  Only unsigned long is used (64 bits both natively and on
   CPUSim64), and values are printed in hex. */

#define BIG 0xffffffffffffffffUL
#define TOP 0x8000000000000000UL

/* constant expressions (folded by the compiler) */
static unsigned long k_div = BIG / 3;
static unsigned long k_mod = BIG % 10;
static unsigned long k_top = BIG / TOP;
static int k_cmp1 = BIG > 1UL;
static int k_cmp2 = TOP <= 5UL;
static int k_cmp3 = -9223372036854775807L - 1 < 1;
static int k_cmp4 = 9223372036854775807L > -2;
static char k_arr[BIG / 0x7fffffffffffffffUL];
enum { K_ENUM = (int)(BIG % 1000) };

static unsigned long vals[] = {
    0, 1, 2, 3, 7, 10, 255, 1000003, 0x7fffffffffffffffUL, TOP, TOP + 1,
    0xfffffffffffffffeUL, BIG, 0x123456789abcdef0UL, 0xfedcba9876543210UL
};

int main(void) {
    int n = sizeof vals / sizeof vals[0];
    unsigned long sum = 0, x = 0;

    printf("consts %lx %lx %lx %d %d %d %d %d %d\n", k_div, k_mod, k_top,
           k_cmp1, k_cmp2, k_cmp3, k_cmp4, (int)sizeof k_arr, K_ENUM);

    /* every pair (a, b) with b != 0 */
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            unsigned long a = vals[i], b = vals[j];
            if (b == 0) continue;
            unsigned long q = a / b, r = a % b;
            if (q * b + r != a || r >= b) printf("identity broken %lx %lx\n", a, b);
            sum = sum * 31 + q;
            x = x * 37 + r;
        }
    printf("pairs %lx %lx\n", sum, x);

    unsigned long u = BIG;
    printf("max %lx %lx %lx %lx\n", u / 2, u % 10, u / TOP, u % TOP);

    /* powers of two (shift / mask) and 1 */
    printf("pow2 %lx %lx %lx %lx %lx\n", u / 8, u % 16, u / 1, u % 1, TOP / 4);

    /* compound assignment */
    unsigned long c = BIG;
    c /= 3;
    printf("divassign %lx\n", c);
    c = BIG;
    c %= 7;
    printf("modassign %lx\n", c);

    /* mixed signed / unsigned: -1 converts to BIG */
    int m = -1;
    unsigned long two = 2;
    printf("mixed %lx %lx\n", m / two, m % two);

    /* signed division is unchanged (truncates toward zero) */
    long s1 = -7, s2 = 2;
    printf("signed %ld %ld %ld %ld\n", s1 / s2, s1 % s2, -s1 / -s2, s1 / 8);
    return 0;
}
