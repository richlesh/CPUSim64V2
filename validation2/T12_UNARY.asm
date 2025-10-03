///////////////////////////////////////////////////////////////////////////////
// Negate, Compliment and Reciprocal Test (Run in Debug Mode)
//
// Tests the Negate, Compliment and Reciprocal instructions.
//
//	NEGATE			X
//	COMPL			R
//	RECIP			F
//
// Author: Richard Lesh
// Modified: 2017/02/08
///////////////////////////////////////////////////////////////////////////////

#define	PI					3.14159265358979323846
#define	NATURAL_LOG_BASE	2.71828182845904523536

BEGIN:
// Negagte Test
	move 	r1,-2
	neg		r1
	move	r2,3
	neg		r2
	move	r3,0
	neg		r3
	move	f1,-2.5
	neg		f1
	move	f2,3.2
	negate	f2
	move	f3,0
	neg		f3

// Compliment Test
	move	r4,-1
	cpl		r4
	move	r5,0xcafebabe
	compl	r5
	move	r6,0xaaaa5555cccc3333
	compliment	r6
	move	r7,0
	compl	r7

// Reciprocal Test
	mov		f4,PI
	rcp		f4
	mov		f5,NATURAL_LOG_BASE
	recip	f5
	move	f6,0
	reciprocal	f6

END:
	stop
	stop
