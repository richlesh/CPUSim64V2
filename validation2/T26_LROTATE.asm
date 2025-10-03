///////////////////////////////////////////////////////////////////////////////
// Left Rotate Test (Run in Debug Mode)
//
// Tests the Left Rotate instructions.
//
//	LROTATE		RR,RC,RRR,RRC
//
// Author: Richard Lesh
// Modified: 2017/02/11
///////////////////////////////////////////////////////////////////////////////

BEGIN:

	move	r1,0xfedcba9876543210

// Mode RC
	move	r2,r1
	lrotate	r2,1
	move	r3,r1
	lrot		r3,2
	move	r4,r1
	lrot		r4,3
	move	r5,r1
	lrot		r5,4

// Mode RR
	move	r0,8
	move	r6,r1
	lrot		r6,r0
	add		r0,4
	move	r7,r1
	lrot		r7,r0

// Mode RRC
	lrot		r8,r7,4
	lrot		r9,r6,12

// Mode RRR
	move	r0,24
	lrot		r10,r1,r0
	lrot		r11,r5,r0
	
	lrot		r12,r1,64

END:
	stop
	stop
