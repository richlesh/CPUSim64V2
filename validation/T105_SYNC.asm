///////////////////////////////////////////////////////////////////////////////
// Preprocessor SYNC Functionality Test
//
// Test the preprocessor SYNC directives
//
// Author: Richard Lesh
// Modified: 2026/06/18
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/thread.asm>

#define NUM_THREADS 20
#define WORK_LOAD 10

#global gVariable: .dci 0
#global PIDS: .dca NUM_THREADS

#call main()
stop

#def_func main()
	#var pid, i
	#for 0, i < NUM_THREADS, 1
		#macro create_thread(worker, 0)
		store r0, PIDS[i]
	#end_for
	#for 0, i < NUM_THREADS, 1
		load r1, PIDS[i]
		int iJOIN_THREAD
	#end_for
	
	load r0, gVariable
	#if_cond r0 == NUM_THREADS * WORK_LOAD
		#call printf("NO RACE CONDITION DETECTED BAD\n")
	#else_cond
		#call printf("RACE CONDITION DETECTED OK\n")
	#end_cond
	
	#call initializeMutex(my_mutex)
	store	0, gVariable
	
	#for 0, i < NUM_THREADS, 1
		#macro create_thread(worker_sync, 0)
		store r0, PIDS[i]
	#end_for
	#for 0, i < NUM_THREADS, 1
		load r1, PIDS[i]
		int iJOIN_THREAD
	#end_for
	
	load r0, gVariable
	#if_cond r0 == NUM_THREADS * WORK_LOAD
		#call printf("NO RACE CONDITION DETECTED OK\n")
	#else_cond
		#call printf("RACE CONDITION DETECTED BAD\n")
	#end_cond

#end_func

#def_func worker(data)
	#var i, val
	#for 0, i < WORK_LOAD, 1
		load val, gVariable
		add val, 1
		store val, gVariable
	#end_for
#end_func

#def_func worker_sync(data)
	#var i, val
	#for 0, i < WORK_LOAD, 1
		#SYNC my_mutex
			load val, gVariable
			add val, 1
			store val, gVariable
		#END_SYNC
	#end_for
#end_func

#macro DEFINE_MUTEX(my_mutex)
