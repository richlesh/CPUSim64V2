#include <system/io.asm>
#include <system/system.def>

//////
//
// Block comment
//
//////

#info True is TRUE
//#error False is FALSE
//#abort Warning Warning Will Robinson! FALSE

#ifdef	TRUE
	#info This should print
	move	r0, TRUE
#endif

#ifdef	BUBBA
	#error This should not print
	move	r0, FALSE
#endif

#ifdef	TRUE
	#info This should print
	move	r0, TRUE
#else
	#error This should not print
	move	r0, FALSE
#endif

#ifdef	BUBBA
	#error This should not print
	move	r0, FALSE
#else
	#info This should print
	move	r0, TRUE
#endif

#ifdef	TRUE
	#info This should print
	move	r0, TRUE
	#ifdef	FALSE
		#info This should print
		move	r0, TRUE
	#endif
#else
	#error This should not print
	move	r0, FALSE
	#ifdef	FALSE
		#error This should not print
		move	r0, FALSE
	#endif
#endif

#ifdef	TRUE
	#info This should print
	move	r0, TRUE
	#ifdef	FALSE
		#info This should print
		move	r0, TRUE
	#else
		#error This should not print
		move	r0, FALSE
	#endif
#else
	#error This should not print
	move	r0, FALSE
	#ifdef	FALSE
		#error This should not print
		move	r0, FALSE
	#else
		#error This should not print
		move	r0, FALSE
	#endif
#endif

#ifdef	BUBBA
	#error This should not print
	move	r0, FALSE
	#ifdef	FALSE
		#error This should not print
		move	r0, FALSE
	#else
		#error This should not print
		move	r0, FALSE
	#endif
#else
	#info This should print
	move	r0, TRUE
	#ifdef	FALSE
		#info This should print
		move	r0, TRUE
	#else
		#error This should not print
		move	r0, FALSE
	#endif
#endif

// Macro start
#def_macro PRINT_INT(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	printf(${fmt}, r0)
	move	r0, TRUE
#end_macro

#def_macro PRINT_FP(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	printf(${fmt}, f0)
	move	r0, FALSE
#end_macro

#def_macro DOUBLE_A(a)
	move	r20, a
#end_macro

#undef	TRUE
#undef	FALSE
#define	TRUE 1
#define	FALSE 0

	#call	main()
	move	r0, 0
	int		iEXIT

#define	FMT_d	": %d\n"
#define FMT_f	": %g\n"
#define TRICKY	"\t\U{9}\0Hello\u{98ab}\'\"TRUE\"\'"

#global	monkey1: .dci 0
#global	monkey2: .dci 0
#global	monkey3: .dci 0

// Main
#def_func	main()
	#svar	svar1, svar2
	#var	intArray, count, size, dest, src
	#fvar	fpArray, fpArray2
	
	store	1, monkey1
	store	2, monkey2
	store	3, monkey3
	move	r0, TRUE
	move	r1, FALSE
	#macro	PRINT_INT("iINT_MIN", iINT_MIN, FMT_d)
	#macro	PRINT_INT("iINT_MAX", iINT_MAX, FMT_d)
	#macro	PRINT_FP("iFLOAT_MIN", iFLOAT_MIN, FMT_f)
	#macro	PRINT_FP("iFLOAT_MAX", iFLOAT_MAX, FMT_f)
	#macro	PRINT_FP("iNEGATIVE_INFINITY", iNEGATIVE_INFINITY, FMT_f)
	#macro	PRINT_FP("iPOSITIVE_INFINITY", iPOSITIVE_INFINITY, FMT_f)
	#macro	PRINT_FP("iNAN", iNAN, FMT_f)

	#macro	PRINT_INT("iCYCLES", iCYCLES, FMT_d)
	#macro	PRINT_INT("iCLOCK", iCLOCK, FMT_d)
	int	iSAVE
	int	iSAVE_FP
	int	iPrintCPUState
	int iRESTORE_FP
	int	iRESTORE
	int	iPrintCPUState
	#macro	PRINT_INT("iCYCLES", iCYCLES, FMT_d)
	#macro	PRINT_INT("iCLOCK", iCLOCK, FMT_d)

	move	size, 11
	#call	ALLOC(size)		// Allocation 10 ints on the heap
	sub		size, 1
	move	intArray, r0
	move	count, 1
	jump	$LOOP1_END
$LOOP1:						// Start of loop
	mult	r0, count, count
	store	r0, intArray[count]
	add		count, 1
$LOOP1_END:
	cmp		count, size
	jump	le, $LOOP1
	store	size, intArray[0]

	#call	printIntArray(intArray)
	#call	put_nl()
	#call	REALLOC(intArray, 21)
	move	intArray, r0

	#call	printIntArray(intArray)
	#call	put_nl()

	move	dest, intArray+11
	move	src, intArray+1
	#call	memmove(dest, src, 10)
	store	20, intArray[0]
	#call	printIntArray(intArray)
	#call	put_nl()

	#call	FREE(intArray)

	#return 0					// Programs should return 0 if all went well.
#end_func

// base is integer, power is integer (non-negtive)
#def_func	ifastpow(base, power)
	#var	product, square, b, p
	load	b, base
	load	p, power
	move	square, b
	move	product, 1
	jump	$LOOP_END
$LOOP_START:
	and		r0, p, 0x1
	jump	z, $LOOP_NEXT
	mult	product, square
$LOOP_NEXT:
	mult	square, square
	rshift	p, 1
$LOOP_END:
	test	p
	jump	nz, $LOOP_START
	#return	product
#end_func

#def_func	max(first, second)	// This pushes first and second on the stack
	#var	a,b					// declare two named integer registers
								// and saves original values on stack
	#macro	DOUBLE_A(r21)
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		gt, r0, a, b
#end_func						// This cleans up the stack and returns

	stop
	stop

	.DCC		'X'
	.DCS		"Hello, world"
	.DCI		326
	.DCF		3.1415
	.DCW		1, 2, 3, 4, 5, 6
