#include <system/system.asm>
#include <system/io.asm>

MAIN:
	#var	i,child_pid
	int		iFORK
	move	child_pid, r0
	cmp		child_pid, -1
	jump	eq, FORK_FAILED
	test	child_pid
	jump	z, CHILD_FORK
	#call	printf("Child forked: %d\n", child_pid)
	#call	sleep(12000)
	move	r1, child_pid
	int		iWAIT_PID
	#call	puts("Wait finished!\n")
	jump	END
CHILD_FORK:
	#call	puts("Child executing...\n")
	#for	0, i, lt, 10, 1
		#call	printf("%d...\n", i)
		#call	sleep(1000)
	#end_for
	#call	puts("Child done!\n")
	jump	END
FORK_FAILED:
	#call	puts("Fork failed!\n")
END:
	stop
	stop
