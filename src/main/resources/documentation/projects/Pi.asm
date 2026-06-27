#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// Program to compute PI to specified number of terms.
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	#var	terms, argc
	int		iARGC
	move	argc, r0
	cmp		argc, 2
	jump	ne, @GET_ARGS_FAILED
	
GET_ARGS:
	move	r0, 1
	int		iARGS
	jump	z, @GET_ARGS_END
	int		iPARSE_INT
	move	terms, r0
	#call	compute_pi(terms)
	#call	put_fp(f0)
	#call	put_nl()
GET_ARGS_END:
	#return	0
	jump	@MAIN_END
GET_ARGS_FAILED:
	#call	puts("You must supply the number of terms 1-11")
	#return 1
MAIN_END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_pi(terms)
// Computes PI using the Bailey–Borwein–Plouffe formula
///////////////////////////////////////////////////////////////////////////////
#def_func compute_pi(terms)
	#var	k, loopLimit
	#fvar	myPi, base, term, termsum, denom
	move	myPi, 0.0
	load	loopLimit, terms
	move	k, 0
	move	base, 16
	jump	@LOOP_END
LOOP_START:
	move	f0, 4
	move	denom, 8
	mult	denom, k
	add		denom, 1
	div		termsum, f0, denom
	move	f0, 2
	move	denom, 8
	mult	denom, k
	add		denom, 4
	div		f0, denom
	sub		termsum, f0
	move	f0, 1
	move	denom, 8
	mult	denom, k
	add		denom, 5
	div		f0, denom
	sub		termsum, f0
	move	f0, 1
	move	denom, 8
	mult	denom, k
	add		denom, 6
	div		f0, denom
	sub		termsum, f0
	move	f0, base
	move	r0, k
	neg		r0
	#call	fastpow(f0, r0)
	mult	f0, termsum
	add		myPi, f0
	add		k, 1
LOOP_END:
	cmp		k, loopLimit
	jump	ne, @LOOP_START
	#freturn myPi
#end_func

	stop
	stop

