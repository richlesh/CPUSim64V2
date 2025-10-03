#include <system/io.asm>
#include <system/system.asm>

#def_macro PRINT_INT(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	fprintf(STDOUT, ${fmt}, r0)
#end_macro

#def_macro PRINT_FP(name, which, fmt)
	#call	puts(${name})
	int		${which}
	#call	fprintf(STDOUT, ${fmt}, f0)
#end_macro

	#call	main()
	int		iEXIT

FMT_d:	DCS	": %d\n"
FMT_f:	DCS	": %g\n"

#def_func	main
	#var	i, intArray, count, size, dest
	#fvar	fpArray
	
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
LOOP1:
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
	#call	memmove(dest, intArray, 10)

	#call	intPrintArray(intArray, size)

	FREE(intArray)

	#for	i, 6, lt, 200000, i
		#call	miniumAllocSize(i)
		#call	fprintf(STDOUT, "%d => %d\n", i, r0)
	#endfor
	#return 0					// Programs should return 0 if all went well.
#end_func
	stop
	stop
