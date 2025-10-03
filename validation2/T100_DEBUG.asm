///////////////////////////////////////////////////////////////////////////////
// Debugging Functionality Test
//
// Test the debug op
//
// Author: Richard Lesh
// Modified: 2022/10/30
///////////////////////////////////////////////////////////////////////////////

BEGIN:
// Test to load sequence into R or F registers 0-12
#def_macro	setup(r)
	move	${r}0,0
	move	${r}1,1
	move	${r}2,2
	move	${r}3,3
	move	${r}4,4
	move	${r}5,5
	move	${r}6,6
	move	${r}7,7
	move	${r}8,8
	move	${r}9,9
	move	${r}10,10
	move	${r}11,11
	move	${r}12,12
	move	${r}13,13
	move	${r}14,14
	move	${r}15,15
	move	${r}16,16
	move	${r}17,17
	move	${r}18,18
	move	${r}19,19
	move	${r}20,20
	move	${r}21,21
	move	${r}22,22
	move	${r}23,23
	move	${r}24,24
	move	${r}25,25
	move	${r}26,26
	move	${r}27,27
	move	${r}28,28
#endif
#end_macro

// Test to load sequence into highest three FP registers
#def_macro	setupF()
	move	F29,29
	move	F30,30
	move	F31,31
#end_macro	testFC

MAIN:	
		nop
		setup(r)
		setup(f)
		setupF
		debug
		debug	r0
		debug	r1,r2
		debug	r3,r4,r5
		debug	r6,r7,r8,r9
		debug	r10,r11,r12,r13
		debug	r14,r15,r16,r17
		debug	r18,r19,r20,r21
		debug	r22,r23,r24,r25
		debug	r26,r27,r28
		debug	sf,sp,pc
		debug	f0
		debug	f1,f2
		debug	f3,f4,f5
		debug	f6,f7,f8,f9
		debug	f10,f11,f12,f13
		debug	f14,f15,f16,f17
		debug	f18,f19,f20,f21
		debug	f22,f23,f24,f25
		debug	f26,f27,f28,f29
		debug	f30,f31

		push	r1
		push	r2
		push	r3
		push	r4
		push	f1
		push	f2
		push	f3
		push	f4
		
		debug	sp,9
		move	r0,@HEAP_START
		dump	r0,-1					// Heap walker

		move	r0,myint
		dump	r0,4				

END:	stop
		stop
		
myint:	dci		19630326
		dci		0xdeadc0de
myfp:	dcf		3.14159265
		dcf		-3.14159265
