///////////////////////////////////////////////////////////////////////////////
// Hailstone3.asm
//
// Finds the longest Hailstone sequence with starting number less than or 
// equal to the argument.
// See https://en.wikipedia.org/wiki/Collatz_conjecture
//
// Author: Richard Lesh
// Original: 2009/03/20
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.def>
#include <system/system.asm>

	#call	main()
	int		iEXIT

#def_func	main()
	#var	i, j, hailstone, limit, argc, arg, imax, max, lastClock
	int		iARGC
	move	argc, r0
	cmp		argc, 2
	jump	lt, GET_ARGS_FAILED
GET_ARGS:
	move	imax, 1
	move	max, 1
	#call	args(1)
	move	arg, r0
	#macro	PARSE_INT(arg)
	move	limit, r0
	int		iCLOCK
	move	lastClock, r0
	#for	2, i <= limit, 1
		#call	compute_hailstone(i)
		move	hailstone, r0
		#if_cond	hailstone, gt, max
			move	imax, i
			move	max, hailstone
		#end_cond
		div		r0, j, i, 1000
		#if_cond	j, eq, 0
			int		iCLOCK
			sub		r0, lastClock
			#call	fprintf(STDOUT,"%d...%d:%d (%d)\n", i, imax, max, r0)
			int		iCLOCK
			move	lastClock, r0
		#end_cond
	#end_for
	#call	fprintf(STDOUT, "%d: %d\n", imax, max)
	#return	0
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

#global PRECOMPUTED: .dci	0
PRECOMPUTED_SIZE: .dci	3000000
#def_func	compute_hailstone(arg)
	#var	i,i0,isOdd,cache,cacheSize,hailstone
	
	load	cacheSize, PRECOMPUTED_SIZE
	load	cache, PRECOMPUTED
	jump	nz, BEGIN_COMPUTE
	#macro	ALLOC(cacheSize)
	move	cache, r0
	store	cache, PRECOMPUTED
	#if_cond	cache == 0
		#call	fprintf(STDOUT, "Can\'t allocate cache size %d\n", cacheSize)
		#call	exit(1)
	#end_cond
	#macro	MEMCLEAR(cache, cacheSize)
	store	1, cache[1]
BEGIN_COMPUTE:
	load	i, arg
	#if_cond	i, lt, cacheSize
		load	hailstone, cache[i]
		#if_cond	hailstone, ne, 0
			#return	hailstone
		#end_cond
	#end_cond
	
	move	i0, i
	and	isOdd, i, 0x1
	#if_cond	isOdd, eq, 0
		div		i, 2
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#else_cond
		mult	i, 3
		add		i, 1
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#end_cond
	#if_cond	i0, lt, cacheSize
		store	hailstone, cache[i0]
	#end_cond
	#return	hailstone
END:
#end_func

	stop
	stop
