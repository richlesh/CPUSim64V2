///////////////////////////////////////////////////////////////////////////////
// Hailstone5.asm
//
// Finds the starting number, less than a given limit, that
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
// PARALLEL, WORK-COUNTER DESIGN:
//   This version improves on the fixed-assignment threading of Hailstone4 by
//   handing out work dynamically.  The search range [0, limit) is divided
//   into fixed-size "work units" of gWORKSIZE candidates each.  A shared
//   counter (gNEXT_UNIT) is atomically incremented with get_and_increment()
//   to claim the next unit; no queue or queue mutex is needed.  One worker
//   thread is spawned per CPU core; each worker repeatedly claims a work
//   unit, processes it, and merges its local best result into the shared
//   global best under MY_MUTEX.  Workers stop once the claimed unit starts
//   at or beyond the limit.
//
//   Claiming work dynamically provides better load balancing than static
//   partitioning, because work units that finish early (e.g. those served
//   largely from the memoization cache) let a thread immediately grab more
//   work instead of sitting idle.
//
//   Memoization (see compute_hailstone) caches previously computed sequence
//   lengths to dramatically reduce redundant work.
//
// Usage:
//     Hailstone5 <limit>
//   <limit>  A positive integer >= 2; candidates 0 .. limit-1 are tested.
//
// Output:
//   Progress messages from threads and a final line
//   "Max <max> found at <imax>" giving the longest sequence length (max) and
//   the starting number (imax) that produced it.
//
// Author:   Richard Lesh
// Modified: 2026/06/28
// Original: 2009/03/20
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.asm>
#include <system/system.asm>
#include <system/thread.asm>

    #call   main()
    #call   exit(r0)

#macro DEFINE_MUTEX(MY_MUTEX)       // Mutex guarding the shared gMAX/gIMAX result
#global gMAX:        .dci 0         // Longest sequence length found (shared)
#global gIMAX:       .dci 0         // Starting number that produced gMAX (shared)
#global gWORKSIZE:   .dci 100000    // Number of candidates per work unit
#global gNEXT_UNIT:  .dci -1        // Last claimed work unit (first claim yields 0)
#global gLIMIT:      .dci 0         // Maximum number to test supplied by user

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
//   1. Validates that one command line argument was supplied and parses it
//      into the search limit (stored in gLIMIT).
//   2. Initializes the shared result mutex MY_MUTEX.
//   3. Calls compute_hailstone(1) so the memoization cache is allocated and
//      seeded while still single-threaded.
//   4. Queries the number of CPU cores and spawns one worker thread per core.
//      Workers claim work units from the shared atomic counter gNEXT_UNIT.
//   5. Joins all worker threads.
//   6. Prints the overall maximum sequence length and the starting number
//      that produced it.
//
// Globals used:
//   gLIMIT (write), gMAX (read), gIMAX (read), MY_MUTEX (init)
//
// Returns:
//   0 on success, 1 if the command line argument is missing or invalid.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    i, argc, arg, mImax, mMax, cores, pid, pids
    int     iARGC
    move    argc, r0
    cmp     argc, 2
    jump    lt, $GET_ARGS_FAILED

    #call   args(1)                 // Get first command line argument
    move    arg, r0
    #macro  PARSE_INT(arg)          // Convert the argument string to an integer
    #if_cond     r0 < 2
        #call putline("Argument must be >= 2!")
        #return 1
    #end_cond
    store   r0, gLIMIT
    #call   initializeMutex(MY_MUTEX)

    // Force compute_hailstone to lazily allocate and seed the memoization
    // cache now, while still single-threaded, avoiding a race on first
    // allocation once the workers start.
    #call   compute_hailstone(1)

    // Spawn one worker thread per CPU core.  Each worker claims work units
    // from the shared atomic counter gNEXT_UNIT until none remain.
    int     iGET_NUM_CORES
    move    cores, r0
    #call   printf("Number of cores: %d\n", cores)
    #call   alloc(cores)            // Allocate an array to hold thread PIDs
    move    pids, r0
    #if_cond    pids, eq, 0
        #call   printf("Can\'t allocate pids array!\n")
        #call   exit(1)
    #end_cond
    #for    0, i < cores, 1
        #macro  create_thread(worker, i)    // i is passed as the thread index
        store   r0, pids[i]         // Save the new thread's PID
    #end_for

    // Wait for all worker threads to finish.
    #for    0, i < cores, 1
        load    pid, pids[i]
        #call   printf("Main is joining %d...\n", pid)
        #macro  join_thread(pid)
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
//   NOTE: The cache is shared global state with no internal locking.  This
//   program relies on main() calling compute_hailstone(1) BEFORE spawning
//   threads, which forces the cache to be allocated and seeded while still
//   single-threaded.  Concurrent writes of the same entry are benign because
//   every thread computes the same value for a given index.
//
// Arguments:
//   arg  The starting number of the hailstone sequence (integer).
//
// Globals used:
//   PRECOMPUTED (read/write)       Heap address of the cache, 0 until first
//                                  allocated.
//   PRECOMPUTED_SIZE (read)        Number of entries in the cache.
//
// Returns:
//   The length of the hailstone sequence (number of terms) in r0.
///////////////////////////////////////////////////////////////////////////////

#global PRECOMPUTED: .dci   0       // Heap address of cache (0 until allocated)
PRECOMPUTED_SIZE: .dci  8000000     // Number of memoized entries
#def_func   compute_hailstone(arg)
    #var    i,i0,isOdd,cache,cacheSize,hailstone
    
    // Lazily allocate the memoization cache on first use.
    load    cacheSize, PRECOMPUTED_SIZE
    load    cache, PRECOMPUTED
    jump    nz, $BEGIN_COMPUTE
    #call   alloc(cacheSize)
    move    cache, r0
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
    store   cache, PRECOMPUTED      // Publish the cache only after seeding
$BEGIN_COMPUTE:
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
#end_func

///////////////////////////////////////////////////////////////////////////////
// worker(threadIndex)
// Worker thread entry point.  Repeatedly claims work units from the shared
// atomic counter gNEXT_UNIT and processes them until none remain.
//
// Arguments:
//   threadIndex  Index of this worker in main's spawn loop (informational
//                only; not used for work assignment).
//
// Behavior:
//   1. Atomically claim the next work unit number u with
//      get_and_increment(gNEXT_UNIT) and compute its start d = u * gWORKSIZE.
//      If d >= gLIMIT there is no more work and the worker exits.
//   2. Compute the hailstone sequence length for every candidate in
//      [d, min(d + gWORKSIZE, gLIMIT)), tracking the local best (wMax) and the
//      starting number that produced it (wImax).
//   3. Under MY_MUTEX, merge the local best into the global best
//      (gMAX/gIMAX).  The global best is updated when the local best is
//      longer, or equal in length but found at a smaller starting number.
//   4. Repeat from step 1.
//
// Globals used:
//   gWORKSIZE (read), gLIMIT (read), gNEXT_UNIT (atomic read/write),
//   gMAX (read/write), gIMAX (read/write), MY_MUTEX (lock)
//
// Returns:
//   Nothing.
///////////////////////////////////////////////////////////////////////////////

#def_func worker(threadIndex)
    #var    i, d, hs, ws, pid, limit, gLim, wImax, wMax
    int     iGET_PID
    move    pid, r0
    load    ws, gWORKSIZE
    load    gLim, gLIMIT
    #while  TRUE
        // Atomically claim the next work unit and compute its start index.
        #call   get_and_increment(gNEXT_UNIT)
        mult    d, r0, ws               // d = unit * gWORKSIZE
        #if_cond    d, ge, gLim
            #break                      // no more work
        #end_cond
        // Process the work unit covering [d, min(d + gWORKSIZE, gLIMIT)).
        add     limit, d, ws
        #macro  MIN_MACRO(limit, gLim)
        move    limit, r0
        #call   printf("Thread work unit %d executing with PID %d...\n", d, pid)
        clear   wMax
        #for    d, i < limit, 1
            #call   compute_hailstone(i)
            move    hs, r0
            #if_cond    hs > wMax
                move    wMax, hs
                move    wImax, i
            #end_cond
        #end_for
        // Merge this work unit's local best into the shared global best.
        #sync   MY_MUTEX
            load    hs, gMAX
            load    i, gIMAX
            // Update if wMax > hs || (wMax == hs && wImax < i)
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
    #end_while
    #call   printf("Thread %d finishing...\n", pid)
#end_func

    stop
    stop