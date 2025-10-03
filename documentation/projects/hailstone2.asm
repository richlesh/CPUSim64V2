#include <system/io.asm>
#include <system/string.def>

///////////////////////////////////////////////////////////////////////////////
// Computes the hailstone sequence
// Finds the longest sequence with starting number less than argument.
// See https://en.wikipedia.org/wiki/Collatz_conjecture
///////////////////////////////////////////////////////////////////////////////
	#call	main()
	int		iEXIT

#def_func	main()
	#var	i, hailstone, limit, argc, imax, max
	int		iARGC
	move	argc, r0
	cmp		argc, 2
	jump	lt, @GET_ARGS_FAILED
GET_ARGS:
	move	imax, 1
	move	max, 1
	move	r0, 1
	int		iARGS
	int		iPARSE_INT
	move	limit, r0
	#for	i, 2, le, limit, 1
		#call	compute_hailstone(i)
		move	hailstone, r0
		#cond	hailstone, gt, max
			move	imax, i
			move	max, hailstone
		#endcond
	#endfor
	#call	fprintf(STDOUT, "%d: %d\n", imax, max)
	#return	0
	jump	@MAIN_END
GET_ARGS_FAILED:
	#call	puts("You must supply a positive integer argument.")
	#return	1
MAIN_END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_hailstone(arg)
// Computes the number of integers in the hailstone sequence starting
// with the argument.  The hailstone sequence conputes the next value
// in the sequence according to the formula...
// f(n) = f(n-1)/2 if f(n-1) is even and 3*f(n-1)+1 if odd.
// The sequence ends when the computed value reaches 1.  This can be
// computed recursively.
///////////////////////////////////////////////////////////////////////////////

#def_func	compute_hailstone(arg)
	#var	i,isOdd
	load	i, arg
	#cond	i, le, 1
		#return	1
	#elsecond
		and	isOdd, i, 0x1
		#cond	isOdd, eq, 0
			div		i, 2
			#call	compute_hailstone(i)
			add		r0, 1
		#elsecond
			mult	i, 3
			add		i, 1
			#call	compute_hailstone(i)
			add		r0, 1
		#endcond
	#endcond
#end_func

	stop
	stop
