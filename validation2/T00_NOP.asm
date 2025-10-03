/******************************************************************************
* NOP/DEBUG Test (Run in Debug Mode)
*
* Tests the basic debug operations.
*
*	DEBUG	Y,YY,YYY,YYYY,AC,CC
*
* Author: Richard Lesh
* Modified: 2017/02/06
******************************************************************************/

SETUP:
	move	r0,-1		// End of line comment
	move	r1,1		
	move	r2,2          
	move	r3,3     	
	move	r4,4
	move	r5,5
	// Comment
// Comment
	move	r6,6
	move	r7,7
	move	r8,8
	move	r9,9
	move	r10,10
	move	r11,11
	move	r12,12
	move	r13,13
	move	r14,14
	move	r15,15
	move	r16,16
	move	r17,17
	move	r18,18
	move	r19,19
	move	r20,20
	move	r21,21
	move	r22,22
	move	r23,23
	move	r24,24
	move	r25,25
	move	r26,26
	move	r27,27
	move	r28,28

	move	f0,-1
	move	f1,1
	move	f2,2
	move	f3,3
	move	f4,4
	move	f5,5
	move	f6,6
	move	f7,7
	move	f8,8
	move	f9,9
	move	f10,10
	move	f11,11
	move	f12,12
	move	f13,13
	move	f14,14
	move	f15,15
	move	f16,16
	move	f17,17
	move	f18,18
	move	f19,19
	move	f20,20
	move	f21,21
	move	f22,22
	move	f23,23
	move	f24,24
	move	f25,25
	move	f26,26
	move	f27,27
	move	f28,28
	move	f29,29
	move	f30,30
	move	f31,31
	
	push	1
	push	2
	push	3
	nop		// Comment
	debug
DEBUG_4:
	debug	r0,r4,r8,r12
	debug	r16,r20,r24,r28
	debug	f0,f4,f8,f12
	debug	f16,f20,f24,f28

DEBUG_3:
	debug	r1,r5,r9
	debug	r13,r17,r21
	debug	r25,r25,r25
	debug	f1,f5,f9
	debug	f13,f17,f21
	debug	f25,f29,f29

DEBUG_2:
	debug	r2,r6
	debug	r10,r14
	debug	r18,r22
	debug	r26,r26
	debug	f2,f6
	debug	f10,f14
	debug	f18,f22
	debug	f26,f30

DEBUG_1:
	debug	r3
	debug	r7
	debug	r11
	debug	r15
	debug	r19
	debug	r23
	debug	r27
	debug 	f3
	debug	f7
	debug	f11
	debug	f15
	debug	f19
	debug	f23
	debug	f27
	debug	f31

STACK_TEST:
	push	SF
	move	SF,SP
	push	4
	push	5
	push	6
	push	7
	push	8
	push	9
	push	10
	push	11
	push	12
	push	13
	push	14
	push	15
	push	16
	push	17
	push	18
	push	19
	push	20
	push	21
	push	22
	push	23
	push	24
	push	25
	push	26
	push	27
	push	28
	push	29
	push	30
	push	31
	push	32
	
	debug
	
	push	33
	push	34
	
	debug

TEST_DUMP:
	debug	SETUP,4
	debug	__HEAP_START__,4
	debug	SF,3
	debug	SP,4
	debug	END, 4
	load	r0,__HEAP_START__[2]	// First free block size in heap which is a neg value
	neg	r0
	move	r0,__HEAP_START__
	debug	r0,4

TEST_HEAP_WALK:
	debug	__HEAP_START__,-1
	debug	r0,-1

	move	SP,SF
	pop		SF

END:
	stop
	stop
