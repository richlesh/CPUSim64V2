/* Separate compilation: main is linked with tests/multi/multi_util.c and
   multi_data.c (each compiled to its own module).  The library is appended
   to each module that uses it. */
#include <stdio.h>
#include "../multi/multi.h"

static int helper(void) { return 100; }     /* multi_util.c has its own helper */

int main(void) {
    printf("add3(10) = %d\n", add3(10));
    printf("counter = %d, calls = %d\n", counter, calls);
    printf("helper = %d, util_helper = %d\n", helper(), util_helper());
    int (*f)(int) = add3;                   /* function pointer across modules */
    printf("via pointer: %d\n", f(1));
    printf("table: %d %d %d, name = %s\n", table[0], table[1], table[2], name);
    apply(twice, 21);
    printf("calls = %d\n", calls);
    return counter;
}

int twice(int x) { return 2 * x; }
