///////////////////////////////////////////////////////////////////////////////
// Loop Test (Run in Debug Mode)
//
// Tests looping.
//
// Computes 90 Fibonacci numbers
//
// Author: Richard Lesh
// Modified: 2017/02/078
///////////////////////////////////////////////////////////////////////////////


BEGIN:

	move	r1,1
	move	r2,1
	debug	r1
	debug	r2
	move	r3,90
LOOP1:
	add		r0,r1,r2
	debug	r0
	move	r1,r2
	move	r2,r0
	sub		r3,1
	jump	nz,@LOOP1

END:
	stop
	stop
