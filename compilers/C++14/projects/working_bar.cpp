/*
 * working_bar.cpp
 *
 * Animates a "working" indicator: a dot (●) swings back and forth inside
 * brackets, easing in and out with a cosine, seven times with shorter
 * delays and different colors.
 * C++ version of documentation/projects/working_bar.asm (with the
 * working_bar function of system/console.asm).
 *
 * Author:   Richard Lesh
 */
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

static const long PROGRESS_SIZE = 40;
static const double PI = 3.14159265358979323846;

static const char *const RESET = "\x1b[0m";
static const char *const BRIGHT_RED = "\x1b[91m";
static const char *const BRIGHT_GREEN = "\x1b[92m";
static const char *const BRIGHT_YELLOW = "\x1b[93m";
static const char *const BRIGHT_BLUE = "\x1b[94m";
static const char *const BRIGHT_MAGENTA = "\x1b[95m";
static const char *const BRIGHT_CYAN = "\x1b[96m";
static const char *const BRIGHT_WHITE = "\x1b[97m";
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

// "[   ●      ]" with len cells; the dot is at (1 - cos(2 pi pct)) / 2 of the way (color null: no dot)
static void working_bar(long len, double pct, const char *color)
{
    double f = -(((std::cos(PI * 2 * pct) + 1) / 2) - 1);
    long bar_len = (long)std::round(f * (len - 1)) + 1, i;
    std::cout << RESET << '[';
    for (i = 0; i < bar_len - 1; i++) std::cout << ' ';
    if (color) std::cout << color << (char)0x25CF << RESET;
    else std::cout << ' ';
    for (i++; i < len; i++) std::cout << ' ';
    std::cout << ']';
}

static void bar_demo(long delay, const char *color)
{
    double p = 0;
    for (long i = 0; i <= 300; i++) {
        std::cout << "Processing: ";
        p = i / 100.0;
        working_bar(PROGRESS_SIZE, p, color);
        delay_ms(delay);
        std::cout << '\r';
    }
    std::cout << "Processing: ";
    working_bar(PROGRESS_SIZE, p, nullptr);
    std::cout << " Done\n";
}

int main()
{
    std::cout << HIDE_CURSOR;
    bar_demo(60, BRIGHT_CYAN);
    bar_demo(50, BRIGHT_MAGENTA);
    bar_demo(40, BRIGHT_YELLOW);
    bar_demo(30, BRIGHT_RED);
    bar_demo(20, BRIGHT_GREEN);
    bar_demo(10, BRIGHT_BLUE);
    bar_demo(5, BRIGHT_WHITE);
    std::cout << SHOW_CURSOR;
    return 0;
}
