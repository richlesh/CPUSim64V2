///////////////////////////////////////////////////////////////////////////////
// Subtract Test (Run in Debug Mode)
//
// Tests the Subtract instructions.
//
//	SUB		AR,FX,YC,RAA,FFX,AAC,FFC
//
// Author: Richard Lesh
// Modified: 2017/02/08
///////////////////////////////////////////////////////////////////////////////

#define	PI					3.14159265358979323846
#define	NATURAL_LOG_BASE	2.71828182845904523536

BEGIN:
// Mode YC
	subtract	r2,1
	sub		r2,1
	sub		pc,-1
	sub		r2,1		// This one should be skipped.
	sub		sp,3
	sub		sf,1
	sub		f2,1
	sub		f2,1

// Mode AR, FX
	move	r1,-1
	sub		r3,r1
	sub		r3,r2
	sub		sp,r1
	sub		f3,r1
	sub		f3,r2

// Mode AAC, FFC
	sub		r4,r3,-2
	sub		f4,f3,-2

// Mode	AAR, FFX
	sub		r5,r4,r3
	sub		sf,sf,-1
	sub		f5,f4,r3
	move	f6,PI
	move	f7,NATURAL_LOG_BASE
	sub		f8,f7,f6

END:
	stop
	stop
