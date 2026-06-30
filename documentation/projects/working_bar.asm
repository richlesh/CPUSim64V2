///////////////////////////////////////////////////////////////////////////////
// working_bar.asm
//
// Displays an animated "working" / activity indicator on the console.
//
// Rather than showing linear progress, this program animates a single dot
// (a "Cylon eye") that smoothly bounces back and forth across a fixed-width
// bar drawn between square brackets, e.g.:
//
//     Processing: [          •                       ]
//
// The dot's position is driven by a cosine function of the percent value,
// which produces a smooth ease-in/ease-out bounce as the value advances.
// On each frame the previous bar is erased (using backspaces) and redrawn
// in place, creating the animation effect.
//
// Program flow:
//   1. Print "Begin", sleep briefly, print "End" (simulated work).
//   2. Draw the bar repeatedly while a percent value advances from 0.0 to
//      6.0, animating the bouncing dot.
//
// Author: Richard Lesh
///////////////////////////////////////////////////////////////////////////////

#include <system/ansi_color.asm>
#include <system/io.asm>
#include <system/math.asm>
#include <system/system.asm>

    // Program entry point: run main(), then exit with its return code.
    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// PROGRESS_SIZE
// The interior width of the animated bar, in characters (the number of cells
// drawn between the '[' and ']' brackets).
///////////////////////////////////////////////////////////////////////////////
#define PROGRESS_SIZE 20

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Drives the demo: prints some status text, then runs the animation loop.
//
// The animation advances a floating point value (pct) from 0.0 to pct_max
// (6.0) in small increments (pct_inc = 0.01). Each iteration redraws the bar
// and pauses 50 ms so the motion is visible. The cosine-based position used
// inside working_bar() means pct_max = 6.0 produces three full back-and-forth
// bounces (cos has a period of 1.0 in the pct units used here).
//
// Returns: 0 in r0
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #fvar   pct, pct_inc, pct_max

    // Label printed in front of the animated bar.
    #call   puts(ANSI_COLOR$HIDE_CURSOR)
    #call   puts("Processing: ")

    // Initialize the animation range and draw the first frame.
    // (redraw = FALSE means "draw fresh" — do not erase a previous bar.)
    load    pct, 0.0
    load    pct_max, 6.0
    #call   working_bar(pct, PROGRESS_SIZE, FALSE)

    // Per-frame increment for the percent value.
    load    pct_inc, 0.01

    // Animation loop: redraw the bar for each pct value up to pct_max.
    #while  pct <= pct_max
         // redraw = TRUE: erase the previous bar first, then draw the new frame.
        #call   working_bar(pct, PROGRESS_SIZE, TRUE)
        #call   sleep(50)               // ~50 ms per frame for a visible animation
        add     pct, pct_inc
    #end_while

    // Draw one final frame at exactly pct_max, then move to a fresh line.
    #call   working_bar(pct_max, PROGRESS_SIZE, TRUE)
    #call   put_nl()
    #call   puts(ANSI_COLOR$SHOW_CURSOR)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// working_bar(percent_done, size, redraw)
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
// When redraw is TRUE, the previously drawn bar (len interior cells plus the
// two brackets = len + 2 characters) is erased with backspaces before the new
// frame is drawn, so successive frames overwrite each other in place.
//
// Output for a given frame looks like:
//   [   <spaces>   •   <spaces>   ]
//   where '•' (U+2022 BULLET) marks the current dot position.
//
// Parameters:
//   percent_done  Floating point animation value driving the dot position.
//   size          Interior width of the bar in characters (number of cells).
//   redraw        TRUE to erase the previous frame first; FALSE to draw fresh.
//
// Returns: nothing meaningful (r0 undefined)
///////////////////////////////////////////////////////////////////////////////
#def_func   working_bar(percent_done, size, redraw)
    #var    i, j, len, barLen
    #fvar   pct, hundred
    load    pct, percent_done
    load    len, size

    // ---- Erase the previous frame (only when redrawing) ----
    // The previously drawn frame is (len + 2) characters wide: the two
    // brackets plus len interior cells. Emit that many backspaces to move
    // the cursor back to the start so the new frame overwrites it.
    load    r0, redraw
    #if_cond    r0
        move    i, len
        add     i, 2
        #for    , i > 0, -1
            #call   putc('\b')
        #end_for
    #end_cond

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
    #call   puts(ANSI_COLOR$BRIGHT_MAGENTA)
    #call   putc('\u{2022}')
    #call   puts(ANSI_COLOR$RESET)
    add     i, 1                    // account for the cell the dot occupies

    // ---- Draw trailing spaces to fill out the bar ----
    #for    , i < len, 1
        #call   putc(' ')
    #end_for

    // ---- Draw the closing bracket ----
    #call   putc(']')
#end_func

    stop
    stop