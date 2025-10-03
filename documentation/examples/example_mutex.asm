#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>
	#call	main()
	int		iEXIT

MY_MUTEX: dci 0
#def_func	main()
	#var	i, j, name, pid, cores, pids
	#call	puts("Main is executing...\n")
	#call	puts("Main is locking...\n")
	#sync	MY_MUTEX
		int		iGET_NUM_CORES
		move	cores, r0
		#call	fprintf(STDOUT, "Number of cores: %d\n", cores)
		#call	alloc(cores)
		move	pids, r0
		#for	i, 0, lt, cores, 1
			#call	strcopy("A")
			move	name, r0
			move	j, 'A'
			add		j, i
			#call	setCharAt(name, 0, j)
			move	r0, runMutex
			move	r1, name
			int		iTHREAD
			store	r0, pids[i]
		#endfor
	
		#call	puts("Main is unlocking...\n")
	#endsync	MY_MUTEX	
	
	#for	i, 0, lt, cores, 1
		load	pid, pids[i]
		#call	fprintf(STDOUT, "Main is joining %d...\n", pid)
		move	r0, pid
		int		iJOIN_THREAD
	#endfor
	#call	puts("Main is done\n")
#end_func

#def_func runMutex(data)
	#var	d, pid
	load	d, data
	int		iGET_PID
	move	pid, r0
	#call	fprintf(STDOUT, "Thread %s executing with PID %d...\n", d, pid)
	#sync	MY_MUTEX
		#call	sleep(100)
		#call	fprintf(STDOUT, "Thread %s finishing...\n", d)
		#call	free(d)
	#endsync	MY_MUTEX
#end_func
	
	stop
	stop
	