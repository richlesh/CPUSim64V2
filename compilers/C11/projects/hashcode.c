/*
 * hashcode.c
 *
 * Computes a 64-bit polynomial rolling hash (hash = hash * 31 + byte, as
 * Java's String.hashCode but over bytes and modulo 2^64) of a file or of
 * STDIN and prints it in hexadecimal.
 * C11 version of documentation/projects/hashcode.asm.
 *
 * Usage:
 *     hashcode [<filename>]
 *
 * Output:
 *     "<16 hex digits> <filename>"   (or only the hash for STDIN)
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>

static unsigned long hashcode(FILE *f)
{
    unsigned long hash = 0;
    int c;
    while ((c = fgetc(f)) != EOF) hash = hash * 31 + (unsigned long)c;
    return hash;
}

int main(int argc, char **argv)
{
    FILE *f;
    if (argc < 2) {
        printf("%016lx\n", hashcode(stdin));
        return 0;
    }
    f = fopen(argv[1], "rb");
    if (!f) {
        printf("Failed to open: %s\n", argv[1]);
        return 2;
    }
    printf("%016lx %s\n", hashcode(f), argv[1]);
    fclose(f);
    return 0;
}
