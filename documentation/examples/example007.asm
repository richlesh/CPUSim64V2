// Three register arithmetic
	move	r1,1
	move	r2,2
	add		r0,r1,r2	// r0 <- r1 + r2
	debug	r0
	sub		r0,r1,r2	// r0 <- r1 - r2
	debug	r0
	move	r1,3
	mult	r0,r1,r2	// r0 <- r1 * r2
	debug	r0
	div		r0,r1,r2	// r0 <- r1 / r2 (integer divide)
	debug	r0

	move	f1,1.234
	move	f2,2.345
	add		f0,f1,f2	// f0 <- f1 + f2
	debug	f0
	sub		f0,f1,f2	// f0 <- f1 - f2
	debug	f0
	move	f1,3.14159
	mult	f0,f1,f2	// f0 <- f1 * f2
	debug	f0
	div		f0,f1,f2	// f0 <- f1 / f2 (FP divide)
	debug	f0

// Two register and literal arithmetic
	move	r1,1
	add		r0,r1,2		// r0 <- r1 + 2
	debug	r0
	sub		r0,r1,2		// r0 <- r1 - 2
	debug	r0
	move	r1,3
	mult	r0,r1,2		// r0 <- r1 * 2
	debug	r0
	div		r0,r1,2		// r0 <- r1 / 2 (integer divide)
	debug	r0

	move	f1,1.234
	add		f0,f1,2		// f0 <- f1 + 2
	debug	f0
	sub		f0,f1,2		// f0 <- f1 - 2
	debug	f0
	move	f1,3.14159
	mult	f0,f1,2		// f0 <- f1 * 2
	debug	f0
	div		f0,f1,2		// f0 <- f1 / 2 (FP divide)
	debug	f0

	stop
	stop
