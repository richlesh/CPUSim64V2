/*
 * BBP_hex_digits.c
 *
 * Computes hexadecimal digits of pi starting at any position n, without the
 * digits before it, with the Bailey-Borwein-Plouffe digit extraction formula:
 *     frac(16^d * pi) = frac(4*S(1) - 2*S(4) - S(5) - S(6)),   d = n - 1
 *     S(j) = sum[k=0..d] (16^(d-k) mod (8k+j)) / (8k+j) + sum[k>d] 16^(d-k) / (8k+j)
 * The fractions are 192-bit fixed point numbers in three 64-bit words; the
 * first 32 hex digits are printed, the rest are guard bits.
 * C11 version of documentation/projects/BBP_hex_digits.asm.
 *
 * Usage:
 *     BBP_hex_digits <n>       (n = 1 is the first hex digit after the point)
 *
 * Example:
 *     > BBP_hex_digits 1
 *     Hex digits of pi at position 1: 243F6A8885A308D313198A2E03707344
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>

#define MAX_POSITION 10000000

typedef unsigned long u64;
typedef struct { u64 hi, mid, lo; } U192;     /* a fraction in [0, 1): hi holds the first 64 bits */

/* num / den as a 192-bit fraction (num < den < 2^32): long division 32 bits at a time */
static U192 frac192(u64 num, u64 den)
{
    U192 f;
    u64 r = num, *w[3];
    int i;
    w[0] = &f.hi;
    w[1] = &f.mid;
    w[2] = &f.lo;
    for (i = 0; i < 3; i++) {
        u64 a, b;
        a = (r << 32) / den;
        r = (r << 32) % den;
        b = (r << 32) / den;
        r = (r << 32) % den;
        *w[i] = (a << 32) | b;
    }
    return f;
}

static void add192(U192 *a, U192 b)
{
    u64 lo = a->lo + b.lo, c1 = lo < a->lo;
    u64 mid = a->mid + b.mid, c2 = mid < a->mid;
    u64 mid2 = mid + c1;
    if (mid2 < mid) c2 = 1;
    a->lo = lo;
    a->mid = mid2;
    a->hi = a->hi + b.hi + c2;
}

static void sub192(U192 *a, U192 b)
{
    u64 lo = a->lo - b.lo, c1 = a->lo < b.lo;
    u64 mid = a->mid - b.mid, c2 = a->mid < b.mid;
    if (c1 && mid == 0) c2 = 1;
    a->lo = lo;
    a->mid = mid - c1;
    a->hi = a->hi - b.hi - c2;
}

/* shifts left by n (0 < n < 64) */
static void shl192(U192 *a, int n)
{
    a->hi = (a->hi << n) | (a->mid >> (64 - n));
    a->mid = (a->mid << n) | (a->lo >> (64 - n));
    a->lo <<= n;
}

/* 16^e mod m */
static u64 powmod16(u64 e, u64 m)
{
    u64 r = 1, b;
    if (m == 1) return 0;
    b = 16 % m;
    while (e > 0) {
        if (e & 1) r = r * b % m;
        b = b * b % m;
        e >>= 1;
    }
    return r;
}

/* the fractional part of 16^d * S(j) */
static U192 series(u64 j, u64 d)
{
    U192 s = {0, 0, 0};
    u64 k;
    long t;
    for (k = 0; k <= d; k++) {
        u64 m = 8 * k + j;
        add192(&s, frac192(powmod16(d - k, m), m));
    }
    /* the tail: 16^-t / m for the next terms, shifted right by 4t bits */
    for (t = 1; t < 48; t++) {
        u64 m = 8 * (d + t) + j;
        U192 f = frac192(1, m);
        long sh = 4 * t;
        while (sh >= 64) {
            f.lo = f.mid;
            f.mid = f.hi;
            f.hi = 0;
            sh -= 64;
        }
        if (sh != 0) {
            f.lo = (f.lo >> sh) | (f.mid << (64 - sh));
            f.mid = (f.mid >> sh) | (f.hi << (64 - sh));
            f.hi >>= sh;
        }
        add192(&s, f);
    }
    return s;
}

int main(int argc, char **argv)
{
    long n, guard;
    u64 d;
    U192 acc, t;
    if (argc != 2) {
        puts("Usage: BBP_hex_digits <n>   (n = 1 is the first hex digit after the point)");
        return 1;
    }
    n = atol(argv[1]);
    if (n < 1) {
        puts("n must be >= 1");
        return 1;
    }
    if (n > MAX_POSITION) {
        printf("n must be <= %d\n", MAX_POSITION);
        return 1;
    }
    d = n - 1;
    acc = series(1, d);
    shl192(&acc, 2);                    /* 4 * S(1) */
    t = series(4, d);
    shl192(&t, 1);                      /* 2 * S(4) */
    sub192(&acc, t);
    sub192(&acc, series(5, d));
    sub192(&acc, series(6, d));
    printf("Hex digits of pi at position %ld: %016lX%016lX\n", n, acc.hi, acc.mid);
    /* the top 32 guard bits: all 0s or all 1s mean the last digits may be off by one */
    guard = (long)(acc.lo >> 32);
    if (guard == 0) puts("Warning: guard bits are all 0s; the last digit(s) may be 1 too high.");
    else if (guard == 0xFFFFFFFF) puts("Warning: guard bits are all 1s; the last digit(s) may be 1 too low.");
    return 0;
}
