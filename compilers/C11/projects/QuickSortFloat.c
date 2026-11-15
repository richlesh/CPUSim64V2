/*
 * QuickSortFloat.c
 *
 * Sorts 10 arrays of 9-14 random doubles in [0, 1) with quicksort (Lomuto
 * partition) and prints each array (6 decimals) before and after.  The
 * arrays keep their length in element 0.  The numbers differ from run to run.
 * C11 version of documentation/projects/QuickSortFloat.asm.
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

/* an array of n random doubles in [0, 1); a[0] holds the length */
static double *create_random_array(long n)
{
    double *a = malloc((n + 1) * sizeof(double));
    long i;
    a[0] = (double)n;
    for (i = 1; i <= n; i++) a[i] = rand() / (RAND_MAX + 1.0);
    return a;
}

/* prints the elements with p decimals, separated by commas */
static void print_float_array(const double *a, int p)
{
    long i, n = (long)a[0];
    for (i = 1; i <= n; i++) printf(i == 1 ? "%.*f" : ",%.*f", p, a[i]);
}

static long partition_float(double *a, long lo, long hi)
{
    double pivot = a[hi], t;
    long i = lo - 1, j;
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

static void quick_sort_range_float(double *a, long lo, long hi)
{
    if (lo < hi) {
        long p = partition_float(a, lo, hi);
        quick_sort_range_float(a, lo, p - 1);
        quick_sort_range_float(a, p + 1, hi);
    }
}

static void quick_sort_float(double *a) { quick_sort_range_float(a, 1, (long)a[0]); }

int main(void)
{
    int i;
    srand((unsigned)time(NULL));
    for (i = 1; i <= 10; i++) {
        double *a = create_random_array(rand_range(9, 14));
        printf("Test %d\n", i);
        fputs("random: ", stdout);
        print_float_array(a, 6);
        putchar('\n');
        quick_sort_float(a);
        fputs("sorted: ", stdout);
        print_float_array(a, 6);
        putchar('\n');
        free(a);
    }
    return 0;
}
