#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// Program to compute Renard numbers
// Pass into program 5, 10, 20 or 40
// Can pass multiple values for multiple series to print.
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	#var	i, argc
	int		iARGC
	move	argc, r0
	cmp		argc, 2
	jump	lt, @GET_ARGS_FAILED
	move	i, 1
GET_ARGS:
	move	r0, i
	int		iARGS
	jump	z, @GET_ARGS_END
	int		iPARSE_INT
	#call	print_renard(r0)
	add		i, 1
	cmp		i, argc
	jump	ne, @GET_ARGS
GET_ARGS_END:
	#return	0
	jump	@MAIN_END
GET_ARGS_FAILED:
	#call	puts("You must supply a series value 5, 10, 20 or 40")
	#return	1
MAIN_END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// print_renard(series)
// Function to compute Renard numbers
// series	Number of Renard numbers in the series
///////////////////////////////////////////////////////////////////////////////
#def_func print_renard(series)
	#var	i, loopLimit
	#fvar	base, factor
	move	base, 10.
	load	loopLimit, series

	#call	put_dec(loopLimit)
	#call	putc(':')

	// Compute factor = 10 ^ (1/series)
	move	f0, loopLimit
	recip	f0
	#call	pow(base, f0)
	move	factor, f0

	clear	i
	jump	@LOOP_COND
LOOP_START:
	move	f0, i				// Convert i to float
	#call	pow(factor, f0)
	#call	fprintf(STDOUT, " %.2f", f0)
	add		i, 1
LOOP_COND:
	cmp		i, loopLimit
	jump	ne, @LOOP_START
	#call	put_nl()
#end_func

	stop
	stop

