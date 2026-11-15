/*
 * Renard.c
 *
 * Computes and prints a Renard series of preferred numbers.  The R5, R10,
 * R20 and R40 series divide a decade into 5, 10, 20 or 40 equal logarithmic
 * steps; term n of the series S is (10 ^ (1/S)) ^ n.
 * C11 version of documentation/projects/Renard.asm.
 *
 * Usage:
 *     Renard <series>
 *
 * Example:
 *     > Renard 10
 *     1.00 1.26 1.58 2.00 2.51 3.16 3.98 5.01 6.31 7.94
 *
 * Author:   Richard Lesh
 * Original: 2022/11/28
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char **argv)
{
    long series, i;
    double factor;
    if (argc != 2) {
        puts("You must supply a series value 5, 10, 20 or 40");
        return 1;
    }
    series = atol(argv[1]);
    if (series < 2) {
        puts("You must supply a series value > 2");
        return 2;
    }
    factor = pow(10.0, 1.0 / series);          /* the common ratio */
    for (i = 0; i < series; i++) printf("%.2f ", pow(factor, (double)i));
    putchar('\n');
    return 0;
}
