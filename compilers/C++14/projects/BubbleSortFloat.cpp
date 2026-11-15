/*
 * BubbleSortFloat.cpp
 *
 * Sorts 10 arrays of 9-14 random doubles in [0, 1) with bubble sort and
 * prints each array (6 decimals) before and after.  The arrays keep their
 * length in element 0.  The numbers differ from run to run.
 * C++ version of documentation/projects/BubbleSortFloat.asm.
 *
 * Author:   Richard Lesh
 */
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <utility>
#include <vector>
#include <iostream>

// a random integer in [low, high]
static long rand_range(long low, long high)
{
    return low + std::rand() % (high - low + 1);
}

// n random doubles in [0, 1)
static std::vector<double> create_random_array(long n)
{
    std::vector<double> a;
    for (long i = 0; i < n; i++) a.push_back(std::rand() / (RAND_MAX + 1.0));
    return a;
}

// prints the elements with p decimals, separated by commas
static void print_float_array(const std::vector<double> &a, int p)
{
    for (std::size_t i = 0; i < a.size(); i++) std::printf(i == 0 ? "%.*f" : ",%.*f", p, a[i]);
}

static void bubble_sort_float(std::vector<double> &a)
{
    long n = (long)a.size();
    for (long i = 1; i < n; i++)
        for (long j = 0; j < n - i; j++)
            if (a[j] > a[j + 1]) std::swap(a[j], a[j + 1]);
}

int main()
{
    std::srand((unsigned)std::time(nullptr));
    for (int i = 1; i <= 10; i++) {
        std::vector<double> a = create_random_array(rand_range(9, 14));
        std::cout << "Test " << i << '\n';
        std::cout << "random: ";
        print_float_array(a, 6);
        std::cout << '\n';
        bubble_sort_float(a);
        std::cout << "sorted: ";
        print_float_array(a, 6);
        std::cout << '\n';
    }
    return 0;
}
