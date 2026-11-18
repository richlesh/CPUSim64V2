/*
 * fill_array.c
 *
 * Allocates an array of n longs, where n comes from the command line,
 * and fills it with 0, 1, 2, ..., n-1.
 *
 * Usage:
 *     fill_array <n>
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Print a wall clock time as "Label: h:mm:ss am/pm"
static void print_clock(const char *label, time_t t)
{
    struct tm *tm = localtime(&t);
    int hour = tm->tm_hour % 12;

    if (hour == 0) {
        hour = 12;
    }
    printf("%s: %d:%02d:%02d %s\n", label, hour, tm->tm_min, tm->tm_sec,
           tm->tm_hour < 12 ? "am" : "pm");
}


int main(int argc, char *argv[])
{
    long n;
    long *a;
    char *end;
    time_t start, stop;
    long elapsed;

    if (argc != 2) {
        puts("Usage: fill_array <n>   (n = number of elements in MiW)");
        return 1;
    }

    n = strtol(argv[1], &end, 10);
    if (*end != '\0' || n < 1) {
        puts("n must be a positive integer");
        return 1;
    }

    n *= 1024 * 1024;
    a = malloc(n * sizeof(long));
    if (!a) {
        puts("Unable to allocate array!");
        return 1;
    }

    start = time(NULL);
    print_clock("Start", start);

    for (long i = 0; i < n; i++) {
        a[i] = i;
    }

    stop = time(NULL);
    print_clock("Stop", stop);

    elapsed = (long)difftime(stop, start);
    printf("Duration: %02ld:%02ld:%02ld\n",
           elapsed / 3600, (elapsed / 60) % 60, elapsed % 60);

    // Print the contents to verify
//    for (long i = 0; i < n; i++) {
//        printf("%ld", a[i]);
//        putchar(i + 1 < n ? ' ' : '\n');
//    }

    free(a);
    return 0;
}
