#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	jump	PROGRAM_START		// must jump to PROGRAM_START
								// to avoid executing the function

// MIN() computes minimum of two integer values
// r1		first integer value
// r2		second integer value
// returns result in r0
MIN:
	cmp		r1, r2
	mov		lt, r0, r1, r2
	return

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
	move	r1, r4				// Min expects values in r1 and r2
	move	r2, r5
	call	MIN					// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	#call	puts("Max: ")
	move	r1, r4				// Max expects values in r1 and r2
	move	r2, r5
	call	MAX					// return will be in r0
	#call	put_dec(r0)
	#call	put_nl()
	jump	ENDIF1
ELSE1:
	#call	puts("You must supply two command line arguments!")
ENDIF1:
	stop						// must stop the program so we don't
								// run into the functions defined below

// MAX() computes maximum of two integer values
// r1		first integer value
// r2		second integer value
// returns result in r0
MAX:
	cmp		r1, r2
	mov		gt, r0, r1, r2
	return

FINIS:	
	stop
	stop
