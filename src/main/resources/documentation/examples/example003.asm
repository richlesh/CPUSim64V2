// Move character constants into a register
	move	r0,'\0'
	move	r1,'\b'
	move	r2,'\t'
	move	r3,'\n'
	move	r4,'\f'
	move	r5,'\r'
	move	r6,' '
	move	r7,'\"'
	move	r8,'\''
	move	r9,'\\'
	move	r10,'0'
	move	r11,'A'
	move	r12,'r'
	move	r13,'©'
	move	r14,'☺'
	move	r15,'\u{263A}'
	move	r16,'\u{1F600}'

// Move integer constants into a register
	move	r17,-5
	move	r18,-1
	move	r19,1
	move	r20,2
	move	r21,4
	move	r22,8
	move	r23,0x10
	move	r24,0x1ffffffffff
	move	r25,-0x20000000000
	
// Move integer or floating point constants into an FP register
	move	f0,-10
	move	f1,-1
	move	f2,1
	move	f3,2
	move	f4,4
	move	f5,0x10
	move	f6,0x10000
	move	f7,-0x20000000000
	load	f8,-1.234e-10
	load	f9,4.56789e200
	load	f10,3.14159265358979323846

	stop
	stop
