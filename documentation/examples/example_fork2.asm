#include <system/io.asm>
#include <system/system.asm>

	#call	main()
	move	r1, 0
	int		iEXIT

#global PIDS: .dca	3
#def_func	main()
	#var	pid, i
	#call	spawnChild("A")
	move	pid, r0
	#if_cond	pid, gt, 0
		store	pid, PIDS[1]
		#call	spawnChild("B")
		move	pid, r0
		#if_cond	pid, gt, 0
			store	pid, PIDS[2]
			#call	spawnChild("C")
			move	pid, r0
			#if_cond	pid, gt, 0
				store	pid, PIDS[3]
			#end_cond
		#end_cond
	#end_cond
	#for	1, i, le, 3, 1
		load	pid, PIDS[i]
		#if_cond	pid, gt, 0
			#call	printf("Waiting for %d...\n", pid)
			move	r1, pid
			int		iWAIT_PID
		#end_cond
	#end_for
#end_func
	
#def_func spawnChild(name)
	#var	child_pid, childName
	load	childName, name
	int		iFORK
	move	child_pid, r0
	cmp		child_pid, -1
	jump	eq, $FORK_FAILED
	test	child_pid
	jump	z, $CHILD_FORK
	#call	printf("Child %s forked: %d\n", childName, child_pid)
	#return	child_pid
	jump	$END
$CHILD_FORK:
	#call	printf("Child %s executing...\n", childName)
	#for	0, r1, lt, 10, 1
		#call	printf("%s %d...\n", childName, r1)
		#call	sleep(1000)
	#end_for
	#call	printf("Child %s done!\n", childName)
	stop
$FORK_FAILED:
	#call	printf("Fork %s failed!\n", childName)
	#return	-1
$END:
#end_func

	stop
	stop
	