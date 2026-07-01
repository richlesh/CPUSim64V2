///////////////////////////////////////////////////////////////////////////////
// random_digits.asm
//
// Retrieves the terminal width (columns) and height (rows), then prints the
// digits 0-9 at random row/column positions on the terminal.
//
// Author:   Richard Lesh
// Original: 2026/06/30
///////////////////////////////////////////////////////////////////////////////

#include <system/ansi_color.asm>
#include <system/io.asm>
#include <system/math.def>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
// Gets the terminal size, clears the screen, then places each digit 0-9 at a
// random location on the terminal.
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    i, cols, rows, digit, row, col

    // Get the terminal width (number of columns).
    int     iTERM_COLS
    move    cols, r0

    // Get the terminal height (number of rows).
    int     iTERM_ROWS
    move    rows, r0

    #call   printf("Terminal: %d x %d\n", rows, cols)
    #call   sleep(2000)
    #for 0, i < 10, 1
        #call   puts(ANSI_COLOR$BG_BLUE)
        int     iTERM_CLEAR
    
        // Clear the screen by moving cursor to top-left and printing newlines.
        // (Simple approach: home the cursor first.)
        move    r0, 0               // row 0
        move    r1, 0               // column 0
        int     iTERM_SETCURSOR
    
        // Print each digit 0 through 9 at a random position.
        #for    0, digit <= 9, 1
            // Pick a random row in the range [0, rows-1].
            move    r1, 0
            move    r2, rows
            sub     r2, 1
            int     iRAND
            move    row, r0
    
            // Pick a random column in the range [0, cols-1].
            move    r1, 0
            move    r2, cols
            sub     r2, 1
            int     iRAND
            move    col, r0
    
            // Move the cursor to the chosen (row, col).
            #call   setcursor(row, col)
    
            // Print the digit character ('0' + digit).
            move    r0, '0'
            add     r0, digit
 //           #call   putc(r0)
            #call   put_dec(row)
        #end_for
    
        // Move the cursor to the bottom of the terminal so the shell prompt
        // appears below our output.
        move    r0, rows
        sub     r0, 1
        #call   setcursor(r0, 0)
        #call   puts(ANSI_COLOR$RESET)
        #call   sleep(500)
    #end_for

    // Test Clear to EOL
    #call   puts(ANSI_COLOR$BG_RED)
    #call   setcursor(1, 10)
    #call   puts(ANSI_COLOR$CLEAR_TO_EOL)
    #call   setcursor(2, 20)
    #call   puts(ANSI_COLOR$CLEAR_TO_EOL)
    #call   setcursor(3, 30)
    #call   puts(ANSI_COLOR$CLEAR_TO_EOL)
    #call   setcursor(4, 40)
    #call   puts(ANSI_COLOR$CLEAR_TO_EOL)

    // Test Clear Line
    #call   puts(ANSI_COLOR$BG_YELLOW)
    sub     rows, 4
    #call   setcursor(rows, 10)
    #call   puts(ANSI_COLOR$CLEAR_LINE)
    add     rows, 1
    #call   setcursor(rows, 20)
    #call   puts(ANSI_COLOR$CLEAR_LINE)
    add     rows, 1
    #call   setcursor(rows, 30)
    #call   puts(ANSI_COLOR$CLEAR_LINE)
    
    add    rows, 1
    #call   setcursor(rows, 0)
    #call   puts(ANSI_COLOR$RESET)
    #call   sleep(5000)
    #call   put_nl()
    #return 0
#end_func

    stop
    stop