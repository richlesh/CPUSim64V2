///////////////////////////////////////////////////////////////////////////////
// Register Clear Test (Run in Debug Mode)
//
// Tests the basic forms of the register clear instructions.
//
//	CLEAR		N,X,XX,XXX,XXXX
//
// Author: Richard Lesh
// Modified: 2017/02/02
///////////////////////////////////////////////////////////////////////////////

SETUP:
	move	r0,-1
	move	r1,-1
	move	r2,-1
	move	r3,-1
	move	r4,-1
	move	r5,-1
	move	r6,-1
	move	r7,-1
	move	r8,-1
	move	r9,-1
	move	r10,-1
	move	r11,-1
	move	r12,-1
	move	r13,-1
	move	r14,-1
	move	r15,-1
	move	r16,-1
	move	r17,-1
	move	r18,-1
	move	r19,-1
	move	r20,-1
	move	r21,-1
	move	r22,-1
	move	r23,-1
	move	r24,-1
	move	r25,-1
	move	r26,-1
	move	r27,-1
	move	r28,-1

	move	f0,-1
	move	f1,-1
	move	f2,-1
	move	f3,-1
	move	f4,-1
	move	f5,-1
	move	f6,-1
	move	f7,-1
	move	f8,-1
	move	f9,-1
	move	f10,-1
	move	f11,-1
	move	f12,-1
	move	f13,-1
	move	f14,-1
	move	f15,-1
	move	f16,-1
	move	f17,-1
	move	f18,-1
	move	f19,-1
	move	f20,-1
	move	f21,-1
	move	f22,-1
	move	f23,-1
	move	f24,-1
	move	f25,-1
	move	f26,-1
	move	f27,-1
	move	f28,-1
	move	f29,-1
	move	f30,-1
	move	f31,-1
	debug

CLEAR_4:
	clear	r0,r4,r8,r12
	clear	r16,r20,r24,r28
	clear	f0,f4,f8,f12
	clear	f16,f20,f24,f28

	debug

CLEAR_3:
	clear	r1,r5,r9
	clear	r13,r17,r21
	clear	r25,r25,r25
	clear	f1,f5,f9
	clear	f13,f17,f21
	clear	f25,f29,f29

	debug

CLEAR_2:
	clear	r2,r6
	clear	r10,r14
	clear	r18,r22
	clear	r26,r26
	clear	f2,f6
	clear	f10,f14
	clear	f18,f22
	clear	f26,f30

	debug

CLEAR_1:
	clear	r3
	clear	r7
	clear	r11
	clear	r15
	clear	r19
	clear	r23
	clear	r27
	clear 	f3
	clear	f7
	clear	f11
	clear	f15
	clear	f19
	clear	f23
	clear	f27
	clear	f31

	debug

CLEAR_ALL:
	move	r0,-1
	move	r1,-1
	move	r2,-1
	move	r3,-1
	move	r4,-1
	move	r5,-1
	move	r6,-1
	move	r7,-1
	move	r8,-1
	move	r9,-1
	move	r10,-1
	move	r11,-1
	move	r12,-1
	move	r13,-1
	move	r14,-1
	move	r15,-1
	move	r16,-1
	move	r17,-1
	move	r18,-1
	move	r19,-1
	move	r20,-1
	move	r21,-1
	move	r22,-1
	move	r23,-1
	move	r24,-1
	move	r25,-1
	move	r26,-1
	move	r27,-1
	move	r28,-1

	move	f0,-1
	move	f1,-1
	move	f2,-1
	move	f3,-1
	move	f4,-1
	move	f5,-1
	move	f6,-1
	move	f7,-1
	move	f8,-1
	move	f9,-1
	move	f10,-1
	move	f11,-1
	move	f12,-1
	move	f13,-1
	move	f14,-1
	move	f15,-1
	move	f16,-1
	move	f17,-1
	move	f18,-1
	move	f19,-1
	move	f20,-1
	move	f21,-1
	move	f22,-1
	move	f23,-1
	move	f24,-1
	move	f25,-1
	move	f26,-1
	move	f27,-1
	move	f28,-1
	move	f29,-1
	move	f30,-1
	move	f31,-1

	debug

	clear
	
	debug
END:
	stop
	stop
