/*
 * working_bar.c
 *
 * Animates a "working" indicator: a dot (●) swings back and forth inside
 * brackets, easing in and out with a cosine, seven times with shorter
 * delays and different colors.
 * C11 version of documentation/projects/working_bar.asm (with the
 * working_bar function of system/console.asm).
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define PROGRESS_SIZE 40

#define RESET "\x1b[0m"
#define BRIGHT_RED "\x1b[91m"
#define BRIGHT_GREEN "\x1b[92m"
#define BRIGHT_YELLOW "\x1b[93m"
#define BRIGHT_BLUE "\x1b[94m"
#define BRIGHT_MAGENTA "\x1b[95m"
#define BRIGHT_CYAN "\x1b[96m"
#define BRIGHT_WHITE "\x1b[97m"
#define HIDE_CURSOR "\x1b[?25l"
#define SHOW_CURSOR "\x1b[?25h"

/* waits ms milliseconds.  The C library has no sleep function, so this busy-waits on clock();
   with the environment variable NO_DELAY set it returns at once (for the tests). */
static void delay_ms(long ms)
{
    clock_t end;
    if (getenv("NO_DELAY")) return;
    end = clock() + (clock_t)(ms * (CLOCKS_PER_SEC / 1000));
    while (clock() < end) {}
}

/* "[   ●      ]" with len cells; the dot is at (1 - cos(2 pi pct)) / 2 of the way (color 0: no dot) */
static void working_bar(long len, double pct, const char *color)
{
    double f = -(((cos(M_PI * 2 * pct) + 1) / 2) - 1);
    long bar_len = (long)round(f * (len - 1)) + 1, i;
    fputs(RESET, stdout);
    putchar('[');
    for (i = 0; i < bar_len - 1; i++) putchar(' ');
    if (color) {
        fputs(color, stdout);
        putchar(0x25CF);
        fputs(RESET, stdout);
    } else putchar(' ');
    for (i++; i < len; i++) putchar(' ');
    putchar(']');
}

static void bar_demo(long delay, const char *color)
{
    long i;
    double p = 0;
    for (i = 0; i <= 300; i++) {
        fputs("Processing: ", stdout);
        p = i / 100.0;
        working_bar(PROGRESS_SIZE, p, color);
        delay_ms(delay);
        fputs("\r", stdout);
    }
    fputs("Processing: ", stdout);
    working_bar(PROGRESS_SIZE, p, 0);
    puts(" Done");
}

int main(void)
{
    fputs(HIDE_CURSOR, stdout);
    bar_demo(60, BRIGHT_CYAN);
    bar_demo(50, BRIGHT_MAGENTA);
    bar_demo(40, BRIGHT_YELLOW);
    bar_demo(30, BRIGHT_RED);
    bar_demo(20, BRIGHT_GREEN);
    bar_demo(10, BRIGHT_BLUE);
    bar_demo(5, BRIGHT_WHITE);
    fputs(SHOW_CURSOR, stdout);
    return 0;
}
