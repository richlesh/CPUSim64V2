/*
 * Base64.cpp
 *
 * Base64 encoder and decoder for files.  Encoding reads the input as bytes
 * and writes lines of 76 characters (57 input bytes); decoding ignores the
 * characters outside the alphabet and stops at the padding character.
 * C++ version of documentation/projects/Base64.asm.
 *
 * Usage:
 *     Base64 <e|d> <input_file> <output_file>
 *
 * Author:   Richard Lesh
 * Original: 2025/11/17
 */
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

namespace {

const char PAD_CHAR = '=';
const std::string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// the value of a Base64 character: 0-63, 64 for the padding, -1 for anything else
int decode_value(int c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == PAD_CHAR) return 64;
    return -1;
}

void encode(std::FILE *in, std::FILE *out)
{
    long index = 0;
    int c, buffer = 0, line_remainder = 0;
    while ((c = std::fgetc(in)) != EOF) {
        int which = index % 3;
        if (which == 0) {
            std::fputc(ALPHABET[c >> 2], out);
            buffer = (c & 0x3) << 8;
        } else if (which == 1) {
            buffer |= c;
            std::fputc(ALPHABET[buffer >> 4], out);
            buffer = (buffer & 0xF) << 8;
        } else {
            buffer |= c;
            std::fputc(ALPHABET[buffer >> 6], out);
            std::fputc(ALPHABET[buffer & 0x3F], out);
        }
        index++;
        line_remainder = index % 57;
        if (line_remainder == 0) std::fputc('\n', out);
    }
    if (index % 3 == 1) {
        std::fputc(ALPHABET[buffer >> 4], out);
        std::fputc(PAD_CHAR, out);
        std::fputc(PAD_CHAR, out);
    } else if (index % 3 == 2) {
        std::fputc(ALPHABET[buffer >> 6], out);
        std::fputc(PAD_CHAR, out);
    }
    if (line_remainder != 0) std::fputc('\n', out);
}

void decode(std::FILE *in, std::FILE *out)
{
    long index = 0;
    int c, buffer = 0;
    while ((c = std::fgetc(in)) != EOF) {
        int v = decode_value(c);
        if (v == 64) break;                 // the padding ends the data
        if (v < 0) continue;                // a newline or another character
        int which = index % 4;
        if (which == 0) {
            buffer = v;
        } else if (which == 1) {
            buffer = (buffer << 6) | v;
            std::fputc(buffer >> 4, out);
            buffer &= 0xF;
        } else if (which == 2) {
            buffer = (buffer << 6) | v;
            std::fputc(buffer >> 2, out);
            buffer &= 0x3;
        } else {
            buffer = (buffer << 6) | v;
            std::fputc(buffer, out);
        }
        index++;
    }
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc < 4) {
        std::cout << "Syntax: Base64 <e|d> <input_file> <output_file>" << std::endl;
        return 1;
    }
    int mode = std::tolower(argv[1][0]);
    if (mode != 'e' && mode != 'd') {
        std::cout << "Mode can only be E or D!" << std::endl;
        return 1;
    }
    // the binary side is read or written as bytes, the Base64 side as text
    std::FILE *in = std::fopen(argv[2], mode == 'e' ? "rb" : "r");
    if (!in) {
        std::cout << "Input file open failed!" << std::endl;
        return 2;
    }
    std::FILE *out = std::fopen(argv[3], mode == 'e' ? "w" : "wb");
    if (!out) {
        std::fclose(in);
        std::cout << "Output file creation failed!" << std::endl;
        return 3;
    }
    if (mode == 'e') encode(in, out);
    else decode(in, out);
    std::fclose(out);
    std::fclose(in);
    return 0;
}
