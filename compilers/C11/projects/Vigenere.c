/*
 * Vigenere.c
 *
 * Vigenere cipher: each of the 94 printable characters '!' to '~' of the
 * input is shifted by the next character of a repeating keyword (its offset
 * from '!'); spaces, newlines and other characters pass through unchanged
 * and do not use a key character.
 * C11 version of documentation/projects/Vigenere.asm.
 *
 * Usage:
 *     Vigenere keyword  input_file output_file   (encrypt)
 *     Vigenere -keyword input_file output_file   (decrypt)
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <string.h>

static void vigenere(const char *key, FILE *in, FILE *out)
{
    int decrypt = 0, c;
    long keylen, index = 0;
    if (key[0] == '-') {
        decrypt = 1;
        key++;
    }
    keylen = (long)strlen(key);
    if (keylen == 0) return;
    while ((c = fgetc(in)) != EOF) {
        if (c > ' ' && c <= '~') {
            long offset = key[index] - '!';
            long v = c - '!' + (decrypt ? -offset : offset);
            while (v < 0) v += 94;
            while (v >= 94) v -= 94;
            c = (int)(v + '!');
            if (++index >= keylen) index = 0;
        }
        fputc(c, out);
    }
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    if (argc < 4) {
        puts("Syntax: Vigenere [-]keyword input_file output_file");
        puts("    a keyword starting with '-' decrypts");
        return 1;
    }
    in = fopen(argv[2], "r");
    if (!in) {
        puts("Input file open failed!");
        return 2;
    }
    out = fopen(argv[3], "w");
    if (!out) {
        fclose(in);
        puts("Output file creation failed!");
        return 3;
    }
    vigenere(argv[1], in, out);
    fclose(out);
    fclose(in);
    return 0;
}
