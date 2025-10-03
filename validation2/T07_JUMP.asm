///////////////////////////////////////////////////////////////////////////////
// Jump Test (Run in Debug Mode)
//
// Tests the Jump instruction.
//
//	JUMP		A,C,ZA,ZC,ZAC,ZCA,ZCC,ZAR
//
// Condition Codes (Z)
// 0	u	Unconditional
// 1	z	Zero
// 2	nz	Not Zero
// 3	n	Negative
// 4	p	Positive
// 5	nn	Not Negative
// 6	np	Not Positive
// 7	o	Overflow
// 8	no	No Overflow
// 9	pe	Even Parity
// 10	po	Odd Parity
//
// Author: Richard Lesh
// Modified: 2017/02/07
///////////////////////////////////////////////////////////////////////////////


SETUP:

// Test for C, ZC and ZCC modes and integer test
TEST_C:
	jmp		@LABEL1
	move	r1,-1
LABEL1:

// Test value 1
	move	r0,1
	jump	z,@LABEL2
	move	r2,-1
LABEL2:
	move	r0,1
	jump	nz,@LABEL3
	move	r3,-1
LABEL3:
	move	r0,1
	jump	n,@LABEL4
	move	r4,-1
LABEL4:
	move	r0,1
	jump	p,@LABEL5
	move	r5,-1
LABEL5:
	move	r0,1
	jump	nn,@LABEL6
	move	r6,-1
LABEL6:
	move	r0,1
	jump	np,@LABEL7
	move	r7,-1
LABEL7:
	move	r0,1
	jump	pe,@LABEL8
	move	r8,-1
LABEL8:
	move	r0,1
	jump	po,@LABEL9
	move	r9,-1
LABEL9:

// Test value -1
	move	r0,-1
	jump	z,@LABEL10
	move	r10,-1
LABEL10:
	move	r0,-1
	jump	nz,@LABEL11
	move	r11,-1
LABEL11:
	move	r0,-1
	jump	n,@LABEL12
	move	r12,-1
LABEL12:
	move	r0,-1
	jump	p,@LABEL13
	move	r13,-1
LABEL13:
	move	r0,-1
	jump	nn,@LABEL14
	move	r14,-1
LABEL14:
	move	r0,-1
	jump	np,@LABEL15
	move	r15,-1
LABEL15:
	move	r0,-1
	jump	pe,@LABEL16
	move	r16,-1
LABEL16:
	move	r0,-1
	jump	po,@LABEL17
	move	r17,-1
LABEL17:

// Test value 0
	move	r0,0
	jump	z,@LABEL18
	move	r18,-1
LABEL18:
	move	r0,0
	jump	nz,@LABEL19
	move	r19,-1
LABEL19:
	move	r0,0
	jump	n,@LABEL20
	move	r20,-1
LABEL20:
	move	r0,0
	jump	p,@LABEL21
	move	r21,-1
LABEL21:
	move	r0,0
	jump	nn,@LABEL22
	move	r22,-1
LABEL22:
	move	r0,0
	jump	np,@LABEL23
	move	r23,-1
LABEL23:
	move	r0,0
	jump	pe,@LABEL24
	move	r24,-1
LABEL24:
	move	r0,0
	jump	po,@LABEL25
	move	r25,-1
LABEL25:

	move	r0,0
	jump	z,@LABEL26+1
	move	r26,-1
LABEL26:
	move	r27,-1
	move	r28,-1

	debug

// Test A,ZA,ZAC,ZCA,ZAR and FP tests
// Test value 0, Mode ZA
	move	r0,@LABEL27
	move	f0,0
	jump	z,r0
	move	f1,-1
LABEL27:
	move	r0,@LABEL28
	move	f0,0
	jump	nz,r0
	move	f2,-1
LABEL28:
	move	r0,@LABEL29
	move	f0,0
	jump	n,r0
	move	f3,-1
LABEL29:
	move	r0,@LABEL30
	move	f0,0
	jump	p,r0
	move	f4,-1
LABEL30:
	move	r0,@LABEL31
	move	f0,0
	jump	nn,r0
	move	f5,-1
LABEL31:
	move	r0,@LABEL32
	move	f0,0
	jump	np,r0
	move	f6,-1
LABEL32:

// Test value 1, Mode ZAC,ZCA,ZAR
	move	r0,@LABEL33[-4]
	move	f0,1
	jump	z,r0[4]
	move	f7,-1
LABEL33:
	move	r0,@LABEL34[-4]
	move	f0,1
	jump	nz,r0+4
	move	f8,-1
LABEL34:
	move	r0,@LABEL35[-4]
	move	f0,1
	jump	n,4+r0
	move	f9,-1
LABEL35:
	move	r0,@LABEL36[-4]
	move	f0,1
	jump	p,4[r0]
	move	f10,-1
LABEL36:
	move	r0,@LABEL37[-4]
	move	r1,4
	move	f0,1
	jump	nn,r0,r1
	move	f11,-1
LABEL37:
	move	r0,@LABEL38,-4
	move	f0,1
	jump	np,r0[r1]
	move	f12,-1
LABEL38:

// Mode A
	move	r0,@LABEL39
	jump	r0
	move	f13,-1
LABEL39:

END:
	stop
	stop
