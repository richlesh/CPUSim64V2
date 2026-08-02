	move	r0,326
	move	r1,r0			// r1 <- r0
	move	f0,3.1415926
	move	f1,f0			// f1 <- f0
	
	move	r2,f1			// r2 <- f1 truncating
	move	f2,r1			// f2 <- r1
	
	debug	r0,r1,r2
	debug	f0,f1,f2

	move	r0,-326
	move	r1,r0
	move	f0,-3.1415926
	move	f1,f0
	
	move	r2,f1
	move	f2,r1
	
	debug	r0,r1,r2
	debug	f0,f1,f2
	stop
	stop
