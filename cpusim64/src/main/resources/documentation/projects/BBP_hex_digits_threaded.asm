///////////////////////////////////////////////////////////////////////////////
// BBP_hex_digits.asm
//
// Computes 100 consecutive windows of 32 hexadecimal digits of pi starting at
// positions n, n+32, n+64, ..., n+3168 (3,200 digits in all) using the
// Bailey-Borwein-Plouffe (BBP) digit-extraction algorithm.  Unlike a spigot,
// BBP jumps directly to any position without computing the preceding digits,
// so every window can be computed independently.
//
// Multi-threading:
//   One worker thread is created per CPU core.  Work slots 0..99 are handed
//   out by an atomic counter (gSLOT, advanced with get_and_increment).  Slot
//   s computes position s*32 + n and stores its hi and mid words in the
//   results vector at elements s*2 and s*2+1.  Workers keep claiming slots
//   until the counter reaches NUM_SLOTS.  After all workers are joined, main
//   prints the windows in slot order.
//   Worker console messages ("Starting #" and guard-bit warnings) are each a
//   single printf call.  printf is thread safe, so no extra locking is needed.
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
// Limits:     every computed position must be <= 10,000,000 so that
//             (8k+j)^2 fits in a signed 64-bit integer (powmod16) and
//             8k+j < 2^31 (FRAC192).  The last window starts at n + 3168,
//             so n <= 9,996,832.
//
// Usage:
//   run.sh BBP_hex_digits <n>
//
// Example:
//   > run.sh BBP_hex_digits 1
//   Number of threads: 8
//   Starting 0
//   ...
//   Position 1: 243F6A8885A308D313198A2E03707344
//   Position 33: A4093822299F31D0082EFA98EC4E6C89
//   ...
//   Position 3169: ...
//
// Author: AI Assistant
// Date:   2026/11/06
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.def>
#include <system/system.asm>
#include <system/thread.asm>

#define HEX_PER_WORD 16         // Hex digits held in one 64-bit word
#define MAX_POSITION 10000000   // Largest supported position n
#define GUARD_SHIFT 32          // Examine the top 64 - GUARD_SHIFT guard bits
#define NUM_SLOTS 100           // Number of work slots (windows) to compute
#define SLOT_STRIDE 32          // Position step between slots (no overlap)
#define MAX_START 9996832       // MAX_POSITION - (NUM_SLOTS - 1) * SLOT_STRIDE
#define RESULT_WORDS 200        // NUM_SLOTS * 2 (hi and mid word per slot)

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

#global gSLOT: .dci -1              // Atomic work-slot counter (first claim -> 0)
#global gN: .dci 0                  // Starting position n from the command line
#global gRESULTS: .dci 0            // Address of the results vector

///////////////////////////////////////////////////////////////////////////////
// main()
// Reads n from the command line, allocates the results vector, starts one
// worker thread per CPU core, waits for all of them to finish, then prints
// the 32 hex digits computed for each work slot.
// Returns 0 on success, 1 on error.
///////////////////////////////////////////////////////////////////////////////
#def_func main()
    #var    n, i, cores, pids, scratch, results, pid, w, idx, pos

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
    #if_cond    n > MAX_START
        #call   printf("n must be <= %d\n", MAX_START)
        #return 1
    #end_cond
    store   n, gN

    // Results vector: slot s uses elements s*2 (hi) and s*2+1 (mid).  Each
    // slot is written by exactly one worker, so no locking is needed.
    #call   alloc(RESULT_WORDS)
    move    results, r0
    #if_cond    results == 0
        #call   putline("Error: results allocation failed!")
        #return 1
    #end_cond
    store   results, gRESULTS

    int     iGET_NUM_CORES
    move    cores, r0
    #call   printf("Number of threads: %d\n", cores)

    #call   alloc(cores)                // Thread PIDs
    move    pids, r0
    #if_cond    pids == 0
        #call   putline("Error: PID array allocation failed!")
        #return 1
    #end_cond

    // Each worker gets a private 3-word scratch buffer for series() results.
    mult    r0, cores, 3
    #call   alloc(r0)
    move    scratch, r0
    #if_cond    scratch == 0
        #call   putline("Error: scratch allocation failed!")
        #return 1
    #end_cond

    // Start the worker threads.
    #for    0, i < cores, 1
        mult    w, i, 3
        add     w, scratch              // w = &scratch[3i]
        #macro  CREATE_THREAD(worker, w)
        move    pid, r0
        #if_cond    pid > 0
            store   pid, pids[i]
        #else_cond
            #call   printf("Error: thread %d failed to start!\n", i)
            store   0, pids[i]
        #end_cond
    #end_for

    // Wait for every worker to finish.
    #for    0, i < cores, 1
        load    pid, pids[i]
        #if_cond    pid > 0
            #macro  JOIN_THREAD(pid)
        #end_cond
    #end_for

    // Print each window in slot order.
    #for    0, i < NUM_SLOTS, 1
        mult    pos, i, SLOT_STRIDE
        add     pos, n                  // pos = slot * SLOT_STRIDE + n
        #call   printf("Position %d: ", pos)
        lshift  idx, i, 1               // idx = slot * 2
        load    w, results[idx]
        #call   put_hex_word(w)         // accHi
        add     idx, 1
        load    w, results[idx]
        #call   put_hex_word(w)         // accMid
        #call   put_nl()
    #end_for

    #call   free(scratch)
    #call   free(pids)
    #call   free(results)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// worker(bufArg)
// Thread entry point.  bufArg is the address of a private 3-word scratch
// buffer.  Repeatedly claims the next work slot from the atomic counter
// gSLOT, computes the 32 hex digits at position slot*32 + n and stores the
// hi and mid words in the results vector at elements slot*2 and slot*2+1.
// Terminates once the counter reaches NUM_SLOTS.
// Each console message is a single (thread safe) printf call.
///////////////////////////////////////////////////////////////////////////////
#def_func worker(bufArg)
    #var    buf, n, results, slot, pos, idx, guard

    load    buf, bufArg
    load    n, gN
    load    results, gRESULTS
    #while  TRUE
        #call   get_and_increment(gSLOT)    // atomically claim the next slot
        move    slot, r0
        #if_cond    slot >= NUM_SLOTS
            #break
        #end_cond
        #call   printf("Starting %d\n", slot)

        mult    pos, slot, SLOT_STRIDE
        add     pos, n                  // pos = slot * SLOT_STRIDE + n
        #call   bbp(pos, buf)           // buf[0..2] = hi:mid:lo

        lshift  idx, slot, 1            // idx = slot * 2
        load    r0, buf[0]
        store   r0, results[idx]        // accHi
        add     idx, 1
        load    r0, buf[1]
        store   r0, results[idx]        // accMid

        // Guard-bit check (see bbp()): warn if the last digit is uncertain.
        load    r0, buf[2]
        arshift guard, r0, GUARD_SHIFT
        #if_cond    guard == 0
            #call   printf("Warning: position %d guard bits are all 0s; the last digit(s) may be 1 too high.\n", pos)
        #else_if_cond   guard == -1
            #call   printf("Warning: position %d guard bits are all 1s; the last digit(s) may be 1 too low.\n", pos)
        #end_cond
    #end_while
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// bbp(posArg, bufArg)
// Computes frac(16^(pos-1) * pi) as a 192-bit fixed-point fraction with the
// BBP formula and stores it in buf[0] (hi), buf[1] (mid) and buf[2] (lo).
// The top 128 bits (hi and mid) are the 32 hex digits starting at pos; the lo
// word only serves as guard bits.  The accumulated error is < 2^-165, i.e.
// < 2^27 units of the lo word, so the printed digits can only be affected
// when the top bits of lo are all 0s or all 1s.
// Uses only registers and the caller's buffer, so it is thread safe.
///////////////////////////////////////////////////////////////////////////////
#def_func bbp(posArg, bufArg)
    #var    d, buf, accHi, accMid, accLo, tHi, tMid, tLo

    load    d, posArg
    sub     d, 1                        // d = pos - 1
    load    buf, bufArg

    // acc = 4 * S(1)   (192-bit shift left by 2)
    #call   series(1, d, buf)
    load    accHi, buf[0]
    load    accMid, buf[1]
    load    accLo, buf[2]
    #macro  SHL192(accHi, accMid, accLo, 2, 62)

    // acc -= 2 * S(4)  (192-bit shift left by 1, then subtract)
    #call   series(4, d, buf)
    load    tHi, buf[0]
    load    tMid, buf[1]
    load    tLo, buf[2]
    #macro  SHL192(tHi, tMid, tLo, 1, 63)
    #macro  SUB192(accHi, accMid, accLo, tHi, tMid, tLo)

    // acc -= S(5)
    #call   series(5, d, buf)
    load    tHi, buf[0]
    load    tMid, buf[1]
    load    tLo, buf[2]
    #macro  SUB192(accHi, accMid, accLo, tHi, tMid, tLo)

    // acc -= S(6)
    #call   series(6, d, buf)
    load    tHi, buf[0]
    load    tMid, buf[1]
    load    tLo, buf[2]
    #macro  SUB192(accHi, accMid, accLo, tHi, tMid, tLo)

    store   accHi, buf[0]
    store   accMid, buf[1]
    store   accLo, buf[2]
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
// series(j, d, out)
// Computes the fractional part of
//   S(j) = SUM_{k=0..d} (16^(d-k) mod (8k+j)) / (8k+j)
//        + SUM_{k>d}    16^(d-k) / (8k+j)
// as a 192-bit fixed-point fraction.  The result is stored in out[0] (hi),
// out[1] (mid) and out[2] (lo).  No globals are used, so it is thread safe.
///////////////////////////////////////////////////////////////////////////////
#def_func series(jArg, dArg, outArg)
    #var    j, d, k, m, t, s, sHi, sMid, sLo, tHi, tMid, tLo, out

    load    j, jArg
    load    d, dArg
    load    out, outArg
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

    store   sHi, out[0]
    store   sMid, out[1]
    store   sLo, out[2]
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