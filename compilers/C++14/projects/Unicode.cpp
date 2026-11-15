/*
 * Unicode.cpp
 *
 * Prints ranges of Unicode characters, 32 per line: ASCII, Latin, math
 * symbols, box drawing, symbols and icons (emoji).  On CPUSim64 a char holds
 * a whole codepoint and the console writes it as UTF-8.
 * C++ version of documentation/projects/Unicode.asm.
 *
 * Author:   Richard Lesh
 */
#include <iostream>

static void print_range(long first, long last)
{
    for (long c = first; c <= last; c++) {
        std::cout << (char)c;                   // char holds a Unicode codepoint
        if ((c & 31) == 31) std::cout << '\n';
    }
}

int main()
{
    std::cout << "ASCII\n";
    print_range(0x20, 0x7f);
    std::cout << "Basic Multilingual\n";
    print_range(0xa0, 0x1ff);
    std::cout << "Math Symbols\n";
    print_range(0x2200, 0x22ff);
    std::cout << "Drawing\n";
    print_range(0x2500, 0x25ff);
    std::cout << "Symbols\n";
    print_range(0x2600, 0x27ff);
    std::cout << "Icons\n";
    print_range(0x1f300, 0x1f4ff);
    return 0;
}
