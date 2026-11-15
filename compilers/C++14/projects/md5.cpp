/*
 * md5.cpp
 *
 * Computes the MD5 message digest (RFC 1321) of a file or of STDIN and
 * prints it as 32 hex digits (upper case, as the assembly version).  The
 * input is read as bytes in 64-byte blocks; the last block is padded with a
 * 1 bit, zeros and the length in bits.
 * C++ version of documentation/projects/md5.asm.
 *
 * Usage:
 *     md5 [<filename>]
 *
 * Output (after an empty line):
 *     "<32 hex digits> <filename>"
 *
 * Author:   Richard Lesh
 */
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>

typedef unsigned long u32;          // 32-bit values in 64-bit words: results are masked
static const u32 M32 = 0xFFFFFFFFul;

static const int SHIFT[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

// K[i] = floor(2^32 * |sin(i + 1)|)
static const u32 K[64] = {
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE, 0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE, 0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA, 0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED, 0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C, 0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05, 0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039, 0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1, 0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

class MD5 {
public:
    MD5() : h{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476} {}

    // hashes everything in the file
    void update(std::FILE *f)
    {
        unsigned char blk[64];
        unsigned long len = 0;
        std::size_t n;
        while ((n = std::fread(blk, 1, 64, f)) == 64) {
            block(blk);
            len += 64;
        }
        len += n;
        // the padding: 0x80, zeros up to 56 bytes mod 64, the length in bits (little endian)
        blk[n++] = 0x80;
        if (n > 56) {
            std::memset(blk + n, 0, 64 - n);
            block(blk);
            n = 0;
        }
        std::memset(blk + n, 0, 56 - n);
        unsigned long bits = len * 8;
        for (int i = 0; i < 8; i++) blk[56 + i] = (unsigned char)((bits >> (8 * i)) & 0xFF);
        block(blk);
    }

    // prints the digest: the bytes of each word, lowest first
    void print_digest() const
    {
        std::cout << '\n';
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) std::printf("%02lX", (h[i] >> (8 * j)) & 0xFF);
    }

private:
    u32 h[4];

    // processes one 64-byte block
    void block(const unsigned char *blk)
    {
        u32 M[16], a = h[0], b = h[1], c = h[2], d = h[3];
        for (int i = 0; i < 16; i++)
            M[i] = (u32)blk[4 * i] | ((u32)blk[4 * i + 1] << 8) | ((u32)blk[4 * i + 2] << 16) | ((u32)blk[4 * i + 3] << 24);
        for (int i = 0; i < 64; i++) {
            u32 F;
            int g;
            if (i < 16) {
                F = ((c ^ d) & b) ^ d;
                g = i;
            } else if (i < 32) {
                F = ((b ^ c) & d) ^ c;
                g = (5 * i + 1) % 16;
            } else if (i < 48) {
                F = b ^ c ^ d;
                g = (3 * i + 5) % 16;
            } else {
                F = c ^ (b | (~d & M32));
                g = (7 * i) % 16;
            }
            F = (F + a + K[i] + M[g]) & M32;
            a = d;
            d = c;
            c = b;
            b = (b + (((F << SHIFT[i]) | (F >> (32 - SHIFT[i]))) & M32)) & M32;
        }
        h[0] = (h[0] + a) & M32;
        h[1] = (h[1] + b) & M32;
        h[2] = (h[2] + c) & M32;
        h[3] = (h[3] + d) & M32;
    }
};

int main(int argc, char **argv)
{
    MD5 md5;
    if (argc < 2) {
        md5.update(stdin);
        md5.print_digest();
        std::cout << '\n';
        return 0;
    }
    std::FILE *f = std::fopen(argv[1], "rb");
    if (!f) {
        std::cout << "File open failed!" << std::endl;
        return 1;
    }
    md5.update(f);
    md5.print_digest();
    std::cout << ' ' << argv[1] << '\n';
    std::fclose(f);
    return 0;
}
