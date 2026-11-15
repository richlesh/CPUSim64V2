/*
 * hashcode2.c
 *
 * The hash of hashcode.c, reading the file 8 bytes at a time: each block is
 * hashed byte by byte, and the bytes after the last full block are hashed
 * one at a time (and reported as "tail byte").
 * C11 version of documentation/projects/hashcode2.asm.
 *
 * Usage:
 *     hashcode2 [<filename>]
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>

static unsigned long hashcode(FILE *f)
{
    unsigned long hash = 0;
    unsigned char block[8];
    size_t n, i;
    while ((n = fread(block, 1, 8, f)) == 8)
        for (i = 0; i < 8; i++) hash = hash * 31 + block[i];
    for (i = 0; i < n; i++) {
        printf("tail byte: %02x\n", block[i]);
        hash = hash * 31 + block[i];
    }
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
