.LINE "T03_LOAD.asm", 13
MODE_XC:
	load	r1,@NEWLINE
	load	r2,@A
	load	r3,@COPYRIGHT
	load	r4,@SMILE
	load	r5,@FAVORITE
	load	f0,@PI
	load	f1,@SPEED_OF_LIGHT
.LINE "T03_LOAD.asm", 23
MODE_XA:
	move	r0,@COPYRIGHT
	load	r5,r0
	move	r20,@PI
	load	f2,r20
.LINE "T03_LOAD.asm", 30
MODE_XAC:
	load	r6,r0+1
	load	r7,r0[-1]
	load	f3,r20[1]
.LINE "T03_LOAD.asm", 36
	move	r8,3
	load	r9,@FIBONACCI[r8]
	move	r8,5
	load	r10,@FIBONACCI[r8]
	move	r8,2
	load	f4,@RENARD[r8]
	move	r8,5
	load	f5,@RENARD[r8]
.LINE "T03_LOAD.asm", 46
	load	r11,@COPYRIGHT+2
	load	f6,@SPEED_OF_LIGHT[-1]
.LINE "T03_LOAD.asm", 50
	move	r0,@FIBONACCI
	move	r8,9
	load	r12,r0,r8
	move	r8,1
	load	f7,r20,1
.LINE "T03_LOAD.asm", 57
MODE_STACK:
	push	1
	push	-1
	push	@PI
	push	f0
	load	f10,sp+1
	load	r10,sp[2]
	load	r11,3+sp
	load	r12,4[sp]
.LINE "T03_LOAD.asm", 67
END:
	stop
	stop
.LINE "T03_LOAD.asm", 71
NEWLINE:
	.DCB		0xA
A:
	.DCB		65
DEL:
	.DCB		0x7f
COPYRIGHT:
	.DCB		0xA9
SMILE:
	.DCB		0x263A
FAVORITE:
	.DCI		326
PI:
	.DCF		3.14159265358979323846
SPEED_OF_LIGHT:
	DCF		299792458
FIBONACCI:
	.DCW		1,1,2,3,5,8,13,21,34,55,89
RENARD:
	.DCW		0.1,0.2,0.5,1.,2.,5.,10.
UTF_16:
	.DCC		0x61, 0x62, 0xA9, 0x263A

