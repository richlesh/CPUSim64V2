/*
 * Spinner.c
 *
 * Animates a text spinner ("Working: |", "/", "-", "\\") for 48 frames,
 * changing the color every 8 frames.
 * C11 version of documentation/projects/Spinner.asm.
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FRAME_COUNT 48
#define FRAME_DELAY 300         /* milliseconds */
#define FRAMES_PER_COLOR 8
#define NUM_COLORS 6

#define RESET "\x1b[0m"
#define HIDE_CURSOR "\x1b[?25l"
#define SHOW_CURSOR "\x1b[?25h"

static const char *const COLORS[NUM_COLORS] = {
    "\x1b[96m", "\x1b[95m", "\x1b[93m", "\x1b[91m", "\x1b[92m", "\x1b[94m"   /* bright cyan, magenta, yellow, red, green, blue */
};
static const char SPINNER[4] = { '|', '/', '-', '\\' };

/* waits ms milliseconds.  The C library has no sleep function, so this busy-waits on clock();
   with the environment variable NO_DELAY set it returns at once (for the tests). */
static void delay_ms(long ms)
{
    clock_t end;
    if (getenv("NO_DELAY")) return;
    end = clock() + (clock_t)(ms * (CLOCKS_PER_SEC / 1000));
    while (clock() < end) {}
}

int main(void)
{
    int i;
    fputs(HIDE_CURSOR, stdout);
    for (i = 0; i < FRAME_COUNT; i++) {
        fputs(COLORS[(i / FRAMES_PER_COLOR) % NUM_COLORS], stdout);
        fputs("Working: ", stdout);
        putchar(SPINNER[i % 4]);
        delay_ms(FRAME_DELAY);
        fputs("\r", stdout);
    }
    fputs(RESET, stdout);
    fputs(SHOW_CURSOR, stdout);
    puts("Working: Done!        ");
    return 0;
}
