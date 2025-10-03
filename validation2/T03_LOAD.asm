///////////////////////////////////////////////////////////////////////////////
// Register Load Test (Run in Debug Mode)
//
// Tests the basic forms of the register load instructions.
//
//	LOAD		YC,YA,YAC,YCA,YCC,YAR
//
// Author: Richard Lesh
// Modified: 2017/02/02
///////////////////////////////////////////////////////////////////////////////

// XC => Literal address load
MODE_XC:
	load	r1,NEWLINE
	load	r2,A
	load	r3,COPYRIGHT
	load	r4,SMILE
	load	r5,FAVORITE
	load	f0,PI
	load	f1,SPEED_OF_LIGHT

// XA => register indirect load
MODE_XA:
	move	r0,COPYRIGHT
	load	r5,r0
	move	r20,PI
	load	f2,r20

// XAC => register indirect with const offset
MODE_XAC:
	load	r6,r0[1]
	load	r7,r0[-1]
	load	f3,r20[1]

// XCA => constant address with register offset
	move	r8,3
	load	r9,FIBONACCI[r8]
	move	r8,5
	load	r10,FIBONACCI[r8]
	move	r8,2
	load	f4,RENARD[r8]
	move	r8,5
	load	f5,RENARD[r8]

// XCC => const address with const offset
	load	r11,COPYRIGHT[2]
	load	f6,SPEED_OF_LIGHT[-1]

// XAR => reg address with reg offset
	move	r0,FIBONACCI
	move	r8,9
	load	r12,r0[r8]
	move	r8,1
	load	f7,r20[1]

// Load from stack
MODE_STACK:
	push	1
	push	-1
	push	PI
	push	f0
	load	f10,sp[1]
	load	r10,sp[2]
	load	r11,3[sp]
	load	r12,4[sp]

END:
	stop
	stop

NEWLINE:
	.DCB		'\n'
A:
	.DCB		65
DEL:
	.DCB		0x7f
COPYRIGHT:
	.DCB		'©'
SMILE:
	.DCB		'\u263a'
FAVORITE:
	.DCI		326
PI:
	.DCF		3.14159265358979323846
SPEED_OF_LIGHT:
	.DCF		299792458.
FIBONACCI:
	.DCW		1,1,2,3,5,8,13,21,34,55,89
RENARD:
	.DCW		0.1,0.2,0.5,1.,2.,5.,10.
UTF_16:
	.DCC		'a', 'b', '©', '\u263a'
