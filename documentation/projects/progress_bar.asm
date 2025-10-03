#include <system/io.asm>
#include <system/math.asm>
#include <system/system.asm>

	#call	main()
	int		iEXIT

#define	PROGRESS_SIZE 50
#def_func	main()
	#fvar	pct, pct_inc, pct_100
	#call	puts("Begin\n")
	#call	sleep(1000)
	#call	puts("End\n")
	#call	puts("Processing: ")
	move	pct, 0.0
	move	pct_100, 1.0
	#call	progress_bar(pct, PROGRESS_SIZE, FALSE)
	move	pct_inc, 0.025
	jump	@PROGRESS_LOOP_COND
PROGRESS_LOOP:
	#call	progress_bar(pct, PROGRESS_SIZE, TRUE)
	SLEEP(100)
	add		pct, pct_inc
PROGRESS_LOOP_COND:
	cmp		pct, pct_100
	jump	le, @PROGRESS_LOOP
	#call	progress_bar(pct_100, PROGRESS_SIZE, TRUE)
	#call	put_nl()
	#return	0
#end_func

#def_func	progress_bar(percent_done, size, redraw)
	#var	i, len, barLen
	#fvar	pct, hundred
	load	pct, percent_done
	load	len, size
	load	r0, redraw
	jump	z, @SKIP_ERASE
	move	i, len
	add		i, 9
	jump	@ERASE_LOOP_COND
ERASE_LOOP:
	PUTC('\b')
	sub		i, 1
ERASE_LOOP_COND:
	test	i
	jump	nz, @ERASE_LOOP
SKIP_ERASE:

	move	f0, len
	mult	f0, pct
	int		iFLOOR
	move	barLen, f0

	PUTC('[')

	move	i, 0
	jump	@BAR_LOOP_COND
BAR_LOOP:
	PUTC('\u2588')
	add		i, 1
BAR_LOOP_COND:
	cmp		i, barLen
	jump	lt, @BAR_LOOP

	jump	@SPACE_LOOP_COND
SPACE_LOOP:
	PUTC(' ')
	add		i, 1
SPACE_LOOP_COND:
	cmp		i, len
	jump	lt, @SPACE_LOOP

	PUTC(']')
	move	hundred, 100.0
	mult	pct, hundred
	#call	fprintf(STDOUT, " %5.1f%%", pct)	
#end_func

	stop
	stop