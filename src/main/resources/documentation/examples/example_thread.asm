#include <system/io.asm>
#include <system/system.asm>
	#call	main()
	move	r0, 0
	int		iEXIT

#global	PIDS: .dca	3
#def_func	main()
	#var	pid, i
	move	r1, run
	move	r2, "A"
	int		iTHREAD
	move	pid, r0
	#if_cond	pid, gt, 0
		store	pid, PIDS[1]
	#end_cond
	move	r1, run
	move	r2, "B"
	int		iTHREAD
	move	pid, r0
	#if_cond	pid, gt, 0
		store	pid, PIDS[2]
	#end_cond
	move	r1, run
	move	r2, "C"
	int		iTHREAD
	move	pid, r0
	#if_cond	pid, gt, 0
		store	pid, PIDS[3]
	#end_cond
	#for	1, i, le, 3, 1
		load	pid, PIDS[i]
		#if_cond	pid, gt, 0
			#call	printf("Waiting for %d...\n", pid)
			move	r1, pid
			int		iJOIN_THREAD
		#end_cond
	#end_for
	#call	puts("Finis\n")
#end_func
	
#def_func run(data)
	#var	d, i
	load	d, data
	#call	printf("Thread %s executing...\n", d)
	#for	0, i, lt, 10, 1
		#call	printf("%s %d...\n", d, i)
		#call	sleep(1000)
	#end_for
	#call	printf("Thread %s done!\n", d)
#end_func

	stop
	stop
	