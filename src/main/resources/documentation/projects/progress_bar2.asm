// ============================================================
// progress_bar – Draw a UTF-8 progress bar to STDOUT
//
// Stack-based function  (#def_func / #end_func)
//
// Arguments:
//   num  (integer) : total bar width in characters  → r1
//   pct  (float)   : fill fraction [0.0, 1.0]       → f1
//
// Example call:
//   move  r1, 40         // 40-character wide bar
//   load  f1, 0.65       // 65 % full
//   #call progress_bar(r1, f1)
// ============================================================

#include <system/math.def>
#include <system/io.asm>
#include <system/ansi_color.asm>

#call main()
stop
stop

#define kSIZE 40
#def_func main()
	#var p, start, stop, incr
    move start, 0
    move stop, 1000
    move incr, 1
    #call puts("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n")
    #call puts(ANSI_COLOR$HIDE_CURSOR)
    #for start, p <= stop, incr
        #call puts("Copying: ")
        #call progress_bar(kSIZE, p)
        #call puts("\r")
        #macro SLEEP(100)
    #end_for
    #call puts("Copying: ")
    #call progress_bar(kSIZE, stop)
    #call puts(" Done")
    #call put_nl()
    #call puts(ANSI_COLOR$SHOW_CURSOR)
#end_func


#def_func progress_bar(num, cnt)
    #svar  sv_filled                    // one stack variable: saved filled count
    #var   total, filled, empty, count  // integer register variables
	
    // ---- 1. load arguments ----
    load  total,  num                   // total  = num  (integer arg)
    load  count, cnt                   	// count = cnt (integer arg)

    // ---- 2. clamp to triangle(count, total - 2) ----
    sub  total, 2
	#call triangle(count, total)
    move  filled, r0

    // ---- 5. empty = total - filled ----
    sub   empty, total, filled          // empty = total - filled
	sub   empty, 1
    // ---- 6. save filled for the second loop ----
    store filled, sv_filled

    // ---- 7. print left blocks: U+2591 LIGHT SHADE (░) ----
    #call puts(ANSI_COLOR$BRIGHT_BLACK)
    #for filled, filled > 0, -1
        move  r1, STDOUT
        move  r2, '\u{2591}'                // LIGHT SHADE codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for

    // ---- 8. print cylon eye: U+2588 FULL BLOCK (█) ----
	#call puts(ANSI_COLOR$BRIGHT_MAGENTA)
	move  r1, STDOUT
	move  r2, '\u{2588}'                // FULL BLOCK codepoint
	out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
	move  r1, STDOUT
	move  r2, '\u{2588}'                // FULL BLOCK codepoint
	out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
	move  r1, STDOUT
	move  r2, '\u{2588}'                // FULL BLOCK codepoint
	out   r2, CHAR, r1                  // emit one UTF-8 char to stdout


    // ---- 9. print empty blocks: U+2591 LIGHT SHADE (░) ----
    load filled, sv_filled
    add  filled, 2
    #call puts(ANSI_COLOR$BRIGHT_BLACK)
    #for filled, filled <= total, 1
        move  r1, STDOUT
        move  r2, '\u{2591}'                // LIGHT SHADE codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for
    #call puts(ANSI_COLOR$RESET)

    #return 0
#end_func

#def_func triangle(xArg, MArg)
	#var x, M, period, x_mod
	load x, xArg
	load M, MArg
	div  r1, r0, x, M
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
