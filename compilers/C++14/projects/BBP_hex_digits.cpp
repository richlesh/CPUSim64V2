/*
 * BBP_hex_digits.cpp
 *
 * Computes hexadecimal digits of pi starting at any position n, without the
 * digits before it, with the Bailey-Borwein-Plouffe digit extraction formula:
 *     frac(16^d * pi) = frac(4*S(1) - 2*S(4) - S(5) - S(6)),   d = n - 1
 *     S(j) = sum[k=0..d] (16^(d-k) mod (8k+j)) / (8k+j) + sum[k>d] 16^(d-k) / (8k+j)
 * The fractions are 192-bit fixed point numbers in three 64-bit words; the
 * first 32 hex digits are printed, the rest are guard bits.
 * C++ version of documentation/projects/BBP_hex_digits.asm.
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
#include <cstdio>
#include <cstdlib>
#include <iostream>

static const long MAX_POSITION = 10000000;

typedef unsigned long u64;

// a fraction in [0, 1): hi holds the first 64 bits
struct U192 {
    u64 hi, mid, lo;

    U192() : hi(0), mid(0), lo(0) {}

    // num / den as a 192-bit fraction (num < den < 2^32): long division 32 bits at a time
    static U192 fraction(u64 num, u64 den)
    {
        U192 f;
        u64 *w[3] = { &f.hi, &f.mid, &f.lo };
        u64 r = num;
        for (int i = 0; i < 3; i++) {
            u64 a = (r << 32) / den;
            r = (r << 32) % den;
            u64 b = (r << 32) / den;
            r = (r << 32) % den;
            *w[i] = (a << 32) | b;
        }
        return f;
    }

    U192 &operator+=(const U192 &b)
    {
        u64 nlo = lo + b.lo, c1 = nlo < lo;
        u64 nmid = mid + b.mid, c2 = nmid < mid;
        u64 mid2 = nmid + c1;
        if (mid2 < nmid) c2 = 1;
        lo = nlo;
        mid = mid2;
        hi = hi + b.hi + c2;
        return *this;
    }

    U192 &operator-=(const U192 &b)
    {
        u64 nlo = lo - b.lo, c1 = lo < b.lo;
        u64 nmid = mid - b.mid, c2 = mid < b.mid;
        if (c1 && nmid == 0) c2 = 1;
        lo = nlo;
        mid = nmid - c1;
        hi = hi - b.hi - c2;
        return *this;
    }

    // shifts left by n (0 < n < 64)
    void shl(int n)
    {
        hi = (hi << n) | (mid >> (64 - n));
        mid = (mid << n) | (lo >> (64 - n));
        lo <<= n;
    }

    // shifts right by n bits
    void shr(long n)
    {
        while (n >= 64) {
            lo = mid;
            mid = hi;
            hi = 0;
            n -= 64;
        }
        if (n != 0) {
            lo = (lo >> n) | (mid << (64 - n));
            mid = (mid >> n) | (hi << (64 - n));
            hi >>= n;
        }
    }
};

// 16^e mod m
static u64 powmod16(u64 e, u64 m)
{
    if (m == 1) return 0;
    u64 r = 1, b = 16 % m;
    while (e > 0) {
        if (e & 1) r = r * b % m;
        b = b * b % m;
        e >>= 1;
    }
    return r;
}

// the fractional part of 16^d * S(j)
static U192 series(u64 j, u64 d)
{
    U192 s;
    for (u64 k = 0; k <= d; k++) {
        u64 m = 8 * k + j;
        s += U192::fraction(powmod16(d - k, m), m);
    }
    // the tail: 16^-t / m for the next terms, shifted right by 4t bits
    for (long t = 1; t < 48; t++) {
        u64 m = 8 * (d + t) + j;
        U192 f = U192::fraction(1, m);
        f.shr(4 * t);
        s += f;
    }
    return s;
}

// the fractional part of 16^d * pi: 4 S(1) - 2 S(4) - S(5) - S(6)
static U192 pi_hex(u64 d)
{
    U192 acc = series(1, d);
    acc.shl(2);                         // 4 * S(1)
    U192 t = series(4, d);
    t.shl(1);                           // 2 * S(4)
    acc -= t;
    acc -= series(5, d);
    acc -= series(6, d);
    return acc;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "Usage: BBP_hex_digits <n>   (n = 1 is the first hex digit after the point)" << std::endl;
        return 1;
    }
    long n = std::atol(argv[1]);
    if (n < 1) {
        std::cout << "n must be >= 1" << std::endl;
        return 1;
    }
    if (n > MAX_POSITION) {
        std::cout << "n must be <= " << MAX_POSITION << std::endl;
        return 1;
    }
    U192 acc = pi_hex(n - 1);
    std::printf("Hex digits of pi at position %ld: %016lX%016lX\n", n, acc.hi, acc.mid);
    // the top 32 guard bits: all 0s or all 1s mean the last digits may be off by one
    long guard = (long)(acc.lo >> 32);
    if (guard == 0) std::cout << "Warning: guard bits are all 0s; the last digit(s) may be 1 too high." << std::endl;
    else if (guard == 0xFFFFFFFF) std::cout << "Warning: guard bits are all 1s; the last digit(s) may be 1 too low." << std::endl;
    return 0;
}
