///////////////////////////////////////////////////////////////////////////////
// Renard.asm
//
// Computes and prints one or more Renard series.
//
// Renard numbers (a.k.a. "preferred numbers") are a system of geometrically
// spaced values introduced by Charles Renard. The R5, R10, R20 and R40 series
// divide a decade (a factor of 10) into 5, 10, 20 or 40 equal geometric steps.
// Each term is computed as:
//
//      term(i) = (10 ^ (1/series)) ^ i,   for i = 0 .. series-1
//
// where 'series' is one of 5, 10, 20 or 40. The result is a sequence that
// ranges from 1.0 up to (but not including) 10.0, rounded to 2 decimals here.
//
// Usage:
//      Renard <series> [<series> ...]
//
//      <series>    A series size (typically 5, 10, 20 or 40). One or more may
//                  be supplied; each is printed on its own line.
//
// Example:
//      > run.sh Renard 10
//       1.00 1.26 1.58 2.00 2.51 3.16 3.98 5.01 6.31 7.94
//
// Author:   Richard Lesh
// Modified: 2026/06/28
// Original: 2022/11/28
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

    #call   main()
    #call   exit(r0)             // Exit to OS, returning main's result code

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses each command line argument as an integer series size and prints the
// corresponding Renard series. Iterates over all supplied arguments.
//
// Arguments: none (reads from the command line via iARGC / iARGS)
// Returns:   0 on success, 1 if no series argument was supplied
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    i, argc             // i: loop index over args, argc: argument count

    // Require at least one series argument (argv[0] is the program name).
    int     iARGC
    move    argc, r0
    #if_cond    r0 >= 2

    // Process each command line argument starting at index 1.
    #for    1, i < argc, 1
        #call   args(i)
        #macro  PARSE_INT(r0)       // r0 <- integer value of the argument string
        #call   print_renard(r0)    // Print the Renard series for this size
    #else_cond
        #call   puts("You must supply a series value 5, 10, 20 or 40")
        #return 1                   // Failure: no argument supplied
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// print_renard(series)
//
// Computes and prints a single Renard series to STDOUT. The output begins with
// the series size followed by a colon, then each term formatted to two decimal
// places.
//
// Argument:
//      series  The number of terms in the series (e.g. 5, 10, 20 or 40)
// Returns:   nothing (output is written directly to STDOUT)
///////////////////////////////////////////////////////////////////////////////

#def_func print_renard(series)
    #var    i, loopLimit        // i: term index, loopLimit: number of terms
    #fvar   base, factor        // base: 10.0, factor: common ratio 10^(1/series)
    move    base, 10
    load    loopLimit, series

    // Print the series size as a label, e.g. "10:".
    #call   put_dec(loopLimit)
    #call   putc(':')

    // Compute the common ratio: factor = 10 ^ (1/series).
    move    f0, loopLimit
    recip   f0                  // f0 <- 1 / series
    #call   pow(base, f0)       // f0 <- 10 ^ (1/series)
    move    factor, f0

    // Print each term: factor^i for i = 0 .. series-1.
    #for    0, i < loopLimit, 1
        move    f0, i               // Convert loop index i to floating point exponent
        #call   pow(factor, f0)     // f0 <- factor ^ i  (the i-th Renard term)
        #call   printf(" %.2f", f0)
    #end_for
    #call   put_nl()            // End the line for this series
#end_func

    stop
    stop