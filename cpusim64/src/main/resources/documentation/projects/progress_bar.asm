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
#include <system/console.asm>

#call main()
stop
stop

#define SIZE 74                         // Total width of the progress bar in chars

#def_macro BAR_DEMO(incr, color)
    // Animate from 0% to 100%.
    #for start, p <= end, ${incr}
        #call puts("Copying: ")
        move f0, p
        div f0, hundred
        #call progress_bar_percent(SIZE, f0, ${color})
        #call sleep(100)                // Pause 100 ms between frames
        #call puts("\r")                // Return to column 0 for the next frame
    #end_for

    // Final 100% frame, replacing the percentage readout with " Done".
    #call puts("Copying: ")
    #call progress_bar_percent(SIZE, one, ANSI_COLOR$RESET)
    #call puts(" Done")
    #call put_nl()
#end_macro

// ----------------------------------------------------------------------------
// main()
//
// Purpose:
//   Entry point. Animates a progress bar from 0% to 100% in
//   3% increments, redrawing it in place once per frame. After reaching
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
    #var    p, start, end
    #fvar   hundred, one
    move    start, 0
    move    end, 100
    load    hundred, 100.0
    load    one, 1.0

    // Print a one-time column ruler above the bar.
    #call puts("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n")

    // Hide the cursor for the duration of the animation.
    #call puts(ANSI_COLOR$HIDE_CURSOR)

    #macro  BAR_DEMO(1, ANSI_COLOR$BRIGHT_CYAN)
    #macro  BAR_DEMO(2, ANSI_COLOR$BRIGHT_MAGENTA)
    #macro  BAR_DEMO(3, ANSI_COLOR$BRIGHT_YELLOW)
    #macro  BAR_DEMO(4, ANSI_COLOR$BRIGHT_RED)
    #macro  BAR_DEMO(5, ANSI_COLOR$BRIGHT_GREEN)
    #macro  BAR_DEMO(6, ANSI_COLOR$BRIGHT_BLUE)

    // Restore the cursor.
    #call puts(ANSI_COLOR$SHOW_CURSOR)
#end_func
