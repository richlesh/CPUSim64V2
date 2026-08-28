///////////////////////////////////////////////////////////////////////////////
// Hailstone4.asm
//
// Finds the longest Hailstone sequence with starting number less than or 
// equal to the argument.
// See https://en.wikipedia.org/wiki/Collatz_conjecture
//
// This version distributes the work across multiple threads (one per CPU
// core) to compute the maximum sequence length in parallel.  Each thread
// processes a contiguous "work unit" of gWORKSIZE candidate starting numbers.
// A mutex guards the shared global maximum.  Memoization is used to cache
// previously computed sequence lengths.
//
// Usage: Hailstone4 <limit>
//   <limit>  The upper bound on starting numbers to test.
//
// Author:   Richard Lesh
// Modified: 2026/06/28
// Original: 2009/03/20
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.def>
#include <system/string.asm>
#include <system/system.asm>
#include <system/thread.asm>

    #call   main()
    #call   exit(r0)

#macro DEFINE_MUTEX(MY_MUTEX)       // Allocate the shared mutex MY_MUTEX
#global gMAX:        .dci 0         // Longest sequence length found (shared)
#global gIMAX:       .dci 0         // Starting number that produced MAX (shared)
#global gWORKSIZE:   .dci 10000     // Number of candidates each thread tests
#global gLIMIT:      .dci 0         // Maximum number to test supplied by user

///////////////////////////////////////////////////////////////////////////////
// main()
// Program entry point.
//
// Arguments:
//   None (reads the command line directly via iARGC / args()).
//
// Command line:
//   argv[1]  A positive integer used to set the approximate limit to test.
//
// Behavior:
//   1. Validates that one command line argument was supplied.
//   2. Parses the argument and stores it in the global gWORKSIZE.
//   3. Initializes the shared mutex MY_MUTEX.
//   4. Processes the first work unit (starting at 0) in the main thread.
//   5. Queries the number of CPU cores and spawns one worker thread per core,
//      each assigned a successive work unit (offset by gWORKSIZE).
//   6. Joins all worker threads.
//   7. Prints the overall maximum sequence length and the starting number
//      that produced it.
//
// Globals used:
//   gWORKSIZE (read/write), gMAX (read), gIMAX (read), MY_MUTEX (init)
//
// Returns:
//   0 on success, 1 if the command line argument is missing.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    i, j, argc, arg, mImax, mMax, ws, cores, pid, pids

    int     iGET_NUM_CORES
    move    cores, r0
    #call   printf("Number of cores: %d\n", cores)

    int     iARGC
    move    argc, r0
    cmp     argc, 2
    jump    lt, $GET_ARGS_FAILED

    #call   args(1)                 // Get first command line argument
    move    arg, r0
    #macro  PARSE_INT(arg)          // Convert the argument string to an integer
    #if_cond     r0 < 2
        #call putline("Argument must be > 2!")
        #return 1
    #end_cond
    store   r0, gLIMIT
    move    f1, r0
    move    f0, cores
    div     f1, f0                  // Worksize is ceil(limit / cores)
    int     iCEIL
    move    ws, f0
    #call   printf("Worksize: %d\n", ws)
    store   ws, gWORKSIZE           // Save it as the per-thread work unit size
    #call   initializeMutex(MY_MUTEX)
    #call   worker(0)               // Process the first work unit on main thread
    move    j, ws                   // j tracks the start of the next work unit
    

    // Spawn worker threads
    #call   alloc(cores)            // Allocate an array to hold thread PIDs
    move    pids, r0
    #if_cond    pids, eq, 0
        #call   printf("Can\'t allocate pids array!\n")
        #call   exit(1)
    #end_cond
    #for    1, i < cores, 1
        #macro  create_thread(worker, j)    // Each thread starts at offset j
        store   r0, pids[i]                 // Save the new thread's PID
        add     j, ws                       // Advance to the next work unit
    #end_for
    
    // Join with threads
    #for    1, i < cores, 1
        load    pid, pids[i]
        #call   printf("Main is joining %d...\n", pid)
        #macro  join_thread(pid)            // Wait for each worker to finish
    #end_for

    load    mImax, gIMAX
    load    mMax, gMAX
    #call   printf("Max %d found at %d\n", mMax, mImax)
    #return 0
$GET_ARGS_FAILED:
    #call   putline("You must supply a positive integer argument.")
    #return 1
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_hailstone(arg)
// Computes the number of integers in the hailstone sequence starting
// with the argument.  The hailstone sequence computes the next value
// in the sequence according to the formula...
//     f(n) = n / 2       if n is even
//     f(n) = 3 * n + 1   if n is odd
// The sequence ends when the computed value reaches 1.  This is computed
// recursively.
//
// Memoization:
//   Results for starting values less than PRECOMPUTED_SIZE are cached in a
//   lazily-allocated heap array (PRECOMPUTED) to dramatically improve
//   performance.  The cache is allocated on the first call and reused
//   thereafter.
//
// Arguments:
//   arg  The starting number of the hailstone sequence (integer).
//
// Globals used:
//   PRECOMPUTED (read/write)       Heap address of the memoization cache.
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
    jump    nz, $BEGIN_COMPUTE
    #call   alloc(cacheSize)
    move    cache, r0
    store   cache, PRECOMPUTED
    #if_cond    cache, eq, 0
        #call   printf("Can\'t allocate cache size %d\n", cacheSize)
        #call   exit(1)
    #end_cond
    #call   memclear(cache, cacheSize)
    // Seed base cases for the cache
    store   1, cache[0]             // degenerate case
    store   1, cache[1]             // by definition
    store   2, cache[2]
    store   8, cache[3]
    store   3, cache[4]
    store   6, cache[5]
$BEGIN_COMPUTE:
    // Return the cached result if available.
    load    i, arg
    #if_cond    i, lt, cacheSize
        load    hailstone, cache[i]
        #if_cond    hailstone, ne, 0
            #return hailstone
        #end_cond
    #end_cond
    
    // Compute recursively: even -> n/2, odd -> 3n+1, add 1 for this step.
    move    i0, i
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
    // Store the newly computed result in the cache when in range.
    #if_cond    i0, lt, cacheSize
        store   hailstone, cache[i0]
    #end_cond
    #return hailstone
#end_func

///////////////////////////////////////////////////////////////////////////////
// worker(data)
// Thread entry point.  Processes a single contiguous "work unit" of
// candidate starting numbers and updates the shared global maximum.
//
// Arguments:
//   data  The starting candidate number for this work unit.  The thread
//         tests starting numbers in the range [data, data + gWORKSIZE).
//
// Behavior:
//   1. Computes the hailstone sequence length for every candidate in its
//      assigned range, tracking the local maximum (wMax) and the starting
//      number that produced it (wImax).
//   2. Acquires MY_MUTEX and compares its local maximum to the shared global
//      maximum (MAX/IMAX).  Updates the global values if its local maximum is
//      larger, or equal but found at a smaller starting number.
//
// Globals used:
//   gWORKSIZE (read), gMAX (read/write), gIMAX (read/write), MY_MUTEX (lock)
//
// Returns:
//   Nothing.
///////////////////////////////////////////////////////////////////////////////

#def_func worker(data)
    #var    i, d, hs, ws, pid, limit, wImax, wMax
    load    d, data
    move    limit, d
    load    ws, gWORKSIZE
    add     limit, ws                   // limit = data + WORKSIZE
    load    r1, gLIMIT
    #macro  MIN(limit, r1)
    move    limit, r0
    int     iGET_PID
    move    pid, r0
    #call   printf("Thread work unit %d executing with PID %d...\n", d, pid)
    clear   wMax
    // Find the longest sequence within this work unit (local max).
    #for    d, i < limit, 1
        #call   compute_hailstone(i)
        move    hs, r0
        #if_cond    hs > wMax
            move    wMax, hs
            move    wImax, i
        #end_cond
    #end_for
    // Merge the local max into the shared global max under the mutex.
    #sync   MY_MUTEX
        load    hs, gMAX
        load    i, gIMAX
        // if wMax > hs || (wMax == hs && wImax < i)
        cmp     wMax, hs
        move    gt, r0, -1, 0
        cmp     wMax, hs
        move    eq, r1, -1, 0
        cmp     wImax, i
        move    lt, r2, -1, 0
        and     r1, r2
        or      r0, r1
        #if_cond_sr nz
            #call   printf("New high: %d:%d (%d)\n", wImax, wMax, pid)
            store   wMax, gMAX
            store   wImax, gIMAX
        #end_cond
    #end_sync

    #call   printf("Thread %d finishing...\n", pid)
#end_func

    stop
    stop