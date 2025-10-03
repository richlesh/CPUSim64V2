///////////////////////////////////////////////////////////////////////////////
// AND Test (Run in Debug Mode)
//
// Tests the Logical AND instructions.
//
//	AND		RR,RC,RRR,RRC
//
// Author: Richard Lesh
// Modified: 2017/02/10
///////////////////////////////////////////////////////////////////////////////

BEGIN:
// Mode RC
	move	r1,0
	move	r2,-1
	move	r3,0xffffffff00000000
	move	r4,0xffffffff
	and		r1,-1
	and		r2,-1
	and		r3,-1
	and		r4,-1
	
	move	r5,0
	move	r6,-1
	move	r7,0xffffffff00000000
	move	r8,0xffffffff
	and		r5,0
	and		r6,0
	and		r7,0
	and		r8,0
	
	move	r9,0
	move	r10,-1
	move	r11,0xffffffff00000000
	move	r12,0xffffffff
	and		r9,0xffffffff00000000
	and		r10,0xffffffff00000000
	and		r11,0xffffffff00000000
	and		r12,0xffffffff00000000
	
	move	r13,0
	move	r14,-1
	move	r15,0xffffffff00000000
	move	r16,0xffffffff
	and		r13,0xffffffff
	and		r14,0xffffffff
	and		r15,0xffffffff
	and		r16,0xffffffff

// Mode RR
	move	r0,0xfedcba9876543210
	move	r17,0
	move	r18,-1
	move	r19,0xffffffff00000000
	move	r20,0x00000000ffffffff
	move	r21,0xf0f0f0f0f0f0f0f0
	move	r22,0x0f0f0f0f0f0f0f0f
	and		r17,r0
	and		r18,r0
	and		r19,r0
	and		r20,r0
	and		r21,r0
	and		r22,r0

// Mode RRC
	and		r23,r18,0xffff
	and		r24,r18,0xff00
	and		r25,r18,0xff
// Mode RRR
	and		r26,r18,r3
	and		r27,r21,r3
	and		r28,r22,r4

END:
	stop
	stop
