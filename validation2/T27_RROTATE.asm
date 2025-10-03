///////////////////////////////////////////////////////////////////////////////
// Right Rotate Test (Run in Debug Mode)
//
// Tests the Right Rotate instructions.
//
//	RROTATE		RR,RC,RRR,RRC
//
// Author: Richard Lesh
// Modified: 2017/02/11
///////////////////////////////////////////////////////////////////////////////

BEGIN:

	move	r1,0xfedcba9876543210

// Mode RC
	move	r2,r1
	rrotate	r2,1
	move	r3,r1
	rrot		r3,2
	move	r4,r1
	rrot		r4,3
	move	r5,r1
	rrot		r5,4

// Mode RR
	move	r0,8
	move	r6,r1
	rrot		r6,r0
	add		r0,4
	move	r7,r1
	rrot		r7,r0

// Mode RRC
	rrot		r8,r7,4
	rrot		r9,r6,12

// Mode RRR
	move	r0,24
	rrot		r10,r1,r0
	rrot		r11,r5,r0
	
	rrot		r12,r1,64

END:
	stop
	stop
