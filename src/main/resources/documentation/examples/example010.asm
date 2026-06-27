#include <system/system.def>

// Conditional 
// IF/ELSE constructs make use of the conditional and unconditional JUMP
// instructions.
	move	r1, 5
STARTIF1:
	jump	n, ELSE1	// Jump to the else part if r1 < 0
	move	r0, TRUE	// TRUE is -1
	jump	ENDIF1
ELSE1:
	move	r0, FALSE	// FALSE is 0
ENDIF1:
	debug	r0			// r0 will be TRUE if r1 >= 0 and FALSE if r1 < 0

	move	r1, -5
STARTIF2:
	jump	n, ELSE2	// Jump to the else part if r1 < 0
	move	r0, TRUE	// TRUE is -1
	jump	ENDIF2
ELSE2:
	move	r0, FALSE	// FALSE is 0
ENDIF2:
	debug	r0			// r0 will be TRUE if r1 >= 0 and FALSE if r1 < 0

// WHILE Loop
// Loop body between START1 and END1 excutes zero or more times
// Condition is effectively checked at the beginning of each loop body.
	move	r0, 10		// Loop variable in r0
LOOP1:
	jump	END1		// unconditional jump to the test at end of loop
START1:
	debug	r0
	sub		r0, 1		// Decrement the loop variable
END1:
	jump	nz, START1	// Jump to the beginning of the loop if not zero

// DO/WHILE Loop
// Loop body between START2 and END2 always excutes at least once.
// Condition is checked at the end of each loop body.
	move	r0, 10		// Loop variable in r0
START2:
	debug	r0
	sub		r0, 1		// Decrement the loop variable
END2:
	jump	nz, START2	// Jump to the beginning of the loop if not zero

	stop
	stop
