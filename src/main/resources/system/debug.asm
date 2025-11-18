#include <system/system.def>
#include <system/debug.def>
#include <system/io.asm>
#include <system/thread.asm>

jump	DEBUG_ASM_END

#ifdef __DEBUG__

// debug_msg(port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port.
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	debug_msg(port, fmt, ...)
	save	r0, r1
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	load	r0, port
	move	r1, "DEBUG: "
	int		iPUTS
	int		iPRINTF
	load	r0, port
	int		iPUT_NL
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r0, r1
#end_func

// cond_debug_msg(cond, port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port.
// cond		Must be TRUE to print
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	cond_debug_msg(b, port, fmt, ...)
	save	r0, r1
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	load	r0, b
	jump	z, $SKIP
	load	r0, port
	move	r1, "DEBUG: "
	int		iPUTS
	int		iCOND_PRINTF
	load	r0, port
	int		iPUT_NL
$SKIP:
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r0, r1
#end_func

#global		ENABLE_ASSERT_EXIT: .dci	1

#def_func	assert_failure_exit()
	load	r0, ENABLE_ASSERT_EXIT
	jump	z, $END
	mov		r0, 1
	int		iEXIT
$END:
#end_func

#def_func	assert_true(isTrue, message, filename, line)
	#var	temp
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	load	r0, isTrue
	jump	nz, $ASSERT_IS_TRUE
	#macro	PUTS("Assertion Failed (")
	load	temp, filename
	#macro	PUTS(temp)
	#macro	PUTS(":")
	load	temp, line
	#macro	PUT_DEC(temp, 10)
	#macro	PUTS(") ")
	load	temp, message
	#macro	PUTS(temp)
	#macro	PUT_NL()
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	#call	assert_failure_exit()
$ASSERT_IS_TRUE:
#end_func

#def_func	assert_false(isFalse, message, filename, line)
	#var	temp
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	load	r0, isFalse
	jump	z, $ASSERT_IS_FALSE
	#macro	PUTS("Assertion Failed (")
	load	temp, filename
	#macro	PUTS(temp)
	#macro	PUTS(":")
	load	temp, line
	#macro	PUT_DEC(temp)
	#macro	PUTS(") ")
	load	temp, message
	#macro	PUTS(temp)
	#macro	PUT_NL()
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	#call	assert_failure_exit()
$ASSERT_IS_FALSE:
#end_func

#def_macro	COND_ASSERT_BUILDER(cond, condSymbol)
	#def_func	assert_${cond}(valueA, valueB, message, filename, line)
		#var	a,b,m,temp
		#call	acquireRecursiveSpinLock(STDOUT_LOCK)
		load	a,valueA
		load	b,valueB
		load	m,message
		cmp		a,b
		jump	${cond}, $ASSERT_${cond}_PASSED
		#macro	PUTS("Assertion Failed (")
		load	temp, filename
		#macro	PUTS(temp)
		#macro	PUTS(":")
		load	temp, line
		#macro	PUT_DEC(temp)
		#macro	PUTS(") ")
		#macro	PUTS(m)
		#macro	PUTS(" ")
		#macro	PUT_DEC(a)
		#macro	PUTS(${condSymbol})
		#macro	PUT_DEC(b)
		#macro	PUT_NL()
		#call	releaseRecursiveSpinLock(STDOUT_LOCK)
		#call	assert_failure_exit()
$ASSERT_${cond}_PASSED:
	#end_func
#end_macro

#def_macro	COND_ASSERT_BUILDER_FP(cond, condSymbol)
	#def_func	assert_${cond}_fp(valueA, valueB, message, filename, line)
		#var	m, temp
		#fvar	a, b
		load	a, valueA
		load	b, valueB
		load	m, message
		#call	acquireRecursiveSpinLock(STDOUT_LOCK)
		cmp		a,b
		jump	${cond}, $ASSERT_${cond}_FP_PASSED
		#macro	PUTS("Assertion Failed (")
		load	temp, filename
		#macro	PUTS(temp)
		#macro	PUTS(":")
		load	temp, line
		#macro	PUT_DEC(temp)
		#macro	PUTS(") ")
		#macro	PUTS(m)
		#macro	PUTS(" ")
		#macro	PUT_FP(a)
		#macro	PUTS(${condSymbol})
		#macro	PUT_FP(b)
		#macro	PUT_NL()
		#call	releaseRecursiveSpinLock(STDOUT_LOCK)
		#call	assert_failure_exit()
$ASSERT_${cond}_FP_PASSED:
	#end_func
#end_macro

#macro COND_ASSERT_BUILDER(nz, "≠")
#macro COND_ASSERT_BUILDER(z, "=")
#macro COND_ASSERT_BUILDER(np, "≤")
#macro COND_ASSERT_BUILDER(p, ">")
#macro COND_ASSERT_BUILDER(nn, "≥")
#macro COND_ASSERT_BUILDER(n, "<")
#macro COND_ASSERT_BUILDER_FP(nz, "≠")
#macro COND_ASSERT_BUILDER_FP(z, "=")
#macro COND_ASSERT_BUILDER_FP(np, "≤")
#macro COND_ASSERT_BUILDER_FP(p, ">")
#macro COND_ASSERT_BUILDER_FP(nn, "≥")
#macro COND_ASSERT_BUILDER_FP(n, "<")

#end_if

DEBUG_ASM_END: nop
