///////////////////////////////////////////////////////////////////////////////
// Pi.asm
//
// Computes an approximation of the mathematical constant π (pi) using the
// Bailey–Borwein–Plouffe (BBP) formula. The number of terms to sum is
// supplied as a command line argument, allowing the caller to trade speed
// for accuracy. Each additional term yields roughly one more correct
// hexadecimal digit, so about 11 terms is sufficient for full double
// precision.
//
// Usage:
//   run.sh Pi <terms>
//
//   <terms>   Number of series terms to sum (1-11 recommended).
//             More terms produce a more accurate result.
//
// Example:
//   > run.sh Pi 11
//   PI(11): 3.1415926535897930
//
// Author: Richard Lesh
// Date:   2025/11/17
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

    #call   main()
    int     iEXIT

///////////////////////////////////////////////////////////////////////////////
// main()
// Program entry point. Reads the number of terms from the command line,
// computes π, and prints the result. Prints a usage message and returns a
// non-zero exit code if no argument is supplied.
//
// Returns: 0 on success, 1 on usage error (in r0).
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    terms, argc
    // Require exactly one command line argument (program name + terms).
    int     iARGC
    move    argc, r0
    cmp     argc, 2
    jump    ne, GET_ARGS_FAILED

GET_ARGS:
    // Convert the first command line argument (a string) to an integer.
    #call   args(1)
    #macro  PARSE_INT(r0)
    move    terms, r0
    // Compute π using the requested number of terms and print it.
    #call   compute_pi(terms)
    #call   printf("PI(%d): %.16f\n", terms, f0)
GET_ARGS_END:
    #return 0
    jump    MAIN_END
GET_ARGS_FAILED:
    // No argument supplied: print usage and signal an error.
    #call   puts("You must supply the number of terms 1-11")
    #return 1
MAIN_END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_pi(terms)
// Computes π using the Bailey–Borwein–Plouffe (BBP) formula:
//
//        ___ inf       1     (   4        2        1        1   )
//   π =  \         --------- ( ------ - ------ - ------ - ------ )
//        /__ k=0    16^k     ( 8k + 1   8k + 4   8k + 5   8k + 6 )
//
// The summation is truncated after the number of terms specified by the
// caller. Each iteration computes the bracketed term sum, divides it by
// 16^k, and accumulates it into the running total.
//
// Arguments:
//   terms   Number of terms to sum (stack argument).
//
// Returns:
//   The computed approximation of π in f0.
///////////////////////////////////////////////////////////////////////////////
#def_func compute_pi(terms)
    #var    k, loopLimit
    #fvar   myPi, base, term, termsum, denom
    clear   myPi                // Running total starts at 0.0
    load    loopLimit, terms    // Number of terms to sum
    move    k, 0                // Term index
    move    base, 16            // Base of the 16^k denominator

    #for    0, k < loopLimit, 1
        // termsum = 4 / (8k + 1)
        move    f0, 4
        move    denom, 8
        mult    denom, k
        add     denom, 1
        div     termsum, f0, denom

        // termsum -= 2 / (8k + 4)
        move    f0, 2
        move    denom, 8
        mult    denom, k
        add     denom, 4
        div     f0, denom
        sub     termsum, f0

        // termsum -= 1 / (8k + 5)
        move    f0, 1
        move    denom, 8
        mult    denom, k
        add     denom, 5
        div     f0, denom
        sub     termsum, f0

        // termsum -= 1 / (8k + 6)
        move    f0, 1
        move    denom, 8
        mult    denom, k
        add     denom, 6
        div     f0, denom
        sub     termsum, f0

        // term = termsum / 16^k, then accumulate into myPi.
        move    f0, base
        move    r0, k
        #call   fastpow(f0, r0) // f0 = 16^k
        div     f0, termsum, f0 // f0 = termsum / 16^k
        add     myPi, f0        // myPi += term
    #end_for
    #freturn myPi
#end_func

    stop
    stop