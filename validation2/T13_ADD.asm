///////////////////////////////////////////////////////////////////////////////
// Add Test (Run in Debug Mode)
//
// Tests the Add instructions.
//
//	ADD		AR,FX,YC,AAR,FFX,AAC,FFC
//
// Author: Richard Lesh
// Modified: 2017/02/08
///////////////////////////////////////////////////////////////////////////////

#define	PI					3.14159265358979323846
#define	NATURAL_LOG_BASE	2.71828182845904523536

BEGIN:
// Mode YC
	add		r2,1
	add		r2,1
	add		pc,1
	add		r2,1		// This one should be skipped.
	add		sp,-3
	add		sf,-1
	add		f2,1
	add		f2,1

// Mode AR, FX
	move	r1,1
	add		r3,r1
	add		r3,r2
	add		sp,r1
	add		f3,r1
	add		f3,f2

// Mode AAC, FFC
	add		r4,r3,2
	add		f4,f3,2

// Mode	AAR, FFX
	add		r5,r4,r3
	add		sf,sf,1
	add		f5,f4,r3
	move	f6,PI
	move	f7,NATURAL_LOG_BASE
	add		f8,f7,f6

END:
	stop
	stop
