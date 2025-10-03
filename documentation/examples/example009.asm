// Negation
	move	r0, 7
	move	r1, -3
	neg		r0			// r0 <- -r0
	neg		r1			// r1 <- -r1
	debug	r0, r1

// Reciprocal
	move	f0, 7
	move	f1, 3.14159
	recip	f0			// f0 <- 1 / f0 (FP division)
	recip	f1			// f1 <- 1 / f1 (FP division)
	debug	f0, f1

	stop
	stop
