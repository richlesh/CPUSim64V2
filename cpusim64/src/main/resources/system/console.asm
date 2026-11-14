#ifndef _CONSOLE_ASM
#define _CONSOLE_ASM
#export *		// all labels are visible to other modules (assemble -r)
#include <system/ansi_color.asm>
#include <system/io.asm>
#include <system/math.asm>

CONSOLE_ASM_START:
jump CONSOLE_ASM_END

#define EYE_WIDTH  3          // number of FULL BLOCK cells in the core
#define TRAIL_LEN  4          // number of fading cells behind the eye

// ----------------------------------------------------------------------------
// progress_bar_percent(num, pct, ANSI_color)
//
// Purpose:
//   Render a single frame of a progress bar to STDOUT. The bar consists of
//   `num` cells; the leftmost portion is drawn as filled blocks proportional
//   to `pct`, and the remainder as light-shade blocks. When `pct` is below
//   100%, the integral percentage is appended at the far right (right-aligned
//   to 3 columns). At exactly 100% the percentage is omitted so the caller's
//   " Done" suffix lines up cleanly.  ANSI_color is the color for the progress
//   bar while it is running.  At 100% the progress bar resets its color to the
//   default terminal color.
//
// Arguments (stack-based):
//   num  (integer) : total bar width in characters
//   pct  (float)   : fill fraction, clamped to [0.0, 1.0]
//
// Returns:
//   r0 = 0   (via #return)
//
// Local variables:
//   #var  total     : total cell count (= num)
//   #var  filled    : number of filled cells = round(pct * total)
//   #var  empty     : number of empty cells  = total - filled (currently unused
//                     for output; retained for clarity/debugging)
//   #var  pctInt    : integral percentage    = round(pct * 100)
//   #var  fgcolor   : ANSI color for the progress bar
//   #fvar fp_pct    : working copy of the clamped fill fraction
//
// Clobbers:
//   r1, r2, f0, f1, f2 (working registers used for output and math interrupts)
//
// Interrupts used:
//   iMAX_FP, iMIN_FP : clamp fp_pct into [0.0, 1.0]
//   iROUND           : round float results to nearest integer
//
// Output format example (pct = 0.60, num = 10):
//   ██████░░░░  60%
// ----------------------------------------------------------------------------
#def_func progress_bar_percent(num, pct, ANSI_color)
    #var   i, total, filled, pctInt, fgcolor
    #fvar  fp_pct

    // ---- 1. Load arguments from the stack ----
    load  total,  num                   // total  = num  (integer arg)
    load  fp_pct, pct                   // fp_pct = pct  (float arg)
    load  fgcolor, ANSI_color

    // ---- 2. Clamp fp_pct to the range [0.0, 1.0] ----
    move  f1, fp_pct
    load  f2, 0.0
    int   iMAX_FP                       // f0 = max(fp_pct, 0.0)
    move  fp_pct, f0

    move  f1, fp_pct
    load  f2, 1.0
    int   iMIN_FP                       // f0 = min(fp_pct, 1.0)
    move  fp_pct, f0

    // ---- 3. filled = round(fp_pct * total) ----
    move  f1, total                     // f1 = (float)total
    mult  f1, fp_pct, f1                // f1 = fp_pct * total
    int   iROUND                        // f0 = round(f1)
    move  filled, f0                    // filled = (int)f0

    // ---- 4. pctInt = round(fp_pct * 100) ----
    move  f1, fp_pct
    mult  f1, 100                       // f1 = fp_pct * 100
    int   iROUND                        // f0 = round(f1)
    move  pctInt, f0                    // pctInt = (int)f0

    // ---- 5. Print the filled cells: U+2588 FULL BLOCK (█) ----
    #call puts(fgcolor)
    #for 0, i < filled, 1
        move  r1, STDOUT
        move  r2, '\u{2588}'                // FULL BLOCK codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for

    // ---- 6. Print the empty cells: U+2591 LIGHT SHADE (░) ----
    add  filled, 1                          // resume just past the filled region
    #call puts(ANSI_COLOR$BRIGHT_BLACK)
    #for filled, i <= total, 1
        move  r1, STDOUT
        move  r2, '\u{2591}'                // LIGHT SHADE codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for
    #call puts(ANSI_COLOR$RESET)

    // ---- 7. Print the integral percentage at the far right ----
    //         Skipped at 100% so the caller's " Done" frame stays clean.
    //         Right-aligned to 3 columns to avoid jitter as digits grow.
    #if_cond pctInt, lt, 100
        #call putc(' ')
        #if_cond pctInt, lt, 10
            #call puts("  ")                // pad: "  5%"
        #else_if_cond pctInt, lt, 100
            #call putc(' ')                 // pad: " 65%"
        #end_cond
        #call put_dec(pctInt)
        #call putc('%')
    #end_cond

    #return 0
#end_func

// ------------------------------------------------------------
// progress_bar(num, cnt, ANSI_color)
// Draws a Cylon-eye scanner of width `num`, with the eye
// positioned by triangle(cnt, num - EYE_WIDTH + 1).
// ANSI_color is the color for the EYE while it is running.
// ------------------------------------------------------------
#def_func progress_bar(num, cnt, ANSI_color)
    #var   total, count, span, pos, dir, i, ch, fgcolor

    // ---- 1. load arguments ----
    load  total, num                    // total bar width
    load  count, cnt
    load  fgcolor, ANSI_color

    // ---- 2. eye position: triangle bounce ----
    //   span = number of valid left-edge positions for the eye
    sub   span, total, EYE_WIDTH
    add   span, 1                        // span = total - EYE_WIDTH + 1
    #call _CONSOLE_triangle(count, span)
    move  pos, r0                        // pos = left edge of the eye [0..span-1]

    // ---- 3. determine direction of travel ----
    //   dir =  1  -> moving right (trail on the LEFT)
    //   dir = -1  -> moving left  (trail on the RIGHT)
    #call _CONSOLE_direction(count, span)
    move  dir, r0

    // ---- 4. draw each cell of the bar ----
    #for  0, i, lt, total, 1
        // compute the character for column i into ch
        #call _CONSOLE_eye_char(i, pos, dir, fgcolor)
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
// _CONSOLE_eye_char(col, pos, dir, ANSI_color)
// Returns in r0 the codepoint to draw at column `col` for an
// eye whose left edge is `pos`, moving in direction `dir`.
//
// ANSI_color is the color for the EYE while it is running.
// Colour escape sequences are emitted as a side effect so the
// returned glyph prints in the correct intensity/colour.
// If ANSI_color is 0, then the EYE disappears.
//
// The eye and its trail are all BRIGHT_MAGENTA; the shade
// characters alone create the fading effect:
//   core █ -> trail ▓ -> ▒ -> ░ (faintest)
// Only the true background remains dim.
// ------------------------------------------------------------
#def_func _CONSOLE_eye_char(col, pos, dir, ANSI_color)
    #var  c, p, d, rel, eyeEnd, t, fgcolor

    load  c, col
    load  p, pos
    load  d, dir
    load  fgcolor, ANSI_color

    #if_cond    fgcolor == 0
        #call puts(ANSI_COLOR$BRIGHT_BLACK)
        #return '\u{2591}'               // ░ LIGHT SHADE
    #end_cond

    // eyeEnd = p + EYE_WIDTH - 1  (last column of the core)
    add   eyeEnd, p, EYE_WIDTH
    sub   eyeEnd, 1

    // ---- inside the bright core? ----
    //   p <= c <= eyeEnd
    #if_cond  c, ge, p
        #if_cond  c, le, eyeEnd
            #call puts(fgcolor)
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
        #call puts(fgcolor)
        #return '\u{2593}'               // ▓ DARK SHADE
    #else_if_cond  t, eq, 1
        #call puts(fgcolor)
        #return '\u{2592}'               // ▒ MEDIUM SHADE
    #else_if_cond  t, eq, 2
        #call puts(fgcolor)
        #return '\u{2591}'               // ░ LIGHT SHADE (faintest trail)
    #else_cond
        // beyond the trail -> faint background
        #call puts(ANSI_COLOR$BRIGHT_BLACK)
        #return '\u{2591}'               // ░ LIGHT SHADE
    #end_cond
#end_func


// ------------------------------------------------------------
// _CONSOLE_triangle(xArg, MArg)
// Bounces a value back and forth in [0, M-1] as x increases.
// Returns the bounced position in r0.
// ------------------------------------------------------------
#def_func _CONSOLE_triangle(xArg, MArg)
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
// _CONSOLE_direction(xArg, MArg)
// Returns in r0 the direction the triangle wave is currently
// moving:  +1 while ascending, -1 while descending.
// ------------------------------------------------------------
#def_func _CONSOLE_direction(xArg, MArg)
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

///////////////////////////////////////////////////////////////////////////////
// working_bar(size, percent_done, ANSI_color)
//
// Draws a single frame of the animated activity bar in place.
//
// The dot's position within the bar is computed from a cosine curve so that
// it eases smoothly to each end and reverses direction, producing a bouncing
// "Cylon eye" effect as percent_done increases.
//
// Position formula:
//   barLen = floor( (1 - (cos(pct * 2π) + 1) / 2) * (len - 1) ) + 1
//
//   - cos(pct * 2π) ranges over [-1, 1] as pct advances.
//   - (cos(...) + 1) / 2 normalizes that to [0, 1].
//   - The (1 - x) inversion and scaling by (len - 1) maps it to a 1-based
//     cell index within the bar, so the dot sweeps fully end to end and back.
//
// Use '\r' to return to the beginning of the line to redraw to make it 
// animate.
//
// Output for a given frame looks like:
//   [   <spaces>   ●   <spaces>   ]
//   where '●' (U+25CF BLACK CIRCLE) marks the current dot position.
//
// Parameters:
//   size           Interior width of the bar in characters (number of cells).
//   percent_done   Floating point animation value driving the dot position.
//                  0.0 - 1.0 is one cycle.  Can exceed 1.0.
//   ANSI_color     ANSI color sequence desired for dot color
//
// Returns: nothing meaningful (r0 undefined)
///////////////////////////////////////////////////////////////////////////////

#def_func   working_bar(size, percent_done, ANSI_color)
    #var    i, j, len, barLen, fgcolor
    #fvar   pct
    load    pct, percent_done
    load    len, size
    load    fgcolor, ANSI_color

    // ---- Compute the dot position (barLen) from the cosine curve ----
    // barLen = floor((1 - (cos(pct * 2π) + 1)/2) * (len-1)) + 1
    int     iPI                     // f0 = π
    mult    f0, 2                   // f0 = 2π
    mult    f0, pct                 // f0 = pct * 2π
    #call   cos(f0)                 // f0 = cos(pct * 2π)         range [-1, 1]
    add     f0, 1                   // f0 = cos(...) + 1          range [0, 2]
    div     f0, 2                   // f0 = (cos(...) + 1) / 2    range [0, 1]
    sub     f0, 1                   // f0 = that - 1              range [-1, 0]
    neg     f0                      // f0 = 1 - that              range [0, 1]
    move    i, len
    sub     i, 1                    // i = len - 1
    mult    f0, i                   // f0 = normalized * (len-1)  range [0, len-1]
    #call   round(f0)               // f0 = nearest integer cell
    add     f0, 1                   // make the position 1-based
    move    barLen, f0              // barLen = dot's cell position (1..len)

    #call   puts(ANSI_COLOR$RESET)
    // ---- Draw the opening bracket ----
    #call   putc('[')

    // ---- Draw leading spaces up to (but not including) the dot ----
    move    j, barLen
    sub     j, 1                    // number of spaces before the dot
    #for    0, i < j, 1
        #call   putc(' ')
    #end_for

    // ---- Draw the dot itself (U+2022 BULLET '•') ----
    #macro  COMPARE(fgcolor, ne, 0)
    #if_cond r0
        #call   puts(fgcolor)
        #call   putc('\u{25CF}')
        #call   puts(ANSI_COLOR$RESET)
    #else_cond
        #call   putc(' ')
    #end_cond
    add     i, 1                    // account for the cell the dot occupies

    // ---- Draw trailing spaces to fill out the bar ----
    #for    , i < len, 1
        #call   putc(' ')
    #end_for

    // ---- Draw the closing bracket ----
    #call   putc(']')
#end_func

CONSOLE_ASM_END:    nop
#endif

