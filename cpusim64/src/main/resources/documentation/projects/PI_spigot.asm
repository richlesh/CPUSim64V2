///////////////////////////////////////////////////////////////////////////////
// PI_spigot.asm
//
// Prints pi as "3." followed by the first n decimal digits after the
// decimal point using the Rabinowitz-Wagon decimal spigot algorithm.
//
// Spigot algorithm (for D digits):
//   len = floor(10 * D / 3) + 1,  a[1..len] = 2
//   repeat D times:
//       q = 0
//       for i = len down to 1:
//           x    = 10 * a[i] + q * i
//           a[i] = x mod (2i - 1)
//           q    = x div (2i - 1)
//       a[1] = q mod 10,  q = q div 10
//       if q == 9:  hold it (nines++)
//       if q == 10: emit predigit+1, then held nines as 0s, predigit = 0
//       else:       emit predigit, then held nines as 9s, predigit = q
//
// Positions:  position 0 is the leading "3" (followed by a '.'),
//             positions 1..n are the digits after the decimal point.
//
// Usage:
//   run.sh PI_spigot <n>
//
// Example:
//   > run.sh PI_spigot 20
//   3.14159265358979323846
//
//   Feynman point (six nines)
//   > run.sh PI_spigot 770
//   3.1415....999999837
//
// Author: AI Assistant
// Date:   2026/11/05
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.def>
#include <system/system.asm>

#define GUARD_DIGITS 10     // Extra digits computed so held 9s resolve

    #call   main()
    #call   exit(r0)

#global gEMIT_INDEX: .dci -2    // Position of the last emitted digit (dummy predigit -> -1)
#global gSTART:      .dci 0     // First position to print (the leading 3)
#global gEND:        .dci 0     // One past last position to print (n + 1)

///////////////////////////////////////////////////////////////////////////////
// emit_digit(digit)
// Advances the emitted digit position and prints the digit if the position
// falls in the range [gSTART, gEND).  A '.' is printed after position 0 (the
// leading 3).  The very first value emitted by the spigot is a dummy predigit
// 0 which lands at position -1 and is never printed.
///////////////////////////////////////////////////////////////////////////////
#def_func emit_digit(digit)
    #var    d, idx, s, e
    load    d, digit
    load    idx, gEMIT_INDEX
    add     idx, 1
    store   idx, gEMIT_INDEX
    load    s, gSTART
    load    e, gEND
    #if_cond    idx >= s
        #if_cond    idx < e
            add     d, 48               // Convert 0-9 to '0'-'9'
            #call   putc(d)
            #if_cond    idx == 0
                #call   putc('.')       // Decimal point after the leading 3
            #end_cond
        #end_cond
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// emit_run(digit, count)
// Emits the same digit count times (used to release held nines).
///////////////////////////////////////////////////////////////////////////////
#def_func emit_run(digit, count)
    #var    d, c, i
    load    d, digit
    load    c, count
    #for    0, i < c, 1
        #call   emit_digit(d)
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// main()
// Reads n from the command line, runs the spigot and prints "3." followed
// by the first n digits after the decimal point.
// Returns 0 on success, 1 on error.
///////////////////////////////////////////////////////////////////////////////
#def_func main()
    #var    n, len, a, j, i, q, x, den, predigit, nines, total

    int     iARGC
    #if_cond    r0 != 2
        #call   putline("Usage: PI_spigot <n>   (n = number of digits after the decimal point)")
        #return 1
    #end_cond
    #call   args(1)
    #macro  PARSE_INT(r0)
    move    n, r0
    #if_cond    n < 1
        #call   putline("n must be >= 1")
        #return 1
    #end_cond

    // Print positions [0, n + 1): the leading 3 plus n digits after the '.'
    // (gSTART is already initialized to 0)
    add     r0, n, 1
    store   r0, gEND

    // Total digits to generate: the leading 3, n digits, plus guard digits
    add     total, n, 1
    add     total, GUARD_DIGITS

    // len = floor(10 * total / 3) + 1
    mult    len, total, 10
    div     len, len, 3
    add     len, 1

    // Allocate len + 1 words so we can index a[1..len]
    add     r1, len, 1
    #call   alloc(r1)
    move    a, r0
    #if_cond    a == 0
        #call   putline("Unable to allocate spigot array!")
        #return 1
    #end_cond

    // a[i] = 2 for all i
    #for    1, i <= len, 1
        store   2, a[i]
    #end_for

    clear   nines, predigit
    #for    0, j < total, 1
        // Multiply the mixed-radix representation by 10 and carry right-to-left
        clear   q
        move    i, len
        #while  i > 0
            add     den, i, i           // den = 2i - 1
            sub     den, 1
            load    x, a[i]             // x = 10 * a[i] + q * i
            mult    x, 10
            mult    r0, q, i
            add     x, r0
            div     q, r1, x, den       // q = x / den, r1 = x % den
            store   r1, a[i]
            sub     i, 1
        #end_while

        // a[1] = q mod 10, q = q div 10  (q is the next raw digit)
        move    x, q
        div     q, r1, x, 10
        store   r1, a[1]

        #if_cond    q == 9
            // Can't decide yet whether a carry will arrive; hold the 9
            add     nines, 1
        #else_if_cond   q == 10
            // Carry: bump predigit, held 9s roll over to 0s
            add     r0, predigit, 1
            #call   emit_digit(r0)
            #call   emit_run(0, nines)
            clear   predigit, nines
        #else_cond
            // No carry: release predigit and held 9s
            #call   emit_digit(predigit)
            move    predigit, q
            #call   emit_run(9, nines)
            clear   nines
        #end_cond

        // Stop early once all requested digits have been printed
        load    r0, gEMIT_INDEX
        add     r0, 1
        load    r1, gEND
        #if_cond    r0 >= r1
            #break
        #end_cond
    #end_for

    #call   put_nl()
    #call   free(a)
    #return 0
#end_func

    stop
    stop