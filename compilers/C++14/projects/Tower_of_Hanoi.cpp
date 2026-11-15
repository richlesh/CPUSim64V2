/*
 * Tower_of_Hanoi.cpp
 *
 * Solves the Tower of Hanoi puzzle for n disks recursively and prints the
 * pegs about ten times along the way (and at the end).  The pegs are an
 * n x 3 array: slot 0 is the top of a peg, 0 means no disk.
 * C++ version of documentation/projects/Tower_of_Hanoi.asm.
 *
 * Usage:
 *     Tower_of_Hanoi <disks>
 *
 * Author:   Richard Lesh
 */
#include <cstdlib>
#include <iostream>
#include <vector>

// the pegs, with the discs as numbers (0 = empty slot); the top of a peg has the lowest slot
class Hanoi {
public:
    explicit Hanoi(long n) : num_discs(n), move_number(0), print_mod(1), pegs(n * 3, 0)
    {
        for (long i = 0; i < n; i++) disk(0, i) = i + 1;
        // print about 10 times: every steps/10 moves, rounded down to a multiple of 5 (100 for n > 10)
        long steps = (1L << n) - 1;
        long pm = steps / 10 + 2;
        pm -= pm % (n > 10 ? 100 : 5);
        if (pm < 1) pm = 1;
        print_mod = pm;
    }

    void run()
    {
        print_pegs();
        solve(num_discs, 0, 2, 1);
        if (move_number % print_mod != 0) print_pegs();
    }

private:
    long num_discs;
    long move_number;
    long print_mod;                 // the pegs are printed after every print_mod moves
    std::vector<long> pegs;         // pegs[peg * num_discs + slot]

    long &disk(long peg, long slot) { return pegs[peg * num_discs + slot]; }

    void print_pegs()
    {
        std::cout << "Move: " << move_number << '\n';
        for (long i = 0; i < num_discs; i++) {
            for (long j = 0; j < 3; j++) {
                long d = disk(j, i);
                if (d == 0) std::cout << "|";
                else std::cout << d;
                std::cout << "   ";
            }
            std::cout << '\n';
        }
        std::cout << "=========\n";
        std::cout << '\n';
    }

    // moves the top disk of src to the top of dst
    void make_move(long src, long dst)
    {
        long d = 0, pos = num_discs - 1;
        move_number++;
        for (long i = 0; i < num_discs; i++) {
            if (disk(src, i) != 0) {
                d = disk(src, i);
                disk(src, i) = 0;
                break;
            }
        }
        for (long i = 0; i < num_discs; i++) {
            if (disk(dst, i) != 0) {
                pos = i - 1;
                break;
            }
        }
        disk(dst, pos) = d;
        if (move_number % print_mod == 0) print_pegs();
    }

    void solve(long n, long src, long dst, long aux)
    {
        if (n == 1) {
            make_move(src, dst);
        } else {
            solve(n - 1, src, aux, dst);
            make_move(src, dst);
            solve(n - 1, aux, dst, src);
        }
    }
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "Syntax: Hanoi [disks]" << std::endl;
        std::exit(1);
    }
    long n = std::atol(argv[1]);
    if (n <= 0) n = 3;
    Hanoi hanoi(n);
    hanoi.run();
    return 0;
}
