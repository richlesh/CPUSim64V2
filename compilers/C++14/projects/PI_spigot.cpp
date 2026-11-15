/*
 * PI_spigot.cpp
 *
 * Prints n digits of pi after the decimal point with the Rabinowitz-Wagon
 * spigot algorithm:
 *     len = floor(10 * D / 3) + 1,  a[1..len] = 2
 *     repeat D times:
 *         q = 0
 *         for i = len down to 1:
 *             x = 10 * a[i] + q * i;  a[i] = x mod (2i - 1);  q = x div (2i - 1)
 *         a[1] = q mod 10,  q = q div 10
 * A digit 9 is held back (it may become 0 after a carry of 10).  A few
 * guard digits are computed past the requested ones.
 * C++ version of documentation/projects/PI_spigot.asm.
 *
 * Usage:
 *     PI_spigot <n>
 *
 * Author:   Richard Lesh
 */
#include <cstdlib>
#include <iostream>
#include <vector>

static const long GUARD_DIGITS = 10;

// prints the digits of pi as the spigot produces them
class Printer {
public:
    explicit Printer(long end) : index(-2), end(end) {}

    void digit(long d)
    {
        index++;
        if (index >= 0 && index < end) {
            std::cout << (char)('0' + d);
            if (index == 0) std::cout << '.';
        }
    }

    void run(long d, long count)
    {
        for (long i = 0; i < count; i++) digit(d);
    }

    bool done() const { return index + 1 >= end; }

private:
    long index;                     // the index of the next digit: -1 is the predigit 0, 0 is the 3
    long end;                       // the digits with index < end are printed
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "Usage: PI_spigot <n>   (n = number of digits after the decimal point)" << std::endl;
        return 1;
    }
    long n = std::atol(argv[1]);
    if (n < 1) {
        std::cout << "n must be >= 1" << std::endl;
        return 1;
    }
    Printer out(n + 1);
    long total = n + 1 + GUARD_DIGITS;
    long len = total * 10 / 3 + 1;
    std::vector<long> a(len + 1, 2);
    long predigit = 0, nines = 0;
    for (long j = 0; j < total; j++) {
        long q = 0;
        for (long i = len; i > 0; i--) {
            long den = 2 * i - 1;
            long x = a[i] * 10 + q * i;
            q = x / den;
            a[i] = x % den;
        }
        a[1] = q % 10;
        q = q / 10;
        if (q == 9) {
            nines++;
        } else if (q == 10) {
            out.digit(predigit + 1);
            out.run(0, nines);
            predigit = 0;
            nines = 0;
        } else {
            out.digit(predigit);
            predigit = q;
            out.run(9, nines);
            nines = 0;
        }
        if (out.done()) break;
    }
    std::cout << '\n';
    return 0;
}
