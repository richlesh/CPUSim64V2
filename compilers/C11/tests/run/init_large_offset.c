/* address initializers of a global array beyond word 2047 (a store to
   label+offset has a 12-bit offset) */
#include <stdio.h>

static int a, b;
static const char *names[] = { "zero", "one" };
static int *ptrs[3000] = { [0] = &a, [2047] = &b, [2048] = &a, [2500] = &b, [2999] = &a };
static struct { long n; const char *s; } table[1200] = { [1100] = { 7, "seven" }, [1199] = { 8, "eight" } };

int main(void)
{
    printf("%d %d %d %d %d\n", ptrs[0] == &a, ptrs[2047] == &b, ptrs[2048] == &a, ptrs[2500] == &b, ptrs[2999] == &a);
    printf("%d %s %ld %s %s\n", ptrs[1] == 0, names[1], table[1100].n, table[1100].s, table[1199].s);
    return 0;
}
