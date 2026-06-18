///////////////////////////////////////////////////////////////////////////////
// Save/Restore (Run in Debug Mode)
//
// Tests the register Save/Restore instructions.
//
//	SAVE	RR, FF
//	RESTORE	RR, FF
//
// Author: Richard Lesh
// Modified: 2026/06/17
///////////////////////////////////////////////////////////////////////////////

	#call main()
	stop

#def_func main()
	move	r0, 0
	move	r1, 1
	move	r2, 2
	move	r3, 3
	move	r4, 4
	move	r5, 5
	move	r6, 6
	move	r7, 7
	move	r8, 8
	move	r9, 9
	move	r10, 10
	move	r11, 11
	move	r12, 12
	move	r13, 13
	move	r14, 14
	move	r15, 15
	move	r16, 16
	move	r17, 17
	move	r18, 18
	move	r19, 19
	move	r20, 20
	move	r21, 21
	move	r22, 22
	move	r23, 23
	move	r24, 24
	move	r25, 25
	move	r26, 26
	move	r27, 27
	move	r28, 28
	load	f0, 0.
	load	f1, 1.
	load	f2, 2.
	load	f3, 3.
	load	f4, 4.
	load	f5, 5.
	load	f6, 6.
	load	f7, 7.
	load	f8, 8.
	load	f9, 9.
	load	f10, 10.
	load	f11, 11.
	load	f12, 12.
	load	f13, 13.
	load	f14, 14.
	load	f15, 15.
	load	f16, 16.
	load	f17, 17.
	load	f18, 18.
	load	f19, 19.
	load	f20, 20.
	load	f21, 21.
	load	f22, 22.
	load	f23, 23.
	load	f24, 24.
	load	f25, 25.
	load	f26, 26.
	load	f27, 27.
	load	f28, 28.
	load	f29, 29.
	load	f30, 30.
	load	f31, 31.

	save	r5, r12
	save	r17, r25
	save	f8, f15
	save	f22, f30
	
	debug
	
	clear
	
	restore	f22, f30
	restore	f8, f15
	restore	r17, r25
	restore	r5, r12
	
	debug
#end_func
