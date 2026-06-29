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

#define SIZE 80
#def_func main()
    #fvar p, start, stop, incr
    load start, 0.0
    load stop, 1.0
    load incr, 0.025
    #call puts("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890\n")
    #call puts(ANSI_COLOR$HIDE_CURSOR)
    #for start, p <= stop, incr
        #call puts("Copying: ")
        #call puts(ANSI_COLOR$BRIGHT_MAGENTA)
        #call progress_bar(SIZE, p)
        #call puts(ANSI_COLOR$RESET)
        #call puts("\r")
        #call sleep(100)
    #end_for
    #call puts("Copying: ")
    #call progress_bar(SIZE, stop)
    #call puts(" Done")
    #call put_nl()
    #call puts(ANSI_COLOR$SHOW_CURSOR)
#end_func


#def_func progress_bar(num, pct)
    #svar  sv_filled                    // one stack variable: saved filled count
    #var   total, filled, empty         // integer register variables
    #fvar  fp_pct                       // float   register variable

    // ---- 1. load arguments ----
    load  total,  num                   // total  = num  (integer arg)
    load  fp_pct, pct                   // fp_pct = pct  (float arg)

    // ---- 2. clamp fp_pct to [0.0, 1.0] ----
    move  f1, fp_pct
    load  f2, 0.0
    int   iMAX_FP                       // f0 = max(fp_pct, 0.0)
    move  fp_pct, f0

    move  f1, fp_pct
    load  f2, 1.0
    int   iMIN_FP                       // f0 = min(fp_pct, 1.0)
    move  fp_pct, f0

   // ---- 3. filled = floor(fp_pct * total) ----
    move  f1, total                     // f1 = (float)total
    mult  f1, fp_pct, f1                // f1 = fp_pct * total
    int   iROUND                        // f0 = round(f1)
    move  filled, f0                    // filled = (int)f0

    // ---- 5. empty = total - filled ----
    sub   empty, total, filled          // empty = total - filled

    // ---- 6. save filled for the second loop ----
    store filled, sv_filled

    // ---- 7. print filled blocks: U+2588 FULL BLOCK (█) ----
    #for filled, filled > 0, -1
        move  r1, STDOUT
        move  r2, '\u{2588}'                // FULL BLOCK codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for

    // ---- 8. print empty blocks: U+2591 LIGHT SHADE (░) ----
    load filled, sv_filled
    add  filled, 1
    #call puts(ANSI_COLOR$BRIGHT_BLACK)
    #for filled, filled <= total, 1
        move  r1, STDOUT
        move  r2, '\u{2591}'                // LIGHT SHADE codepoint
        out   r2, CHAR, r1                  // emit one UTF-8 char to stdout
    #end_for
    #call puts(ANSI_COLOR$RESET)

    #return 0
#end_func
