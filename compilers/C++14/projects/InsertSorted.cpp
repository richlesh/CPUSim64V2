/*
 * InsertSorted.cpp
 *
 * Inserts 30 random integers in [1, 999] one at a time into a sorted,
 * growing array and prints the array after each insertion.  The place of a
 * value is found by binary search; the array doubles its capacity when it
 * is full.  The numbers differ from run to run.
 * C++ version of documentation/projects/InsertSorted.asm.
 *
 * Author:   Richard Lesh
 */
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <vector>

// inserts val into the sorted vector, after any equal elements
static void insert_sorted(std::vector<long> &a, long val)
{
    // the first position whose element is greater than val (binary search)
    std::size_t lo = 0, hi = a.size();
    while (lo < hi) {
        std::size_t mid = (lo + hi) / 2;
        if (a[mid] <= val) lo = mid + 1;
        else hi = mid;
    }
    a.insert(a.begin() + lo, val);
}

static void print_int_array(const std::vector<long> &a)
{
    for (std::size_t i = 0; i < a.size(); i++) std::cout << (i ? "," : "") << a[i];
}

int main()
{
    std::vector<long> a;
    std::srand((unsigned)std::time(nullptr));
    std::cout << "Inserting 30 random integers [1,999] one at a time:" << std::endl;
    for (int i = 1; i <= 30; i++) {
        long v = 1 + std::rand() % 999;
        insert_sorted(a, v);
        std::cout << "Insert " << std::setw(3) << v << ": ";
        print_int_array(a);
        std::cout << '\n';
    }
    return 0;
}
