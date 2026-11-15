/*
 * BBP_hex_digits_threaded.c
 *
 * Computes 100 blocks of 32 hexadecimal digits of pi, starting at position n,
 * n + 32, n + 64, ..., with the Bailey-Borwein-Plouffe digit extraction
 * formula (see BBP_hex_digits.c).  One thread per processor core takes the
 * next block with an atomic counter until all 100 are done; the results are
 * written to a shared array and printed in order by the main thread.
 * C11 version of documentation/projects/BBP_hex_digits_threaded.asm; it uses
 * <thread.h>.
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
#include <stdio.h>
#include <stdlib.h>
#include <thread.h>

#define MAX_POSITION 10000000
#define NUM_SLOTS 100               /* the positions n, n+32, ..., n+32*99 */
#define SLOT_STRIDE 32              /* the 32 digits of a slot are followed by those of the next */
#define MAX_START (MAX_POSITION - SLOT_STRIDE * (NUM_SLOTS - 1))

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


static long gSlot = -1;             /* the last block taken */
static long gN;                     /* the first position */
static u64 *gResults;               /* 2 words (the 32 digits) for each block */

/* a thread: computes blocks until there are none left */
static void worker(void *unused)
{
    (void)unused;
    for (;;) {
        long slot = atomic_increment(&gSlot);       /* the next block */
        long pos, guard;
        U192 acc, t;
        if (slot >= NUM_SLOTS) break;
        printf("Starting %ld\n", slot);
        pos = gN + slot * SLOT_STRIDE;
        acc = series(1, pos - 1);
        shl192(&acc, 2);                /* 4 * S(1) */
        t = series(4, pos - 1);
        shl192(&t, 1);                  /* 2 * S(4) */
        sub192(&acc, t);
        sub192(&acc, series(5, pos - 1));
        sub192(&acc, series(6, pos - 1));
        gResults[2 * slot] = acc.hi;
        gResults[2 * slot + 1] = acc.mid;
        guard = (long)(acc.lo >> 32);   /* the top 32 guard bits: all 0s or all 1s mean the last digits may be off by one */
        if (guard == 0) printf("Warning: position %ld guard bits are all 0s; the last digit(s) may be 1 too high.\n", pos);
        else if (guard == 0xFFFFFFFF) printf("Warning: position %ld guard bits are all 1s; the last digit(s) may be 1 too low.\n", pos);
    }
}

int main(int argc, char **argv)
{
    long i, cores;
    long *pids;
    if (argc != 2) {
        puts("Usage: BBP_hex_digits <n>   (n = 1 is the first hex digit after the point)");
        return 1;
    }
    gN = atol(argv[1]);
    if (gN < 1) {
        puts("n must be >= 1");
        return 1;
    }
    if (gN > MAX_START) {
        printf("n must be <= %d\n", MAX_START);
        return 1;
    }
    gResults = malloc(2 * NUM_SLOTS * sizeof(u64));
    if (!gResults) {
        puts("Error: results allocation failed!");
        return 1;
    }
    cores = get_num_cores();
    printf("Number of threads: %ld\n", cores);
    pids = malloc(cores * sizeof(long));
    if (!pids) {
        puts("Error: PID array allocation failed!");
        return 1;
    }
    for (i = 0; i < cores; i++) {
        pids[i] = thread_create(worker, NULL);
        if (pids[i] <= 0) {
            printf("Error: thread %ld failed to start!\n", i);
            pids[i] = 0;
        }
    }
    for (i = 0; i < cores; i++)
        if (pids[i] > 0) thread_join(pids[i]);
    for (i = 0; i < NUM_SLOTS; i++)
        printf("Position %ld: %016lX%016lX\n", gN + i * SLOT_STRIDE, gResults[2 * i], gResults[2 * i + 1]);
    free(pids);
    free(gResults);
    return 0;
}
