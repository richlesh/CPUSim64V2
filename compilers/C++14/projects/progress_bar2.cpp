/*
 * progress_bar2.cpp
 *
 * Animates an indeterminate progress bar: a three-cell "eye" with a fading
 * trail (▓▒░) bounces back and forth in a 40-cell bar, six times with
 * shorter delays and different colors.
 * C++ version of documentation/projects/progress_bar2.asm (with the
 * progress_bar function of system/console.asm).
 *
 * Author:   Richard Lesh
 */
#include <cstdlib>
#include <ctime>
#include <iostream>

static const long SIZE = 40;
static const long EYE_WIDTH = 3;

static const char *const RESET = "\x1b[0m";
static const char *const BRIGHT_BLACK = "\x1b[90m";
static const char *const BRIGHT_RED = "\x1b[91m";
static const char *const BRIGHT_GREEN = "\x1b[92m";
static const char *const BRIGHT_YELLOW = "\x1b[93m";
static const char *const BRIGHT_BLUE = "\x1b[94m";
static const char *const BRIGHT_MAGENTA = "\x1b[95m";
static const char *const BRIGHT_CYAN = "\x1b[96m";
static const char *const HIDE_CURSOR = "\x1b[?25l";
static const char *const SHOW_CURSOR = "\x1b[?25h";

// waits ms milliseconds.  The C library has no sleep function, so this busy-waits on clock();
// with the environment variable NO_DELAY set it returns at once (for the tests).
static void delay_ms(long ms)
{
    if (std::getenv("NO_DELAY")) return;
    std::clock_t end = std::clock() + (std::clock_t)(ms * (CLOCKS_PER_SEC / 1000));
    while (std::clock() < end) {}
}

// the position of the eye at step x: 0, 1, ... m-1, m-2, ... 1, 0, 1, ...
static long triangle(long x, long m)
{
    long period = (m - 1) * 2, xm = x % period;
    return xm < m ? xm : period - xm;
}

// 1 while the eye moves right, -1 while it moves left
static long direction(long x, long m)
{
    long period = (m - 1) * 2, xm = x % period;
    return xm < m - 1 ? 1 : -1;
}

// writes the color of the cell at col and returns its character (color null: the empty bar)
static int eye_char(long col, long pos, long dir, const char *color)
{
    long eye_end = pos + EYE_WIDTH - 1;
    if (!color) {
        std::cout << BRIGHT_BLACK;
        return 0x2591;
    }
    if (col >= pos && col <= eye_end) {
        std::cout << color;
        return 0x2588;                          // the eye
    }
    long t = dir > 0 ? pos - col - 1 : col - eye_end - 1;    // the distance behind the eye
    if (t < 0 || t > 2) {
        std::cout << BRIGHT_BLACK;
        return 0x2591;
    }
    std::cout << color;
    return t == 0 ? 0x2593 : t == 1 ? 0x2592 : 0x2591;      // the trail fades
}

static void progress_bar(long total, long count, const char *color)
{
    long span = total - EYE_WIDTH + 1, pos = triangle(count, span), dir = direction(count, span);
    for (long i = 0; i < total; i++) std::cout << (char)eye_char(i, pos, dir, color);
    std::cout << RESET;
}

static void bar_demo(long delay, const char *color)
{
    for (long p = 0; p <= 150; p++) {
        std::cout << "Copying: ";
        progress_bar(SIZE, p, color);
        delay_ms(delay);
        std::cout << '\r';
    }
    std::cout << "Copying: ";
    progress_bar(SIZE, 150, nullptr);
    std::cout << " Done\n";
}

int main()
{
    std::cout << "Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n";
    std::cout << HIDE_CURSOR;
    bar_demo(100, BRIGHT_CYAN);
    bar_demo(80, BRIGHT_MAGENTA);
    bar_demo(60, BRIGHT_YELLOW);
    bar_demo(40, BRIGHT_RED);
    bar_demo(20, BRIGHT_GREEN);
    bar_demo(10, BRIGHT_BLUE);
    std::cout << SHOW_CURSOR;
    return 0;
}
