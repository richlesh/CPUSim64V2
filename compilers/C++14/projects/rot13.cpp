/*
 * rot13.cpp
 *
 * Applies the ROT13 substitution cipher to a text file: each ASCII letter
 * is replaced by the letter 13 places later, wrapping around, so applying
 * it twice restores the text.  Other characters pass through unchanged.
 * C++ version of documentation/projects/rot13.asm.
 *
 * Usage:
 *     rot13 [<input_file> [<output_file>]]     (STDIN / STDOUT by default)
 *
 * Example:
 *     > rot13 plain.txt cipher.txt
 *     > rot13 cipher.txt roundtrip.txt         (roundtrip.txt == plain.txt)
 *
 * Author:   Richard Lesh
 * Original: 2022/11/28
 */
#include <cstdio>
#include <iostream>

static void rot13(std::FILE *in, std::FILE *out)
{
    int c;
    while ((c = std::fgetc(in)) != EOF) {
        if (c >= 'A' && c <= 'Z') c = 'A' + (c - 'A' + 13) % 26;
        else if (c >= 'a' && c <= 'z') c = 'a' + (c - 'a' + 13) % 26;
        std::fputc(c, out);
    }
}

int main(int argc, char **argv)
{
    std::FILE *in = stdin, *out = stdout;
    if (argc >= 2) {
        in = std::fopen(argv[1], "r");
        if (!in) {
            std::cout << "Can't open " << argv[1] << " for reading.\n";
            return 1;
        }
    }
    if (argc >= 3) {
        out = std::fopen(argv[2], "w");
        if (!out) {
            std::cout << "Can't open " << argv[2] << " for writing.\n";
            return 1;
        }
    }
    rot13(in, out);
    if (out != stdout) std::fclose(out);
    if (in != stdin) std::fclose(in);
    return 0;
}
