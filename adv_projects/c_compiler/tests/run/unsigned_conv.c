#include <stdio.h>
#include <string.h>

/* Unsigned <-> double conversion and printf("%u") for full 64-bit values.
   Only unsigned long is used (64 bits both natively and on CPUSim64). */

#define BIG 0xffffffffffffffffUL
#define TOP 0x8000000000000000UL

static double k_big = BIG;                  /* folded by the compiler */
static double k_top = TOP;
static unsigned long k_back = (unsigned long)1.5e19;
static unsigned long k_small = (unsigned long)12345.9;

static double to_d(unsigned long u) { return u; }
static unsigned long to_u(double d) { return d; }

int main(void) {
    /* conversion at run time */
    /* exact comparisons: the CPUSim64 formatter prints large doubles with
       only 17 significant digits */
    printf("u2d %d %d %d %.1f %e\n", to_d(BIG) == 18446744073709551616.0,
           to_d(TOP) == 9223372036854775808.0, to_d(TOP + 1) == 9223372036854775808.0,
           to_d(12345), to_d(BIG));
    printf("u2d rnd %d %d\n", to_d(0xfffffffffffff801UL) == 18446744073709549568.0,
           to_d(0x8000000000000c01UL) == 9223372036854779904.0);
    printf("d2u %d %d %d %d\n", to_u(1.5e19) == 15000000000000000000UL,
           to_u(9223372036854775808.0) == TOP, to_u(42.9) == 42,
           to_u(18446744073709549568.0) == 0xfffffffffffff800UL);
    printf("consts %d %d %d %d\n", k_big == 18446744073709551616.0, k_top == 9223372036854775808.0,
           k_back == 15000000000000000000UL, k_small == 12345);

    /* compound assignment mixing unsigned and double */
    unsigned long u = 10000000000000000000UL;
    u *= 1.5;
    printf("opassign %d\n", u == 15000000000000000000UL);
    double d = 2.0;
    d *= TOP;
    printf("dmul %d\n", d == 18446744073709551616.0);

    /* printf %u */
    printf("%u %lu %llu %zu\n", 7UL, BIG, TOP, (unsigned long)0);
    printf("[%5lu] [%-5lu] [%05lu] [%.3lu] [%lu%%]\n", 42UL, 42UL, 42UL, 7UL, 99UL);
    printf("mixed %d %lu %s %lu %x\n", -3, BIG - 1, "str", 12UL, 255);
    char buf[64];
    int n = sprintf(buf, "%lu|%lu", BIG, 18446744073709551000UL);
    printf("sprintf %d %s %d\n", n, buf, (int)strlen(buf));
    fprintf(stdout, "fprintf %lu\n", TOP);
    return 0;
}
