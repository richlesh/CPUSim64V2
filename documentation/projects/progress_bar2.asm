// ============================================================
// progress_bar – Draw a UTF-8 "Cylon eye" scanner to STDOUT
//
// Stack-based function  (#def_func / #end_func)
//
// Arguments:
//   num  (integer) : total bar width in characters
//   cnt  (integer) : animation tick (bounces back and forth)
//
// The bright 3-cell eye leaves a fading trail behind it that
// points in the direction the eye most recently came from.
//
// Author: Richard Lesh
// Modified: 2026/06/28
// Original: 2022/11/26
// ============================================================

#include <system/math.def>
#include <system/io.asm>
#include <system/ansi_color.asm>

#call main()
stop
stop

#define kSIZE      40
#define EYE_WIDTH  3          // number of FULL BLOCK cells in the core
#define TRAIL_LEN  4          // number of fading cells behind the eye

#def_func main()
    #var p, start, stop, incr
    move start, 0
    move stop, 1000
    move incr, 1
    #call puts("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n")
    #call puts(ANSI_COLOR$HIDE_CURSOR)
    #for start, p <= stop, incr
        #call puts("Copying: ")
        #call progress_bar(kSIZE, p)
        #call sleep(100)
        #call puts("\r")
    #end_for
    #call puts("Copying: ")
    #call progress_bar(kSIZE, stop)
    #call puts(" Done")
    #call put_nl()
    #call puts(ANSI_COLOR$SHOW_CURSOR)
#end_func


// ------------------------------------------------------------
// progress_bar(num, cnt)
// Draws a Cylon-eye scanner of width `num`, with the eye
// positioned by triangle(cnt, num - EYE_WIDTH + 1).
// ------------------------------------------------------------
#def_func progress_bar(num, cnt)
    #var   total, count, span, pos, dir, i, ch

    // ---- 1. load arguments ----
    load  total, num                    // total bar width
    load  count, cnt

    // ---- 2. eye position: triangle bounce ----
    //   span = number of valid left-edge positions for the eye
    sub   span, total, EYE_WIDTH
    add   span, 1                        // span = total - EYE_WIDTH + 1
    #call triangle(count, span)
    move  pos, r0                        // pos = left edge of the eye [0..span-1]

    // ---- 3. determine direction of travel ----
    //   dir =  1  -> moving right (trail on the LEFT)
    //   dir = -1  -> moving left  (trail on the RIGHT)
    #call direction(count, span)
    move  dir, r0

    // ---- 4. draw each cell of the bar ----
    #for  0, i, lt, total, 1
        // compute the character for column i into ch
        #call eye_char(i, pos, dir)
        move  ch, r0
        // emit it
        move  r1, STDOUT
        move  r2, ch
        out   r2, CHAR, r1
    #end_for

    #call puts(ANSI_COLOR$RESET)
    #return 0
#end_func


// ------------------------------------------------------------
// eye_char(col, pos, dir)
// Returns in r0 the codepoint to draw at column `col` for an
// eye whose left edge is `pos`, moving in direction `dir`.
//
// Colour escape sequences are emitted as a side effect so the
// returned glyph prints in the correct intensity/colour.
//
// The eye and its trail are all BRIGHT_MAGENTA; the shade
// characters alone create the fading effect:
//   core █ -> trail ▓ -> ▒ -> ░ (faintest)
// Only the true background remains dim.
// ------------------------------------------------------------
#def_func eye_char(col, pos, dir)
    #var  c, p, d, rel, eyeEnd, t

    load  c, col
    load  p, pos
    load  d, dir

    // eyeEnd = p + EYE_WIDTH - 1  (last column of the core)
    add   eyeEnd, p, EYE_WIDTH
    sub   eyeEnd, 1

    // ---- inside the bright core? ----
    //   p <= c <= eyeEnd
    #if_cond  c, ge, p
        #if_cond  c, le, eyeEnd
            #call puts(ANSI_COLOR$BRIGHT_MAGENTA)
            #return '\u{2588}'           // █ FULL BLOCK
        #end_cond
    #end_cond

    // ---- compute trail distance `t` behind the eye ----
    //   moving right (d > 0): trail is to the LEFT, t = p - 1 - c
    //   moving left  (d < 0): trail is to the RIGHT, t = c - (eyeEnd + 1)
    #if_cond  d, gt, 0
        sub   t, p, c
        sub   t, 1                       // t = p - 1 - c
    #else_cond
        sub   t, c, eyeEnd
        sub   t, 1                       // t = c - eyeEnd - 1
    #end_cond

    // ---- map trail distance to a fading glyph ----
    //   all trail cells share the eye's BRIGHT_MAGENTA colour;
    //   the shade glyphs supply the fade: ▓ -> ▒ -> ░
    #if_cond  t, lt, 0
        // not part of the trail at all -> faint background
        #call puts(ANSI_COLOR$BRIGHT_BLACK)
        #return '\u{2591}'               // ░ LIGHT SHADE
    #else_if_cond  t, eq, 0
        #call puts(ANSI_COLOR$BRIGHT_MAGENTA)
        #return '\u{2593}'               // ▓ DARK SHADE
    #else_if_cond  t, eq, 1
        #call puts(ANSI_COLOR$BRIGHT_MAGENTA)
        #return '\u{2592}'               // ▒ MEDIUM SHADE
    #else_if_cond  t, eq, 2
        #call puts(ANSI_COLOR$BRIGHT_MAGENTA)
        #return '\u{2591}'               // ░ LIGHT SHADE (faintest trail)
    #else_cond
        // beyond the trail -> faint background
        #call puts(ANSI_COLOR$BRIGHT_BLACK)
        #return '\u{2591}'               // ░ LIGHT SHADE
    #end_cond
#end_func


// ------------------------------------------------------------
// triangle(xArg, MArg)
// Bounces a value back and forth in [0, M-1] as x increases.
// Returns the bounced position in r0.
// ------------------------------------------------------------
#def_func triangle(xArg, MArg)
    #var x, M, period, x_mod
    load x, xArg
    load M, MArg
//    period = 2 * (M - 1)
    sub r0, M, 1
    mult period, r0, 2
//    x_mod = x % period
    div r0, x_mod, x, period
//    if x_mod < M:
    #if_cond x_mod lt M
//        return x_mod
        move r0, x_mod
//    else:
    #else_cond
//        return period - x_mod
        sub  r0, period, x_mod
    #end_cond
#end_func


// ------------------------------------------------------------
// direction(xArg, MArg)
// Returns in r0 the direction the triangle wave is currently
// moving:  +1 while ascending, -1 while descending.
// ------------------------------------------------------------
#def_func direction(xArg, MArg)
    #var x, M, period, x_mod
    load x, xArg
    load M, MArg
//    period = 2 * (M - 1)
    sub r0, M, 1
    mult period, r0, 2
//    x_mod = x % period
    div r0, x_mod, x, period
//    if x_mod < M-1: ascending (+1) else descending (-1)
    sub  r0, M, 1
    #if_cond x_mod lt r0
        move r0, 1
    #else_cond
        move r0, -1
    #end_cond
#end_func