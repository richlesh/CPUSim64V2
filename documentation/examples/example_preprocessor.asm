#include <system/io.def>
#include <system/system.def>

//////
//
// Block comment
//
//////

#info True is ${TRUE}
//#error False is ${FALSE}
//#abort Warning Warning Will Robinson! ${FALSE}

#ifdef	TRUE
	#info This should print
	move	r0, ${TRUE}
#endif

#ifdef	BUBBA
	#info This should not print
	move	r0, ${FALSE}
#endif

#ifdef	TRUE
	#info This should print
	move	r0, ${TRUE}
#else
	#info This should not print
	move	r0, ${FALSE}
#endif

#ifdef	BUBBA
	#info This should not print
	move	r0, ${FALSE}
#else
	#info This should print
	move	r0, ${TRUE}
#endif

#ifdef	TRUE
	#info This should print
	move	r0, ${TRUE}
	#ifdef	FALSE
		#info This should print
		move	r0, ${TRUE}
	#endif
#else
	#info This should not print
	move	r0, ${FALSE}
	#ifdef	FALSE
		#info This should not print
		move	r0, ${FALSE}
	#endif
#endif

#ifdef	TRUE
	#info This should print
	move	r0, ${TRUE}
	#ifdef	FALSE
		#info This should print
		move	r0, ${TRUE}
	#else
		#info This should not print
		move	r0, ${FALSE}
	#endif
#else
	#info This should not print
	move	r0, ${FALSE}
	#ifdef	FALSE
		#info This should not print
		move	r0, ${FALSE}
	#else
		#info This should not print
		move	r0, ${FALSE}
	#endif
#endif

#ifdef	BUBBA
	#info This should not print
	move	r0, ${FALSE}
	#ifdef	FALSE
		#info This should not print
		move	r0, ${FALSE}
	#else
		#info This should not print
		move	r0, ${FALSE}
	#endif
#else
	#info This should print
	move	r0, ${TRUE}
	#ifdef	FALSE
		#info This should print
		move	r0, ${TRUE}
	#else
		#info This should not print
		move	r0, ${FALSE}
	#endif
#endif

// Macro start
#def_macro PRINT_INT(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	fprintf(STDOUT, ${fmt}, r0)
	move	r0, ${TRUE}
#end_macro

#def_macro PRINT_FP(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	fprintf(STDOUT, ${fmt}, f0)
	move	r0, ${FALSE}
#end_macro

#def_macro DOUBLE_A(a)
	move	r20, a
#end_macro

#undef	TRUE
#undef	FALSE
#define	TRUE 1

	#call	main()
	int		iEXIT

#define	FMT_d	": %d\n"
#define FMT_f	": %g\n"
#define TRICKY	"\t\x09\0Hello\u98ab\'\"${TRUE}\"\'"

#gvar	monkey1, monkey2, monkey3
// Main
#def_func	main
	#svar	svar1, svar2
	#var	intArray, count, size, dest
	#fvar	fpArray, fpArray2
	
	move	monkey1, 1
	move	monkey2, 2
	move	monkey3, 3
	move	r0, ${TRUE}
	move	r1, ${FALSE}
	PRINT_INT("iINT_MIN", iINT_MIN, FMT_d)
	PRINT_INT("iINT_MAX", iINT_MAX, FMT_d)
	PRINT_FP("iFLOAT_MIN", iFLOAT_MIN, FMT_f)
	PRINT_FP("iFLOAT_MAX", iFLOAT_MAX, FMT_f)
	PRINT_FP("iNEGATIVE_INFINITY", iNEGATIVE_INFINITY, FMT_f)
	PRINT_FP("iPOSITIVE_INFINITY", iPOSITIVE_INFINITY, FMT_f)
	PRINT_FP("iNAN", iNAN, FMT_f)

	PRINT_INT("iCYCLES", iCYCLES, FMT_d)
	PRINT_INT("iCLOCK", iCLOCK, FMT_d)
	int	iSAVE
	int	iSAVE_FP
	int	iPrintCPUState
	int iRESTORE_FP
	int	iRESTORE
	int	iPrintCPUState
	PRINT_INT("iCYCLES", iCYCLES, FMT_d)
	PRINT_INT("iCLOCK", iCLOCK, FMT_d)

	move	size, 10
	ALLOC(size)				// Allocation 10 ints on the heap
	move	intArray, r0
	clear	count
	jump	@LOOP1_END
LOOP1:						// Start of loop
	mult	r0, count, count
	store	r0, intArray[count]
	add		count, 1
LOOP1_END:
	cmp		count, size
	jump	nz, @LOOP1
	
	#call	intPrintArray(intArray, size)
	
	move	size, 20
	REALLOC(intArray, 20)
	move	intArray, r0

	#call	intPrintArray(intArray, size)
	
	move	dest, intArray+10
	#call	memcopy(dest, intArray, 10)

	#call	intPrintArray(intArray, size)

	FREE(intArray)

	#return 0					// Programs should return 0 if all went well.
#end_func

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
	#return	product
#end_func	ifastpow

#def_func	max(first, second)	// This pushes first and second on the stack
	#var	a,b					// declare two named integer registers
								// and saves original values on stack
	DOUBLE_A(r21)
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		gt, r0, a, b
#end_func						// This cleans up the stack and returns

	stop
	stop

	DCC		'X'
	DCS		"Hello, world"
	DCI		326
	DCF		3.1415
	DCA		1, 2, 3, 4, 5, 6
