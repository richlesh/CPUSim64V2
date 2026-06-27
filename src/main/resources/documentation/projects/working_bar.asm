#include <system/io.asm>
#include <system/math.asm>
#include <system/system.asm>

	#call	main()
	int		iEXIT

#define	PROGRESS_SIZE 50
#def_func	main()
	#fvar	pct, pct_inc, pct_max
	#call	puts("Begin\n")
	#call	sleep(1000)
	#call	puts("End\n")
	#call	puts("Processing: ")
	move	pct, 0.0
	move	pct_max, 6.0
	#call	working_bar(pct, PROGRESS_SIZE, FALSE)
	move	pct_inc, 0.01
	jump	@PROGRESS_LOOP_COND
PROGRESS_LOOP:
	#call	working_bar(pct, PROGRESS_SIZE, TRUE)
	SLEEP(50)
	add		pct, pct_inc
PROGRESS_LOOP_COND:
	cmp		pct, pct_max
	jump	le, @PROGRESS_LOOP
	#call	working_bar(pct_max, PROGRESS_SIZE, TRUE)
	#call	put_nl()
	#return	0
#end_func

#def_func	working_bar(percent_done, size, redraw)
	#var	i, j, len, barLen
	#fvar	pct, hundred
	load	pct, percent_done
	load	len, size
	load	r0, redraw
	jump	z, @SKIP_ERASE
	move	i, len
	add		i, 2
	jump	@ERASE_LOOP_COND
ERASE_LOOP:
	PUTC('\b')
	sub		i, 1
ERASE_LOOP_COND:
	test	i
	jump	nz, @ERASE_LOOP
SKIP_ERASE:

	// barLen = floor((1-(cos(pct * 2π) + 1)/2) * (len-1))
	int		iPI
	mult	f0, 2
	mult	f0, pct
	int		iCOS
	add		f0, 1
	div		f0, 2
	sub		f0, 1
	neg		f0
	move	i, len
	sub		i, 1
	mult	f0, i
	int		iROUND
	add		f0, 1
	move	barLen, f0

	PUTC('[')

	move	j, barLen
	sub		j, 1
	move	i, 0
	jump	@BAR_LOOP_COND
BAR_LOOP:
	PUTC(' ')
	add		i, 1
BAR_LOOP_COND:
	cmp		i, j
	jump	lt, @BAR_LOOP

	PUTC('\u2022')
	add		i, 1

	jump	@SPACE_LOOP_COND
SPACE_LOOP:
	PUTC(' ')
	add		i, 1
SPACE_LOOP_COND:
	cmp		i, len
	jump	lt, @SPACE_LOOP

	PUTC(']')
#end_func

	stop
	stop