///////////////////////////////////////////////////////////////////////////////
// Call Test (Run in Debug Mode)
//
// Tests the Call instruction.
//
//	CALL	A,C,ZA,ZC,ZAC,ZCA,ZCC,ZAR
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
// Modified: 2017/02/08
///////////////////////////////////////////////////////////////////////////////


SETUP:

// Test for C, ZC and ZCC modes and integer test
TEST_C:
	call	@LABEL_R1
	
// Test value 1
	move	r0,1
	call	z,@LABEL_R2
	move	r0,1
	call	nz,@LABEL_R3
	move	r0,1
	call	n,@LABEL_R4
	move	r0,1
	call	p,@LABEL_R5
	move	r0,1
	call	nn,@LABEL_R6
	move	r0,1
	call	np,@LABEL_R7
	move	r0,1
	call	pe,@LABEL_R8
	move	r0,1
	call	po,@LABEL_R9

// Test value -1
	move	r0,-1
	call	z,@LABEL_R10
	move	r0,-1
	call	nz,@LABEL_R11
	move	r0,-1
	call	n,@LABEL_R12
	move	r0,-1
	call	p,@LABEL_R13
	move	r0,-1
	call	nn,@LABEL_R14
	move	r0,-1
	call	np,@LABEL_R15
	move	r0,-1
	call	pe,@LABEL_R16
	move	r0,-1
	call	po,@LABEL_R17

// Test value 0
	move	r0,0
	call	z,@LABEL_R18
	move	r0,0
	call	nz,@LABEL_R19
	move	r0,0
	call	n,@LABEL_R20
	move	r0,0
	call	p,@LABEL_R21
	move	r0,0
	call	nn,@LABEL_R22
	move	r0,0
	call	np,@LABEL_R23
	move	r0,0
	call	pe,@LABEL_R24
	move	r0,0
	call	po,@LABEL_R25

	move	r0,0
	call	z,@LABEL_R26+2

	debug

// Test A,ZA,ZAC,ZCA,ZAR and FP tests
// Test value 0, Mode ZA
	move	r0,@LABEL_F1
	move	f0,0
	call	z,r0
	move	r0,@LABEL_F2
	move	f0,0
	call	nz,r0
	move	r0,@LABEL_F3
	move	f0,0
	call	n,r0
	move	r0,@LABEL_F4
	move	f0,0
	call	p,r0
	move	r0,@LABEL_F5
	move	f0,0
	call	nn,r0
]	move	r0,@LABEL_F6
	move	f0,0
	call	np,r0

// Test value 1, Mode ZAC,ZCA,ZAR
	move	r0,@LABEL_F7[-4]
	move	f0,1
	call	z,r0[4]
	move	r0,@LABEL_F8[-4]
	move	f0,1
	call	nz,r0+4
	move	r0,@LABEL_F9[-4]
	move	f0,1
	call	n,4+r0
	move	r0,@LABEL_F10[-4]
	move	f0,1
	call	p,4[r0]
	move	r0,@LABEL_F11[-4]
	move	r1,4
	move	f0,1
	call	nn,r0,r1
	move	r0,@LABEL_F12,-4
	move	f0,1
	call	np,r0[r1]

// Mode A
	move	r0,@LABEL_F13
	call	r0

END:
	stop

#def_macro	subroutine(type,num)
LABEL_${type}${num}:
	move	${type}${num},-1
	return
#end_macro

	subroutine(r,1)
	subroutine(r,2)
	subroutine(r,3)
	subroutine(r,4)
	subroutine(r,5)
	subroutine(r,6)
	subroutine(r,7)
	subroutine(r,8)
	subroutine(r,9)
	subroutine(r,10)
	subroutine(r,11)
	subroutine(r,12)
	subroutine(r,13)
	subroutine(r,14)
	subroutine(r,15)
	subroutine(r,16)
	subroutine(r,17)
	subroutine(r,18)
	subroutine(r,19)
	subroutine(r,20)
	subroutine(r,21)
	subroutine(r,22)
	subroutine(r,23)
	subroutine(r,24)
	subroutine(r,25)
	subroutine(r,26)
	subroutine(r,27)
	subroutine(r,28)
	subroutine(f,1)
	subroutine(f,2)
	subroutine(f,3)
	subroutine(f,4)
	subroutine(f,5)
	subroutine(f,6)
	subroutine(f,7)
	subroutine(f,8)
	subroutine(f,9)
	subroutine(f,10)
	subroutine(f,11)
	subroutine(f,12)
	subroutine(f,13)
	subroutine(f,14)
	subroutine(f,15)
	subroutine(f,16)
	subroutine(f,17)
	subroutine(f,18)
	subroutine(f,19)
	subroutine(f,20)
	subroutine(f,21)
	subroutine(f,22)
	subroutine(f,23)
	subroutine(f,24)
	subroutine(f,25)
	subroutine(f,26)
	subroutine(f,27)
	subroutine(f,28)
	subroutine(f,29)
	subroutine(f,30)
	subroutine(f,31)

	stop
	stop
