/*
 * QuickSort.c
 *
 * Sorts 10 arrays of 15-30 random integers in [1, 999] with quicksort
 * (Lomuto partition, the last element is the pivot) and prints each array
 * before and after.  The arrays keep their length in element 0.  The numbers
 * differ from run to run.
 * C11 version of documentation/projects/QuickSort.asm.
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

static long partition(long *a, long lo, long hi)
{
    long pivot = a[hi], i = lo - 1, j, t;
    for (j = lo; j < hi; j++)
        if (a[j] <= pivot) {
            i++;
            t = a[i];
            a[i] = a[j];
            a[j] = t;
        }
    i++;
    t = a[i];
    a[i] = a[hi];
    a[hi] = t;
    return i;
}

static void quick_sort_range(long *a, long lo, long hi)
{
    if (lo < hi) {
        long p = partition(a, lo, hi);
        quick_sort_range(a, lo, p - 1);
        quick_sort_range(a, p + 1, hi);
    }
}

static void quick_sort(long *a) { quick_sort_range(a, 1, a[0]); }

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
        quick_sort(a);
        fputs("sorted: ", stdout);
        print_int_array(a);
        putchar('\n');
        free(a);
    }
    return 0;
}
