///////////////////////////////////////////////////////////////////////////////
// quicksort.asm
//
// Demonstrates a quicksort on an array of integers.
//
// The program repeats the following 10 times:
//   1. Allocates a heap array (standard "count in index 0" format) with a
//      random length between 15 and 30 elements.
//   2. Fills the array with random integer values.
//   3. Sorts the array in place using quicksort.
//   4. Prints the sorted array as a comma separated list on one line.
//   5. Frees the heap allocated array.
//
// Author: CPUSim64 Assistant
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Runs 10 test iterations, each with a freshly allocated random array.
//
// Returns 0 in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    i, count, array

    #for    1, i <= 10, 1
        #call   rand(15, 30)             // random array length in [15,30]
        move    count, r0
        #call   createRandomArray(count)
        move    array, r0

        #call   printf("Test %d\n", i)
        #call   puts("random: ")
        #call   printIntArray(array)
        #call   put_nl()

        #call   quickSort(array)

        #call   puts("sorted: ")
        #call   printIntArray(array)
        #call   put_nl()

        #call   free(array)
    #end_for
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// createRandomArray(count)
//
// Allocates a heap array in standard format (count in index 0, elements in
// indices 1..count) and fills it with random integers in [1, 999].
//
// Arguments:
//   count  Number of elements to place in the array.
//
// Returns:
//   Address of the newly allocated array in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   createRandomArray(count)
    #var    n, total, array, i

    load    n, count
    move    total, n
    add     total, 1                     // one extra word for the count itself
    #call   alloc(total)
    move    array, r0
    store   n, array[0]

    #for    1, i <= n, 1
        #call   rand(1, 999)
        store   r0, array[i]
    #end_for

    #return array
#end_func

///////////////////////////////////////////////////////////////////////////////
// quickSort(array)
//
// Sorts an integer array (standard "count in index 0" format) in place using
// quicksort with a Lomuto partition scheme (last element as pivot).
//
// Arguments:
//   array  Address of the array to sort.
//
// No return value (array is modified in place).
///////////////////////////////////////////////////////////////////////////////
#def_func   quickSort(array)
    #var    addr, n

    load    addr, array
    load    n, addr[0]

    #call   quickSortRange(addr, 1, n)
#end_func

///////////////////////////////////////////////////////////////////////////////
// quickSortRange(array, low, high)
//
// Recursively sorts the portion of an array between indices low and high
// (inclusive) using quicksort.
//
// Arguments:
//   array  Address of the array to sort.
//   low    Lower index (inclusive) of the range to sort.
//   high   Upper index (inclusive) of the range to sort.
//
// No return value (array is modified in place).
///////////////////////////////////////////////////////////////////////////////
#def_func   quickSortRange(array, low, high)
    #var    addr, lo, hi, p, newLow, newHigh

    load    addr, array
    load    lo, low
    load    hi, high

    #if_cond    lo < hi
        #call   partition(addr, lo, hi)
        move    p, r0
        move    newLow, p
        sub     newLow, 1
        #call   quickSortRange(addr, lo, newLow)
        move    newHigh, p
        add     newHigh, 1
        #call   quickSortRange(addr, newHigh, hi)
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// partition(array, low, high)
//
// Partitions the range [low, high] of an array using the element at high as
// the pivot (Lomuto partition scheme). Elements less than or equal to the
// pivot are moved before it; the pivot is then placed at its final sorted
// position.
//
// Arguments:
//   array  Address of the array to partition.
//   low    Lower index (inclusive) of the range to partition.
//   high   Upper index (inclusive) of the range to partition; also the index
//          of the pivot element.
//
// Returns:
//   The final index of the pivot element in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   partition(array, low, high)
    #var    addr, lo, hi, pivot, i, j, tmp1, tmp2

    load    addr, array
    load    lo, low
    load    hi, high
    load    pivot, addr[hi]
    move    i, lo
    sub     i, 1

    #for    lo, j < hi, 1
        load    tmp1, addr[j]
        #if_cond    tmp1 <= pivot
            add     i, 1
            load    tmp1, addr[i]
            load    tmp2, addr[j]
            store   tmp2, addr[i]
            store   tmp1, addr[j]
        #end_cond
    #end_for

    add     i, 1
    load    tmp1, addr[i]
    load    tmp2, addr[hi]
    store   tmp2, addr[i]
    store   tmp1, addr[hi]
    #return i
#end_func

    stop
    stop