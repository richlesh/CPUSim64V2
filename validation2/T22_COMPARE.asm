///////////////////////////////////////////////////////////////////////////////
// Compare Test (Run in Debug Mode)
//
// Tests the Compare instructions.
//
//	CMP		AA,AC,FF
//
// Observe changes in status register bits.
//
// Author: Richard Lesh
// Modified: 2017/02/11
///////////////////////////////////////////////////////////////////////////////

BEGIN:

	move	r1,-1
	move	r2,0
	move	r3,1
	move	f1,-1
	move	f2,0
	move	f3,1

// Integer Test
	compare	r1,-1
	debug	r1
	cmp		r1,1
	debug	r1
	
	cmp		r1,r2
	debug	r1,r2
	cmp		r1,r3
	debug	r1,r3
	cmp		r2,r1
	debug	r2,r1
	cmp		r2,r3
	debug	r2,r3
	cmp		r3,r1
	debug	r3,r1
	cmp		r3,r2
	debug	r3,r2
	cmp		r3,r3
	debug	r3,r3

	cmp		sf,sp
	debug	sf,sp
	
	push	1
	cmp		sf,sp
	debug	sf,sp
	cmp		sp,sf
	debug	sp,sf
	
// FP Test
	cmp		f1,f2
	debug	f1,f2
	cmp		f1,f3
	debug	f1,f3
	cmp		f2,f1
	debug	f2,f1
	cmp		f2,f3
	debug	f2,f3
	cmp		f3,f1
	debug	f3,f1
	cmp		f3,f2
	debug	f3,f2
	cmp		f3,f3
	debug	f3,f3

END:
	stop
	stop
