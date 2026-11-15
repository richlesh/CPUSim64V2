/*
 * BubbleSort.c
 *
 * Sorts 10 arrays of 15-30 random integers in [1, 999] with bubble sort
 * and prints each array before and after.  The arrays keep their length in
 * element 0.  The numbers differ from run to run.
 * C11 version of documentation/projects/BubbleSort.asm.
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* a random integer in [low, high] */
static long rand_range(long low, long high)
{
    return low + rand() % (high - low + 1);
}

/* an array of n random integers in [1, 999]; a[0] holds the length */
static long *create_random_array(long n)
{
    long *a = malloc((n + 1) * sizeof(long));
    long i;
    a[0] = n;
    for (i = 1; i <= n; i++) a[i] = rand_range(1, 999);
    return a;
}

/* prints the elements separated by commas */
static void print_int_array(const long *a)
{
    long i;
    for (i = 1; i <= a[0]; i++) printf(i == 1 ? "%ld" : ",%ld", a[i]);
}

static void bubble_sort(long *a)
{
    long n = a[0], i, j;
    for (i = 1; i < n; i++)
        for (j = 1; j <= n - i; j++)
            if (a[j] > a[j + 1]) {
                long t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
            }
}

int main(void)
{
    int i;
    srand((unsigned)time(NULL));
    for (i = 1; i <= 10; i++) {
        long *a = create_random_array(rand_range(15, 30));
        printf("Test %d\n", i);
        fputs("random: ", stdout);
        print_int_array(a);
        putchar('\n');
        bubble_sort(a);
        fputs("sorted: ", stdout);
        print_int_array(a);
        putchar('\n');
        free(a);
    }
    return 0;
}
