///////////////////////////////////////////////////////////////////////////////
// Hailstone2.asm
//
// Finds the starting number, less than or equal to a given limit, that
// produces the LONGEST Hailstone (Collatz) sequence.
// See https://en.wikipedia.org/wiki/Collatz_conjecture
//
// The Hailstone (Collatz) sequence is defined for a positive integer n by
// repeatedly applying:
//     f(n) = n / 2       if n is even
//     f(n) = 3 * n + 1   if n is odd
// and terminating when the value reaches 1.  The "length" of the sequence is
// the count of terms generated (including the starting value and the final 1).
//
// This single-threaded version tests every starting number from 2 up to the
// supplied limit, tracking the one that yields the longest sequence.  Unlike
// Hailstone3, this version computes each sequence purely recursively with NO
// memoization, so it is considerably slower for large limits (every value is
// recomputed from scratch).  When finished, it prints the best starting number
// and its sequence length.
//
// Usage:
//     Hailstone2 <limit>
//   <limit>  A positive integer; the largest starting number to test.
//
// Output:
//   A single final line "<imax>: <max>" giving the starting number (imax) and
//   the longest sequence length (max) found.
//
// Author:   Richard Lesh
// Modified: 2026/06/28
// Original: 2009/03/20
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.def>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
// Program entry point.
//
// Arguments:
//   None (reads the command line directly via iARGC / args()).
//
// Command line:
//   argv[1]  A positive integer limit; the largest starting number to test.
//
// Behavior:
//   1. Validates that one command line argument was supplied.
//   2. Parses the argument into the search limit.
//   3. Iterates i from 2 to limit, computing each hailstone sequence length
//      and tracking the maximum length (max) and the starting number that
//      produced it (imax).
//   4. Prints the final best starting number and its sequence length.
//
// Returns:
//   0 on success, 1 if the command line argument is missing.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    i, hailstone, limit, argc, arg, imax, max
    int     iARGC
    move    argc, r0
    cmp     argc, 2
    jump    lt, $GET_ARGS_FAILED

    move    imax, 1                 // Best starting number found so far
    move    max, 1                  // Longest sequence length found so far
    #call   args(1)                 // Get first command line argument
    move    arg, r0
    #macro  PARSE_INT(arg)          // Convert the argument string to an integer
    move    limit, r0               // limit = upper bound on starting numbers
    #for    2, i <= limit, 1
        #call   compute_hailstone(i)
        move    hailstone, r0
        // Track the longest sequence seen and the number that produced it.
        #if_cond    hailstone, gt, max
            move    imax, i
            move    max, hailstone
        #end_cond
    #end_for
    #call   fprintf(STDOUT, "%d: %d\n", imax, max)
    #return 0
$GET_ARGS_FAILED:
    #call   putline("You must supply a positive integer argument.")
    #return 1
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_hailstone(arg)
// Computes the number of terms in the hailstone sequence that starts with the
// given argument.  The next value in the sequence is computed by the rule:
//     f(n) = n / 2       if n is even
//     f(n) = 3 * n + 1   if n is odd
// The sequence ends when the computed value reaches 1.  The length is computed
// recursively as 1 + (length of the sequence for the next value).
//
// Note: This implementation performs NO memoization; each call recomputes the
// full tail of the sequence.  For repeated calls over a large range this is
// much slower than the cached version in Hailstone3.
//
// Arguments:
//   arg  The starting number of the hailstone sequence (integer).
//
// Returns:
//   The length of the hailstone sequence (number of terms) in r0.
//   The base case (value <= 1) returns 1.
///////////////////////////////////////////////////////////////////////////////

#def_func   compute_hailstone(arg)
    #var    i,isOdd
    load    i, arg
    #if_cond    i, le, 1
        // Base case: a sequence at 1 (or below) has length 1.
        #return 1
    #else_cond
        and isOdd, i, 0x1
        #if_cond    isOdd, eq, 0
            // Even: next term is i / 2, then add 1 for this step.
            div     i, 2
            #call   compute_hailstone(i)
            add     r0, 1
        #else_cond
            // Odd: next term is 3*i + 1, then add 1 for this step.
            mult    i, 3
            add     i, 1
            #call   compute_hailstone(i)
            add     r0, 1
        #end_cond
    #end_cond
#end_func

    stop
    stop