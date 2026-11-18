///////////////////////////////////////////////////////////////////////////////
// OS ROM prototype (Run in Debug Mode)
//
// A DOS-style mini operating system built on vectored interrupts.
//
//   ROM boot   allocates the 256-entry vector table, installs the INT 21h
//              handler and points control register 0 at the table.
//   Loader     saves SP and SF, then calls the user program.
//   INT 21h    r1 = function: 09h prints the string at r2,
//              4Ch terminates the program with exit code r2 and returns
//              control to the loader with the saved SP and SF restored.
//              Any other function returns -1 in r0 and uses IRET.
//
// Expect r9 = 7 (exit code seen by the loader), r8 = -1 (unknown function)
// and the greeting printed once.
//
// Author: Richard Lesh
///////////////////////////////////////////////////////////////////////////////

#include <system/system.def>
#include <system/io.def>

START:
	// ---- ROM boot ----
	move	r1, 260
	int		iALLOC					// 256 vectors + 4 words of OS data
	move	r10, r0
	move	r11, 0
ZERO:	store	r11, r10[r11]		// a fresh block is not cleared
	add		r11, 1
	cmp		r11, 260
	jump	nz, ZERO
	move	r2, DOS21
	store	r2, r10[0x21]
	stcr	0, r10

	// ---- loader ----
	store	sp, r10[256]			// the stack to return to when the program ends
	store	sf, r10[257]
	call	USERPROG
	move	r0, 0					// the program returned instead of terminating
RESUME:	move	r9, r0
	stop
	stop

// ---- the "program" the loader runs ----
USERPROG:
	move	r1, 9
	move	r2, "Hello from the OS ROM\n"
	int		0x21
	move	r1, 0x77				// not a function this OS knows
	int		0x21
	move	r8, r0
	move	r1, 0x4C
	move	r2, 7
	int		0x21
	move	r9, 111					// never reached

// ---- INT 21h dispatcher ----
DOS21:
	cmp		r1, 9
	jump	nz, NOT09
	move	r1, 1					// port 1 = standard output
	int		iPUTS
	iret
NOT09:
	cmp		r1, 0x4C
	jump	nz, UNKNOWN
	move	r0, r2
	load	sp, r10[256]			// abandon the program's stack and the INT frame
	load	sf, r10[257]
	jump	RESUME
UNKNOWN:
	move	r0, -1
	iret
