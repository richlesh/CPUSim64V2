/*
 * QuickSort.cpp
 *
 * Sorts 10 arrays of 15-30 random integers in [1, 999] with quicksort
 * (Lomuto partition, the last element is the pivot) and prints each array
 * before and after.  The arrays keep their length in element 0.  The numbers
 * differ from run to run.
 * C++ version of documentation/projects/QuickSort.asm.
 *
 * Author:   Richard Lesh
 */
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <utility>
#include <vector>

// a random integer in [low, high]
static long rand_range(long low, long high)
{
    return low + std::rand() % (high - low + 1);
}

// n random integers in [1, 999]
static std::vector<long> create_random_array(long n)
{
    std::vector<long> a;
    for (long i = 0; i < n; i++) a.push_back(rand_range(1, 999));
    return a;
}

// prints the elements separated by commas
static void print_int_array(const std::vector<long> &a)
{
    for (std::size_t i = 0; i < a.size(); i++) std::cout << (i ? "," : "") << a[i];
}

static long partition(std::vector<long> &a, long lo, long hi)
{
    long pivot = a[hi], i = lo - 1;
    for (long j = lo; j < hi; j++)
        if (a[j] <= pivot) std::swap(a[++i], a[j]);
    std::swap(a[++i], a[hi]);
    return i;
}

static void quick_sort_range(std::vector<long> &a, long lo, long hi)
{
    if (lo < hi) {
        long p = partition(a, lo, hi);
        quick_sort_range(a, lo, p - 1);
        quick_sort_range(a, p + 1, hi);
    }
}

static void quick_sort(std::vector<long> &a) { quick_sort_range(a, 0, (long)a.size() - 1); }

int main()
{
    std::srand((unsigned)std::time(nullptr));
    for (int i = 1; i <= 10; i++) {
        std::vector<long> a = create_random_array(rand_range(15, 30));
        std::cout << "Test " << i << '\n';
        std::cout << "random: ";
        print_int_array(a);
        std::cout << '\n';
        quick_sort(a);
        std::cout << "sorted: ";
        print_int_array(a);
        std::cout << '\n';
    }
    return 0;
}
