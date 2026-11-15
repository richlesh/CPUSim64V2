/*
 * progress_bar.c
 *
 * Animates a percentage progress bar ("Copying: ████░░░░  42%") six times,
 * with steps of 1% to 6% and a different color each time.  The bar is
 * redrawn on the same line with a carriage return; the cursor is hidden
 * while it runs.
 * C11 version of documentation/projects/progress_bar.asm (with the
 * progress_bar_percent function of system/console.asm).
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SIZE 74

#define RESET "\x1b[0m"
#define BRIGHT_BLACK "\x1b[90m"
#define BRIGHT_RED "\x1b[91m"
#define BRIGHT_GREEN "\x1b[92m"
#define BRIGHT_YELLOW "\x1b[93m"
#define BRIGHT_BLUE "\x1b[94m"
#define BRIGHT_MAGENTA "\x1b[95m"
#define BRIGHT_CYAN "\x1b[96m"
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

/* a bar of total cells, the fraction pct of them filled, then the percentage (right aligned, omitted at 100%) */
static void progress_bar_percent(long total, double pct, const char *color)
{
    long i, filled, pct_int;
    if (pct < 0.0) pct = 0.0;
    if (pct > 1.0) pct = 1.0;
    filled = lround(total * pct);
    pct_int = lround(pct * 100);
    fputs(color, stdout);
    for (i = 0; i < filled; i++) putchar(0x2588);           /* full block */
    fputs(BRIGHT_BLACK, stdout);
    for (i = filled + 1; i <= total; i++) putchar(0x2591);  /* light shade */
    fputs(RESET, stdout);
    if (pct_int < 100) printf(" %3ld%%", pct_int);
}

static void bar_demo(long incr, const char *color)
{
    long p;
    for (p = 0; p <= 100; p += incr) {
        fputs("Copying: ", stdout);
        progress_bar_percent(SIZE, p / 100.0, color);
        delay_ms(100);
        fputs("\r", stdout);
    }
    fputs("Copying: ", stdout);
    progress_bar_percent(SIZE, 1.0, RESET);
    fputs(" Done", stdout);
    putchar('\n');
}

int main(void)
{
    fputs("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n", stdout);
    fputs(HIDE_CURSOR, stdout);
    bar_demo(1, BRIGHT_CYAN);
    bar_demo(2, BRIGHT_MAGENTA);
    bar_demo(3, BRIGHT_YELLOW);
    bar_demo(4, BRIGHT_RED);
    bar_demo(5, BRIGHT_GREEN);
    bar_demo(6, BRIGHT_BLUE);
    fputs(SHOW_CURSOR, stdout);
    return 0;
}
