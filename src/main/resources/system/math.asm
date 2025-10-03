#include <system/math.def>
#include <system/system.def>

jump	@MATH_ASM_END

#def_func	pi()
	int		iPI
#end_func

#def_func	ln_base()
	int		iE
#end_func

#def_func	fdiv(int i, int j)
	#var    
	#fvar	fi, fj
	#loadargs
	move	fi, i
	move	fj, j
	div		fi, fj
#end_func

#def_func	fabs(x)
	load	f0,x
	int		iABS_FP
#end_func	fabs

#def_func	abs(i)
	load	r0,i
	int		iABS
#end_func	abs

#def_func	ceil(x)
	load	f0,x
	int		iCEIL
#end_func	ceil

#def_func	floor(x)
	load	f0,x
	int		iFLOOR
#end_func	floor

#def_func	round(x)
	load	f0,x
	int		iROUND
#end_func	round

#def_func	sqrt(x)
	load	f0,x
	int		iSQRT
#end_func	sqrt

#def_func	exp(x)
	load	f0,x
	int		iEXP
#end_func	exp

#def_func	log(x)
	load	f0,x
	int		iLOG
#end_func	log

#def_func	exp10(x)
	push	f1
	move	f0, 10.
	load	f1, x
	int		iPOW
	pop		f1
#end_func	exp10

#def_func	log10(x)
	load	f0,x
	int		iLOG
	push	f1
	move	f1,2.3025850929940456840179914546844
	div		f0,f1
	pop		f1
#end_func	log10

#def_func	exp2(x)
	push	f1
	move	f0, 2.
	load	f1, x
	int		iPOW
	pop		f1
#end_func	exp10

#def_func	log2(x)
	load	f0,x
	int		iLOG
	push	f1
	move	f1,0.693147180559945309417232121458
	div		f0,f1
	pop		f1
#end_func	log10

#def_func	pow(x,y)
	push	f1
	load	f0,x
	load	f1,y
	int		iPOW
	pop		f1
#end_func	pow

// base is integer, power is integer (non-negtive)
#def_func	ifastpow(base, power)
	#var	product, square, b, p
	load	b, base
	load	p, power
	move	square, b
	move	product, 1
	jump	@LOOP_END
LOOP_START:
	and		r0, p, 0x1
	jump	z, @LOOP_NEXT
	mult	product, square
LOOP_NEXT:
	mult	square, square
	rshift	p, 1
LOOP_END:
	test	p
	jump	nz, @LOOP_START
	move	r0, product
#end_func	ifastpow

// base is FP, power is integer (non-negtive)
#def_func	fastpow(base, power)
	#var	p, negPower
	#fvar	product, square, b
	load	b, base
	load	p, power
	move	negPower, FALSE
	test	p
	jump	nn, @SKIP_NEG
	neg		p
	move	negPower, TRUE
SKIP_NEG:
	move	square, b
	move	product, 1
	jump	@LOOP_END
LOOP_START:
	and		r0, p, 0x1
	jump	z, @LOOP_NEXT
	mult	product, square
LOOP_NEXT:
	mult	square, square
	rshift	p, 1
LOOP_END:
	test	p
	jump	nz, @LOOP_START
	move	f0, product
	test	negPower
	jump	z, @SKIP_NEG2
	recip	f0
SKIP_NEG2:
#end_func	fastpow

#def_func	random()
	int		iRANDOM
#end_func	random

#def_func	rand(low,high)
	push	r1
	load	r0,low
	load	r1,high
	int		iRAND
	pop		r1
#end_func	random

///////////////////////////////////////////////////////////////////////////////
// min(first, second) computes minimum of two integer values
// a		first integer value
// b		second integer value
// returns result in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	min(first, second)	// This pushes first and second on the stack
	#var	a,b					// declare two named integer registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		lt, r0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// max(first, second) computes minimum of two integer values
// a		first integer value
// b		second integer value
// returns result in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	max(first, second)	// This pushes first and second on the stack
	#var	a,b					// declare two named integer registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		gt, r0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// fmin(first, second) computes minimum of two FP values
// a		first FP value
// b		second FP value
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	fmin(first, second)	// This pushes first and second on the stack
	#fvar	a,b					// declare two named FP registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		lt, f0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// fmax(first, second) computes minimum of two FP values
// a		first FP value
// b		second FP value
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	fmax(first, second)	// This pushes first and second on the stack
	#fvar	a,b					// declare two named FP registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		gt, f0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// sum(count, array) computes the sum of an integer array
// count	number of elements in the array
// array	address of integer array to sum
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	sum(count, array)
	#var	i, max, addr, sum
	load	max, count			// Load count argument from stack
	load	addr, array			// Load array argument from stack
	clear	sum
	clear	i
	jump	@END_LOOP1
LOOP1:
	load	r0, addr[i]			// Load what is in addr[i] to temp f0
	add		sum, r0
	add		i, 1
END_LOOP1:
	cmp		i, max
	jump	lt, @LOOP1
	#return	sum
#end_func

///////////////////////////////////////////////////////////////////////////////
// fsum(count, array) computes the sum of an FP array
// count	number of elements in the array
// array	address of FP array to sum
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	fsum(count, array)
	#var	i, max, addr
	#fvar	sum
	load	max, count			// Load count argument from stack
	load	addr, array			// Load array argument from stack
	clear	sum
	clear	i
	jump	@END_LOOP1
LOOP1:
	load	f0, addr[i]			// Load what is in addr[i] to temp f0
	add		sum, f0
	add		i, 1
END_LOOP1:
	cmp		i, max
	jump	lt, @LOOP1
	#freturn	sum
#end_func

MATH_ASM_END: nop
