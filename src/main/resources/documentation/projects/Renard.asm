///////////////////////////////////////////////////////////////////////////////
// Renard.asm
//
// Computes and prints a Renard series of preferred numbers.
//
// Renard numbers are a system of preferred numbers (defined by Charles Renard)
// that divide a decade (a factor of 10) into a geometric series of equal
// logarithmic steps. The R5, R10, R20, and R40 series divide a decade into
// 5, 10, 20, or 40 steps respectively.
//
// The nth value in an R-series of length S is computed as:
//
//     value(n) = (10 ^ (1/S)) ^ n,   for n = 0, 1, 2, ... S-1
//
// where (10 ^ (1/S)) is the common ratio between consecutive terms.
//
// Usage:
//     Renard <series>
//
// where <series> is one of: 5, 10, 20 or 40 (the R-series to generate).
//
// Example:
//     > run.sh Renard 10
//     1.00 1.26 1.58 2.00 2.51 3.16 3.98 5.01 6.31 7.94
//
// Author:   Richard Lesh
// Modified: 2026/06/28
// Original: 2022/11/28
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

    #call   main()              // Call main(), exit with its return code
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses the command-line argument for the series length, computes the common
// ratio for the series, then prints each term of the series to STDOUT.
//
// Returns 0 on success, 1 if the required command-line argument is missing.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    series, i           // series = R-series length, i = loop counter
    #fvar   base, factor        // base = 10.0, factor = common ratio
    move    base, 10            // The decade base (10) used for the ratio
GET_ARGS:
    // Require exactly one command-line argument (argc == 2: program + arg).
    int     iARGC
    #if_cond    r0 == 2
        #call   args(1)             // Get the first command-line argument
        #macro  PARSE_INT(r0)       // Convert the argument string to an integer
        move    series, r0          // series = requested R-series length
    #else_cond
        #call   puts("You must supply a series value 5, 10, 20 or 40")
        #return 1                   // Error: missing/invalid argument
    #end_cond

    // Compute the common ratio: factor = 10 ^ (1/series)
    move    f0, series          // f0 = (float) series
    recip   f0                  // f0 = 1 / series
    #call   pow(base, f0)       // f0 = 10 ^ (1/series)
    move    factor, f0          // Save the common ratio

    // Print each term of the series: factor ^ i for i = 0 .. series-1
    #for    0, i < series, 1
        move    f0, i               // Convert loop index i to float
        #call   pow(factor, f0)     // f0 = factor ^ i  (the nth Renard value)
        #call   fprintf(STDOUT, " %.2f", f0)  // Print with 2 decimal places
    #end_for
    #call   put_nl()            // Terminate the output line
    #return 0                   // Success
MAIN_END:
#end_func

    stop
    stop