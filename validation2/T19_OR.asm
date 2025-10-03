///////////////////////////////////////////////////////////////////////////////
// OR Test (Run in Debug Mode)
//
// Tests the Logical OR instructions.
//
//	OR		RR,RC,RRR,RRC
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
	or		r1,-1
	or		r2,-1
	or		r3,-1
	or		r4,-1
	
	move	r5,0
	move	r6,-1
	move	r7,0xffffffff00000000
	move	r8,0xffffffff
	or		r5,0
	or		r6,0
	or		r7,0
	or		r8,0
	
	move	r9,0
	move	r10,-1
	move	r11,0xffffffff00000000
	move	r12,0xffffffff
	or		r9,0xffffffff00000000
	or		r10,0xffffffff00000000
	or		r11,0xffffffff00000000
	or		r12,0xffffffff00000000
	
	move	r13,0
	move	r14,-1
	move	r15,0xffffffff00000000
	move	r16,0xffffffff
	or		r13,0xffffffff
	or		r14,0xffffffff
	or		r15,0xffffffff
	or		r16,0xffffffff

// Mode RR
	move	r0,0xfedcba9876543210
	move	r17,0
	move	r18,-1
	move	r19,0xffffffff00000000
	move	r20,0x00000000ffffffff
	move	r21,0xf0f0f0f0f0f0f0f0
	move	r22,0x0f0f0f0f0f0f0f0f
	or		r17,r0
	or		r18,r0
	or		r19,r0
	or		r20,r0
	or		r21,r0
	or		r22,r0

// Mode RRC
	or		r23,r17,0xffff
	or		r24,r17,0xff00
	or		r25,r17,0xff
// Mode RRR
	or		r26,r17,r7
	or		r27,r21,r7
	or		r28,r22,r8

END:
	stop
	stop
