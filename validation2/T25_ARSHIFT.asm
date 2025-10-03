///////////////////////////////////////////////////////////////////////////////
// Arithmetic Right Shift Test (Run in Debug Mode)
//
// Tests the Arithmetic Right Shift instructions.
//
//	ARSHIFT		RR,RC,RRR,RRC
//
// Author: Richard Lesh
// Modified: 2017/02/11
///////////////////////////////////////////////////////////////////////////////

BEGIN:

	move	r1,0xfedcba9876543210

// Mode RC
	move	r2,r1
	arshift	r2,1
	move	r3,r1
	arsh	r3,2
	move	r4,r1
	arsh	r4,3
	move	r5,r1
	arsh	r5,4

// Mode RR
	move	r0,8
	move	r6,r1
	arsh	r6,r0
	add		r0,4
	move	r7,r1
	arsh	r7,r0

// Mode RRC
	arsh	r8,r7,4
	arsh	r9,r6,12

// Mode RRR
	move	r0,24
	arsh	r10,r1,r0
	arsh	r11,r5,r0
	
	arsh	r12,r1,64

END:
	stop
	stop
