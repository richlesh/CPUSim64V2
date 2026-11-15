/*
 * Renard2.c
 *
 * Computes and prints one or more Renard series, one line per series, each
 * labelled with its size.
 * C11 version of documentation/projects/Renard2.asm.
 *
 * Usage:
 *     Renard2 <series> [<series> ...]
 *
 * Author:   Richard Lesh
 * Original: 2022/11/28
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* prints "S: t0 t1 ..." for the series of size s */
static void print_renard(long s)
{
    double factor = pow(10.0, 1.0 / s);
    long i;
    printf("%ld:", s);
    for (i = 0; i < s; i++) printf(" %.2f", pow(factor, (double)i));
    putchar('\n');
}

int main(int argc, char **argv)
{
    int i;
    if (argc < 2) {
        puts("You must supply a series value 5, 10, 20 or 40");
        return 1;
    }
    for (i = 1; i < argc; i++) print_renard(atol(argv[i]));
    return 0;
}
