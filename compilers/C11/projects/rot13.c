/*
 * rot13.c
 *
 * Applies the ROT13 substitution cipher to a text file: each ASCII letter
 * is replaced by the letter 13 places later, wrapping around, so applying
 * it twice restores the text.  Other characters pass through unchanged.
 * C11 version of documentation/projects/rot13.asm.
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
#include <stdio.h>

static void rot13(FILE *in, FILE *out)
{
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= 'A' && c <= 'Z') c = 'A' + (c - 'A' + 13) % 26;
        else if (c >= 'a' && c <= 'z') c = 'a' + (c - 'a' + 13) % 26;
        fputc(c, out);
    }
}

int main(int argc, char **argv)
{
    FILE *in = stdin, *out = stdout;
    if (argc >= 2) {
        in = fopen(argv[1], "r");
        if (!in) {
            printf("Can't open %s for reading.\n", argv[1]);
            return 1;
        }
    }
    if (argc >= 3) {
        out = fopen(argv[2], "w");
        if (!out) {
            printf("Can't open %s for writing.\n", argv[2]);
            return 1;
        }
    }
    rot13(in, out);
    if (out != stdout) fclose(out);
    if (in != stdin) fclose(in);
    return 0;
}
