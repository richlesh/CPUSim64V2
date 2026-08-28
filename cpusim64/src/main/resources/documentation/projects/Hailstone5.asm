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
// PARALLEL, WORK-QUEUE DESIGN:
//   This version improves on the fixed-assignment threading of Hailstone4 by
//   using a shared, thread-safe work queue.  The search range [0, limit) is
//   divided into fixed-size "work units" of gWORKSIZE candidates each.  Each
//   work unit's starting index is pushed onto a shared Vector (gWORKQUEUE),
//   guarded by the Vector's own mutex.  One worker thread is spawned per CPU
//   core; each worker repeatedly pops a work unit from the queue, processes
//   it, and merges its local best result into the shared global best under
//   MY_MUTEX.  Workers continue until the queue is empty.
//
//   Pulling work dynamically from a shared queue provides better load
//   balancing than static partitioning, because work units that finish early
//   (e.g. those served entirely from the memoization cache) let a thread
//   immediately grab more work instead of sitting idle.
//
//   Memoization (see compute_hailstone) caches previously computed sequence
//   lengths to dramatically reduce redundant work.
//
// Usage:
//     Hailstone5 <limit>
//   <limit>  A positive integer; the largest starting number to test.
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
#include <adt/vector.asm>

    #call   main()
    #call   exit(r0)

#macro DEFINE_MUTEX(MY_MUTEX)       // Mutex guarding the shared gMAX/gIMAX result
#global gMAX:        .dci 0         // Longest sequence length found (shared)
#global gIMAX:       .dci 0         // Starting number that produced gMAX (shared)
#global gWORKSIZE:   .dci 100000    // Number of candidates per work unit
#global gWORKQUEUE:  .dci 0         // Address of the shared work-queue Vector
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
//      into the search limit.
//   2. Initializes the shared result mutex MY_MUTEX.
//   3. Creates the shared work-queue Vector and stores it in gWORKQUEUE.
//   4. Pushes work-unit start indices onto the queue.  Work unit 0 is handled
//      directly (to force the memoization cache to be allocated before threads
//      run), then the remaining work units are pushed in descending order.
//   5. Queries the number of CPU cores and spawns one worker thread per core,
//      each sharing the same queue.
//   6. Joins all worker threads.
//   7. Prints the overall maximum sequence length and the starting number
//      that produced it.
//
// Globals used:
//   gWORKSIZE (read), gWORKQUEUE (write), gMAX (read), gIMAX (read),
//   MY_MUTEX (init)
//
// Returns:
//   0 on success, 1 if the command line argument is missing.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    i, j, argc, arg, mImax, mMax, mWorksize, limit, queue, \
            workunits, cores, pid, pids
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
    move    limit, r0               // limit = upper bound on starting numbers
    load    mWorksize, gWORKSIZE
    #call   initializeMutex(MY_MUTEX)

    // Create the shared work queue and publish its address globally.
    #call   newVector(10)
    move    queue, r0
    store   queue, gWORKQUEUE
    
    // Process work unit 0 directly first.  This forces compute_hailstone to
    // lazily allocate and seed the memoization cache before any threads start,
    // avoiding a race on first allocation.
    #macro  PUSH_BACK(queue, 0)
    #call   worker(queue)

    // Push the remaining work-unit start indices (multiples of gWORKSIZE) onto
    // the queue in descending order: workunits, ..., 2*gWORKSIZE, gWORKSIZE.
    move    f0, limit
    move    f1, mWorksize
    div     f0, f1
    #call   ceil(f0)
    move    workunits, f0
    sub     workunits, 1
    mult    workunits, mWorksize
    neg     mWorksize               // step downward by gWORKSIZE
    #for    workunits, i > 0, mWorksize
        #macro  PUSH_BACK(queue, i)
    #end_for
    
    // Spawn one worker thread per CPU core, all sharing the same queue.
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
        #macro  create_thread(worker, queue)
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
    #call   puts("You must supply a positive integer argument.")
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
//   program relies on main() invoking worker() once (for work unit 0) BEFORE
//   spawning threads, which forces the cache to be allocated and seeded while
//   still single-threaded.
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
    jump    nz, BEGIN_COMPUTE
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

///////////////////////////////////////////////////////////////////////////////
// worker(queue)
// Worker thread entry point.  Repeatedly pulls work units from the shared
// queue and processes them until the queue is exhausted.
//
// Arguments:
//   queue  Address of the shared work-queue Vector.  Each element is the
//          starting index of a work unit; a work unit covers the candidate
//          range [d, d + gWORKSIZE).
//
// Behavior:
//   1. Pop a work-unit start index d from the queue (protected by the
//      Vector's own mutex, _VECTOR_MUTEX).  If the queue is empty, use -1 as
//      a sentinel meaning "no more work".
//   2. While a work unit is available:
//        a. Compute the hailstone sequence length for every candidate in
//           [d, d + gWORKSIZE), tracking the local best (wMax) and the starting
//           number that produced it (wImax).
//        b. Under MY_MUTEX, merge the local best into the global best
//           (gMAX/gIMAX).  The global best is updated when the local best is
//           longer, or equal in length but found at a smaller starting number.
//        c. Pop the next work unit from the queue.
//   3. Exit when no more work units remain.
//
// Globals used:
//   gWORKSIZE (read), gMAX (read/write), gIMAX (read/write),
//   MY_MUTEX (lock), queue's _VECTOR_MUTEX (lock)
//
// Returns:
//   Nothing.
///////////////////////////////////////////////////////////////////////////////

#def_func worker(queue)
    #var    i, d, hs, ws, pid, limit, wImax, wMax, quo, remain, q
    int     iGET_PID
    move    pid, r0
    load    q, queue
    // Pop the first work unit (or -1 if the queue is already empty).
    #sync   q[_VECTOR_MUTEX]
        #call   vectorIsEmpty(q)
        #if_cond    r0, eq, 0
            #macro  POP_BACK(q)
            move    d, r0
        #else_cond
            move    d, -1           // sentinel: no work available
        #end_cond
    #end_sync
    #while  d, ne, -1
        // Process the work unit covering [d, d + gWORKSIZE).
        move    limit, d
        load    ws, gWORKSIZE
        add     limit, ws
        load    r1, gLIMIT
        #macro  MIN(limit, r1)
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

        // Grab the next work unit (or -1 if the queue is now empty).
        #sync   q[_VECTOR_MUTEX]
            #call   vectorIsEmpty(q)
            #if_cond    r0, eq, 0
                #macro  POP_BACK(q)
                move    d, r0
            #else_cond
                move    d, -1
            #end_cond
        #end_sync
    #end_while
    #call   printf("Thread %d finishing...\n", pid)
#end_func

    stop
    stop