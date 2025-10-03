///////////////////////////////////////////////////////////////////////////////
// XOR Test (Run in Debug Mode)
//
// Tests the Logical XOR instructions.
//
//	XOR		RR,RC,RRR,RRC
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
	xor		r1,-1
	xor		r2,-1
	xor		r3,-1
	xor		r4,-1
	
	move	r5,0
	move	r6,-1
	move	r7,0xffffffff00000000
	move	r8,0xffffffff
	xor		r5,0
	xor		r6,0
	xor		r7,0
	xor		r8,0
	
	move	r9,0
	move	r10,-1
	move	r11,0xffffffff00000000
	move	r12,0xffffffff
	xor		r9,0xffffffff00000000
	xor		r10,0xffffffff00000000
	xor		r11,0xffffffff00000000
	xor		r12,0xffffffff00000000
	
	move	r13,0
	move	r14,-1
	move	r15,0xffffffff00000000
	move	r16,0xffffffff
	xor		r13,0xffffffff
	xor		r14,0xffffffff
	xor		r15,0xffffffff
	xor		r16,0xffffffff

// Mode RR
	move	r0,0xfedcba9876543210
	move	r17,0
	move	r18,-1
	move	r19,0xffffffff00000000
	move	r20,0x00000000ffffffff
	move	r21,0xf0f0f0f0f0f0f0f0
	move	r22,0x0f0f0f0f0f0f0f0f
	xor		r17,r0
	xor		r18,r0
	xor		r19,r0
	xor		r20,r0
	xor		r21,r0
	xor		r22,r0

// Mode RRC
	xor		r23,r17,0xffff
	xor		r24,r17,0xff00
	xor		r25,r17,0xff
// Mode RRR
	xor		r26,r17,r7
	xor		r27,r21,r7
	xor		r28,r22,r8

END:
	stop
	stop
