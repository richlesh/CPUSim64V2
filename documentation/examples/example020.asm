#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	jump	@PROGRAM_START		// must jump to PROGRAM_START
								// to avoid executing the function

// MIN() computes minimum of two integer values
// r0		first integer value (gets destroyed)
// r1		second integer value
// returns result in r0
MIN:
	cmp		r0, r1
	mov		lt, r0, r0, r1
	return

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
	move	r0,r2				// Max expects values in r0 and r1
								// but our values are in r1 and r2
								// so we just move r2 into r0 to setup
	call	@MIN				// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	#call	puts("Max: ")
	move	r0,r2				// Max expects values in r0 and r1
								// but our values are in r1 and r2
								// so we just move r2 into r0 to setup
	call	@MAX				// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	jump	@ENDIF1
ELSE1:
	#call	puts("You must supply two command line arguments!")
ENDIF1:
	stop						// must stop the program so we don't
								// run into the functions defined below

// MAX() computes maximum of two integer values
// r0		first integer value (gets destroyed)
// r1		second integer value
// returns result in r0
MAX:
	push	r1					// We don't destroy r1 in this function
								// but if we did, we would need to push
								// its original value onto the stack to
								// preserve it
	cmp		r0, r1
	mov		gt, r0, r0, r1
	pop		r1					// When we are don we would pop r1 back
								// off the stack restoring its origianl
								// value.  Pops must match the number
								// of pushes but in the reverse order.
	return

FINIS:	
	stop
	stop
