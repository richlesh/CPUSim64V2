/*
 * BubbleSort.cpp
 *
 * Sorts 10 arrays of 15-30 random integers in [1, 999] with bubble sort
 * and prints each array before and after.  The arrays keep their length in
 * element 0.  The numbers differ from run to run.
 * C++ version of documentation/projects/BubbleSort.asm.
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

static void bubble_sort(std::vector<long> &a)
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
        std::vector<long> a = create_random_array(rand_range(15, 30));
        std::cout << "Test " << i << '\n';
        std::cout << "random: ";
        print_int_array(a);
        std::cout << '\n';
        bubble_sort(a);
        std::cout << "sorted: ";
        print_int_array(a);
        std::cout << '\n';
    }
    return 0;
}
