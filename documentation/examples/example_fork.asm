#include <system/io.asm>
#include <system/system.asm>

MAIN:
	#var	child_pid
	int		iFORK
	move	child_pid, r0
	cmp		child_pid, -1
	jump	eq, @FORK_FAILED
	test	child_pid
	jump	z, @CHILD_FORK
	#call	fprintf(STDOUT,"Child forked: %d\n", r0)
	move	r0, child_pid
	int		iWAIT_PID
	#call	puts("Wait finished!\n")
	jump	@END
CHILD_FORK:
	#call	puts("Child executing...\n")
	#for	r1, 0, lt, 10, 1
		#call	fprintf(STDOUT, "%d...\n", r1)
		#call	sleep(1000)
	#endfor
	#call	puts("Child done!\n")
	jump	@END
FORK_FAILED:
	#call	puts("Fork failed!\n")
END:
	stop
	stop
	
