// Two register arithmetic
	move	r0,1
	move	r1,2
	add		r0,r1		// r0 <- r0 + r1
	debug	r0
	sub		r0,r1		// r0 <- r0 - r1
	debug	r0
	move	r0,3
	mult	r0,r1		// r0 <- r0 * r1
	debug	r0
	div		r0,r1		// r0 <- r0 / r1 (integer divide)
	debug	r0

	move	f0,1.234
	move	f1,2.345
	add		f0,f1		// f0 <- f0 + f1
	debug	f0
	sub		f0,f1		// f0 <- f0 - f1
	debug	f0
	move	f0,3.14159
	mult	f0,f1		// f0 <- f0 * f1
	debug	f0
	div		f0,f1		// f0 <- f0 / f1 (FP divide)
	debug	f0

// One register and literal arithmetic
	move	r0,1
	move	r1,2
	add		r0,r1		// r0 <- r0 + r1
	debug	r0
	sub		r0,r1		// r0 <- r0 - r1
	debug	r0
	move	r0,3
	mult	r0,r1		// r0 <- r0 * r1
	debug	r0
	div		r0,r1		// r0 <- r0 / r1 (integer divide)
	debug	r0

	move	f0,1.234
	add		f0,2		// f0 <- f0 + 2
	debug	f0
	sub		f0,2		// f0 <- f0 - 2
	debug	f0
	move	f0,3.14159
	mult	f0,2		// f0 <- f0 * 2
	debug	f0
	div		f0,2		// f0 <- f0 / 2 (FP divide)
	debug	f0

	stop
	stop
