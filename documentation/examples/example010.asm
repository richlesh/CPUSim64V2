// WHILE CONDITION/WEND Loop
// Loop body between START2 and END2 always excutes zero or more times
// Condition is effectively checked at the beginning of each loop body.
	move	r0, 10		// Loop variable in r0
	jump	@END1		// unconditional jump to the test at end of loop
START1:
	debug	r0
	sub		r0, 1
END1:
	jump	nz, @START1	// Jump to the beginning of the loop if not zero

// DO/WHILE CONDITION Loop
// Loop body between START2 and END2 always excutes at least once
// Condition is checked at the end of each loop body.
	move	r0, 10		// Loop variable in r0
START2:
	debug	r0
	sub		r0, 1
END2:
	jump	nz, @START2	// Jump to the beginning of the loop if not zero

	stop
	stop
