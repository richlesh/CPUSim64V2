/*
 * Spinner.cpp
 *
 * Animates a text spinner ("Working: |", "/", "-", "\\") for 48 frames,
 * changing the color every 8 frames.
 * C++ version of documentation/projects/Spinner.asm.
 *
 * Author:   Richard Lesh
 */
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

static const int FRAME_COUNT = 48;
static const int FRAME_DELAY = 300;         // milliseconds
static const int FRAMES_PER_COLOR = 8;
static const int NUM_COLORS = 6;

static const char *const RESET = "\x1b[0m";
static const char *const HIDE_CURSOR = "\x1b[?25l";
static const char *const SHOW_CURSOR = "\x1b[?25h";

// bright cyan, magenta, yellow, red, green, blue
static const std::vector<const char *> COLORS = {
    "\x1b[96m", "\x1b[95m", "\x1b[93m", "\x1b[91m", "\x1b[92m", "\x1b[94m"
};
static const char SPINNER[4] = { '|', '/', '-', '\\' };

// waits ms milliseconds.  The C library has no sleep function, so this busy-waits on clock();
// with the environment variable NO_DELAY set it returns at once (for the tests).
static void delay_ms(long ms)
{
    if (std::getenv("NO_DELAY")) return;
    std::clock_t end = std::clock() + (std::clock_t)(ms * (CLOCKS_PER_SEC / 1000));
    while (std::clock() < end) {}
}

int main()
{
    std::cout << HIDE_CURSOR;
    for (int i = 0; i < FRAME_COUNT; i++) {
        std::cout << COLORS[(i / FRAMES_PER_COLOR) % NUM_COLORS];
        std::cout << "Working: " << SPINNER[i % 4];
        delay_ms(FRAME_DELAY);
        std::cout << '\r';
    }
    std::cout << RESET << SHOW_CURSOR;
    std::cout << "Working: Done!        \n";
    return 0;
}
