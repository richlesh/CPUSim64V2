#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	jump	PROGRAM_START		// must jump to PROGRAM_START
								// to avoid executing the function

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

PROGRAM_START:
IF1:// if (argc >= 3)
	int		iARGC
	cmp		r0, 3
	jump	lt, ELSE1			// we need two command arguments
THEN1:
	move	r1, 1
	int		iARGS
	move	r1, r0
	int		iPARSE_INT
	move	r4, r0				// save A
	move	r1, 2
	int		iARGS
	move	r1, r0
	int		iPARSE_INT
	move	r5, r0				// save B
	#call	puts("Min: ")
	#call	min(r4, r5)			// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	#call	puts("Max: ")
	#call	max(r4, r5)			// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	jump	ENDIF1
ELSE1:
	#call	puts("You must supply two command line arguments!")
ENDIF1:

	#call	puts("FPArray Sum: ")
	#call	sum(FPArray)
	#call	put_fp(f0, 2)
	#call	put_nl()
	stop						// must stop the program so we don't
								// run into the functions defined below
FPArray:
	.DCW		1.1, 2.2, 3.3, 4.4, 5.5

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
// sum(count, array) computes the sum of an FP array
// array	address of FP array to sum
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	sum(array)
	#var	i, max, addr
	#fvar	sum
	load	addr, array			// Load array argument from stack
	load	max, addr[0]		// Load count argument from stack
	clear	sum
	move	i, 1
	jump	END_LOOP1
LOOP1:
	load	f0, addr[i]			// Load what is in addr[i] to temp f0
	add		sum, f0
	add		i, 1
END_LOOP1:
	cmp		i, max
	jump	le, LOOP1
	#freturn	sum
#end_func

FINIS:	
	stop
	stop
