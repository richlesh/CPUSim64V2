/*
 * Caesar.c
 *
 * Caesar cipher: shifts the 95 printable ASCII characters (' ' to '~') by
 * a key, wrapping around; other characters pass through unchanged.
 * C11 version of documentation/projects/Caesar.asm.
 *
 * Usage:
 *     Caesar <key> <input_file> <output_file>
 *         key in [1, 94] encrypts, key in [-94, -1] decrypts, 0 does nothing
 *
 * Author:   Richard Lesh
 * Original: 2025/11/04
 */
#include <stdio.h>
#include <stdlib.h>

static void caesar(long key, FILE *in, FILE *out)
{
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= ' ' && c <= '~') {
            long v = c - ' ' + key;
            while (v < 0) v += 95;
            while (v >= 95) v -= 95;
            c = (int)(v + ' ');
        }
        fputc(c, out);
    }
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    if (argc < 4) {
        puts("Syntax: Caesar <key> <input_file> <output_file>");
        puts("    key can be in the range [1,94] to encrypt.");
        puts("    key can be in the range [-94,-1] to decrypt.");
        puts("    key == 0 is a no-op");
        return 1;
    }
    in = fopen(argv[2], "r");
    if (!in) {
        puts("Input file open failed!");
        return 3;
    }
    out = fopen(argv[3], "w");
    if (!out) {
        fclose(in);
        puts("Output file creation failed!");
        return 2;
    }
    caesar(atol(argv[1]) % 95, in, out);
    fclose(out);
    fclose(in);
    return 0;
}
