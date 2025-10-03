///////////////////////////////////////////////////////////////////////////////
// Right Shift Test (Run in Debug Mode)
//
// Tests the Logical Right Shift instructions.
//
//	RSHIFT		RR,RC,RRR,RRC
//
// Author: Richard Lesh
// Modified: 2017/02/11
///////////////////////////////////////////////////////////////////////////////

BEGIN:

	move	r1,0xfedcba9876543210

// Mode RC
	move	r2,r1
	rshift	r2,1
	move	r3,r1
	rsh		r3,2
	move	r4,r1
	rsh		r4,3
	move	r5,r1
	rsh		r5,4

// Mode RR
	move	r0,8
	move	r6,r1
	rsh		r6,r0
	add		r0,4
	move	r7,r1
	rsh		r7,r0

// Mode RRC
	rsh		r8,r7,4
	rsh		r9,r6,12

// Mode RRR
	move	r0,24
	rsh		r10,r1,r0
	rsh		r11,r5,r0
	
	rsh		r12,r1,64

END:
	stop
	stop
