#include <system/io.asm>
#include <system/system.asm>
	#call	main()
	int		iEXIT

PIDS: dca	3
#def_func	main()
	#var	pid, i
	#call	spawnChild("A")
	move	pid, r0
	#cond	pid, gt, 0
		store	pid, PIDS[0]
		#call	spawnChild("B")
		move	pid, r0
		#cond	pid, gt, 0
			store	pid, PIDS[1]
			#call	spawnChild("C")
			move	pid, r0
			#cond	pid, gt, 0
				store	pid, PIDS[2]
			#endcond
		#endcond
	#endcond
	#for	i, 0, lt, 3, 1
		load	pid, PIDS[i]
		#cond	pid, gt, 0
			#call	fprintf(STDOUT, "Waiting for %d...\n", pid)
			move	r0, pid
			int		iWAIT_PID
		#endcond
	#endfor
#end_func
	
#def_func spawnChild(name)
	#var	child_pid, childName
	load	childName, name
	int		iFORK
	move	child_pid, r0
	cmp		child_pid, -1
	jump	eq, @FORK_FAILED
	test	child_pid
	jump	z, @CHILD_FORK
	#call	fprintf(STDOUT,"Child %s forked: %d\n", childName, r0)
	#return	child_pid
	jump	@END
CHILD_FORK:
	#call	fprintf(STDOUT,"Child %s executing...\n", childName)
	#for	r1, 0, lt, 10, 1
		#call	fprintf(STDOUT, "%s %d...\n", childName, r1)
		#call	sleep(1000)
	#endfor
	#call	fprintf(STDOUT, "Child %s done!\n", childName)
	stop
FORK_FAILED:
	#call	fprintf(STDOUT, "Fork %s failed!\n", childName)
	#return	-1
END:
#end_func

	stop
	stop
	