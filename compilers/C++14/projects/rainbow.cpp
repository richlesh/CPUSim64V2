/*
 * rainbow.cpp
 *
 * Shows the ANSI colors: a full-width bar in each of the 16 background
 * colors, then a line of text in each of the 16 foreground colors, plain,
 * bold and dim.
 * C++ version of documentation/projects/rainbow.asm.
 *
 * Author:   Richard Lesh
 */
#include <iostream>
#include <vector>

static const char *const RESET = "\x1b[0m";
static const char *const BOLD = "\x1b[1m";
static const char *const DIM = "\x1b[2m";
static const char *const CLEAR_TO_EOL = "\x1b[0K";

// black, red, green, yellow, blue, magenta, cyan, white
static const std::vector<const char *> FG = {
    "\x1b[30m", "\x1b[31m", "\x1b[32m", "\x1b[33m", "\x1b[34m", "\x1b[35m", "\x1b[36m", "\x1b[37m"
};
static const std::vector<const char *> FG_BRIGHT = {
    "\x1b[90m", "\x1b[91m", "\x1b[92m", "\x1b[93m", "\x1b[94m", "\x1b[95m", "\x1b[96m", "\x1b[97m"
};
static const std::vector<const char *> BG = {
    "\x1b[40m", "\x1b[41m", "\x1b[42m", "\x1b[43m", "\x1b[44m", "\x1b[45m", "\x1b[46m", "\x1b[47m",
    "\x1b[100m", "\x1b[101m", "\x1b[102m", "\x1b[103m", "\x1b[104m", "\x1b[105m", "\x1b[106m", "\x1b[107m"
};

// a line filled with the background color
static void draw_bar(const char *bg)
{
    std::cout << bg << CLEAR_TO_EOL << RESET << '\n';
}

static void draw_text(const char *fg, const char *modifier)
{
    std::cout << fg << modifier << "Lorem ipsum dolor sit amet, consectetur adipiscing elit" << RESET << '\n';
}

// a heading and the eight colors of the table with the modifier
static void draw_group(const char *title, const std::vector<const char *> &colors, const char *modifier)
{
    std::cout << '\n' << title << '\n';
    for (const char *color : colors) draw_text(color, modifier);
}

int main()
{
    for (const char *bg : BG) draw_bar(bg);
    draw_group("Regular", FG, "");
    draw_group("Bright", FG_BRIGHT, "");
    draw_group("Regular Bold", FG, BOLD);
    draw_group("Bright Bold", FG_BRIGHT, BOLD);
    draw_group("Regular Dim", FG, DIM);
    draw_group("Bright Dim", FG_BRIGHT, DIM);
    return 0;
}
