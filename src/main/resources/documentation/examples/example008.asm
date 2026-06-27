// Four register division
	move	r2, 7
	move	r3, 3
	div		r0,r1,r2,r3		// r0 <- r2 / r3; r1 <- r2 % r3
	debug	r0,r1
	move	r2, 27
	move	r3, 5
	div		r0,r1,r2,r3		// r0 <- r2 / r3; r1 <- r2 % r3
	debug	r0,r1

	stop
	stop
