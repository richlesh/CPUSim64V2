#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>

///////////////////////////////////////////////////////////////////////////////
// Computes the hailstone sequence
// Finds the longest sequence with starting number less than argument.
// See https://en.wikipedia.org/wiki/Collatz_conjecture
///////////////////////////////////////////////////////////////////////////////
	#call	main()
	int		iEXIT

MY_MUTEX:	dci 0
MAX:		dci 0
IMAX:		dci 0
WORKSIZE:	dci 10000
#def_func	main()
	#var	i, j, argc, imax, max, worksize, cores, pid, pids
	int		iARGC
	move	argc, r0
	cmp		argc, 2
	jump	lt, @GET_ARGS_FAILED
GET_ARGS:
	move	r0, 1
	int		iARGS
	int		iPARSE_INT
	move	worksize, r0
	store	worksize, WORKSIZE
	#call	worker(0)
	move	j, worksize
	
	// Spawn worker threads
	int		iGET_NUM_CORES
	move	cores, r0
	#call	fprintf(STDOUT, "Number of cores: %d\n", cores)
	#call	alloc(cores)
	move	pids, r0
	#for	i, 0, lt, cores, 1
		move	r0, worker
		move	r1, j
		int		iTHREAD
		store	r0, pids[i]
		add		j, worksize
	#endfor
	
	// Join with threads
	#for	i, 0, lt, cores, 1
		load	pid, pids[i]
		#call	fprintf(STDOUT, "Main is joining %d...\n", pid)
		move	r0, pid
		int		iJOIN_THREAD
	#endfor

	load	imax, IMAX
	load	max, MAX
	#call	fprintf(STDOUT,"Max %d found at %d\n", max, imax)
	#return	0
	jump	@MAIN_END
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

PRECOMPUTED: dci	0
PRECOMPUTED_SIZE: dci	30000000
#def_func	compute_hailstone(arg)
	#var	i,i0,isOdd,cache,cacheSize,hailstone
	
	load	cacheSize, PRECOMPUTED_SIZE
	load	cache, PRECOMPUTED
	jmp		nz, @BEGIN_COMPUTE
	move	r0, cacheSize
	int		iALLOC
	move	cache, r0
	store	cache, PRECOMPUTED
	#cond	cache, eq, 0
		#call	fprintf(STDOUT, "Can\'t allocate cache size %d\n", cacheSize)
		move	r0,1
		int		iEXIT
	#endcond
	store	1, cache[0]
	store	1, cache[1]
	store	2, cache[2]
	store	3, cache[4]
BEGIN_COMPUTE:
	load	i, arg
	#cond	i, lt, cacheSize
		load	hailstone, cache[i]
		#cond	hailstone, ne, 0
			#return	hailstone
			jump	@END
		#endcond
	#endcond
	
	move	i0, i
	and	isOdd, i, 0x1
	#cond	isOdd, eq, 0
		div		i, 2
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#elsecond
		mult	i, 3
		add		i, 1
		#call	compute_hailstone(i)
		add		hailstone, r0, 1
	#endcond
	#cond	i0, lt, cacheSize
		store	hailstone, cache[i0]
	#endcond
	#return	hailstone
END:
#end_func

MOD: dci 10000
#def_func worker(data)
	#var	i, d, hs, ws, pid, limit, imax, max, quo, remain, mod
	load	d, data
	move	limit, d
	load	ws, WORKSIZE
	add		limit, ws
	int		iGET_PID
	move	pid, r0
	#call	fprintf(STDOUT, "Thread work unit %d executing with PID %d...\n", d, pid)
	#for	i, d, lt, limit, 1
		#call	compute_hailstone(i)
		move	hs, r0
		load	max, MAX
		#cond	hs, gt, max
			#sync	MY_MUTEX
				load	max, MAX
				#cond	hs, gt, max
					store	hs, MAX
					store	i, IMAX
				#endcond
			#endsync
		#endcond
//		load	mod, MOD
//		div		quo, remain, i, mod
//		#cond	remain, eq, 0
//			#call fprintf(STDOUT,"worker(%d)...%d\n",pid,i)
//		#endcond
	#endfor
	#call	fprintf(STDOUT, "Thread %d finishing...\n", pid)
#end_func

	stop
	stop
