///////////////////////////////////////////////////////////////////////////////
// Hanoi.asm
//
// Solves the Tower of Hanoi puzzle.
//
// Usage: Hanoi [disks]
//
// Author:   Richard Lesh
// Original: 2026/06/30
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.asm>

    #call  main()
    #call  exit(r0)

///////////////////////////////////////////////////////////////////////////////
// Global state
///////////////////////////////////////////////////////////////////////////////

#global gNUM_DISCS:   .dci 0
#global gMOVE_NUMBER: .dci 0
#global gPRINT_MOD:   .dci 1
// gPEGS will hold a heap allocated block of 3 * gNUM_DISCS ints, laid out
// peg-major: index = peg * gNUM_DISCS + slot
#global gPEGS:        .dci 0

///////////////////////////////////////////////////////////////////////////////
// pegIndex(peg, slot) -> r0
// Computes the linear array index for peg/slot into the gPEGS block.
// index = peg * gNUM_DISCS + slot
///////////////////////////////////////////////////////////////////////////////

#def_func   pegIndex(peg, slot)
    #var    p, s, n
    load    p, peg
    load    s, slot
    load    n, gNUM_DISCS
    mult    p, n
    add     p, s
    #return p
#end_func

///////////////////////////////////////////////////////////////////////////////
// getDisk(peg, slot) -> r0
// Returns gPEGS[peg][slot]
///////////////////////////////////////////////////////////////////////////////

#def_func   getDisk(peg, slot)
    #var    p, s, base, idx
    load    p, peg
    load    s, slot
    #call   pegIndex(p, s)
    move    idx, r0
    load    base, gPEGS
    load    r0, base[idx]
    #return r0
#end_func

///////////////////////////////////////////////////////////////////////////////
// setDisk(peg, slot, value)
// Sets gPEGS[peg][slot] = value
///////////////////////////////////////////////////////////////////////////////

#def_func   setDisk(peg, slot, value)
    #var    p, s, v, base, idx
    load    p, peg
    load    s, slot
    load    v, value
    #call   pegIndex(p, s)
    move    idx, r0
    load    base, gPEGS
    store   v, base[idx]
#end_func

///////////////////////////////////////////////////////////////////////////////
// print_pegs()
// Prints the current state of the three pegs.
///////////////////////////////////////////////////////////////////////////////

#def_func   print_pegs()
    #var    i, j, n, disk, moveNum
    load    moveNum, gMOVE_NUMBER
    #call   printf("Move: %d\n", moveNum)
    load    n, gNUM_DISCS
    #for    0, i, lt, n, 1
        #for    0, j, lt, 3, 1
            #call   getDisk(j, i)
            move    disk, r0
            #if_cond    disk, eq, 0
                #call   puts("|")
            #else_cond
                #call   put_dec(disk)
            #end_cond
            #call   puts("   ")
        #end_for
        #call   put_nl()
    #end_for
    #call   puts("=========\n")
    #call   put_nl()
#end_func

///////////////////////////////////////////////////////////////////////////////
// make_move(source, dest)
// Moves the top disk from source peg to dest peg.
///////////////////////////////////////////////////////////////////////////////

#def_func   make_move(source, dest)
    #var    src, dst, n, i, disk, pos, mn, pm, quo, rem
    load    src, source
    load    dst, dest

    // ++gMOVE_NUMBER
    load    mn, gMOVE_NUMBER
    add     mn, 1
    store   mn, gMOVE_NUMBER

    load    n, gNUM_DISCS

    // Find top disk on source peg
    move    disk, 0
    #for    0, i, lt, n, 1
        #call   getDisk(src, i)
        #if_cond    r0, ne, 0
            move    disk, r0
            // gPEGS[source][i] = 0
            #call   setDisk(src, i, 0)
            #break
        #end_cond
    #end_for

    // Find landing position on dest peg (default n-1)
    sub     pos, n, 1
    #for    0, i, lt, n, 1
        #call   getDisk(dst, i)
        #if_cond    r0, ne, 0
            sub     pos, i, 1
            #break
        #end_cond
    #end_for

    // gPEGS[dest][pos] = disk
    #call   setDisk(dst, pos, disk)

    // if (gMOVE_NUMBER % gPRINT_MOD == 0) print_pegs()
    load    mn, gMOVE_NUMBER
    load    pm, gPRINT_MOD
    div     quo, rem, mn, pm
    #if_cond    rem, eq, 0
        #call   print_pegs()
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// solve(num_disks, source, dest, aux)
// Recursively solves the Tower of Hanoi.
///////////////////////////////////////////////////////////////////////////////

#def_func   solve(num_disks, source, dest, aux)
    #var    nd, src, dst, ax, nd1
    load    nd, num_disks
    load    src, source
    load    dst, dest
    load    ax, aux
    #if_cond    nd, eq, 1
        #call   make_move(src, dst)
    #else_cond
        sub     nd1, nd, 1
        // solve(num_disks - 1, source, aux, dest)
        #call   solve(nd1, src, ax, dst)
        // move(source, dest)
        #call   make_move(src, dst)
        // solve(num_disks - 1, aux, dest, source)
        #call   solve(nd1, ax, dst, src)
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// main()
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    argc, n, i, j, base, steps, pm, rem, quo, mn
    #fvar   fsteps

    // if (argc != 2) error
    int     iARGC
    move    argc, r0
    #if_cond    argc, ne, 2
        #call   puts("Syntax: Hanoi [disks]\n")
        #call   exit(1)
    #end_cond

    // gNUM_DISCS = stoi(argv[1], default 3)
    #call   args(1)
    #macro  PARSE_INT(r0)
    move    n, r0
    #if_cond    n, le, 0
        move    n, 3
    #end_cond
    store   n, gNUM_DISCS

    // Allocate gPEGS block: 3 * gNUM_DISCS ints
    mult    r1, n, 3
    add     r1, 1           // account for size word from iALLOC
    int     iALLOC
    #if_cond    r0, eq, 0
        #call   puts("Allocation failed!\n")
        #call   exit(1)
    #end_cond
    move    base, r0
    store   base, gPEGS

    // Initialize all positions to 0
    mult    r1, n, 3        // number of ints to clear
    move    r2, r1
    move    r1, base
    int     iMEMCLEAR

    // gPEGS[0][i] = i + 1 for all disks on peg 0
    #for    0, i, lt, n, 1
        add     r0, i, 1
        #call   setDisk(0, i, r0)
    #end_for

    // SOLUTION_STEPS = pow(2, gNUM_DISCS) - 1
    load    f1, 2.0
    move    f2, n
    int     iPOW
    sub     f0, 1.0
    int     iROUND
    move    steps, f0

    // gPRINT_MOD = SOLUTION_STEPS / 5 + 2
    div     pm, steps, 5
    add     pm, 2
    // gPRINT_MOD -= gPRINT_MOD % 5
    div     quo, rem, pm, 5
    sub     pm, rem
    // gPRINT_MOD = max(gPRINT_MOD, 1)
    #if_cond    pm, lt, 1
        move    pm, 1
    #end_cond
    store   pm, gPRINT_MOD

    // Print initial state
    #call   print_pegs()

    // solve(gNUM_DISCS, 0, 2, 1)
    #call   solve(n, 0, 2, 1)

    // if (gMOVE_NUMBER % gPRINT_MOD != 0) print_pegs()
    load    mn, gMOVE_NUMBER
    load    pm, gPRINT_MOD
    div     quo, rem, mn, pm
    #if_cond    rem, ne, 0
        #call   print_pegs()
    #end_cond

    // Free gPEGS block
    load    r1, gPEGS
    int     iFREE

    #return 0
#end_func

    stop
    stop