///////////////////////////////////////////////////////////////////////////////
// BBP_hex_digits.asm
//
// Computes 32 hexadecimal digits of pi starting at the nth position using the
// Bailey-Borwein-Plouffe (BBP) digit-extraction algorithm.  Unlike a spigot,
// BBP jumps directly to position n without computing the preceding digits.
//
// BBP formula:
//          inf    1    (   4        2        1        1   )
//   pi =   SUM  ----- ( ------ - ------ - ------ - ------ )
//          k=0   16^k  ( 8k + 1   8k + 4   8k + 5   8k + 6 )
//
// Digit extraction (d = n - 1):
//   frac(16^d * pi) = frac(4*S(1) - 2*S(4) - S(5) - S(6))
// where
//   S(j) = SUM_{k=0..d}  (16^(d-k) mod (8k+j)) / (8k+j)     (left sum)
//        + SUM_{k>d}     16^(d-k) / (8k+j)                   (right tail)
//
// Fixed-point arithmetic:
//   All fractions are held as 192-bit unsigned fixed-point values in three
//   64-bit words (hi:mid:lo) representing value * 2^192.  Integer overflow
//   past 2^192 simply wraps, which automatically keeps only the fractional
//   part.  Each term r/m is computed exactly to 192 bits by integer long
//   division in 32-bit chunks, so the truncation error per term is < 2^-192.
//   With the 4/2/1/1 coefficients and at most d+48 terms per series, the
//   accumulated error is < 8*(d+48)*2^-192 < 2^-165, leaving more than 37
//   guard bits below the top 128 bits (32 hex digits in the hi:mid words).
//   As with any BBP computation, a long run of 0s or Fs just past the last
//   printed digit could still let the error carry into that digit, so the
//   program checks the guard bits in the lo word and prints a warning when
//   the last printed digit could be off by one.
//
// Positions:  n = 1 is the first hex digit after the point.
//             pi = 3.243F6A8885A308D313198A2E03707344...  (hexadecimal)
//
// Limits:     n <= 10,000,000 so that (8k+j)^2 fits in a signed 64-bit
//             integer (powmod16) and 8k+j < 2^31 (FRAC192).
//
// Usage:
//   run.sh BBP_hex_digits <n>
//
// Example:
//   > run.sh BBP_hex_digits 1
//   Hex digits of pi at position 1: 243F6A8885A308D313198A2E03707344
//
// Author: AI Assistant
// Date:   2026/11/05
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.def>
#include <system/system.asm>

#define HEX_PER_WORD 16         // Hex digits held in one 64-bit word
#define MAX_POSITION 10000000   // Largest supported position n
#define GUARD_SHIFT 32          // Examine the top 64 - GUARD_SHIFT guard bits

///////////////////////////////////////////////////////////////////////////////
// FRAC192(num, den, hi, mid, lo)
// hi:mid:lo = floor(num * 2^192 / den) computed by long division in six
// 32-bit chunks.  Requires 0 <= num < den < 2^31.  Uses r0, r1, r2 as
// temporaries.
///////////////////////////////////////////////////////////////////////////////
#def_macro FRAC192(num, den, hi, mid, lo)
    move    r0, ${num}              // r0 = running remainder
    lshift  r1, r0, 32
    div     ${hi}, r0, r1, ${den}   // bits 191..160
    lshift  r1, r0, 32
    div     r2, r0, r1, ${den}      // bits 159..128
    lshift  ${hi}, 32
    or      ${hi}, r2
    lshift  r1, r0, 32
    div     ${mid}, r0, r1, ${den}  // bits 127..96
    lshift  r1, r0, 32
    div     r2, r0, r1, ${den}      // bits  95..64
    lshift  ${mid}, 32
    or      ${mid}, r2
    lshift  r1, r0, 32
    div     ${lo}, r0, r1, ${den}   // bits  63..32
    lshift  r1, r0, 32
    div     r2, r0, r1, ${den}      // bits  31..0
    lshift  ${lo}, 32
    or      ${lo}, r2
#end_macro

///////////////////////////////////////////////////////////////////////////////
// ADD192(ahi, amid, alo, bhi, bmid, blo)
// ahi:amid:alo = ahi:amid:alo + bhi:bmid:blo  (mod 2^192)
// Unsigned carry out of a + b:
//   carry = ((a & b) | ((a | b) & ~sum)) >>> 63
// Adding the low-word carry (0 or 1) to the middle word overflows only when
// the result wraps to zero, which can't happen if amid + bmid already
// carried, so the two middle-word carries are simply OR'ed together.
// Uses r0, r1, r2, r3 as temporaries.
///////////////////////////////////////////////////////////////////////////////
#def_macro ADD192(ahi, amid, alo, bhi, bmid, blo)
    and     r1, ${alo}, ${blo}
    or      r2, ${alo}, ${blo}
    add     ${alo}, ${blo}
    move    r3, ${alo}
    compl   r3
    and     r2, r3
    or      r1, r2
    rshift  r1, 63                  // r1 = carry out of low word
    and     r0, ${amid}, ${bmid}
    or      r2, ${amid}, ${bmid}
    add     ${amid}, ${bmid}
    move    r3, ${amid}
    compl   r3
    and     r2, r3
    or      r0, r2
    rshift  r0, 63                  // r0 = carry out of amid + bmid
    add     ${amid}, r1
    test    ${amid}
    move    z, r1, r1, 0            // r1 = carry from adding low carry
    or      r0, r1                  // r0 = carry out of middle word
    add     ${ahi}, ${bhi}
    add     ${ahi}, r0
#end_macro

///////////////////////////////////////////////////////////////////////////////
// SUB192(ahi, amid, alo, bhi, bmid, blo)
// ahi:amid:alo = ahi:amid:alo - bhi:bmid:blo  (mod 2^192)
// Unsigned borrow out of a - b:
//   borrow = ((~a & b) | (~(a ^ b) & diff)) >>> 63
// Subtracting the low-word borrow (0 or 1) from the middle word borrows only
// when the result wraps to all ones (-1), which can't happen if amid - bmid
// already borrowed, so the two middle-word borrows are simply OR'ed together.
// Uses r0, r1, r2 as temporaries.
///////////////////////////////////////////////////////////////////////////////
#def_macro SUB192(ahi, amid, alo, bhi, bmid, blo)
    move    r1, ${alo}
    compl   r1
    and     r1, ${blo}              // ~a & b
    xor     r2, ${alo}, ${blo}
    compl   r2                      // ~(a ^ b)
    sub     ${alo}, ${blo}          // diff
    and     r2, ${alo}
    or      r1, r2
    rshift  r1, 63                  // r1 = borrow out of low word
    move    r0, ${amid}
    compl   r0
    and     r0, ${bmid}             // ~a & b
    xor     r2, ${amid}, ${bmid}
    compl   r2                      // ~(a ^ b)
    sub     ${amid}, ${bmid}        // diff
    and     r2, ${amid}
    or      r0, r2
    rshift  r0, 63                  // r0 = borrow out of amid - bmid
    sub     ${amid}, r1
    cmp     ${amid}, -1
    move    z, r1, r1, 0            // r1 = borrow from subtracting low borrow
    or      r0, r1                  // r0 = borrow out of middle word
    sub     ${ahi}, ${bhi}
    sub     ${ahi}, r0
#end_macro

///////////////////////////////////////////////////////////////////////////////
// SHL192(hi, mid, lo, n, rn)
// hi:mid:lo = hi:mid:lo << n  (mod 2^192) for a constant 0 < n < 64.
// rn must be the constant 64 - n.  Uses r1 as a temporary.
///////////////////////////////////////////////////////////////////////////////
#def_macro SHL192(hi, mid, lo, n, rn)
    lshift  ${hi}, ${n}
    rshift  r1, ${mid}, ${rn}
    or      ${hi}, r1
    lshift  ${mid}, ${n}
    rshift  r1, ${lo}, ${rn}
    or      ${mid}, r1
    lshift  ${lo}, ${n}
#end_macro

    #call   main()
    #call   exit(r0)

#global gSERIES_HI: .dci 0          // High word of the last series() result
#global gSERIES_MID: .dci 0         // Middle word of the last series() result
#global gSERIES_LO: .dci 0          // Low word of the last series() result

///////////////////////////////////////////////////////////////////////////////
// main()
// Reads n from the command line, computes frac(16^(n-1) * pi) as a 192-bit
// fixed-point fraction with the BBP formula and prints the top 32 hex digits.
// Returns 0 on success, 1 on error.
///////////////////////////////////////////////////////////////////////////////
#def_func main()
    #var    n, d, accHi, accMid, accLo, tHi, tMid, tLo, guard

    int     iARGC
    #if_cond    r0 != 2
        #call   putline("Usage: BBP_hex_digits <n>   (n = 1 is the first hex digit after the point)")
        #return 1
    #end_cond
    #call   args(1)
    #macro  PARSE_INT(r0)
    move    n, r0
    #if_cond    n < 1
        #call   putline("n must be >= 1")
        #return 1
    #end_cond
    #if_cond    n > MAX_POSITION
        #call   printf("n must be <= %d\n", MAX_POSITION)
        #return 1
    #end_cond

    sub     d, n, 1                     // d = n - 1

    // acc = 4 * S(1)   (192-bit shift left by 2)
    #call   series(1, d)
    load    accHi, gSERIES_HI
    load    accMid, gSERIES_MID
    load    accLo, gSERIES_LO
    #macro  SHL192(accHi, accMid, accLo, 2, 62)

    // acc -= 2 * S(4)  (192-bit shift left by 1, then subtract)
    #call   series(4, d)
    load    tHi, gSERIES_HI
    load    tMid, gSERIES_MID
    load    tLo, gSERIES_LO
    #macro  SHL192(tHi, tMid, tLo, 1, 63)
    #macro  SUB192(accHi, accMid, accLo, tHi, tMid, tLo)

    // acc -= S(5)
    #call   series(5, d)
    load    tHi, gSERIES_HI
    load    tMid, gSERIES_MID
    load    tLo, gSERIES_LO
    #macro  SUB192(accHi, accMid, accLo, tHi, tMid, tLo)

    // acc -= S(6)
    #call   series(6, d)
    load    tHi, gSERIES_HI
    load    tMid, gSERIES_MID
    load    tLo, gSERIES_LO
    #macro  SUB192(accHi, accMid, accLo, tHi, tMid, tLo)

    #call   printf("Hex digits of pi at position %d: ", n)

    // The top 128 bits of the fraction (hi and mid words) are the next 32
    // hex digits.  The lo word only serves as guard bits.
    #call   put_hex_word(accHi)
    #call   put_hex_word(accMid)
    #call   put_nl()

    // Guard-bit check: the accumulated error is < 2^-165, i.e. < 2^27 units
    // of the lo word.  The printed digits can only be affected if accLo is
    // within that error of a word boundary, which shows up as the top bits
    // of accLo being all 0s or all 1s.  The arithmetic shift collapses those
    // cases to 0 and -1.  GUARD_SHIFT = 32 gives 5 bits of extra margin.
    arshift guard, accLo, GUARD_SHIFT
    #if_cond    guard == 0
        #call   putline("Warning: guard bits are all 0s; the last digit(s) may be 1 too high.")
    #else_if_cond   guard == -1
        #call   putline("Warning: guard bits are all 1s; the last digit(s) may be 1 too low.")
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// put_hex_word(wArg)
// Prints all 64 bits of wArg as 16 uppercase hex digits (zero padded) by
// rotating each nibble into the low 4 bits, most significant first.
///////////////////////////////////////////////////////////////////////////////
#def_func put_hex_word(wArg)
    #var    w, i, digit

    load    w, wArg
    #for    0, i < HEX_PER_WORD, 1
        lrotate w, 4
        and     digit, w, 15
        #if_cond    digit < 10
            add     digit, 48           // '0' .. '9'
        #else_cond
            add     digit, 55           // 'A' .. 'F'  ('A' - 10 = 55)
        #end_cond
        #call   putc(digit)
    #end_for
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// series(j, d)
// Computes the fractional part of
//   S(j) = SUM_{k=0..d} (16^(d-k) mod (8k+j)) / (8k+j)
//        + SUM_{k>d}    16^(d-k) / (8k+j)
// as a 192-bit fixed-point fraction.  The result is stored in the globals
// gSERIES_HI, gSERIES_MID and gSERIES_LO.
///////////////////////////////////////////////////////////////////////////////
#def_func series(jArg, dArg)
    #var    j, d, k, m, t, s, sHi, sMid, sLo, tHi, tMid, tLo

    load    j, jArg
    load    d, dArg
    clear   sHi, sMid, sLo

    // Left sum: k = 0 .. d, exact (16^(d-k) mod m) / m
    #for    0, k <= d, 1
        mult    m, k, 8
        add     m, j                    // m = 8k + j
        sub     r1, d, k                // p = d - k
        #call   powmod16(r1, m)         // r0 = 16^p mod m
        #macro  FRAC192(r0, m, tHi, tMid, tLo)
        #macro  ADD192(sHi, sMid, sLo, tHi, tMid, tLo)
    #end_for

    // Right tail: k = d + t, term = (1/m) / 16^t = (1/m) >> 4t
    // Terms are zero in 192 bits once 4t >= 192.
    #for    1, t < 48, 1
        add     k, d, t
        mult    m, k, 8
        add     m, j                    // m = 8k + j
        #macro  FRAC192(1, m, tHi, tMid, tLo) // tHi:tMid:tLo = 1/m
        mult    s, t, 4                 // shift amount = 4t
        // Whole-word shifts first (s >= 64), then any remaining bit shift.
        #while  s >= 64
            move    tLo, tMid
            move    tMid, tHi
            clear   tHi
            sub     s, 64
        #end_while
        #if_cond    s != 0
            move    r1, 64
            sub     r1, s               // r1 = 64 - s  (1 .. 63)
            rshift  tLo, s
            lshift  r2, tMid, r1
            or      tLo, r2             // tLo = (tLo >> s) | (tMid << (64-s))
            rshift  tMid, s
            lshift  r2, tHi, r1
            or      tMid, r2            // tMid = (tMid >> s) | (tHi << (64-s))
            rshift  tHi, s
        #end_cond
        #macro  ADD192(sHi, sMid, sLo, tHi, tMid, tLo)
    #end_for

    store   sHi, gSERIES_HI
    store   sMid, gSERIES_MID
    store   sLo, gSERIES_LO
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// powmod16(exp, mod)
// Returns 16^exp mod mod in r0 using binary exponentiation with exact 64-bit
// integer arithmetic.  Requires mod^2 to fit in a signed 64-bit integer.
///////////////////////////////////////////////////////////////////////////////
#def_func powmod16(expArg, modArg)
    #var    e, m, b, r, q

    load    e, expArg
    load    m, modArg
    #if_cond    m == 1
        #return 0                       // anything mod 1 is 0
    #end_cond

    move    r, 1                        // result
    move    r0, 16
    div     q, b, r0, m                 // b = 16 mod m

    #while  e > 0
        and     r0, e, 1
        #if_cond    r0 != 0
            mult    r0, r, b
            div     q, r, r0, m         // r = (r * b) mod m
        #end_cond
        mult    r0, b, b
        div     q, b, r0, m             // b = (b * b) mod m
        rshift  e, 1                    // e = e / 2
    #end_while
    #return r
#end_func

    stop
    stop