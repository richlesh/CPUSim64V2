#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// Program to compute Renard numbers
// Pass into program 5, 10, 20 or 40
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	#var	series, i
	#fvar	base, factor
	move	base, 10.
GET_ARGS:
	move	r0, 1
	int		iARGS
	jump	z, @GET_ARGS_FAILED
	int		iPARSE_INT
	move	series, r0
	jump	@GET_ARGS_END
GET_ARGS_FAILED:
	#call	puts("You must supply a series value 5, 10, 20 or 40")
	#return	1
	jump	@MAIN_END
GET_ARGS_END:
	
	// Compute factor = 10 ^ (1/series)
	move	f0, series
	recip	f0
	#call	pow(base, f0)
	move	factor, f0
	
	// Loop Series times to print out all numbers
	clear	i
	jump	@LOOP_COND
LOOP_START:
	move	f0, i				// Convert i to float
	#call	pow(factor, f0)
	#call	fprintf(STDOUT, " %.2f", f0)
	add		i, 1
LOOP_COND:
	cmp		i, series
	jump	ne, @LOOP_START
	#call	put_nl()
	#return	0
MAIN_END:
#end_func

	stop
	stop

