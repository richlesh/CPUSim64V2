			#include <system/system.def>
			#include <system/io.def>
			#include <system/thread.asm>

			call	main
			move	r1, 0
			int		iEXIT

			#def_macro put_dec2(pid, val)
				push ${val}
				push ${pid}
				#call	acquireRecursiveSpinLock(SPINLOCK)
				move R3, 4
				pop R2
				move R1, STDOUT
				int iPUT_DEC
				move R2, ":"
				move R1, STDOUT
				int iPUTS
				move R3, 4
				pop R2
				move R1, STDOUT
				int iPUT_DEC
				move R1, STDOUT
				int	iPUT_NL
				#call	releaseRecursiveSpinLock(SPINLOCK)
			#end_macro

			#global	PIDS: .dca	3
			#def_func	MAIN()
				#CALL	initializeRecursiveSpinLock(SPINLOCK)
				#var	pid, i
				#macro	create_thread(run, 1)
				store	r0, PIDS[1]
				#macro	put_dec2(0, r0)
				#macro	create_thread(run, 2)
				store	r0, PIDS[2]
				#macro	put_dec2(0, r0)
				#macro	create_thread(run, 3)
				store	r0, PIDS[3]
				#macro	put_dec2(0, r0)
				load	r1, PIDS[1]
				int		iJOIN_THREAD
				load	r1, PIDS[2]
				int		iJOIN_THREAD
				load	r1, PIDS[3]
				int		iJOIN_THREAD
			#end_func

			#macro DEFINE_RECURSIVE_SPINLOCK(SPINLOCK)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i, start, stop
				load	d, data
				int	iGET_PID
				debug	r0, d
				#macro	put_dec2(r0, d)
				#if_cond	d == 0
					#return 0
				#end_cond
				mult	start, 100, d
				add		stop, 100, start
				#for	start, i <= stop, 1
					#if_cond	i == start
						sub		d, 1
						#call	run(d)
					#end_cond
					load	c, COUNTER
					add		c, 1
					store	c, COUNTER
				#end_for
			#end_func
				stop
				stop

