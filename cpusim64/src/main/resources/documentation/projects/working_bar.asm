///////////////////////////////////////////////////////////////////////////////
// working_bar.asm
//
// Displays an animated "working" / activity indicator on the console.
//
// Rather than showing linear progress, this program animates a single dot
// (a "Cylon eye") that smoothly bounces back and forth across a fixed-width
// bar drawn between square brackets, e.g.:
//
//     Processing: [          ●                       ]
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
#define PROGRESS_SIZE 40

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Drives the demo: prints some status text, then runs the animation loop.
//
// The animation advances a floating point value (pct) from 0.0 to pct_max
// (6.0) in small increments (pct_inc = 0.01). Each iteration redraws the bar
// and pauses so the motion is visible. The cosine-based position used
// inside working_bar() means pct_max = 3.0 produces three full back-and-forth
// bounces (cos has a period of 1.0 in the pct units used here).
//
// Returns: 0 in r0
///////////////////////////////////////////////////////////////////////////////

#def_macro  BAR_DEMO(delay, color)
    #for start, i <= end, 1
        #call puts("Processing: ")
// i is scaled by 100 to allow exact integer stepping/comparison;
// divide by 100 before passing to working_bar()
        move    p, i
        div     p, 100
        #call working_bar(PROGRESS_SIZE, p, ${color})
        #call sleep(${delay})
        #call puts("\r")
    #end_for
    #call puts("Processing: ")
    #call working_bar(PROGRESS_SIZE, p, 0)
    #call putline(" Done")
#end_macro

#def_func   main()
    #var   start, end, i
    #fvar   p

    move    start, 0
    move    end, 300

    // Label printed in front of the animated bar.
    #call   puts(ANSI_COLOR$HIDE_CURSOR)

    #macro  BAR_DEMO(60, ANSI_COLOR$BRIGHT_CYAN)
    #macro  BAR_DEMO(50, ANSI_COLOR$BRIGHT_MAGENTA)
    #macro  BAR_DEMO(40, ANSI_COLOR$BRIGHT_YELLOW)
    #macro  BAR_DEMO(30, ANSI_COLOR$BRIGHT_RED)
    #macro  BAR_DEMO(20, ANSI_COLOR$BRIGHT_GREEN)
    #macro  BAR_DEMO(10, ANSI_COLOR$BRIGHT_BLUE)
    #macro  BAR_DEMO(5, ANSI_COLOR$BRIGHT_WHITE)

    #call   puts(ANSI_COLOR$SHOW_CURSOR)
    #return 0
#end_func

    stop
    stop