///////////////////////////////////////////////////////////////////////////////
// rainbow_bars.asm
//
// Prints eight "rainbow bars" to the console. For each of the eight ANSI
// background colors, it sets that background color, clears to the end of the
// line (filling the line with the color), resets the styling, and moves to a
// new line.
//
// Author: (generated for CPUSim64)
///////////////////////////////////////////////////////////////////////////////

#include <system/ansi_color.asm>
#include <system/io.asm>

    #call   main()
    stop
    stop

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Draws one full-width colored bar for each of the eight ANSI background
// colors: black, red, green, yellow, blue, magenta, cyan and white.
//
// For each bar:
//   1. Set the background color.
//   2. Clear from the cursor to the end of the line (fills it with the color).
//   3. Reset all styles/colors so the next line starts clean.
//   4. Emit a newline to advance to the next row.
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #call   draw_bar(ANSI_COLOR$BG_BLACK)
    #call   draw_bar(ANSI_COLOR$BG_RED)
    #call   draw_bar(ANSI_COLOR$BG_GREEN)
    #call   draw_bar(ANSI_COLOR$BG_YELLOW)
    #call   draw_bar(ANSI_COLOR$BG_BLUE)
    #call   draw_bar(ANSI_COLOR$BG_MAGENTA)
    #call   draw_bar(ANSI_COLOR$BG_CYAN)
    #call   draw_bar(ANSI_COLOR$BG_WHITE)

    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_BLACK)
    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_RED)
    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_GREEN)
    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_YELLOW)
    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_BLUE)
    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_MAGENTA)
    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_CYAN)
    #call   draw_bar(ANSI_COLOR$BG_BRIGHT_WHITE)

    #call   putline("\nRegular")
    #call   draw_text(ANSI_COLOR$BLACK, "")
    #call   draw_text(ANSI_COLOR$RED, "")
    #call   draw_text(ANSI_COLOR$GREEN, "")
    #call   draw_text(ANSI_COLOR$YELLOW, "")
    #call   draw_text(ANSI_COLOR$BLUE, "")
    #call   draw_text(ANSI_COLOR$MAGENTA, "")
    #call   draw_text(ANSI_COLOR$CYAN, "")
    #call   draw_text(ANSI_COLOR$WHITE, "")

    #call   putline("\nBright")
    #call   draw_text(ANSI_COLOR$BRIGHT_BLACK, "")
    #call   draw_text(ANSI_COLOR$BRIGHT_RED, "")
    #call   draw_text(ANSI_COLOR$BRIGHT_GREEN, "")
    #call   draw_text(ANSI_COLOR$BRIGHT_YELLOW, "")
    #call   draw_text(ANSI_COLOR$BRIGHT_BLUE, "")
    #call   draw_text(ANSI_COLOR$BRIGHT_MAGENTA, "")
    #call   draw_text(ANSI_COLOR$BRIGHT_CYAN, "")
    #call   draw_text(ANSI_COLOR$BRIGHT_WHITE, "")

    #call   putline("\nRegular Bold")
    #call   draw_text(ANSI_COLOR$BLACK, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$RED, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$GREEN, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$YELLOW, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BLUE, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$MAGENTA, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$CYAN, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$WHITE, ANSI_COLOR$BOLD)

    #call   putline("\nBright Bold")
    #call   draw_text(ANSI_COLOR$BRIGHT_BLACK, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BRIGHT_RED, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BRIGHT_GREEN, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BRIGHT_YELLOW, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BRIGHT_BLUE, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BRIGHT_MAGENTA, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BRIGHT_CYAN, ANSI_COLOR$BOLD)
    #call   draw_text(ANSI_COLOR$BRIGHT_WHITE, ANSI_COLOR$BOLD)

    #call   putline("\nRegular Dim")
    #call   draw_text(ANSI_COLOR$BLACK, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$RED, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$GREEN, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$YELLOW, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BLUE, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$MAGENTA, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$CYAN, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$WHITE, ANSI_COLOR$DIM)

    #call   putline("\nBright Dim")
    #call   draw_text(ANSI_COLOR$BRIGHT_BLACK, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BRIGHT_RED, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BRIGHT_GREEN, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BRIGHT_YELLOW, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BRIGHT_BLUE, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BRIGHT_MAGENTA, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BRIGHT_CYAN, ANSI_COLOR$DIM)
    #call   draw_text(ANSI_COLOR$BRIGHT_WHITE, ANSI_COLOR$DIM)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// draw_bar(bgColor)
//
// Draws a single colored bar spanning the width of the terminal.
//
// Parameters:
//   bgColor  Address of the ANSI escape-sequence string that sets the desired
//            background color.
///////////////////////////////////////////////////////////////////////////////
#def_func   draw_bar(bgColor)
    #var    color
    load    color, bgColor
    #call   puts(color)                       // Set the background color
    #call   puts(ANSI_COLOR$CLEAR_TO_EOL)     // Fill the line with that color
    #call   puts(ANSI_COLOR$RESET)            // Restore default styling
    #call   put_nl()                          // Advance to the next line
#end_func

///////////////////////////////////////////////////////////////////////////////
// draw_text(bgColor)
//
// Draws a row of colored text.
//
// Parameters:
//   fgColor  Address of the ANSI escape-sequence string that sets the desired
//            foreground color.
//   modifier Address of the ANSI escape-sequence string that modifies the text.
///////////////////////////////////////////////////////////////////////////////
#def_func   draw_text(fgColor, modifier)
    #var    color, mod
    load    color, fgColor
    load    mod, modifier
    #call   puts(color)                       // Set the foreground color
    #call   puts(mod)                         // Set the modifier DIM, BOLD, etc
    #call   puts("Lorem ipsum dolor sit amet, consectetur adipiscing elit")
    #call   puts(ANSI_COLOR$RESET)            // Restore default styling
    #call   put_nl()                          // Advance to the next line
#end_func

    stop
    stop