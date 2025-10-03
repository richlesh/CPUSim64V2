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
	#var	i, j, hailstone, limit, argc, imax, max, lastClock
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
	int		iCLOCK
	move	lastClock, r0
	#for	i, 2, le, limit, 1
		#call	compute_hailstone(i)
		move	hailstone, r0
		#cond	hailstone, gt, max
			move	imax, i
			move	max, hailstone
		#endcond
		div		r0, j, i, 1000
		#cond	j, eq, 0
			int		iCLOCK
			sub		r0, lastClock
			#call	fprintf(STDOUT,"%d...%d:%d (%d)\n", i, imax, max, r0)
			int		iCLOCK
			move	lastClock, r0
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
// Use memoization to dramatically improve performance.
///////////////////////////////////////////////////////////////////////////////

PRECOMPUTED: dci	0
PRECOMPUTED_SIZE: dci	3000000
#def_func	compute_hailstone(arg)
	#var	i,i0,isOdd,cache,cacheSize,hailstone
	
	load	cache, PRECOMPUTED
	jmp		nz, @BEGIN_COMPUTE
	load	cacheSize, PRECOMPUTED_SIZE
	move	r0, cacheSize
	int		iALLOC
	move	cache, r0
	store	cache, PRECOMPUTED
	store	1, cache[1]
BEGIN_COMPUTE:
	load	i, arg
	#cond	i, lt, cacheSize
		load	hailstone, cache[i]
		#cond	hailstone, ne, 0
			#return	hailstone
			jump	@END
		#endcond
	#endcond
	
	move	i0, i
	and	isOdd, i, 0x1
	#cond	isOdd, eq, 0
		div		i, 2
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#elsecond
		mult	i, 3
		add		i, 1
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#endcond
	#cond	i0, lt, cacheSize
		store	hailstone, cache[i0]
	#endcond
	#return	hailstone
END:
#end_func

	stop
	stop
