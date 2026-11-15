/*
 * Fibonacci.cpp
 *
 * Prints the first 45 numbers of the Fibonacci sequence.
 * C++ version of documentation/projects/Fibonacci.asm.
 *
 * Author:   Richard Lesh
 * Original: 2009/03/20
 */
#include <iostream>

int main()
{
    long a = 1, b = 1;
    std::cout << a << '\n' << b << '\n';
    for (int i = 0; i < 43; i++) {
        long c = a + b;
        std::cout << c << '\n';
        b = a;
        a = c;
    }
    return 0;
}
