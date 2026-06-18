///////////////////////////////////////////////////////////////////////////////
// Register Store Test (Run in Debug Mode)
//
// Tests the basic forms of the register store instructions.
//
//	STORE		QC,QA,QAC,QCA,QCC,QAR
//
// Author: Richard Lesh
// Modified: 2017/02/03
///////////////////////////////////////////////////////////////////////////////

// In this test we destroy the HEAP linked list structure and just use 
// the heap as a scratch area to test storing.
SETUP:
	move	r0,__HEAP_START__
	move	r1,r0[4095]
	move	r2,r1[-4]
	move	r3,326
	move	r4,321
	move	r5,805
	load	f1,3.14159265358979323846
	load	f2,-3.14159265358979323846
	push	0
	push	0
	push	0
	push	0

// CC, CCC => Literal stored in literal address (plus const offset)
MODE_CC:
	st		1,__HEAP_START__
	sto		0x100,__HEAP_START__[1]
	store	"Hello!", __HEAP_START__[2]
	store	'a',__HEAP_START__[3]
	store	"Hello!",__HEAP_START__[4]
	store	2047,__HEAP_START__[4095]
	store	2046,__HEAP_START__[4094]
	store	2045,__HEAP_START__[4093]
	store	2044,4092[__HEAP_START__]
	store	2043,4091[__HEAP_START__]
	debug	r0,5
	debug	r2,5

// YC, YCC => Register stored in literal address (plus const offset)
	store	r3,__HEAP_START__
	store	r4,__HEAP_START__[1]
	store	r5, __HEAP_START__[2]
	store	f1,__HEAP_START__[3]
	store	f2,__HEAP_START__[4]
	store	r3,__HEAP_START__[4095]
	store	r4,__HEAP_START__[4094]
	store	r5,__HEAP_START__[4093]
	store	f1,4092[__HEAP_START__]
	store	f2,4091[__HEAP_START__]
	debug	r0,5
	debug	r2,5

// CA, CAC, CCA => Literal stored in register indirect (+ const offset)
MODE_CA:
	store	2,r0
	store	0x101,r0[1]
	store	"Goodbye!",r0[2]
	store	'b',r0[3]
	store	"Goodbye!",r0[4]
	store	1,r0[4095]
	store	2,r0[4094]
	store	3,r0[4093]
	store	4,4092[r0]
	store	5,4091[r0]
	debug	r0,5
	debug	r2,5

// YA, YAC, YCA => Register stored in register indirect (+ const offset)
MODE_YA:
	store	r0,r0
	store	r1,r0[1]
	store	r2,r0[2]
	store	r3,r0[3]
	store	r4,r0[4]
	store	r3,r0[4095]
	store	r4,r0[4094]
	store	r5,r0[4093]
	store	f1,4092[r0]
	store	f2,4091[r0]
	debug	r0,5
	debug	r2,5

// QAR => Register/const stored into register indirect (+ reg offset)
MODE_QAR:
	store	r5,r0
	move	r10,1
	store	r4,r0[r10]
	move	r10,2
	store	r3,r0[r10]
	move	r10,3
	store	r2,r0[r10]
	move	r10,4
	store	r1,r0[r10]
	move	r10,4095
	store	r1,r0[r10]
	move	r10,4094
	store	r2,r0[r10]
	move	r10,4093
	store	r3,r0[r10]
	move	r10,4092
	store	f2,r10[r0]
	move	r10,4091
	store	f1,r10[r0]
	debug	r0,5
	debug	r2,5

// Store onto stack
MODE_STACK:
	store	r3,sp[1]
	store	r4,sp[2]
	store	r5,3[sp]
	store	f1,4[sp]

END:
	stop
	stop
