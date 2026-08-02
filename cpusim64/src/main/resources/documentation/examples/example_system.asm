#include <system/io.asm>
#include <system/system.asm>

#def_macro PRINT_INT(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	printf(${fmt}, r0)
#end_macro

#def_macro PRINT_FP(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	printf(${fmt}, f0)
#end_macro

	#call	main()
	move	r0, 0
	int		iEXIT

FMT_d:	.DCS	": %d\n"
FMT_f:	.DCS	": %g\n"

#def_func	main()
	#var	i, intArray, count, size, dest
	
	#macro PRINT_INT("iINT_MIN", iINT_MIN, FMT_d)
	#macro PRINT_INT("iINT_MAX", iINT_MAX, FMT_d)
	#macro PRINT_FP("iFLOAT_MIN", iFLOAT_MIN, FMT_f)
	#macro PRINT_FP("iFLOAT_MAX", iFLOAT_MAX, FMT_f)
	#macro PRINT_FP("iNEGATIVE_INFINITY", iNEGATIVE_INFINITY, FMT_f)
	#macro PRINT_FP("iPOSITIVE_INFINITY", iPOSITIVE_INFINITY, FMT_f)
	#macro PRINT_FP("iNAN", iNAN, FMT_f)

	#macro PRINT_INT("iCYCLES", iCYCLES, FMT_d)
	#macro PRINT_INT("iCLOCK", iCLOCK, FMT_d)
	int	iSAVE
	int	iSAVE_FP
	int	iPrintCPUState
	int iRESTORE_FP
	int	iRESTORE
	int	iPrintCPUState
	#macro PRINT_INT("iCYCLES", iCYCLES, FMT_d)
	#macro PRINT_INT("iCLOCK", iCLOCK, FMT_d)

	move	size, 11
	#call	ALLOC(size)				// Allocation 10 ints + count on the heap
	move	intArray, r0
	sub		size, 1
	store	size, intArray			// Store the number of ints in intArray[0]
	clear	count
	jump	$LOOP1_END
$LOOP1:
	mult	r0, count, count
	add		count, 1
	store	r0, intArray[count]
$LOOP1_END:
	cmp		count, size
	jump	nz, $LOOP1
	#call	printIntArray(intArray)
	#call	put_nl()
	
	move	size, 21
	#call	REALLOC(intArray, size)
	move	intArray, r0
	sub		size, 1
	store	size, intArray	
	move	dest, intArray[11]
	add		r0, intArray, 1
	#call	memmove(dest, r0, 10)

	#call	printIntArray(intArray)
	#call	put_nl()

	#call	FREE(intArray)

	#for	6, i, lt, 200000, i
		#call	miniumAllocSize(i)
		#call	printf("%d => %d\n", i, r0)
	#end_for
	#return 0					// Programs should return 0 if all went well.
#end_func
	stop
	stop
