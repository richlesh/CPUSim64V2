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
#include <system/console.asm>

#call main()
stop
stop

#define kSIZE      40

#def_macro  BAR_DEMO(delay, color)
    #for start, p <= stop, incr
        #call puts("Copying: ")
        #call progress_bar(kSIZE, p, ${color})
        #call sleep(${delay})
        #call puts("\r")
    #end_for
    #call puts("Copying: ")
    #call progress_bar(kSIZE, stop, 0)
    #call putline(" Done")
#end_macro

#def_func main()
    #var p, start, stop, incr
    move start, 0
    move stop, 150
    move incr, 1
    #call puts("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n")
    #call puts(ANSI_COLOR$HIDE_CURSOR)

    #macro  BAR_DEMO(100, ANSI_COLOR$BRIGHT_CYAN)
    #macro  BAR_DEMO(80, ANSI_COLOR$BRIGHT_MAGENTA)
    #macro  BAR_DEMO(60, ANSI_COLOR$BRIGHT_YELLOW)
    #macro  BAR_DEMO(40, ANSI_COLOR$BRIGHT_RED)
    #macro  BAR_DEMO(20, ANSI_COLOR$BRIGHT_GREEN)
    #macro  BAR_DEMO(10, ANSI_COLOR$BRIGHT_BLUE)

    #call puts(ANSI_COLOR$SHOW_CURSOR)
#end_func


