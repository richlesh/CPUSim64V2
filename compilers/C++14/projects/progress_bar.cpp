/*
 * progress_bar.cpp
 *
 * Animates a percentage progress bar ("Copying: ████░░░░  42%") six times,
 * with steps of 1% to 6% and a different color each time.  The bar is
 * redrawn on the same line with a carriage return; the cursor is hidden
 * while it runs.
 * C++ version of documentation/projects/progress_bar.asm (with the
 * progress_bar_percent function of system/console.asm).
 *
 * Author:   Richard Lesh
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>

static const long SIZE = 74;

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

// a bar of total cells, the fraction pct of them filled, then the percentage (right aligned, omitted at 100%)
static void progress_bar_percent(long total, double pct, const char *color)
{
    if (pct < 0.0) pct = 0.0;
    if (pct > 1.0) pct = 1.0;
    long filled = std::lround(total * pct);
    long pct_int = std::lround(pct * 100);
    std::cout << color;
    for (long i = 0; i < filled; i++) std::cout << (char)0x2588;           // full block
    std::cout << BRIGHT_BLACK;
    for (long i = filled + 1; i <= total; i++) std::cout << (char)0x2591;  // light shade
    std::cout << RESET;
    if (pct_int < 100) std::cout << ' ' << std::setw(3) << pct_int << '%';
}

static void bar_demo(long incr, const char *color)
{
    for (long p = 0; p <= 100; p += incr) {
        std::cout << "Copying: ";
        progress_bar_percent(SIZE, p / 100.0, color);
        delay_ms(100);
        std::cout << '\r';
    }
    std::cout << "Copying: ";
    progress_bar_percent(SIZE, 1.0, RESET);
    std::cout << " Done\n";
}

int main()
{
    std::cout << "Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n";
    std::cout << HIDE_CURSOR;
    bar_demo(1, BRIGHT_CYAN);
    bar_demo(2, BRIGHT_MAGENTA);
    bar_demo(3, BRIGHT_YELLOW);
    bar_demo(4, BRIGHT_RED);
    bar_demo(5, BRIGHT_GREEN);
    bar_demo(6, BRIGHT_BLUE);
    std::cout << SHOW_CURSOR;
    return 0;
}
