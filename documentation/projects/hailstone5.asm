///////////////////////////////////////////////////////////////////////////////
// Hailstone4.asm
//
// Finds the longest Hailstone sequence with starting number less than or 
// equal to the argument.
// See https://en.wikipedia.org/wiki/Collatz_conjecture
//
// Author: Richard Lesh
// Original: 2009/03/20
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>
#include <system/thread.asm>
#include <adt/vector.asm>

	#call	main()
	int		iEXIT

#macro DEFINE_MUTEX(MY_MUTEX)
#global MAX:		.dci 0
#undef iMAX
#global IMAX:		.dci 0
#global WORKSIZE:	.dci 100000
#global WORKQUEUE:	.dci 0

#def_func	main()
	#var	i, j, argc, arg, mImax, mMax, mWorksize, limit, queue, \
			workunits, cores, pid, pids
	int		iARGC
	move	argc, r0
	cmp		argc, 2
	jump	lt, GET_ARGS_FAILED
GET_ARGS:
	#call	args(1)
	move	arg, r0
	#macro	PARSE_INT(arg)
	move	limit, r0
	load	mWorksize, WORKSIZE
	#call	initializeMutex(MY_MUTEX)

	#call	newVector(10)
	move	queue, r0
	store	queue, WORKQUEUE
	
	#macro	PUSH_BACK(queue, 0)
	#call	worker(queue)

	div		workunits, limit, mWorksize
	sub		workunits, 1
	mult	workunits, mWorksize
	neg		mWorksize
	#for	workunits, i > 0, mWorksize
		#macro	PUSH_BACK(queue, i)
	#end_for
	
	// Spawn worker threads
	int		iGET_NUM_CORES
	move	cores, r0
	#call	printf("Number of cores: %d\n", cores)
	#call	alloc(cores)
	move	pids, r0
	#if_cond	pids, eq, 0
		#call	printf("Can\'t allocate pids array!\n")
		#call	exit(1)
	#end_cond
	#for	0, i < cores, 1
		#macro	create_thread(worker, queue)
		store	r0, pids[i]
	#end_for
	
	// Join with threads
	#for	0, i < cores, 1
		load	pid, pids[i]
		#call	printf("Main is joining %d...\n", pid)
		#macro	join_thread(pid)
	#end_for

	load	mImax, IMAX
	load	mMax, MAX
	#call	printf("Max %d found at %d\n", mMax, mImax)
	#return	0
GET_ARGS_FAILED:
	#call	puts("You must supply a positive integer argument.")
	#return	1
MAIN_END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_hailstone(arg)
// Computes the number of integers in the hailstone sequence starting
// with the argument.  The hailstone sequence conputes the next value
// in the sequence according to the formula...
// f(n) = f(n-1)/2 if f(n-1) is even and 3*f(n-1)+1 if odd.
// The sequence ends when the computed value reaches 1.  This can be
// computed recursively.
// Use memoization to dramatically improve performance.
///////////////////////////////////////////////////////////////////////////////

#global PRECOMPUTED: .dci	0
PRECOMPUTED_SIZE: .dci	500000
#def_func	compute_hailstone(arg)
	#var	i,i0,isOdd,cache,cacheSize,hailstone
	
	load	cacheSize, PRECOMPUTED_SIZE
	load	cache, PRECOMPUTED
	jump	nz, BEGIN_COMPUTE
	#call	ALLOC(cacheSize)
	move	cache, r0
	store	cache, PRECOMPUTED
	#if_cond	cache, eq, 0
		#call	printf("Can\'t allocate cache size %d\n", cacheSize)
		#call	exit(1)
	#end_cond
	#call	MEMCLEAR(cache, cacheSize)
	store	1, cache[0]
	store	1, cache[1]
	store	2, cache[2]
	store	3, cache[4]
BEGIN_COMPUTE:
	load	i, arg
	#if_cond	i, lt, cacheSize
		load	hailstone, cache[i]
		#if_cond	hailstone, ne, 0
			#return	hailstone
		#end_cond
	#end_cond
	
	move	i0, i
	and	isOdd, i, 0x1
	#if_cond	isOdd, eq, 0
		div		i, 2
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#else_cond
		mult	i, 3
		add		i, 1
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#end_cond
	#if_cond	i0, lt, cacheSize
		store	hailstone, cache[i0]
	#end_cond
	#return	hailstone
END:
#end_func

#def_func worker(queue)
	#var	i, d, hs, ws, pid, limit, wImax, wMax, quo, remain, q
	int	iGET_PID
	move	pid, r0
	load	q, queue
	#sync	q[_VECTOR_MUTEX]
		#call	vectorIsEmpty(q)
		#if_cond	r0, eq, 0
			#macro	POP_BACK(q)
			move	d, r0
		#else_cond
			move	d, -1
		#end_cond
	#end_sync
	#while	d, ne, -1
		move	limit, d
		load	ws, WORKSIZE
		add	limit, ws
		#call	printf("Thread work unit %d executing with PID %d...\n", d, pid)
		clear	wMax
		#for	d, i < limit, 1
			#call	compute_hailstone(i)
			move	hs, r0
			#if_cond	hs > wMax
				move	wMax, hs
				move	wImax, i
			#end_cond
		#end_for
		#sync	MY_MUTEX
			load	hs, MAX
			load	i, IMAX
			// if wMax > hs || (wMax == hs && wImax < i)
			cmp		wMax, hs
			move	gt, r0, -1, 0
			cmp 	wMax, hs
			move	eq, r1, -1, 0
			cmp		wImax, i
			move	lt, r2, -1, 0
			and		r1, r2
			or		r0, r1
			#if_cond_sr	nz
				#call	printf("New high: %d:%d (%d)\n", wImax, wMax, pid)
				store	wMax, MAX
				store	wImax, IMAX
			#end_cond
		#end_sync

		#sync	q[_VECTOR_MUTEX]
			#call	vectorIsEmpty(q)
			#if_cond	r0, eq, 0
				#macro	POP_BACK(q)
				move	d, r0
			#else_cond
				move	d, -1
			#end_cond
		#end_sync
	#end_while
	#call	printf("Thread %d finishing...\n", pid)
#end_func

	stop
	stop
