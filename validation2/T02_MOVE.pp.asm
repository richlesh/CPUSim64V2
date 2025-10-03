.LINE "T02_MOVE.asm", 13
MODE_RC:
	move	r0,0x0
	move	r1,0x8
	move	r2,0x9
	move	r3,0xA
	move	r4,0xC
	move	r5,0x20
	move	r6,0x22
	move	r7,0x27
	move	r8,0x30
	move	r9,0x41
	move	r10,0x5C
	move	r11,0x61
	move	r12,0x7f
	move	r13,0xA9
	move	r14,0x263A
	move	r15,0x263A
	move	r16,__STR_1
	move	r17,__STR_2
	move	r18, __STR_3
	move	r19, __STR_4
	move	r20, __STR_5
	move	r21, __STR_6
	move	r22,__HEAP_START__
	debug
.LINE "T02_MOVE.asm", 39
	move	r0,-1000
	move	r1,-500
	move	r2,-250
	move	r3,-50
	move	r4,-25
	move	r5,-10
	move	r6,-5
	move	r7,-1
	move	r8,1
	move	r9,2
	move	r10,4
	move	r11,8
	move	r12,0x10
	move	r13,0x100
	move	r14,0x1000
	move	r15,0x10000
	move	r16,0x100000
	move	r17,0x1000000
	move	r18,0x10000000
	move	r19,0x100000000
	move	r20,0x1000000000
	move	r21,0x10000000000
	move	r22,0x10000000000
	move	r23,0x10000000000
	move	r24,0x10000000000
	move	r25,0x10000000000
	move	r26,0x10000000000
	move	r27,0x10000000000
	move	r28,0x10000000000
	move	sf, 0x10000000000
	move	sp, 0x10000000000
	move	pc,pc
.LINE "T02_MOVE.asm", 72
MODE_FC:
	move	f0,-1000
	move	f1,-500
	move	f2,-250
	move	f3,-50
	move	f4,-25
	move	f5,-10
	move	f6,-5
	move	f7,-1
	move	f8,1
	move	f9,2
	move	f10,4
	move	f11,8
	move	f12,0x10
	move	f13,0x100
	move	f14,0x1000
	move	f15,0x10000
	move	f16,0x100000
	move	f17,0x1000000
	move	f18,0x10000000
	move	f19,0x100000000
	move	f20,0x1000000000
	move	f21,0x10000000000
	move	f22,0x10000000000
	move	f23,0x10000000000
	move	f24,0x10000000000
	move	f25,0x10000000000
	move	f26,0x10000000000
	move	f27,0x10000000000
	move	f28,0x10000000000
	debug
.LINE "T02_MOVE.asm", 105
MODE_YY:
	move	r0,pc
	move	r1,sp
	move	r2,sf
	move	r3,r26
	move	r4,r25
	move	r5,r24
	move	r6,f23
	move	r7,f22
	move	r8,f21
	move	f3,r26
	move	f4,r25
	move	f5,r24
	move	f6,f23
	move	f7,f22
	move	f8,f21
	load	f29,__FP_1
	load	f30,__FP_2
	load	f31,__FP_3
	move	r9,f29
	move	r10,f30
	move	r11,f31
	debug
.LINE "T02_MOVE.asm", 129
	clear
.LINE "T02_MOVE.asm", 132
MODE_ACC:
	move	r0,__HEAP_START__,r2
	move	r1,__STR_7,r2
.LINE "T02_MOVE.asm", 136
MODE_AAC:
	move	r2,pc+4
	move	r3,sp+2
	move	r4,sf[-2]
	move	r5,r1[3]
	move	r6,r3[0x1000000]
	move	r7,r3[134217727]
	move	sf,sf[2]
	move	sp,sp[-2]
	move	pc,pc[1]
	move	r28,-1
.LINE "T02_MOVE.asm", 148
MODE_ACA:
	move	r8,4,r1
	move	r9,5,r2
	move	r10,134217727,r3
	move	r11,0x7ffffff[r3]
.LINE "T02_MOVE.asm", 154
MODE_AAR:
	move	r12,r1,r2
	move	r13,r0[r5]
	debug
.LINE "T02_MOVE.asm", 159
	clear
.LINE "T02_MOVE.asm", 161
END:
	stop
	stop

__FP_1: .DCF 1e-10
__FP_2: .DCF 1e100
__FP_3: .DCF 3.1415926
__STR_1: .DCS "Hello, World!"
__STR_2: .DCS "☺\u263A©"
__STR_3: .DCS "\\'"
__STR_4: .DCS "\"\t\r\n"
__STR_5: .DCS "\b\f"
__STR_6: .DCS "\0"
__STR_7: .DCS "Hello"
