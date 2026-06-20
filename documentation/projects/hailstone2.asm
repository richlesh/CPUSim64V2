///////////////////////////////////////////////////////////////////////////////
// Hailstone2.asm
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
	#var	i, hailstone, limit, argc, arg, imax, max
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
	#for	2, i <= limit, 1
		#call	compute_hailstone(i)
		move	hailstone, r0
		#if_cond	hailstone, gt, max
			move	imax, i
			move	max, hailstone
		#end_cond
	#end_for
	#call	fprintf(STDOUT, "%d: %d\n", imax, max)
	#return	0
	jump	MAIN_END
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
	#if_cond	i, le, 1
		#return	1
	#else_cond
		and	isOdd, i, 0x1
		#if_cond	isOdd, eq, 0
			div		i, 2
			#call	compute_hailstone(i)
			add		r0, 1
		#else_cond
			mult	i, 3
			add		i, 1
			#call	compute_hailstone(i)
			add		r0, 1
		#end_cond
	#end_cond
#end_func

	stop
	stop
