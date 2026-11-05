///////////////////////////////////////////////////////////////////////////////
// Quadratic.asm
//
// Solves a quadratic equation of the form:
//
//      A*x^2 + B*x + C = 0
//
// using the quadratic formula:
//
//               -B +/- sqrt(B^2 - 4AC)
//          x = ------------------------
//                        2A
//
// The three coefficients A, B and C are supplied as floating point values on
// the command line. Depending on the discriminant (B^2 - 4AC) the program
// prints one of three results:
//
//      Discriminant > 0   -> two distinct real roots
//      Discriminant == 0  -> one repeated (double) root
//      Discriminant < 0   -> zero real roots (roots are complex)
//
// If A is zero the equation is not actually quadratic and an error message
// is printed instead.
//
// Usage:
//      Quadratic A B C
//
//      A  Coefficient of x^2 (must be non-zero)
//      B  Coefficient of x
//      C  Constant term
//
// Examples:
//      > run.sh Quadratic 1 -3 2
//      Two real roots: x1 = 2.000000, x2 = 1.000000
//
//      > run.sh Quadratic 1 2 1
//      One repeated (double) root: x = -1.000000
//
//      > run.sh Quadratic 1 0 1
//      No real roots (discriminant = -4.000000 is negative).
//
// Author: CPUSim64 Assistant
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses exactly three command line arguments as floating point coefficients
// A, B and C, then dispatches to solve_quadratic() to compute and print the
// roots. Prints a usage message and returns a non-zero exit code if the
// wrong number of arguments is supplied.
//
// Returns: 0 on success, 1 on usage error (in r0).
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    argc
    #fvar   a, b, c

    // Require exactly 4 command line entries: program name + A + B + C.
    int     iARGC
    move    argc, r0
    #if_cond    argc != 4
        #call   putline("Syntax: Quadratic A B C   (solves A*x^2 + B*x + C = 0)")
        #return 1
    #end_cond

    // ---- Parse coefficient A ----
    #call   args(1)
    #macro  PARSE_FLOAT(r0)
    move    a, f0

    // ---- Parse coefficient B ----
    #call   args(2)
    #macro  PARSE_FLOAT(r0)
    move    b, f0

    // ---- Parse coefficient C ----
    #call   args(3)
    #macro  PARSE_FLOAT(r0)
    move    c, f0

    // Solve and print the result.
    #call   solve_quadratic(a, b, c)

    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// solve_quadratic(a, b, c)
//
// Computes the discriminant (B^2 - 4AC) and, based on its sign, prints:
//      - two distinct real roots (discriminant > 0)
//      - one repeated (double) root (discriminant == 0)
//      - a message indicating there are no real roots (discriminant < 0)
//
// If A is zero, the equation is not quadratic; an error message is printed
// and no roots are computed.
//
// Arguments:
//      a, b, c   The floating point coefficients of the equation.
//
// Returns: nothing meaningful (r0/f0 undefined)
///////////////////////////////////////////////////////////////////////////////
#def_func   solve_quadratic(a0, b0, c0)
    #fvar   A, B, C, disc, temp, sqrtD, twoA, negB, root1, root2, zero, negDisc, realPart, imagPart

    load    A, a0
    load    B, b0
    load    C, c0
    move    zero, 0             // 0.0 used for float comparisons

    // A quadratic equation requires a non-zero leading coefficient.
    #if_cond    A == zero
        #call   putline("Not a quadratic equation: coefficient A must be non-zero.")
        #return 0
    #end_cond

    // discriminant = B^2 - 4*A*C
    move    disc, B
    mult    disc, B             // disc = B * B
    move    temp, A
    mult    temp, C             // temp = A * C
    mult    temp, 4             // temp = 4 * A * C
    sub     disc, temp          // disc = B^2 - 4AC

    // twoA = 2 * A (denominator used by both roots)
    move    twoA, A
    mult    twoA, 2

    #if_cond    disc > zero
        // Two distinct real roots.
        #call   sqrt(disc)
        move    sqrtD, f0

        move    negB, B
        neg     negB

        move    root1, negB
        add     root1, sqrtD
        div     root1, twoA     // root1 = (-B + sqrt(disc)) / 2A

        move    root2, negB
        sub     root2, sqrtD
        div     root2, twoA     // root2 = (-B - sqrt(disc)) / 2A

        #call   printf("Two real roots: x1 = %.6f, x2 = %.6f\n", root1, root2)
    #else_if_cond    disc == zero
        // Exactly one repeated (double) root.
        move    negB, B
        neg     negB
        move    root1, negB
        div     root1, twoA     // root1 = -B / 2A

        #call   printf("One repeated (double) root: x = %.6f\n", root1)
    #else_cond
        // Discriminant is negative: roots are a complex conjugate pair.
        //   x = -B/2A +/- (sqrt(-disc)/2A) i
        move    negDisc, disc
        neg     negDisc              // negDisc = -disc, now positive

        #call   sqrt(negDisc)
        move    imagPart, f0
        div     imagPart, twoA       // imaginary part magnitude = sqrt(-disc) / 2A
        #call   fabs(imagPart)
        move    imagPart, f0         // normalize sign for clean +/- display

        move    negB, B
        neg     negB
        move    realPart, negB
        div     realPart, twoA       // real part = -B / 2A

        #call   printf("Two complex roots: x1 = %.6f + %.6fi, x2 = %.6f - %.6fi\n", realPart, imagPart, realPart, imagPart)
    #end_cond

    #return 0
#end_func

    stop
    stop