/*
 * Caesar.cpp
 *
 * Caesar cipher: shifts the 95 printable ASCII characters (' ' to '~') by
 * a key, wrapping around; other characters pass through unchanged.
 * C++ version of documentation/projects/Caesar.asm.
 *
 * Usage:
 *     Caesar <key> <input_file> <output_file>
 *         key in [1, 94] encrypts, key in [-94, -1] decrypts, 0 does nothing
 *
 * Author:   Richard Lesh
 * Original: 2025/11/04
 */
#include <cstdio>
#include <cstdlib>
#include <iostream>

static void caesar(long key, std::FILE *in, std::FILE *out)
{
    int c;
    while ((c = std::fgetc(in)) != EOF) {
        if (c >= ' ' && c <= '~') {
            long v = c - ' ' + key;
            while (v < 0) v += 95;
            while (v >= 95) v -= 95;
            c = (int)(v + ' ');
        }
        std::fputc(c, out);
    }
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        std::cout << "Syntax: Caesar <key> <input_file> <output_file>\n"
                  << "    key can be in the range [1,94] to encrypt.\n"
                  << "    key can be in the range [-94,-1] to decrypt.\n"
                  << "    key == 0 is a no-op" << std::endl;
        return 1;
    }
    std::FILE *in = std::fopen(argv[2], "r");
    if (!in) {
        std::cout << "Input file open failed!" << std::endl;
        return 3;
    }
    std::FILE *out = std::fopen(argv[3], "w");
    if (!out) {
        std::fclose(in);
        std::cout << "Output file creation failed!" << std::endl;
        return 2;
    }
    caesar(std::atol(argv[1]) % 95, in, out);
    std::fclose(out);
    std::fclose(in);
    return 0;
}
