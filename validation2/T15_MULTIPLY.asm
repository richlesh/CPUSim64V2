///////////////////////////////////////////////////////////////////////////////
// Multiply Test (Run in Debug Mode)
//
// Tests the Multiply instructions.
//
//	MULTIPLY	RR,FX,XC,RRR,FFX,RRC,FFC
//
// Author: Richard Lesh
// Modified: 2017/02/09
///////////////////////////////////////////////////////////////////////////////

#define	PI					3.14159265358979323846
#define	NATURAL_LOG_BASE	2.71828182845904523536

BEGIN:
// Mode XC
	move		r2,1
	multiply	r2,2
	move		f2,1
	multiply	f2,2

// Mode RR, FX
	move	r1,3
	move	r3,1
	mult	r3,r1
	mult	r3,r2
	move	f3,1
	mul		f3,r1
	mul		f3,f2

// Mode RRC, FFC
	mul		r4,r3,2
	mul		f4,f3,2

// Mode	RRR, FFX
	mul		r5,r4,r3
	mul		f5,f4,r3
	move	f6,PI
	move	f7,NATURAL_LOG_BASE
	mul		f8,f7,f6

END:
	stop
	stop
