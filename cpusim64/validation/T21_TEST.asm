///////////////////////////////////////////////////////////////////////////////
// Test instruction Test (Run in Debug Mode)
//
// Tests the Test instructions.
//
//	TEST	X
//
// Observe changes in status register bits.
//
// Author: Richard Lesh
// Modified: 2017/02/10
///////////////////////////////////////////////////////////////////////////////

BEGIN:

	move	r1,-1
	move	r2,0
	move	r3,1
	move	f1,-1
	move	f2,0
	move	f3,1

// Integer Test
	tst		r1
	debug	sr
	test	r2
	debug	sr
	test	r3
	debug	sr
	
// FP Test
	tst		f1
	debug	sr
	test	f2
	debug	sr
	test	f3
	debug	sr

END:
	stop
	stop
