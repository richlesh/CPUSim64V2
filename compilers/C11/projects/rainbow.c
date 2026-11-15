/*
 * rainbow.c
 *
 * Shows the ANSI colors: a full-width bar in each of the 16 background
 * colors, then a line of text in each of the 16 foreground colors, plain,
 * bold and dim.
 * C11 version of documentation/projects/rainbow.asm.
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>

#define RESET "\x1b[0m"
#define BOLD "\x1b[1m"
#define DIM "\x1b[2m"
#define CLEAR_TO_EOL "\x1b[0K"

static const char *const FG[8] = {          /* black, red, green, yellow, blue, magenta, cyan, white */
    "\x1b[30m", "\x1b[31m", "\x1b[32m", "\x1b[33m", "\x1b[34m", "\x1b[35m", "\x1b[36m", "\x1b[37m"
};
static const char *const FG_BRIGHT[8] = {
    "\x1b[90m", "\x1b[91m", "\x1b[92m", "\x1b[93m", "\x1b[94m", "\x1b[95m", "\x1b[96m", "\x1b[97m"
};
static const char *const BG[16] = {
    "\x1b[40m", "\x1b[41m", "\x1b[42m", "\x1b[43m", "\x1b[44m", "\x1b[45m", "\x1b[46m", "\x1b[47m",
    "\x1b[100m", "\x1b[101m", "\x1b[102m", "\x1b[103m", "\x1b[104m", "\x1b[105m", "\x1b[106m", "\x1b[107m"
};

/* a line filled with the background color */
static void draw_bar(const char *bg)
{
    fputs(bg, stdout);
    fputs(CLEAR_TO_EOL, stdout);
    fputs(RESET, stdout);
    putchar('\n');
}

static void draw_text(const char *fg, const char *modifier)
{
    fputs(fg, stdout);
    fputs(modifier, stdout);
    fputs("Lorem ipsum dolor sit amet, consectetur adipiscing elit", stdout);
    fputs(RESET, stdout);
    putchar('\n');
}

/* a heading and the eight colors of the table with the modifier */
static void draw_group(const char *title, const char *const *colors, const char *modifier)
{
    int i;
    printf("\n%s\n", title);
    for (i = 0; i < 8; i++) draw_text(colors[i], modifier);
}

int main(void)
{
    int i;
    for (i = 0; i < 16; i++) draw_bar(BG[i]);
    draw_group("Regular", FG, "");
    draw_group("Bright", FG_BRIGHT, "");
    draw_group("Regular Bold", FG, BOLD);
    draw_group("Bright Bold", FG_BRIGHT, BOLD);
    draw_group("Regular Dim", FG, DIM);
    draw_group("Bright Dim", FG_BRIGHT, DIM);
    return 0;
}
