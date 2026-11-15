/*
 * XORCipher.cpp
 *
 * A repeating-key XOR stream cipher for any file: each byte is XORed with
 * the next character of the keyword.  XOR is its own inverse, so the same
 * command decrypts.  The files are read and written as bytes.
 * Not secure: this is an educational example.
 * C++ version of documentation/projects/XORCipher.asm.
 *
 * Usage:
 *     XORCipher keyword input_file output_file
 *
 * Author:   Richard Lesh
 */
#include <cstdio>
#include <iostream>
#include <string>

static void xor_cipher(const std::string &key, std::FILE *in, std::FILE *out)
{
    std::size_t index = 0;
    int c;
    while ((c = std::fgetc(in)) != EOF) {
        std::fputc(c ^ key[index], out);
        if (++index >= key.size()) index = 0;
    }
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        std::cout << "Syntax: XORCipher keyword input_file output_file" << std::endl;
        return 0;
    }
    std::FILE *in = std::fopen(argv[2], "rb");
    if (!in) {
        std::cout << "Input file open failed!" << std::endl;
        return 0;
    }
    std::FILE *out = std::fopen(argv[3], "wb");
    if (!out) {
        std::fclose(in);
        std::cout << "Output file creation failed!" << std::endl;
        return 0;
    }
    xor_cipher(argv[1], in, out);
    std::fclose(out);
    std::fclose(in);
    return 0;
}
