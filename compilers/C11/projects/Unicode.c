/*
 * Unicode.c
 *
 * Prints ranges of Unicode characters, 32 per line: ASCII, Latin, math
 * symbols, box drawing, symbols and icons (emoji).  On CPUSim64 a char holds
 * a whole codepoint and the console writes it as UTF-8.
 * C11 version of documentation/projects/Unicode.asm.
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>

static void print_range(long first, long last)
{
    long c;
    for (c = first; c <= last; c++) {
        putchar((int)c);
        if ((c & 31) == 31) putchar('\n');
    }
}

int main(void)
{
    puts("ASCII");
    print_range(0x20, 0x7f);
    puts("Basic Multilingual");
    print_range(0xa0, 0x1ff);
    puts("Math Symbols");
    print_range(0x2200, 0x22ff);
    puts("Drawing");
    print_range(0x2500, 0x25ff);
    puts("Symbols");
    print_range(0x2600, 0x27ff);
    puts("Icons");
    print_range(0x1f300, 0x1f4ff);
    return 0;
}
