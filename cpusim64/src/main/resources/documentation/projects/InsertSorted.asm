///////////////////////////////////////////////////////////////////////////////
// InsertSorted.asm
//
// Demonstrates maintaining a sorted array by inserting values one at a time.
//
// The program:
//   1. Allocates a small, empty heap array (standard "count in index 0"
//      format).
//   2. Inserts 30 random integers in the range [1, 999] into the array one
//      at a time, using insertSorted() to find the correct position with a
//      binary search and shift later elements to make room.
//   3. Prints the array after each insertion so you can watch it remain
//      sorted at every step.
//   4. Frees the heap allocated array.
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
// Test harness. Creates an empty array, then inserts 30 random integers in
// [1, 999] one at a time, printing the array after every insertion.
//
// Returns 0 in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    i, value, array

    #call   createEmptyArray()
    move    array, r0

    #call   putline("Inserting 30 random integers [1,999] one at a time:")

    #for    1, i <= 30, 1
        #call   rand(1, 999)
        move    value, r0

        #call   insertSorted(array, value)
        move    array, r0            // array may have been reallocated

        #call   printf("Insert %3d: ", value)
        #call   printIntArray(array)
        #call   put_nl()
    #end_for

    #call   free(array)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// createEmptyArray()
//
// Allocates a small heap array in standard format (count in index 0) with
// zero elements and a little spare capacity so the first few insertions
// don't immediately trigger a reallocation.
//
// Returns:
//   Address of the newly allocated, empty array in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   createEmptyArray()
    #var    array

    #call   alloc(5)                 // count word + room for 4 elements
    move    array, r0
    store   0, array[0]              // element count starts at 0

    #return array
#end_func

///////////////////////////////////////////////////////////////////////////////
// insertSorted(array, value)
//
// Inserts value into a sorted integer array (standard "count in index 0"
// format), growing the underlying heap block if necessary, then finding the
// insertion point with a binary search and shifting later elements right to
// make room.
//
// Arguments:
//   array  Address of the array to insert into.
//   value  The integer value to insert.
//
// Returns:
//   The (possibly reallocated) array address in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   insertSorted(array, value)
    #var    addr, val, n, cap, required, newCap, lo, hi, mid, pos, i, j, cur

    load    addr, array
    load    val, value
    load    n, addr[0]

    // Grow the array if there isn't room for one more element. "cap" is the
    // total allocated size in words (count word + element slots).
    // addr[-1] reports the size of the whole heap block in words, which
    // includes a 3-word block header, so subtract that off to get the
    // number of words actually usable for our array data.
    load    cap, addr[-1]
    sub     cap, 3
    move    required, n
    add     required, 2              // count word + n existing + 1 new
    #if_cond    cap < required
        move    newCap, cap
        mult    newCap, 2            // double capacity to amortize growth
        #if_cond    newCap < required
            move    newCap, required
        #end_cond
        #call   realloc(addr, newCap)
        move    addr, r0
    #end_cond

    // Binary search for the insertion position: the first index in
    // [1, n+1] where addr[pos] > val (or n+1 if val is the new largest).
    move    lo, 1
    move    hi, n
    #while  lo <= hi
        move    mid, lo
        add     mid, hi
        div     mid, 2
        load    cur, addr[mid]
        #if_cond    cur <= val
            move    lo, mid
            add     lo, 1
        #else_cond
            move    hi, mid
            sub     hi, 1
        #end_cond
    #end_while
    move    pos, lo

    // Shift elements [pos, n] right by one to make room for the new value.
    #for    n, i >= pos, -1
        load    cur, addr[i]
        move    j, i
        add     j, 1
        store   cur, addr[j]
    #end_for

    // Insert the new value and update the element count.
    store   val, addr[pos]
    add     n, 1
    store   n, addr[0]

    #return addr
#end_func

    stop
    stop