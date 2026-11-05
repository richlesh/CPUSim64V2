///////////////////////////////////////////////////////////////////////////////
// Congruent.asm
//
// Tests whether a positive integer n is a "congruent number" using Tunnell's
// theorem.  See https://en.wikipedia.org/wiki/Congruent_number
//
// A positive integer n is a congruent number if it is the area of a right
// triangle with three rational number sides.  Tunnell's theorem (1983) gives
// an effective (though BSD-conjecture-dependent) test:
//
//   Let m be the squarefree kernel of n (n divided by its largest square
//   factor -- congruence is invariant under multiplication by a rational
//   square, so testing m is equivalent to testing n).
//
//   If m is ODD, define:
//       t1 = #{(x,y,z) in Z^3 : 2x^2 + y^2 +  8z^2 = m}
//       t2 = #{(x,y,z) in Z^3 : 2x^2 + y^2 + 32z^2 = m}
//   then m is congruent  <=>  t1 == 2*t2   (assuming BSD for y^2=x^3-m^2*x)
//
//   If m is EVEN, define:
//       t1 = #{(x,y,z) in Z^3 : 8x^2 + 2y^2 + 16z^2 = m}
//       t2 = #{(x,y,z) in Z^3 : 8x^2 + 2y^2 + 64z^2 = m}
//   then m is congruent  <=>  t1 == 2*t2   (assuming BSD)
//
// This program brute-force counts the lattice points for each quadratic form
// by nested loops over x and y, solving for z^2 and checking it's a perfect
// square.  This is fine for modest values of n but gets slow for very large
// inputs since the work is roughly O(n).
//
// Usage:
//     Congruent <n>
//   <n>  A positive integer to test.
//
// Example:
//     > run.sh Congruent 5
//     n = 5 (squarefree kernel = 5)
//     t1 = 0, 2*t2 = 0
//     5 IS a congruent number (assuming BSD)
//
// Author: Richard Lesh
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.asm>

#undef  isqrt

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
// Program entry point.  Parses the command line integer, reduces it to its
// squarefree kernel, applies Tunnell's theorem using the appropriate pair of
// quadratic forms (odd or even case), and prints the result.
//
// Returns:
//   0 on success, 1 on a usage error (in r0).
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    argc, arg, n, m, t1, t2, isOdd, doubleT2

    int     iARGC
    move    argc, r0
    #if_cond    argc < 2
        #call   putline("Usage: Congruent <n>")
        #return 1
    #end_cond

    #call   args(1)                 // Get first command line argument
    move    arg, r0
    #macro  PARSE_INT(arg)          // Convert the argument string to an integer
    move    n, r0

    #if_cond    n <= 0
        #call   putline("n must be a positive integer")
        #return 1
    #end_cond

    // Reduce to the squarefree kernel; congruence is invariant under
    // multiplication by a rational square, so it suffices to test the kernel.
    #call   squarefreeKernel(n)
    move    m, r0

    and     isOdd, m, 1

    #if_cond    isOdd == 1
        // ODD case: 2x^2+y^2+8z^2=m  vs  2x^2+y^2+32z^2=m
        #call   countSolutions(m, 2, 1, 8)
        move    t1, r0
        #call   countSolutions(m, 2, 1, 32)
        move    t2, r0
    #else_cond
        // EVEN case: 8x^2+2y^2+16z^2=m  vs  8x^2+2y^2+64z^2=m
        #call   countSolutions(m, 8, 2, 16)
        move    t1, r0
        #call   countSolutions(m, 8, 2, 64)
        move    t2, r0
    #end_cond

    mult    doubleT2, t2, 2

    #call   printf("n = %d (squarefree kernel = %d)\n", n, m)
    #call   printf("t1 = %d, 2*t2 = %d\n", t1, doubleT2)

    #if_cond    t1 == doubleT2
        #call   printf("%d IS a congruent number (assuming BSD)\n", n)
    #else_cond
        #call   printf("%d is NOT a congruent number\n", n)
    #end_cond

    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// squarefreeKernel(n)
// Reduces n by repeatedly dividing out square factors i^2 (for i = 2, 3, ...)
// until none remain, leaving the squarefree kernel.  Does not require
// factoring n into primes; simply strips out any perfect-square divisor.
//
// Arguments:
//   n  A positive integer.
//
// Returns:
//   The squarefree kernel of n, in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   squarefreeKernel(n)
    #var    m, i, sq, quotient, remainder
    load    m, n
    move    i, 2
    mult    sq, i, i
    #while  sq <= m
        div     quotient, remainder, m, sq
        #if_cond    remainder == 0
            move    m, quotient          // strip out the square factor i^2
        #else_cond
            add     i, 1                 // try the next candidate
            mult    sq, i, i
        #end_cond
    #end_while
    #return m
#end_func

///////////////////////////////////////////////////////////////////////////////
// isqrt(n)
// Computes the integer square root (floor of the real square root) of a
// non-negative integer n.
//
// Arguments:
//   n  A non-negative integer.
//
// Returns:
//   floor(sqrt(n)) in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   isqrt(n)
    #var    i0
    #fvar   x
    load    i0, n
    move    x, i0
    #call   sqrt(x)
    #call   floor(f0)
    move    i0, f0
    #return i0
#end_func

///////////////////////////////////////////////////////////////////////////////
// computeLimit(target, divisor)
// Computes floor(sqrt(target / divisor)), the largest integer magnitude a
// variable could have in a*var^2 <= target.
//
// Arguments:
//   target    The target value (numerator).
//   divisor   The coefficient being solved for (denominator).
//
// Returns:
//   floor(sqrt(target / divisor)) in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   computeLimit(target, divisor)
    #var    t, d
    #fvar   ft, fd
    load    t, target
    load    d, divisor
    move    ft, t
    move    fd, d
    div     ft, fd
    #call   sqrt(ft)
    #call   floor(f0)
    move    t, f0
    #return t
#end_func

///////////////////////////////////////////////////////////////////////////////
// countSolutions(target, a, b, c)
// Counts the number of integer triples (x,y,z) satisfying:
//     a*x^2 + b*y^2 + c*z^2 = target
// by iterating x and y over their full (positive and negative) ranges and
// solving for z^2, then checking whether it is a perfect square.  Solutions
// with z == 0 count once; solutions with z != 0 count twice (for +z and -z).
//
// Arguments:
//   target  The right hand side of the equation.
//   a       Coefficient of x^2.
//   b       Coefficient of y^2.
//   c       Coefficient of z^2.
//
// Returns:
//   The number of integer triples (x,y,z) satisfying the equation, in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   countSolutions(target, a, b, c)
    #var    tgt, aa, bb, cc, x, y, xlim, ylim, remX, remXY, \
            zsq, zrem, zroot, count, tmp

    load    tgt, target
    load    aa, a
    load    bb, b
    load    cc, c
    move    count, 0

    #call   computeLimit(tgt, aa)
    move    xlim, r0
    move    tmp, xlim
    neg     tmp
    #for    tmp, x <= xlim, 1
        mult    remX, x, x
        mult    remX, aa                // remX = a*x^2
        #if_cond    remX <= tgt
            sub     remX, tgt, remX     // remX = target - a*x^2

            #call   computeLimit(remX, bb)
            move    ylim, r0
            move    tmp, ylim
            neg     tmp
            #for    tmp, y <= ylim, 1
                mult    remXY, y, y
                mult    remXY, bb       // remXY = b*y^2
                #if_cond    remXY <= remX
                    sub     remXY, remX, remXY   // remXY = remX - b*y^2

                    // remXY must equal c*z^2 for some integer z.
                    div     zsq, zrem, remXY, cc
                    #if_cond    zrem == 0
                        #call   isqrt(zsq)
                        move    zroot, r0
                        mult    tmp, zroot, zroot
                        #if_cond    tmp == zsq
                            #if_cond    zroot == 0
                                add     count, 1     // only z = 0
                            #else_cond
                                add     count, 2     // both +z and -z
                            #end_cond
                        #end_cond
                    #end_cond
                #end_cond
            #end_for
        #end_cond
    #end_for
    #return count
#end_func

    stop
    stop