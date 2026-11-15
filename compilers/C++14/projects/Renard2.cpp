/*
 * Renard2.cpp
 *
 * Computes and prints one or more Renard series, one line per series, each
 * labelled with its size.
 * C++ version of documentation/projects/Renard2.asm.
 *
 * Usage:
 *     Renard2 <series> [<series> ...]
 *
 * Author:   Richard Lesh
 * Original: 2022/11/28
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// prints "S: t0 t1 ..." for the series of size s
static void print_renard(long s)
{
    double factor = std::pow(10.0, 1.0 / s);
    std::cout << s << ':';
    for (long i = 0; i < s; i++) std::printf(" %.2f", std::pow(factor, (double)i));
    std::cout << '\n';
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "You must supply a series value 5, 10, 20 or 40" << std::endl;
        return 1;
    }
    for (int i = 1; i < argc; i++) print_renard(std::atol(argv[i]));
    return 0;
}
