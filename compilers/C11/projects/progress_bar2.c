/*
 * progress_bar2.c
 *
 * Animates an indeterminate progress bar: a three-cell "eye" with a fading
 * trail (▓▒░) bounces back and forth in a 40-cell bar, six times with
 * shorter delays and different colors.
 * C11 version of documentation/projects/progress_bar2.asm (with the
 * progress_bar function of system/console.asm).
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 40
#define EYE_WIDTH 3

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

/* the position of the eye at step x: 0, 1, ... m-1, m-2, ... 1, 0, 1, ... */
static long triangle(long x, long m)
{
    long period = (m - 1) * 2, xm = x % period;
    return xm < m ? xm : period - xm;
}

/* 1 while the eye moves right, -1 while it moves left */
static long direction(long x, long m)
{
    long period = (m - 1) * 2, xm = x % period;
    return xm < m - 1 ? 1 : -1;
}

/* writes the color of the cell at col and returns its character (color 0: the empty bar) */
static int eye_char(long col, long pos, long dir, const char *color)
{
    long eye_end = pos + EYE_WIDTH - 1, t;
    if (!color) {
        fputs(BRIGHT_BLACK, stdout);
        return 0x2591;
    }
    if (col >= pos && col <= eye_end) {
        fputs(color, stdout);
        return 0x2588;                          /* the eye */
    }
    t = dir > 0 ? pos - col - 1 : col - eye_end - 1;    /* the distance behind the eye */
    if (t < 0 || t > 2) {
        fputs(BRIGHT_BLACK, stdout);
        return 0x2591;
    }
    fputs(color, stdout);
    return t == 0 ? 0x2593 : t == 1 ? 0x2592 : 0x2591;  /* the trail fades */
}

static void progress_bar(long total, long count, const char *color)
{
    long span = total - EYE_WIDTH + 1, pos = triangle(count, span), dir = direction(count, span), i;
    for (i = 0; i < total; i++) putchar(eye_char(i, pos, dir, color));
    fputs(RESET, stdout);
}

static void bar_demo(long delay, const char *color)
{
    long p;
    for (p = 0; p <= 150; p++) {
        fputs("Copying: ", stdout);
        progress_bar(SIZE, p, color);
        delay_ms(delay);
        fputs("\r", stdout);
    }
    fputs("Copying: ", stdout);
    progress_bar(SIZE, 150, 0);
    puts(" Done");
}

int main(void)
{
    fputs("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n", stdout);
    fputs(HIDE_CURSOR, stdout);
    bar_demo(100, BRIGHT_CYAN);
    bar_demo(80, BRIGHT_MAGENTA);
    bar_demo(60, BRIGHT_YELLOW);
    bar_demo(40, BRIGHT_RED);
    bar_demo(20, BRIGHT_GREEN);
    bar_demo(10, BRIGHT_BLUE);
    fputs(SHOW_CURSOR, stdout);
    return 0;
}
