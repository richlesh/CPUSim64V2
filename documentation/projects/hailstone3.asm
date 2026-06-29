///////////////////////////////////////////////////////////////////////////////
// Hailstone3.asm
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
// supplied limit, tracking the one that yields the longest sequence.  It uses
// memoization (see compute_hailstone) to cache previously computed lengths,
// dramatically reducing redundant work.  Progress is reported every 1000
// candidates.
//
// Usage:
//     Hailstone3 <limit>
//   <limit>  A positive integer; the largest starting number to test.
//
// Output:
//   Periodic progress lines of the form "<i>...<imax>:<max>" and a final
//   line "<imax>: <max>" giving the starting number (imax) and the longest
//   sequence length (max) found.
//
// Author:   Richard Lesh
// Modified: 2026/06/28
// Original: 2009/03/20
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.def>
#include <system/system.asm>

    #call   main()
    int     iEXIT

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
//   4. Prints a progress line every 1000 candidates.
//   5. Prints the final best starting number and its sequence length.
//
// Returns:
//   0 on success, 1 if the command line argument is missing.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    i, j, hailstone, limit, argc, arg, imax, max
    int     iARGC
    move    argc, r0
    cmp     argc, 2
    jump    lt, GET_ARGS_FAILED
GET_ARGS:
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
        // Print a progress line every 1000 candidates (when i % 1000 == 0).
        div     r0, j, i, 1000      // j = i mod 1000
        #if_cond    j, eq, 0
            #call   fprintf(STDOUT,"%d...%d:%d\n", i, imax, max)
        #end_cond
    #end_for
    #call   fprintf(STDOUT, "%d: %d\n", imax, max)
    #return 0
GET_ARGS_FAILED:
    #call   puts("You must supply a positive integer argument.")
    #return 1
MAIN_END:
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
// Memoization:
//   Results for starting values less than PRECOMPUTED_SIZE are cached in a
//   lazily-allocated heap array (PRECOMPUTED), dramatically improving
//   performance by avoiding recomputation.  The cache is allocated and seeded
//   on the first call and reused thereafter.  A cache entry of 0 means "not
//   yet computed".
//
// Arguments:
//   arg  The starting number of the hailstone sequence (integer).
//
// Globals used:
//   PRECOMPUTED (read/write)       Heap address of the memoization cache,
//                                  0 until first allocated.
//   PRECOMPUTED_SIZE (read)        Number of entries in the cache.
//
// Returns:
//   The length of the hailstone sequence (number of terms) in r0.
///////////////////////////////////////////////////////////////////////////////

#global PRECOMPUTED: .dci   0       // Heap address of cache (0 until allocated)
PRECOMPUTED_SIZE: .dci  3000000     // Number of memoized entries
#def_func   compute_hailstone(arg)
    #var    i,i0,isOdd,cache,cacheSize,hailstone
    
    // Lazily allocate the memoization cache on first use.
    load    cacheSize, PRECOMPUTED_SIZE
    load    cache, PRECOMPUTED
    jump    nz, BEGIN_COMPUTE
    #call   ALLOC(cacheSize)
    move    cache, r0
    store   cache, PRECOMPUTED
    #if_cond    cache == 0
        #call   fprintf(STDOUT, "Can\'t allocate cache size %d\n", cacheSize)
        #call   exit(1)
    #end_cond
    #call   MEMCLEAR(cache, cacheSize)
    store   1, cache[1]             // Seed base case: sequence length of 1 is 1
BEGIN_COMPUTE:
    // Return the cached result if it has already been computed.
    load    i, arg
    #if_cond    i, lt, cacheSize
        load    hailstone, cache[i]
        #if_cond    hailstone, ne, 0
            #return hailstone
        #end_cond
    #end_cond
    
    // Compute recursively: even -> n/2, odd -> 3n+1, then add 1 for this step.
    move    i0, i                   // Remember the original value for caching
    and isOdd, i, 0x1
    #if_cond    isOdd, eq, 0
        div     i, 2
        #call   compute_hailstone(i)
        add     hailstone, r0, 1
    #else_cond
        mult    i, 3
        add     i, 1
        #call   compute_hailstone(i)
        add     hailstone, r0, 1
    #end_cond
    // Cache the newly computed result when the start value is in range.
    #if_cond    i0, lt, cacheSize
        store   hailstone, cache[i0]
    #end_cond
    #return hailstone
END:
#end_func

    stop
    stop