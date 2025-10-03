// Loop from 0 to 9
	move	r0, 0		// Start the loop variable with 0
	jump	@END1
START1:
	debug	r0
	add		r0,1
END1:
	cmp		r0,10		// Compare to 10 to see if we should stop
	jump	ne, @START1	// Loop again if loop variable was not 10
	
// Try to find value that when squared is 25
	move	r0, 10		// Check 0 - 9
	jump	@END2
START2:
	sub		r0,1
	mult	r1,r0,r0	// Compute the square
	cmp		r1,25
	jump	eq,@FOUND2	// Break out of loop if we found it
END2:
	test	r0			// Test loop variable to see it we stop at 0
	jump	nz, @START2	// Loop again if loop variable was not 10
	jump	@FINIS		// Didn't find it so don't print anything
FOUND2:
	debug	r0

FINIS:	
	stop
	stop
