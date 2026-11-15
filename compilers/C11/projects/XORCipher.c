/*
 * XORCipher.c
 *
 * A repeating-key XOR stream cipher for any file: each byte is XORed with
 * the next character of the keyword.  XOR is its own inverse, so the same
 * command decrypts.  The files are read and written as bytes.
 * Not secure: this is an educational example.
 * C11 version of documentation/projects/XORCipher.asm.
 *
 * Usage:
 *     XORCipher keyword input_file output_file
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <string.h>

static void xor_cipher(const char *key, FILE *in, FILE *out)
{
    long keylen = (long)strlen(key), index = 0;
    int c;
    while ((c = fgetc(in)) != EOF) {
        fputc(c ^ key[index], out);
        if (++index >= keylen) index = 0;
    }
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    if (argc < 4) {
        puts("Syntax: XORCipher keyword input_file output_file");
        return 0;
    }
    in = fopen(argv[2], "rb");
    if (!in) {
        puts("Input file open failed!");
        return 0;
    }
    out = fopen(argv[3], "wb");
    if (!out) {
        fclose(in);
        puts("Output file creation failed!");
        return 0;
    }
    xor_cipher(argv[1], in, out);
    fclose(out);
    fclose(in);
    return 0;
}
