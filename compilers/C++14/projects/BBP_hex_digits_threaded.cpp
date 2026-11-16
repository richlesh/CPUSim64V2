/*
 * BBP_hex_digits_threaded.cpp
 *
 * Computes 100 blocks of 32 hexadecimal digits of pi, starting at position n,
 * n + 32, n + 64, ..., with the Bailey-Borwein-Plouffe digit extraction
 * formula (see BBP_hex_digits.cpp).  One thread per processor core takes the
 * next block with an atomic counter until all 100 are done; the results are
 * written to a shared array and printed in order by the main thread.
 * C++ version of documentation/projects/BBP_hex_digits_threaded.asm; it uses
 * std::thread and std::atomic.
 *
 * Usage:
 *     BBP_hex_digits_threaded <n>      (n = 1 is the first hex digit after the point)
 *
 * Example:
 *     > BBP_hex_digits_threaded 1
 *     Number of threads: 8
 *     Starting 0
 *     ...
 *     Position 1: 243F6A8885A308D313198A2E03707344
 *     Position 33: A4093822299F31D0082EFA98EC4E6C89
 *     ...
 *
 * Author:   Richard Lesh
 */
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

static const long MAX_POSITION = 10000000;
static const long NUM_SLOTS = 100;          // the positions n, n+32, ..., n+32*99
static const long SLOT_STRIDE = 32;         // the 32 digits of a slot are followed by those of the next
static const long MAX_START = MAX_POSITION - SLOT_STRIDE * (NUM_SLOTS - 1);

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

static std::atomic<long> gSlot(-1);     // the last block taken
static long gN;                     // the first position
static std::vector<u64> gResults;   // 2 words (the 32 digits) for each block

// a thread: computes blocks until there are none left
static void worker()
{
    for (;;) {
        long slot = ++gSlot;                        // the next block
        if (slot >= NUM_SLOTS) break;
        std::printf("Starting %ld\n", slot);
        long pos = gN + slot * SLOT_STRIDE;
        U192 acc = pi_hex(pos - 1);
        gResults[2 * slot] = acc.hi;
        gResults[2 * slot + 1] = acc.mid;
        long guard = (long)(acc.lo >> 32);   // the top 32 guard bits: all 0s or all 1s mean the last digits may be off by one
        if (guard == 0) std::printf("Warning: position %ld guard bits are all 0s; the last digit(s) may be 1 too high.\n", pos);
        else if (guard == 0xFFFFFFFF) std::printf("Warning: position %ld guard bits are all 1s; the last digit(s) may be 1 too low.\n", pos);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "Usage: BBP_hex_digits <n>   (n = 1 is the first hex digit after the point)" << std::endl;
        return 1;
    }
    gN = std::atol(argv[1]);
    if (gN < 1) {
        std::cout << "n must be >= 1" << std::endl;
        return 1;
    }
    if (gN > MAX_START) {
        std::cout << "n must be <= " << MAX_START << std::endl;
        return 1;
    }
    gResults.assign(2 * NUM_SLOTS, 0);
    long cores = (long)std::thread::hardware_concurrency();
    if (cores < 1) cores = 4;
    std::printf("Number of threads: %ld\n", cores);
    std::vector<std::thread> threads;
    for (long i = 0; i < cores; i++) threads.push_back(std::thread(worker));
    for (long i = 0; i < cores; i++) threads[i].join();
    for (long i = 0; i < NUM_SLOTS; i++)
        std::printf("Position %ld: %016lX%016lX\n", gN + i * SLOT_STRIDE, gResults[2 * i], gResults[2 * i + 1]);
    return 0;
}
