///////////////////////////////////////////////////////////////////////////////
// Divide Test (Run in Debug Mode)
//
// Tests the Divide instructions.
//
// DIVIDE	F,RR,FX,XC,RRR,FFX,RRC,FFC,RRRR
//
// Author: Richard Lesh
// Modified: 2017/02/09
///////////////////////////////////////////////////////////////////////////////

#define	PI					3.14159265358979323846
#define	NATURAL_LOG_BASE	2.71828182845904523536

BEGIN:
// Mode XC
	move		r2,5
	divide		r2,2
	move		f2,5
	divide		f2,2

// Mode RR, FX
	move	r1,2
	move	r3,30
	div		r3,r1
	div		r3,r2
	move	f3,30
	div		f3,r1
	div		f3,f2

// Mode RRC, FFC
	div		r4,r3,2
	div		f4,f3,2

// Mode	RRR, FFX
	div		r5,r4,r2
	div		f5,f4,r2
	move	f6,PI
	move	f7,NATURAL_LOG_BASE
	div		f8,f7,f6

// Mode RRRR
	div		r6,r7,r3,r1
	div		r8,r9,r3,3
END:
	stop
	stop
