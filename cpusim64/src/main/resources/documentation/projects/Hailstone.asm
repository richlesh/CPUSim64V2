///////////////////////////////////////////////////////////////////////////////
// Hailstone.asm
//
// Computes the length of the Hailstone (Collatz) sequence for a starting
// number supplied on the command line.
// See https://en.wikipedia.org/wiki/Collatz_conjecture
//
// The Hailstone (Collatz) sequence is defined for a positive integer n by
// repeatedly applying:
//     f(n) = n / 2       if n is even
//     f(n) = 3 * n + 1   if n is odd
// and terminating when the value reaches 1.  The "length" of the sequence is
// the count of terms generated (including the starting value and the final 1).
//
// This is the simplest version in the Hailstone family.  It computes the
// length of a single sequence recursively with NO memoization.  If a starting
// number is supplied on the command line, that one sequence length is printed.
// If no argument is supplied, a demonstration table of lengths for the
// starting values 1 through 100 is printed instead.
//
// Usage:
//     Hailstone <n>     Print the sequence length for starting number n.
//     Hailstone         Print a demo table of lengths for n = 1...100.
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
//   argv[1]  (optional) A positive integer starting number.
//
// Behavior:
//   - If a command line argument is supplied, it is parsed and the length of
//     that single hailstone sequence is computed and printed.
//   - If no argument is supplied, a demonstration table is printed showing the
//     sequence length for each starting value from 1 through 100.
//
// Returns:
//   0 on success.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    i, argc, arg
    int     iARGC
    move    argc, r0
    cmp     argc, 2
    jump    lt, PRINT_DEMO          // No argument supplied: print the demo table
GET_ARGS:
    #call   args(1)                 // Get first command line argument
    move    arg, r0
    #macro  PARSE_INT(arg)          // Convert the argument string to an integer
    #call   compute_hailstone(r0)   // Compute the sequence length
    #call   put_dec(r0)             // Print the length
    #call   put_nl()
    #return 0
PRINT_DEMO:
    // No argument: print a demonstration table for starting values 1..100.
    #for    1, i <= 100, 1
        #call   compute_hailstone(i)
        #call   fprintf(STDOUT,"%d: %d\n", i, r0)
    #end_for
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
// full tail of the sequence.
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
            #return r0
        #else_cond
            // Odd: next term is 3*i + 1, then add 1 for this step.
            mult    i, 3
            add     i, 1
            #call   compute_hailstone(i)
            add     r0, 1
            #return r0
        #end_cond
    #end_cond
#end_func

    stop
    stop