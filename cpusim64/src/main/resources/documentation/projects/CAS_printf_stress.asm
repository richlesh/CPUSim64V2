///////////////////////////////////////////////////////////////////////////////
// CAS_printf_stress.asm
//
// Stress tests for emulator thread safety.
//
// Phase 1 - CAS:
//   One thread per core.  Each thread increments gLIB_COUNTER CAS_ITERS times
//   with get_and_increment() and gRAW_COUNTER CAS_ITERS times with an inline
//   CAS retry loop.  Both final values must equal threads * CAS_ITERS.  Any
//   shortfall means CAS is not atomic.
//
// Phase 2 - printf:
//   One thread per core.  Each thread prints PRINT_ITERS lines of the form
//     T<t> L<i> K<t*10000+i> abcdefghijklmnopqrstuvwxyz END
//   with a single printf call per line.  Every line must appear exactly once
//   and intact.  Verify with the shell commands described in the docs.
//
// Define USE_PRINT_MUTEX to wrap each printf in a mutex for comparison.
//
// Exit code: number of failed CAS checks (0 = pass).
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/system.asm>
#include <system/thread.asm>

#define CAS_ITERS 100000        // Increments per thread per counter
#define PRINT_ITERS 1000        // printf lines per thread
#define PAUSE_MS 10000          // Pause between phases (milliseconds)
// #define USE_PRINT_MUTEX      // Uncomment to serialize printf with a mutex

    #call   main()
    #call   exit(r0)

#global gLIB_COUNTER: .dci 0    // Incremented with get_and_increment()
#global gRAW_COUNTER: .dci 0    // Incremented with an inline CAS loop
#ifdef USE_PRINT_MUTEX
#macro  DEFINE_MUTEX(PRINT_MUTEX)
#endif

///////////////////////////////////////////////////////////////////////////////
// main()
// Runs the CAS phase, checks the counters, then runs the printf phase.
// Returns the number of failed CAS checks.
///////////////////////////////////////////////////////////////////////////////
#def_func main()
    #var    cores, pids, started, expected, actual, lost, failures

    int     iGET_NUM_CORES
    move    cores, r0
    #call   printf("Threads: %d\n", cores)
    #call   alloc(cores)
    move    pids, r0
    #if_cond    pids == 0
        #call   putline("Error: PID array allocation failed!")
        #return 1
    #end_cond
    clear   failures

    // ---------------- Phase 1: CAS ----------------
    #call   putline("=== CAS TEST BEGIN ===")
    #call   run_threads(cas_worker, pids, cores)
    move    started, r0
    move    expected, CAS_ITERS
    mult    expected, started           // expected = started * CAS_ITERS

    load    actual, gLIB_COUNTER
    #if_cond    actual == expected
        #call   printf("PASS get_and_increment: %d\n", actual)
    #else_cond
        sub     lost, expected, actual
        #call   printf("FAIL get_and_increment: expected %d, got %d (lost %d)\n", expected, actual, lost)
        add     failures, 1
    #end_cond

    load    actual, gRAW_COUNTER
    #if_cond    actual == expected
        #call   printf("PASS raw CAS loop: %d\n", actual)
    #else_cond
        sub     lost, expected, actual
        #call   printf("FAIL raw CAS loop: expected %d, got %d (lost %d)\n", expected, actual, lost)
        add     failures, 1
    #end_cond
    #call   putline("=== CAS TEST END ===")

    // Pause between phases so the CAS results can be read before printf floods the console.
    #call   putline("Pausing 10 seconds...")
    #call   sleep(PAUSE_MS)

    // ---------------- Phase 2: printf ----------------
#ifdef USE_PRINT_MUTEX
    #call   initializeMutex(PRINT_MUTEX)
    #call   putline("(printf serialized with PRINT_MUTEX)")
#endif
    move    expected, PRINT_ITERS
    mult    expected, cores
    #call   printf("=== PRINTF TEST BEGIN (expect %d lines) ===\n", expected)
    #call   run_threads(print_worker, pids, cores)
    #call   printf("=== PRINTF TEST END (%d threads started) ===\n", r0)
#ifdef USE_PRINT_MUTEX
    #call   freeMutex(PRINT_MUTEX)
#endif

    #call   free(pids)
    #return failures
#end_func

///////////////////////////////////////////////////////////////////////////////
// run_threads(entryArg, pidsArg, countArg)
// Starts countArg threads at entryArg, passing each its index 0..count-1,
// then joins them all.  Returns the number of threads successfully started.
///////////////////////////////////////////////////////////////////////////////
#def_func run_threads(entryArg, pidsArg, countArg)
    #var    entry, pids, count, i, pid, started

    load    entry, entryArg
    load    pids, pidsArg
    load    count, countArg
    clear   started
    #for    0, i < count, 1
        #macro  CREATE_THREAD(entry, i)
        move    pid, r0
        #if_cond    pid > 0
            store   pid, pids[i]
            add     started, 1
        #else_cond
            #call   printf("Error: thread %d failed to start!\n", i)
            store   0, pids[i]
        #end_cond
    #end_for
    #for    0, i < count, 1
        load    pid, pids[i]
        #if_cond    pid > 0
            #macro  JOIN_THREAD(pid)
        #end_cond
    #end_for
    #return started
#end_func

///////////////////////////////////////////////////////////////////////////////
// cas_worker(tArg)
// Increments gLIB_COUNTER via the library and gRAW_COUNTER via raw CAS,
// CAS_ITERS times each.
///////////////////////////////////////////////////////////////////////////////
#def_func cas_worker(tArg)
    #var    i, old, nxt, addr

    #for    0, i < CAS_ITERS, 1
        #call   get_and_increment(gLIB_COUNTER)
    #end_for

    move    addr, gRAW_COUNTER          // addr = &gRAW_COUNTER
    #for    0, i < CAS_ITERS, 1
$RETRY:
        load    old, addr               // old = *addr
        add     nxt, old, 1
        cas     old, nxt, addr, 0       // if *addr == old then *addr = nxt
        jump    no, $RETRY              // overflow clear -> CAS failed, retry
    #end_for
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// print_worker(tArg)
// Prints PRINT_ITERS uniquely keyed lines, one printf call per line.
///////////////////////////////////////////////////////////////////////////////
#def_func print_worker(tArg)
    #var    t, i, key

    load    t, tArg
    #for    0, i < PRINT_ITERS, 1
        mult    key, t, 10000
        add     key, i                  // key = t * 10000 + i
#ifdef USE_PRINT_MUTEX
        #sync   PRINT_MUTEX
            #call   printf("T%d L%d K%d abcdefghijklmnopqrstuvwxyz END\n", t, i, key)
        #end_sync
#else
        #call   printf("T%d L%d K%d abcdefghijklmnopqrstuvwxyz END\n", t, i, key)
#endif
    #end_for
    #return 0
#end_func

    stop
    stop
