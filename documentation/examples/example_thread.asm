#include <system/io.asm>
#include <system/system.asm>
	#call	main()
	int		iEXIT

PIDS: dca	3
#def_func	main()
	#var	pid, i
	move	r0, run
	move	r1, "A"
	int		iTHREAD
	move	pid, r0
	#if_cond	pid, gt, 0
		store	pid, PIDS[0]
	#end_cond
	move	r0, run
	move	r1, "B"
	int		iTHREAD
	move	pid, r0
	#if_cond	pid, gt, 0
		store	pid, PIDS[1]
	#end_cond
	move	r0, run
	move	r1, "C"
	int		iTHREAD
	move	pid, r0
	#if_cond	pid, gt, 0
		store	pid, PIDS[2]
	#end_cond
	#for	i, 0, lt, 3, 1
		load	pid, PIDS[i]
		#if_cond	pid, gt, 0
			#call	fprintf(STDOUT, "Waiting for %d...\n", pid)
			move	r0, pid
			int		iJOIN_THREAD
		#end_cond
	#end_for
	#call	puts("Finis\n")
#end_func
	
#def_func run(data)
	#var	d
	load	d, data
	#call	fprintf(STDOUT, "Thread %s executing...\n", d)
	#for	r1, 0, lt, 10, 1
		#call	fprintf(STDOUT, "%s %d...\n", d, r1)
		#call	sleep(1000)
	#end_for
	#call	fprintf(STDOUT, "Thread %s done!\n", d)
#end_func

	stop
	stop
	