///////////////////////////////////////////////////////////////////////////////
// Push/Pop Test (Run in Debug Mode)
//
// Tests the push and pop instructions.
//
//	POP				N,Y
//	PUSH			Y,C
//
// Author: Richard Lesh
// Modified: 2017/02/04
///////////////////////////////////////////////////////////////////////////////


SETUP:

// Y,C
PUSH_TEST:

	push	1
	push	0x100
	push	'a'
	push	'\x7f'
	push	'©'
	push	'☺'
	push	'\u263A'
	push	"Hello, World!"
	push	"☺\u263A©\x7f\\\'\"\t\r\n\b\f\a\c\0"
	push	@HEAP_START
	move	r0,326
	move	r1,@HEAP_START
	push	r0
	push	r1
	push	sf
	push	sp
	push	pc
	move	f0,3.14
	move	f1,-1.
	move	f2,1.e-20
	push	f0
	push	f1
	push	f2
	
	debug
	
POP_TEST:

	clear
	pop		f2
	pop		f1
	pop		f0
	pop
	pop
	pop
	pop		r0
	pop		r1
	pop		r2
	pop		r3
	pop		r4
	pop		r5
	pop		r6
	pop		r7
	pop		r8
	pop		r9
	pop		r10
	pop		r11

BIG_STACK_TEST:
	push	1
	push	2
	push	3
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

END:
	stop
	stop
