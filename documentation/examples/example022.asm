#include <system/io.asm>
#include <system/system.def>

	#call	main()
	int		iEXIT
	
#def_func	main()
	#var	arrayAddr, i, max, result
	clear	result
	move	max, 0x10
	#call	puts("Array Size: ")
	#call	put_dec(max)
	#call	put_nl()
	move	r0, max
	int		iALLOC			// Allocate a block in the heap
	jump	z, @ALLOC_FAILED
	move	arrayAddr, r0
	#call	puts("Allocated Size: ")
	load	r0, arrayAddr[-1]
	#call	put_dec(r0)
	#call	put_nl()

// Initialize values in array
	clear	i
	jump	@LOOP_END1
LOOP1:
	store	i, arrayAddr[i]
	add		i, 1
LOOP_END1:
	cmp		i, max
	jump	lt, @LOOP1

// Print out array in reverse order
	move	i, max
	sub		i, 1
	jump	@LOOP_END2
LOOP2:
	load	r0, arrayAddr[i]
	#call	put_dec(r0)
	#call	put_nl()
	sub		i, 1
LOOP_END2:
	cmp		i, 0
	jump	ge, @LOOP2

	move	r0, arrayAddr
	int		iFREE			// Free block when we are done with it
	jump	@FINIS
ALLOC_FAILED:
	#call	puts("Allocation Failed!")
	move	result, 1		// Non-zero error code to indicate failure	

FINIS:
	#return	result
#end_func