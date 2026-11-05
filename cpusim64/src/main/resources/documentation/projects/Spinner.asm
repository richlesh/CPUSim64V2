///////////////////////////////////////////////////////////////////////////////
// spinner.asm
//
// Displays an animated "working" spinner using the classic rotation glyphs
// '|', '/', '-' and '\'. Each frame overwrites the previous one in place by
// returning the cursor to the start of the line with a carriage return
// ('\r'), producing a simple spinning animation while some task is "in
// progress".
//
// Program flow:
//   1. Hide the terminal cursor so it doesn't flicker over the spinner.
//   2. Loop FRAME_COUNT times, each time:
//        - printing the "Working: " label followed by the current glyph,
//        - pausing briefly (FRAME_DELAY ms),
//        - returning to the start of the line for the next frame.
//   3. Restore the cursor and print a final "Done!" message.
//
// Author: Richard Lesh
///////////////////////////////////////////////////////////////////////////////

#include <system/ansi_color.asm>
#include <system/io.asm>
#include <system/system.asm>

    // Program entry point: run main(), then exit with its return code.
    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// SPINNER
// The four spinner glyphs, in rotation order. Element 0 holds the array
// length (4) by convention; the glyphs themselves occupy elements 1-4.
///////////////////////////////////////////////////////////////////////////////
SPINNER: .dcw '|', '/', '-', '\\'

#define FRAME_COUNT 48      // Total number of animation frames to display
#define FRAME_DELAY 300     // Delay between frames, in milliseconds
#define FRAMES_PER_COLOR 8  // Two full 4-glyph cycles per color (2 * 4 = 8)
#define NUM_COLORS 6        // Number of colors in the rotation

///////////////////////////////////////////////////////////////////////////////
// print_color(idx)
//
// Prints the ANSI color escape sequence corresponding to idx (0..5),
// cycling through six bright colors. Used to recolor the spinner glyph
// every FRAMES_PER_COLOR frames.
//
// Arguments:
//   idx : color index, 0 .. NUM_COLORS-1
//
// Returns: nothing meaningful (output goes to STDOUT)
///////////////////////////////////////////////////////////////////////////////
#def_func   print_color(idx)
    #var    i
    load    i, idx
    #if_cond    i == 0
        #call   puts(ANSI_COLOR$BRIGHT_CYAN)
    #else_if_cond i == 1
        #call   puts(ANSI_COLOR$BRIGHT_MAGENTA)
    #else_if_cond i == 2
        #call   puts(ANSI_COLOR$BRIGHT_YELLOW)
    #else_if_cond i == 3
        #call   puts(ANSI_COLOR$BRIGHT_RED)
    #else_if_cond i == 4
        #call   puts(ANSI_COLOR$BRIGHT_GREEN)
    #else_cond
        #call   puts(ANSI_COLOR$BRIGHT_BLUE)
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Animates the spinner for FRAME_COUNT frames, then prints a final "Done!"
// message on the same line.
//
// Local variables (#var - integer registers):
//   i          : frame counter, 0 .. FRAME_COUNT-1
//   index      : current position within SPINNER (1-4)
//   cp         : the spinner glyph's codepoint for the current frame
//   cycleCount : number of full 4-glyph cycles completed so far (i/8)
//   colorIdx   : current color rotation index, 0 .. NUM_COLORS-1
//
// Returns:
//   0 in r0
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    i, index, cp, cycleCount, colorIdx

    // Hide the cursor for the duration of the animation.
    #call   puts(ANSI_COLOR$HIDE_CURSOR)

    #for    0, i < FRAME_COUNT, 1
        // index cycles 1,2,3,4,1,2,3,4,... (skip element 0 which is the count)
        div     r0, r1, i, 4        // r0 = i/4 (unused), r1 = i mod 4
        move    index, r1
        add     index, 1
        load    cp, SPINNER[index]

        // Change color every FRAMES_PER_COLOR frames, rotating through
        // NUM_COLORS colors: colorIdx = (i / FRAMES_PER_COLOR) mod NUM_COLORS
        div     cycleCount, r1, i, FRAMES_PER_COLOR   // cycleCount = i/8 (unused r1)
        div     r0, colorIdx, cycleCount, NUM_COLORS  // colorIdx = cycleCount mod 6
        #call   print_color(colorIdx)

        #call   puts("Working: ")
        #call   putc(cp)
        #call   sleep(FRAME_DELAY)  // Pause so the animation is visible
        #call   puts("\r")          // Return to column 0 for the next frame
    #end_for

    // Restore the cursor and print the final frame.
    #call   puts(ANSI_COLOR$RESET)
    #call   puts(ANSI_COLOR$SHOW_CURSOR)
    #call   putline("Working: Done!        ")
    #return 0
#end_func

    stop
    stop