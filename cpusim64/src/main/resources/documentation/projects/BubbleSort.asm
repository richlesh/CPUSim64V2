///////////////////////////////////////////////////////////////////////////////
// bubblesort.asm
//
// Demonstrates a stable bubble sort on an array of integers.
//
// The program repeats the following 10 times:
//   1. Allocates a heap array (standard "count in index 0" format) with a
//      random length between 15 and 30 elements.
//   2. Fills the array with random integer values.
//   3. Sorts the array in place using a stable bubble sort.
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

        #call   bubbleSort(array)

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
// bubbleSort(array)
//
// Sorts an integer array (standard "count in index 0" format) in place using
// a stable bubble sort. Only strictly-greater adjacent pairs are swapped so
// equal elements never change relative order, preserving stability.
//
// Arguments:
//   array  Address of the array to sort.
//
// No return value (array is modified in place).
///////////////////////////////////////////////////////////////////////////////
#def_func   bubbleSort(array)
    #var    addr, n, i, j, limit, jp1, a, b

    load    addr, array
    load    n, addr[0]

    #for    1, i < n, 1
        move    limit, n
        sub     limit, i
        #for    1, j <= limit, 1
            move    jp1, j
            add     jp1, 1
            load    a, addr[j]
            load    b, addr[jp1]
            #if_cond    a > b
                store   b, addr[j]
                store   a, addr[jp1]
            #end_cond
        #end_for
    #end_for
#end_func

    stop
    stop