/*
 * PI_spigot.c
 *
 * Prints n digits of pi after the decimal point with the Rabinowitz-Wagon
 * spigot algorithm:
 *     len = floor(10 * D / 3) + 1,  a[1..len] = 2
 *     repeat D times:
 *         q = 0
 *         for i = len down to 1:
 *             x = 10 * a[i] + q * i;  a[i] = x mod (2i - 1);  q = x div (2i - 1)
 *         a[1] = q mod 10,  q = q div 10
 * A digit 9 is held back (it may become 0 after a carry of 10).  A few
 * guard digits are computed past the requested ones.
 * C11 version of documentation/projects/PI_spigot.asm.
 *
 * Usage:
 *     PI_spigot <n>
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>

#define GUARD_DIGITS 10

static long emit_index = -2;        /* the index of the next digit: -1 is the predigit 0, 0 is the 3 */
static long emit_end;               /* the digits with index < emit_end are printed */

static void emit_digit(long d)
{
    emit_index++;
    if (emit_index >= 0 && emit_index < emit_end) {
        putchar((int)('0' + d));
        if (emit_index == 0) putchar('.');
    }
}

static void emit_run(long d, long count)
{
    long i;
    for (i = 0; i < count; i++) emit_digit(d);
}

int main(int argc, char **argv)
{
    long n, len, total, j, i, q, predigit = 0, nines = 0;
    long *a;
    if (argc != 2) {
        puts("Usage: PI_spigot <n>   (n = number of digits after the decimal point)");
        return 1;
    }
    n = atol(argv[1]);
    if (n < 1) {
        puts("n must be >= 1");
        return 1;
    }
    emit_end = n + 1;
    total = n + 1 + GUARD_DIGITS;
    len = total * 10 / 3 + 1;
    a = malloc((len + 1) * sizeof(long));
    if (!a) {
        puts("Unable to allocate spigot array!");
        return 1;
    }
    for (i = 1; i <= len; i++) a[i] = 2;
    for (j = 0; j < total; j++) {
        q = 0;
        for (i = len; i > 0; i--) {
            long den = 2 * i - 1;
            long x = a[i] * 10 + q * i;
            q = x / den;
            a[i] = x % den;
        }
        a[1] = q % 10;
        q = q / 10;
        if (q == 9) nines++;
        else if (q == 10) {
            emit_digit(predigit + 1);
            emit_run(0, nines);
            predigit = 0;
            nines = 0;
        } else {
            emit_digit(predigit);
            predigit = q;
            emit_run(9, nines);
            nines = 0;
        }
        if (emit_index + 1 >= emit_end) break;
    }
    putchar('\n');
    free(a);
    return 0;
}
