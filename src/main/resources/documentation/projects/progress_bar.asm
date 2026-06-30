// ============================================================================
// progress_bar.asm – Animated UTF-8 progress bar demo for CPUSim64
//
// Demonstrates rendering a colored progress bar to STDOUT that animates in
// place using a carriage return ('\r'). The bar is drawn with Unicode block
// characters and ANSI color escape sequences.
//
//   Filled segment : U+2588 FULL BLOCK  (█)  in BRIGHT_MAGENTA
//   Empty  segment : U+2591 LIGHT SHADE (░)  in BRIGHT_BLACK
//
// Author: Richard Lesh
// Modified: 2026/06/28
// Original: 2022/11/26
// ============================================================================

#include <system/math.def>
#include <system/io.asm>
#include <system/ansi_color.asm>

#call main()
stop
stop

#define SIZE 80                         // Total width of the progress bar in chars

// ----------------------------------------------------------------------------
// main()
//
// Purpose:
//   Entry point. Animates an 80-character progress bar from 0% to 100% in
//   2.5% increments, redrawing it in place once per frame. After reaching
//   100%, prints a final " Done" frame followed by a newline.
//
// Arguments:
//   none
//
// Returns:
//   none (falls through to stop)
//
// Local variables (#fvar – floating point registers):
//   p      : current fill fraction being displayed   [0.0 .. 1.0]
//   start  : loop start value (0.0)
//   stop   : loop end value   (1.0 = 100%)
//   incr   : per-frame increment (0.01 = 1%)
//
// Notes:
//   - HIDE_CURSOR / SHOW_CURSOR bracket the animation so the blinking cursor
//     does not flicker over the bar while it redraws.
//   - The header line of digits is printed once to act as a column ruler.
//   - Each frame: reprint the label, draw the bar, pause, then emit '\r' to
//     return the cursor to the start of the line for the next frame.
// ----------------------------------------------------------------------------
#def_func main()
    #fvar p, start, stop, incr
    load start, 0.0
    load stop, 1.0
    load incr, 0.01

    // Print a one-time column ruler above the bar.
    #call puts("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n")

    // Hide the cursor for the duration of the animation.
    #call puts(ANSI_COLOR$HIDE_CURSOR)

    // Animate from 0% to 100%.
    #for start, p <= stop, incr
        #call puts("Copying: ")
        #call puts(ANSI_COLOR$BRIGHT_MAGENTA)
        #call progress_bar(SIZE, p)
        #call puts(ANSI_COLOR$RESET)
        #call sleep(100)                // Pause 100 ms between frames
        #call puts("\r")               // Return to column 0 for the next frame
    #end_for

    // Final 100% frame, replacing the percentage readout with " Done".
    #call puts("Copying: ")
    #call progress_bar(SIZE, stop)
    #call puts(" Done")
    #call put_nl()

    // Restore the cursor.
    #call puts(ANSI_COLOR$SHOW_CURSOR)
#end_func


// ----------------------------------------------------------------------------
// progress_bar(num, pct)
//
// Purpose:
//   Render a single frame of a progress bar to STDOUT. The bar consists of
//   `num` cells; the leftmost portion is drawn as filled blocks proportional
//   to `pct`, and the remainder as light-shade blocks. When `pct` is below
//   100%, the integral percentage is appended at the far right (right-aligned
//   to 3 columns). At exactly 100% the percentage is omitted so the caller's
//   " Done" suffix lines up cleanly.
//
// Arguments (stack-based):
//   num  (integer) : total bar width in characters
//   pct  (float)   : fill fraction, clamped to [0.0, 1.0]
//
// Returns:
//   r0 = 0   (via #return)
//
// Local variables:
//   #svar sv_filled : saved count of filled cells (preserved across the
//                     filled-drawing loop so it can seed the empty loop)
//   #var  total     : total cell count (= num)
//   #var  filled    : number of filled cells = round(pct * total)
//   #var  empty     : number of empty cells  = total - filled (currently unused
//                     for output; retained for clarity/debugging)
//   #var  pctInt    : integral percentage    = round(pct * 100)
//   #fvar fp_pct    : working copy of the clamped fill fraction
//
// Clobbers:
//   r1, r2, f0, f1, f2 (working registers used for output and math interrupts)
//
// Interrupts used:
//   iMAX_FP, iMIN_FP : clamp fp_pct into [0.0, 1.0]
//   iROUND           : round float results to nearest integer
//
// Output format example (pct = 0.65, num = 80):
//   ██████...░░░░░░  65%
// ----------------------------------------------------------------------------
#def_func progress_bar(num, pct)
    #svar  sv_filled                    // one stack variable: saved filled count
    #var   total, filled, empty, pctInt // integer register variables
    #fvar  fp_pct                       // float   register variable

    // ---- 1. Load arguments from the stack ----
    load  total,  num                   // total  = num  (integer arg)
    load  fp_pct, pct                   // fp_pct = pct  (float arg)

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

    // ---- 5. empty = total - filled ----
    sub   empty, total, filled          // empty = total - filled

    // ---- 6. Save filled so the empty loop can resume from it ----
    store filled, sv_filled

    // ---- 7. Print the filled cells: U+2588 FULL BLOCK (█) ----
    #for filled, filled > 0, -1
        move  r1, STDOUT
        move  r2, '\u{2588}'                // FULL BLOCK codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for

    // ---- 8. Print the empty cells: U+2591 LIGHT SHADE (░) ----
    load filled, sv_filled
    add  filled, 1                          // resume just past the filled region
    #call puts(ANSI_COLOR$BRIGHT_BLACK)
    #for filled, filled <= total, 1
        move  r1, STDOUT
        move  r2, '\u{2591}'                // LIGHT SHADE codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for
    #call puts(ANSI_COLOR$RESET)

    // ---- 9. Print the integral percentage at the far right ----
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