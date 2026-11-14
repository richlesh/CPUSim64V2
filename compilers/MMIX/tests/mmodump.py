#!/usr/bin/env python3
"""Loads an MMIX .mmo object file as the MMIX loader does and prints its
memory image and initial global registers in the format of `mmixc --dump`:

    G <rG>
    $<n> <16 hex digits>                 global registers $rG..$255
    <16 hex digit address> <8 hex>       every nonzero tetrabyte, ascending

Used by the tests to compare the MMIXAL front end with the real mmixal.
"""
import sys


def load(path):
    data = open(path, "rb").read()
    tetras = [int.from_bytes(data[i:i + 4], "big") for i in range(0, len(data) - 3, 4)]
    mem = {}           # tetra address -> value (xor-loaded)
    loc = 0
    i = 0
    g = 255
    gregs = {}

    def put(addr, v):
        a = addr & ~3 & 0xFFFFFFFFFFFFFFFF
        mem[a] = mem.get(a, 0) ^ v

    while i < len(tetras):
        t = tetras[i]
        i += 1
        if (t >> 24) != 0x98:
            put(loc, t)
            loc = (loc & ~3) + 4
            continue
        x, y, z = (t >> 16) & 0xFF, (t >> 8) & 0xFF, t & 0xFF
        yz = t & 0xFFFF
        if x == 0x0:            # lop_quote
            put(loc, tetras[i]); i += 1
            loc = (loc & ~3) + 4
        elif x == 0x1:          # lop_loc
            v = 0
            for _ in range(z):
                v = (v << 32) | tetras[i]; i += 1
            loc = (v + (y << 56)) & 0xFFFFFFFFFFFFFFFF
        elif x == 0x2:          # lop_skip
            loc += yz
        elif x == 0x3:          # lop_fixo
            v = 0
            for _ in range(z):
                v = (v << 32) | tetras[i]; i += 1
            p = (v + (y << 56)) & 0xFFFFFFFFFFFFFFFF
            put(p, (loc >> 32) & 0xFFFFFFFF)
            put(p + 4, loc & 0xFFFFFFFF)
        elif x == 0x4:          # lop_fixr
            p = loc - 4 * yz
            put(p, yz)
        elif x == 0x5:          # lop_fixrx
            d = tetras[i]; i += 1
            delta = d & 0xFFFFFF
            if (d >> 24) == 1:
                delta -= 1 << z
            put(loc - 4 * delta, d)
        elif x == 0x6:          # lop_file
            i += z
        elif x == 0x7:          # lop_line
            pass
        elif x == 0x8:          # lop_spec: skip special data
            while i < len(tetras) and ((tetras[i] >> 24) != 0x98 or ((tetras[i] >> 16) & 0xFF) == 0):
                if (tetras[i] >> 24) == 0x98:
                    i += 1
                i += 1
        elif x == 0x9:          # lop_pre
            i += z
        elif x == 0xA:          # lop_post
            g = z
            for r in range(g, 256):
                gregs[r] = (tetras[i] << 32) | tetras[i + 1]
                i += 2
            break
    return g, gregs, mem


def main():
    g, gregs, mem = load(sys.argv[1])
    out = ["G %d" % g]
    for r in range(g, 256):
        out.append("$%d %016x" % (r, gregs[r]))
    for a in sorted(mem):
        if mem[a]:
            out.append("%016x %08x" % (a, mem[a]))
    print("\n".join(out))


if __name__ == "__main__":
    main()
