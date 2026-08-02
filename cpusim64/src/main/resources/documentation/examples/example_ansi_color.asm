// ANSI Color Example

#include <system/ansi_color.asm>
#include <system/io.asm>

#call	puts(ANSI_COLOR$RED)
#call	puts("This is in red\n")
#call	puts(ANSI_Color$RESET)

#call	puts(ANSI_COLOR$BLUE)
#call	puts(ANSI_COLOR$REVERSE)
#call	puts("This is in blue reversed")
#call	puts(ANSI_Color$RESET)
#call   put_nl()

#call	puts("This is normal\n")

#call	puts(ANSI_Color$BRIGHT_RED)
#call	puts(ANSI_COLOR$BG_GREEN)
#call	puts("This is in bright red on green")
#call	puts(ANSI_Color$RESET)
#call   put_nl()

#call	puts("This is normal\n")

#call	puts(ANSI_COLOR$WHITE)
#call	puts(ANSI_COLOR$BG_CYAN)
#call	puts("This is in white on cyan")
#call	puts(ANSI_Color$RESET)
#call   put_nl()

#call	puts(ANSI_COLOR$BLINK)
#call	puts("This is in blinking")
#call	puts(ANSI_Color$RESET)
#call   put_nl()

#call	puts("This is normal\n")

stop
stop
