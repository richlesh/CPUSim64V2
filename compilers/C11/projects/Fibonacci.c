/*
 * Fibonacci.c
 *
 * Prints the first 45 numbers of the Fibonacci sequence.
 * C11 version of documentation/projects/Fibonacci.asm.
 *
 * Author:   Richard Lesh
 * Original: 2009/03/20
 */
#include <stdio.h>

int main(void)
{
    long a = 1, b = 1;
    int i;
    printf("%ld\n%ld\n", a, b);
    for (i = 0; i < 43; i++) {
        long c = a + b;
        printf("%ld\n", c);
        b = a;
        a = c;
    }
    return 0;
}
