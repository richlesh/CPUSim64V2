/*
 * Base64.c
 *
 * Base64 encoder and decoder for files.  Encoding reads the input as bytes
 * and writes lines of 76 characters (57 input bytes); decoding ignores the
 * characters outside the alphabet and stops at the padding character.
 * C11 version of documentation/projects/Base64.asm.
 *
 * Usage:
 *     Base64 <e|d> <input_file> <output_file>
 *
 * Author:   Richard Lesh
 * Original: 2025/11/17
 */
#include <stdio.h>
#include <ctype.h>

#define PAD_CHAR '='

static const char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* the value of a Base64 character: 0-63, 64 for the padding, -1 for anything else */
static int decode_value(int c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == PAD_CHAR) return 64;
    return -1;
}

static void encode(FILE *in, FILE *out)
{
    long index = 0;
    int c, buffer = 0, line_remainder = 0;
    while ((c = fgetc(in)) != EOF) {
        int which = index % 3;
        if (which == 0) {
            fputc(ALPHABET[c >> 2], out);
            buffer = (c & 0x3) << 8;
        } else if (which == 1) {
            buffer |= c;
            fputc(ALPHABET[buffer >> 4], out);
            buffer = (buffer & 0xF) << 8;
        } else {
            buffer |= c;
            fputc(ALPHABET[buffer >> 6], out);
            fputc(ALPHABET[buffer & 0x3F], out);
        }
        index++;
        line_remainder = index % 57;
        if (line_remainder == 0) fputc('\n', out);
    }
    if (index % 3 == 1) {
        fputc(ALPHABET[buffer >> 4], out);
        fputc(PAD_CHAR, out);
        fputc(PAD_CHAR, out);
    } else if (index % 3 == 2) {
        fputc(ALPHABET[buffer >> 6], out);
        fputc(PAD_CHAR, out);
    }
    if (line_remainder != 0) fputc('\n', out);
}

static void decode(FILE *in, FILE *out)
{
    long index = 0;
    int c, buffer = 0;
    while ((c = fgetc(in)) != EOF) {
        int v = decode_value(c), which;
        if (v == 64) break;                 /* the padding ends the data */
        if (v < 0) continue;                /* a newline or another character */
        which = index % 4;
        if (which == 0) buffer = v;
        else if (which == 1) {
            buffer = (buffer << 6) | v;
            fputc(buffer >> 4, out);
            buffer &= 0xF;
        } else if (which == 2) {
            buffer = (buffer << 6) | v;
            fputc(buffer >> 2, out);
            buffer &= 0x3;
        } else {
            buffer = (buffer << 6) | v;
            fputc(buffer, out);
        }
        index++;
    }
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    int mode;
    if (argc < 4) {
        puts("Syntax: Base64 <e|d> <input_file> <output_file>");
        return 1;
    }
    mode = tolower(argv[1][0]);
    if (mode != 'e' && mode != 'd') {
        puts("Mode can only be E or D!");
        return 1;
    }
    /* the binary side is read or written as bytes, the Base64 side as text */
    in = fopen(argv[2], mode == 'e' ? "rb" : "r");
    if (!in) {
        puts("Input file open failed!");
        return 2;
    }
    out = fopen(argv[3], mode == 'e' ? "w" : "wb");
    if (!out) {
        fclose(in);
        puts("Output file creation failed!");
        return 3;
    }
    if (mode == 'e') encode(in, out);
    else decode(in, out);
    fclose(out);
    fclose(in);
    return 0;
}
