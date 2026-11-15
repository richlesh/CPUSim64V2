/*
 * hashcode2.cpp
 *
 * The hash of hashcode.cpp, reading the file 8 bytes at a time: each block is
 * hashed byte by byte, and the bytes after the last full block are hashed
 * one at a time (and reported as "tail byte").
 * C++ version of documentation/projects/hashcode2.asm.
 *
 * Usage:
 *     hashcode2 [<filename>]
 *
 * Author:   Richard Lesh
 */
#include <cstddef>
#include <cstdio>

static unsigned long hashcode(std::FILE *f)
{
    unsigned long hash = 0;
    unsigned char block[8];
    std::size_t n;
    while ((n = std::fread(block, 1, 8, f)) == 8)
        for (std::size_t i = 0; i < 8; i++) hash = hash * 31 + block[i];
    for (std::size_t i = 0; i < n; i++) {
        std::printf("tail byte: %02x\n", block[i]);
        hash = hash * 31 + block[i];
    }
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
