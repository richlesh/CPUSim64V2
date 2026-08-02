#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>
#include <system/thread.asm>

#macro	DEFINE_MUTEX(MY_MUTEX)
#call	initializeMutex(MY_MUTEX)

	#call	main()
	move	r0, 0
	int		iEXIT

#global CP: .dca 1
#def_func	main()
	#var	i, j, name, pid, cores, pids
	#call	puts("Main is executing...\n")
	#call	puts("Main is locking...\n")
	#sync	MY_MUTEX
		int		iGET_NUM_CORES
		move	cores, r0
		#call	printf("Number of cores: %d\n", cores)
		#call	alloc(cores)
		move	pids, r0
		store	1, CP[0]
		#for	0, i, lt, cores, 1
			move	j, 'A'
			add		j, i
			store	j, CP[1]
			#macro	FROM_CODEPOINTS(CP)
			move	name, r0
			move	r1, runMutex
			move	r2, name
			int		iTHREAD
			store	r0, pids[i]
		#end_for
	
		#call	puts("Main is unlocking...\n")
	#end_sync	
	
	#for	0, i, lt, cores, 1
		load	pid, pids[i]
		#call	printf("Main is joining %d...\n", pid)
		move	r1, pid
		int		iJOIN_THREAD
	#end_for
	#call	puts("Main is done\n")
#end_func

#def_func runMutex(data)
	#var	d, pid
	load	d, data
	int		iGET_PID
	move	pid, r0
	#call	printf("Thread %s executing with PID %d...\n", d, pid)
	#sync	MY_MUTEX
		#call	sleep(100)
		#call	printf("Thread %s finishing...\n", d)
		#call	free(d)
	#end_sync
#end_func
	
	stop
	stop
	