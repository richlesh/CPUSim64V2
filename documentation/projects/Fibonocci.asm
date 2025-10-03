///////////////////////////////////////////////////////////////////////////////
// Test Program
//
// Prints Fibonocci sequence.
//
// Author: Richard Lesh
// Original: 2009/03/20
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>

MAIN:
	#call	put_dec(1)
	#call	put_nl()
	#call	put_dec(1)
	#call	put_nl()
	move	r2,1
	move	r3,1
	move	r4,43
LOOP:	
	add		r1,r2,r3
	#call	put_dec(r1)
	#call	put_nl()
	move	r3,r2
	move	r2,r1
	sub		r4,1
	jmp		nz,LOOP

	stop
	stop
