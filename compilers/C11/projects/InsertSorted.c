/*
 * InsertSorted.c
 *
 * Inserts 30 random integers in [1, 999] one at a time into a sorted,
 * growing array and prints the array after each insertion.  The place of a
 * value is found by binary search; the array doubles its capacity when it
 * is full.  The numbers differ from run to run.
 * C11 version of documentation/projects/InsertSorted.asm.
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* a growing array: a[0] holds the length, the elements are a[1..length] */
typedef struct {
    long *a;
    long cap;                       /* the number of elements a can hold, element 0 included */
} SortedArray;

static void insert_sorted(SortedArray *s, long val)
{
    long n = s->a[0], lo = 1, hi = n, i;
    if (s->cap < n + 2) {
        long nc = s->cap * 2;
        if (nc < n + 2) nc = n + 2;
        s->a = realloc(s->a, nc * sizeof(long));
        s->cap = nc;
    }
    /* the first position whose element is greater than val */
    while (lo <= hi) {
        long mid = (lo + hi) / 2;
        if (s->a[mid] <= val) lo = mid + 1;
        else hi = mid - 1;
    }
    for (i = n; i >= lo; i--) s->a[i + 1] = s->a[i];
    s->a[lo] = val;
    s->a[0] = n + 1;
}

static void print_int_array(const long *a)
{
    long i;
    for (i = 1; i <= a[0]; i++) printf(i == 1 ? "%ld" : ",%ld", a[i]);
}

int main(void)
{
    SortedArray s;
    int i;
    srand((unsigned)time(NULL));
    s.cap = 2;
    s.a = malloc(s.cap * sizeof(long));
    s.a[0] = 0;
    puts("Inserting 30 random integers [1,999] one at a time:");
    for (i = 1; i <= 30; i++) {
        long v = 1 + rand() % 999;
        insert_sorted(&s, v);
        printf("Insert %3ld: ", v);
        print_int_array(s.a);
        putchar('\n');
    }
    free(s.a);
    return 0;
}
