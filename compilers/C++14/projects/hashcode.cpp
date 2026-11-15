/*
 * hashcode.cpp
 *
 * Computes a 64-bit polynomial rolling hash (hash = hash * 31 + byte, as
 * Java's String.hashCode but over bytes and modulo 2^64) of a file or of
 * STDIN and prints it in hexadecimal.
 * C++ version of documentation/projects/hashcode.asm.
 *
 * Usage:
 *     hashcode [<filename>]
 *
 * Output:
 *     "<16 hex digits> <filename>"   (or only the hash for STDIN)
 *
 * Author:   Richard Lesh
 */
#include <cstdio>

static unsigned long hashcode(std::FILE *f)
{
    unsigned long hash = 0;
    int c;
    while ((c = std::fgetc(f)) != EOF) hash = hash * 31 + (unsigned long)c;
    return hash;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::printf("%016lx\n", hashcode(stdin));
        return 0;
    }
    std::FILE *f = std::fopen(argv[1], "rb");
    if (!f) {
        std::printf("Failed to open: %s\n", argv[1]);
        return 2;
    }
    std::printf("%016lx %s\n", hashcode(f), argv[1]);
    std::fclose(f);
    return 0;
}
