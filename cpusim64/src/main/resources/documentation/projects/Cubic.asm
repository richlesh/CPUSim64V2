///////////////////////////////////////////////////////////////////////////////
// Cubic.asm
//
// Solves cubic equations of the form:
//
//      A*x^3 + B*x^2 + C*x + D = 0
//
// using Cardano's formula. The coefficients A, B, C and D are supplied as
// floating point command line arguments. Depending on the sign of the
// discriminant, the program prints either:
//
//   - One real root and a pair of complex conjugate roots, or
//   - Three real roots where two are equal (discriminant ~ 0), or
//   - Three distinct real roots (trigonometric/Viete method).
//
// Usage:
//   Cubic A B C D
//
//   A B C D   Coefficients of A*x^3 + B*x^2 + C*x + D = 0 (floats). A must
//             be non-zero.
//
// Example:
//   > run.sh Cubic 1 -6 11 -6
//   Solving 1.000000*x^3 + -6.000000*x^2 + 11.000000*x + -6.000000 = 0
//   x1 = 3.000000
//   x2 = 2.000000
//   x3 = 1.000000
//
// Author: CPUSim64 Assistant
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

#define EPSILON 0.000000001

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses the four command line coefficients and dispatches to solve_cubic().
// Prints a usage message if the argument count is wrong or A is zero.
//
// Returns: 0 on success, 1 on usage error (in r0).
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    argc
    #fvar   A, B, C, D

    int     iARGC
    move    argc, r0
    #if_cond    argc != 5
        #call   putline("Syntax: Cubic A B C D   (solves A*x^3 + B*x^2 + C*x + D = 0)")
        #return 1
    #end_cond

    #call   args(1)
    #macro  PARSE_FLOAT(r0)
    move    A, f0
    #call   args(2)
    #macro  PARSE_FLOAT(r0)
    move    B, f0
    #call   args(3)
    #macro  PARSE_FLOAT(r0)
    move    C, f0
    #call   args(4)
    #macro  PARSE_FLOAT(r0)
    move    D, f0

    test    A
    #if_cond_sr z
        #call   putline("Error: coefficient A must be non-zero for a cubic equation.")
        #return 1
    #end_cond

    #call   printf("Solving %.6f*x^3 + %.6f*x^2 + %.6f*x + %.6f = 0\n", A, B, C, D)
    #call   solve_cubic(A, B, C, D)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// solve_cubic(A, B, C, D)
//
// Solves A*x^3 + B*x^2 + C*x + D = 0 using Cardano's method.
//
// The equation is normalized (divided by A) then depressed via the
// substitution x = t - B/(3A). The depressed cubic t^3 + p*t + q = 0 is then
// solved based on the sign of the discriminant:
//
//   disc > 0   -> one real root, two complex conjugate roots
//   disc ~ 0   -> three real roots, at least two equal
//   disc < 0   -> three distinct real roots (trigonometric method)
//
// Arguments:
//   A, B, C, D  Coefficients of the cubic (floats).
//
// Prints the roots to STDOUT. No return value.
///////////////////////////////////////////////////////////////////////////////
#def_func   solve_cubic(A0, B0, C0, D0)
    #var    k, kPlus1
    #fvar   a, b, c, d, p, q, disc, shift, sqrtDisc, negQhalf, u, v, t1, realp, \
            imagp, x1, x2, tmp1, tmp2, m, theta, pi_val, sqrt3reg, eps, neg_eps

    load    a, A0
    load    b, B0
    load    c, C0
    load    d, D0

    load    eps, EPSILON
    move    neg_eps, eps
    neg     neg_eps

    // Normalize by A: b = B/A, c = C/A, d = D/A
    div     b, a
    div     c, a
    div     d, a

    // shift = b/3, used to undo the depressing substitution (x = t - shift)
    move    shift, b
    div     shift, 3

    // p = c - b^2/3
    mult    f0, b, b
    div     f0, 3
    sub     p, c, f0

    // q = 2*b^3/27 - b*c/3 + d
    mult    f0, b, b
    mult    f0, b
    mult    f0, 2
    div     f0, 27
    mult    f1, b, c
    div     f1, 3
    sub     f0, f1
    add     q, f0, d

    // disc = (q/2)^2 + (p/3)^3
    move    f0, q
    div     f0, 2
    mult    f0, f0
    move    f1, p
    div     f1, 3
    mult    f2, f1, f1
    mult    f2, f1
    add     disc, f0, f2

    #call   pi()
    move    pi_val, f0
    load    f1, 3.0
    #call   sqrt(f1)
    move    sqrt3reg, f0

    #if_cond    disc > eps
        // ---- One real root, two complex conjugate roots ----
        #call   sqrt(disc)
        move    sqrtDisc, f0

        move    negQhalf, q
        div     negQhalf, 2
        neg     negQhalf

        add     f0, negQhalf, sqrtDisc
        #call   cbrt(f0)
        move    u, f0

        sub     f0, negQhalf, sqrtDisc
        #call   cbrt(f0)
        move    v, f0

        add     t1, u, v
        sub     x1, t1, shift

        // realp = -(u+v)/2  (depressed-space real part of complex pair)
        move    realp, t1
        div     realp, 2
        neg     realp

        // imagp = (u-v) * sqrt(3) / 2
        sub     f0, u, v
        mult    f0, sqrt3reg
        div     f0, 2
        move    imagp, f0

        // Shift back to original x-space for the complex pair
        sub     realp, realp, shift

        #call   printf("x1 = %.6f\n", x1)
        #call   printf("x2 = %.6f + %.6fi\n", realp, imagp)
        #call   printf("x3 = %.6f - %.6fi\n", realp, imagp)
    #else_if_cond    disc < neg_eps
        // ---- Three distinct real roots (trigonometric method) ----
        move    f0, p
        div     f0, 3
        neg     f0
        #call   sqrt(f0)
        move    m, f0
        mult    m, 2

        move    f0, q
        mult    f0, 3
        move    f1, p
        mult    f1, 2
        div     f0, f1
        move    tmp1, f0

        move    f0, 3
        div     f0, p
        neg     f0
        #call   sqrt(f0)
        move    tmp2, f0

        mult    tmp1, tmp2
        #call   acos(tmp1)
        move    theta, f0
        div     theta, 3

        #for    0, k < 3, 1
            move    f1, pi_val
            mult    f1, 2
            mult    f1, k
            div     f1, 3
            sub     f0, theta, f1
            #call   cos(f0)
            mult    f0, m
            sub     f0, shift
            move    kPlus1, k
            add     kPlus1, 1
            #call   printf("x%d = %.6f\n", kPlus1, f0)
        #end_for
    #else_cond
        // ---- Discriminant ~ 0: at least two real roots are equal ----
        move    f0, q
        div     f0, 2
        neg     f0
        #call   cbrt(f0)
        move    u, f0

        move    x1, u
        mult    x1, 2
        sub     x1, shift

        move    x2, u
        neg     x2
        sub     x2, shift

        #call   printf("x1 = %.6f\n", x1)
        #call   printf("x2 = %.6f\n", x2)
        #call   printf("x3 = %.6f\n", x2)
    #end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// cbrt(x)
//
// Computes the real (signed) cube root of x, handling negative values
// correctly since pow() only supports fractional exponents on non-negative
// bases. cbrt(x) = sign(x) * |x|^(1/3).
//
// Arguments:
//   x  Value to take the cube root of (float).
//
// Returns:
//   The signed cube root of x in f0.
///////////////////////////////////////////////////////////////////////////////
#def_func   cbrt(x)
    #fvar   val, absval, result

    load    val, x
    #call   fabs(val)
    move    absval, f0

    move    f1, 3
    recip   f1
    #call   pow(absval, f1)
    move    result, f0

    test    val
    #if_cond_sr n
        neg     result
    #end_cond
    #freturn result
#end_func

    stop
    stop