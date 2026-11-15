/*
 * Vigenere.cpp
 *
 * Vigenere cipher: each of the 94 printable characters '!' to '~' of the
 * input is shifted by the next character of a repeating keyword (its offset
 * from '!'); spaces, newlines and other characters pass through unchanged
 * and do not use a key character.
 * C++ version of documentation/projects/Vigenere.asm.
 *
 * Usage:
 *     Vigenere keyword  input_file output_file   (encrypt)
 *     Vigenere -keyword input_file output_file   (decrypt)
 *
 * Author:   Richard Lesh
 */
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

static void vigenere(std::string key, std::FILE *in, std::FILE *out)
{
    bool decrypt = false;
    if (!key.empty() && key[0] == '-') {
        decrypt = true;
        key.erase(0, 1);
    }
    long keylen = (long)key.size(), index = 0;
    if (keylen == 0) return;
    int c;
    while ((c = std::fgetc(in)) != EOF) {
        if (c > ' ' && c <= '~') {
            long offset = key[index] - '!';
            long v = c - '!' + (decrypt ? -offset : offset);
            while (v < 0) v += 94;
            while (v >= 94) v -= 94;
            c = (int)(v + '!');
            if (++index >= keylen) index = 0;
        }
        std::fputc(c, out);
    }
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        std::cout << "Syntax: Vigenere [-]keyword input_file output_file\n"
                  << "    a keyword starting with '-' decrypts" << std::endl;
        return 1;
    }
    std::FILE *in = std::fopen(argv[2], "r");
    if (!in) {
        std::cout << "Input file open failed!" << std::endl;
        return 2;
    }
    std::FILE *out = std::fopen(argv[3], "w");
    if (!out) {
        std::fclose(in);
        std::cout << "Output file creation failed!" << std::endl;
        return 3;
    }
    vigenere(argv[1], in, out);
    std::fclose(out);
    std::fclose(in);
    return 0;
}
