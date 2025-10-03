#include <system/system.def>

	jump	@SYSTEM_ASM_END

///////////////////////////////////////////////////////////////////////////////
// intPrintArray(a, size)
// Prints an array of integers.
// a	Base address of the array to print
// size	Number of elements to print
///////////////////////////////////////////////////////////////////////////////
#def_func intPrintArray(a, size)
	#var	base, i, len, v
	load	base, a
	load	len, size
	clear	i
	jump	@LOOP1_END
LOOP1:
	load	v, base[i]
	#call	put_dec(i)
	#call	putc(':')
	#call	put_dec(v)
	#call	put_nl()
	add		i, 1
LOOP1_END:
	cmp		i, len
	jump	ne, @LOOP1
#end_func

///////////////////////////////////////////////////////////////////////////////
// fpPrintArray(a, size)
// Prints an array of FP.
// a	Base address of the array to print
// size	Number of elements to print
///////////////////////////////////////////////////////////////////////////////
#def_func fpPrintArray(a, size)
	#var	base, i, len
	#fvar	v
	load	base, a
	load	len, size
	clear	i
	jump	@LOOP1_END
LOOP1:
	load	v, base[i]
	#call	put_dec(i)
	#call	putc(':')
	#call	put_fp(v)
	#call	put_nl()
	add		i, 1
LOOP1_END:
	cmp		i, len
	jump	ne, @LOOP1
#end_func

///////////////////////////////////////////////////////////////////////////////
// memmove(dest, src, count)
// Moves data from src address to dest address.  Handles overlapping memory.
// dest	Base address of the destination array
// src	Base address of the source array
// count	Number of elements to move
///////////////////////////////////////////////////////////////////////////////
#def_func memmove(dest, src, count)
	push	r1
	push	r2
	load	r0, dest
	load	r1, src
	load	r2,	count
	int		iMEMMOVE
	pop		r2
	pop		r1
#end_func

///////////////////////////////////////////////////////////////////////////////
// memclear(dest, count)
// Sets count words of memory starting with dest to 0.
// dest	Base address of the destination array
// count	Number of elements to clear
///////////////////////////////////////////////////////////////////////////////
#def_func memclear(dest, count)
	push	r1
	load	r0, dest
	load	r1, count
	int		iMEMCLEAR
	pop		r1
#end_func

///////////////////////////////////////////////////////////////////////////////
// Sleep(delay)
// Pauses the current thread for delay milliseconds
///////////////////////////////////////////////////////////////////////////////
#def_func sleep(delay)
	load	r0, delay
	int		iSLEEP
#end_func

#def_func exit(code)
	load	r0, code
	int		iEXIT
#end_func

#def_func system(cmd)
	load	r0, cmd
	int		iSYSTEM
#end_func

#def_func alloc(size)
	load	r0, size
	int		iALLOC
#end_func

#def_func realloc(addr, size)
	push	r1
	load	r0, addr
	load	r1, size
	int		iREALLOC
	pop		r1
#end_func

#def_func free(addr)
	load	r0, addr
	int		iFREE
#end_func

#def_func free2(addr, offset)
	#var	a, o
	load	a, addr
	load	o, offset
	move	r0, a, o
	int		iFREE
#end_func

#def_func args(index)
	load	r0, index
	int		iARGS
#end_func


///////////////////////////////////////////////////////////////////////////////
// get_and_increment(addr)
// Atomically increments the value at addr.  Returns the old value.
///////////////////////////////////////////////////////////////////////////////
#def_func get_and_increment(int addr)
	#var	oldValue, newValue
	#load_args
_TRY_GET_AND_INCREMENT:
		load	oldValue, addr
		add		newValue, oldValue, 1
		cas		oldValue, newValue, addr
		jump	z, @_TRY_GET_AND_INCREMENT
	#return	oldValue
#end_func

_MUTEX_EXP_WAIT_FACTOR: dcf 1.2
_MUTEX_MAX_EXP_WAIT: dcf 500.

///////////////////////////////////////////////////////////////////////////////
// mutex_lock2(addr, offset, timeout)
// Pauses the current thread until the mutex at addr[offset] can be acquired or
// until timeout is reached.  Timeout is in milliseconds. Use -1 for no timeout
// Returns TRUE if successful.
///////////////////////////////////////////////////////////////////////////////
#def_func mutex_lock2(addr, offset, timeout)
	#var	a, o, t
	load	a, addr
	load	o, offset
	load	t, timeout
//#call debug(STDOUT,"Acquiring mutex2: %x %d %d\n", a, o, t)
	add		a, o
	#call	mutex_lock(a, t)
#end_func

///////////////////////////////////////////////////////////////////////////////
// mutex_lock(addr, timeout)
// Pauses the current thread until the mutex at addr can be acquired or until
// timeout is reached.  Timeout is in milliseconds. Use -1 for no timeout.
// Returns TRUE if successful.
///////////////////////////////////////////////////////////////////////////////
#def_func mutex_lock(addr, timeout)
	#var	a, newValue, oldValue, pid, oldPID, sleepDuration, start, duration, endDuration
	#fvar	mutex_exp_wait_factor, mutex_max_exp_wait
	load	a, addr
	int		iGET_PID
	move	pid, r0
	load	mutex_exp_wait_factor, _MUTEX_EXP_WAIT_FACTOR
	load	mutex_max_exp_wait, _MUTEX_MAX_EXP_WAIT
	
	load	endDuration, timeout
//#call debug(STDOUT,"Acquiring mutex: %x %d\n", a, endDuration)
	#cond	endDuration, lt, 0
		int		iINT_MAX
		move	endDuration, r0
	#elsecond
		mult	endDuration, 1000000
	#endcond
	move	sleepDuration, 10
	int		iCLOCK
	move	start, r0
	move	duration, 0
	#while	duration, le, endDuration
		load	oldPID, a
		unpack	oldPID, oldValue
//#call debug(STDOUT,"Old value: %x %x\n", oldPID, oldValue)
		#cond	oldPID, eq, 0
			pack	oldPID, oldValue
			move	newValue, pid
			move	r0, 1
			pack	newValue, r0
//#call debug(STDOUT,"Next value: %x\n", newValue)
			cas		oldPID, newValue, a
			#cond_sr	nz
				#break
			#endcond
		#elseifcond	oldPID, eq, pid
			pack	oldPID, oldValue
			move	newValue, pid
			add		oldValue, 1
			pack	newValue, oldValue
//#call debug(STDOUT,"Same thread next value: %x\n", newValue)
			cas		oldPID, newValue, a
			#cond_sr	nz
				#break
			#endcond
		#endcond
		#endcond
//#call debug(STDOUT, "Sleeping: %d...\n", sleepDuration)
		#call	sleep(sleepDuration)
		move	f0, sleepDuration
		mult	f0, mutex_exp_wait_factor
		#cond	f0, gt, mutex_max_exp_wait
			move	f0, mutex_max_exp_wait
		#endcond
		move	sleepDuration, f0
		int	iCLOCK
		sub	duration, r0, start
	#endwhile
	#cond	duration, le, endDuration
//#call puts("Lock succeeded\n")
		#return	TRUE
	#elsecond
//#call puts("Lock failed\n")
		#return	FALSE
	#endcond
END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// mutex_unlock2(addr, offset)
// Unlocks the mutex at addr[offset].
// Returns TRUE if successful.
///////////////////////////////////////////////////////////////////////////////
#def_func mutex_unlock2(addr, offset)
	#var	a, o
	load	a, addr
	load	o, offset
	add		a, o
	#call	mutex_unlock(a)
#end_func

///////////////////////////////////////////////////////////////////////////////
// mutex_unlock(addr)
// Unlocks the mutex at addr.
// Returns TRUE if successful.
///////////////////////////////////////////////////////////////////////////////
#def_func mutex_unlock(addr)
	#var	a, v, ov, newValue, oldValue, pid, oldPID
	load	a, addr
	int		iGET_PID
	move	pid, r0
	load	ov, a
	move	oldPID, ov
	unpack	oldPID, oldValue
	#cond	oldPID, eq, pid
		sub	newValue, oldValue, 1
		#cond_sr	nz
			move	v, oldPID
			pack	v, newValue
		#elsecond
			move	v, 0
		#endcond
		cas		ov, v, a
		#cond_sr	nz
//#call puts("Unlock succeeded\n")
			#return	TRUE
			jump	@END
		#endcond
	#endcond
//#call puts("Unlock failed\n")
	#return	0
END:
#end_func

_fibonacci: DCA 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, \
				4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393
#def_func _fibonacciAlloc(x0)
	#var	x, i, f, fib, fiblen
	load	x, x0
	load	fiblen, _fibonacci[-1]
	#for	i, 0, lt, fiblen, 1
		load	f, _fibonacci[i]
		#cond	f, ge, x
			#break
		#endcond
	#endfor
	COMPARE(i, eq, fiblen)
	move	nz, r0, x, f
#end_func

// Computes the minimum size of a heap allocated block
// that will be returned by iALLOC for the given size.
#def_func miniumAllocSize(size)
	#var	s
	load	s, size
	add		s, 3
	#call	_fibonacciAlloc(s)
	sub		r0, 3
#end_func
SYSTEM_ASM_END: nop
