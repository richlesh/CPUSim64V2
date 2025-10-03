// Move character constants into a register
	move	r0,'\0'
	move	r1,'\t'
	move	r2,'\n'
	move	r3,' '
	move	r4,'\"'
	move	r5,'\''
	move	r6,'0'
	move	r7,'A'
	move	r8,'r'
	move	r9,'©'
	move	r10,'☺'
	move	r11,'\u263A'

// Move integer constants into a register
	move	r12,-5
	move	r13,-1
	move	r14,1
	move	r15,2
	move	r16,4
	move	r17,8
	move	r18,0x10
	move	r19,0x123456789abcd
	
// Move integer or floating point constants into an FP register
	move	f0,-10
	move	f1,-1
	move	f2,1
	move	f3,2
	move	f4,4
	move	f5,0x10
	move	f6,0x10000
	move	f7,0x123456789abcd
	move	f8,-1.234e-10
	move	f9,4.56789e200
	move	f10,3.14159265358979323846

	stop
	stop
