#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	jump	@PROGRAM_START		// must jump to PROGRAM_START
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
	jump	lt, @ELSE1			// we need two command arguments
THEN1:
	move	r0, 1
	int		iARGS
	int		iPARSE_INT
	move	r1, r0				// save A
	move	r0, 2
	int		iARGS
	int		iPARSE_INT
	move	r2, r0				// save B
	#call	puts("Min: ")
	#call	min(r1,r2)			// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	#call	puts("Max: ")
	#call	max(r1,r2)			// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	jump	@ENDIF1
ELSE1:
	#call	puts("You must supply two command line arguments!")
ENDIF1:

	#call	puts("FPArray Sum: ")
	load	r0, FPArray[-1]
	#call	sum(r0, @FPArray)
	#call	put_fp(f0)
	#call	put_nl()
	stop						// must stop the program so we don't
								// run into the functions defined below
FPArray:
	DCA		1.1, 2.2, 3.3, 4.4, 5.5

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
// count	number of elements in the array
// array	address of FP array to sum
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	sum(count, array)
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

FINIS:	
	stop
	stop
