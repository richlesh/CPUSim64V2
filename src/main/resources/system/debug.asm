#include <system/io.def>
#include <system/system.def>

jump	DEBUG_ASM_END

#ifdef __DEBUG
	#defmacro	DEBUG_MSG(...)
		#call	debug_msg(STDOUT, ${...})
	#end_macro

	#defmacro	COND_DEBUG_MSG(b, ...)
		#call	cond_debug_msg(${b}, STDOUT, ${...})
	#end_macro

	#defmacro	SET_EXIT_ON_ASSERT_FAILURE(b)
	#end_macro

	#def_macro	printCPU()
		int		iPrintCPUState
	#end_macro

	#def_macro	assert_true(isTrue, message)
		#call	assert_true(${isTrue}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_false(isFalse, message)
		#call	assert_false(${isFalse}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_eq(valueA, valueB, message)
		#call	assert_z(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ne(valueA, valueB, message)
		#call	assert_nz(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_gt(valueA, valueB, message)
		#call	assert_p(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ge(valueA, valueB, message)
		#call	assert_nn(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_lt(valueA, valueB, message)
		#call	assert_n(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_le(valueA, valueB, message)
		#call	assert_np(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_eq_fp(valueA, valueB, message)
		#call	assert_z_fp(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ne_fp(valueA, valueB, message)
		#call	assert_nz_fp(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_gt_fp(valueA, valueB, message)
		#call	assert_p_fp(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ge_fp(valueA, valueB, message)
		#call	assert_nn_fp(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_lt_fp(valueA, valueB, message)
		#call	assert_n_fp(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_le_fp(valueA, valueB, message)
		#call	assert_np_fp(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro
#else
	#defmacro	DEBUG_MSG(...)
	#end_macro

	#defmacro	COND_DEBUG_MSG(...)
	#end_macro

	#def_macro	printCPU()
	#end_macro

	#def_macro	assert_true(isTrue, message)
	#end_macro

	#def_macro	assert_false(isFalse, message)
	#end_macro

	#def_macro	assert_eq(valueA, ValueB, message)
	#end_macro

	#def_macro	assert_ne(valueA, ValueB, message)
	#end_macro

	#def_macro	assert_gt(valueA, valueB, message)
	#end_macro

	#def_macro	assert_ge(valueA, valueB, message)
	#end_macro

	#def_macro	assert_lt(valueA, valueB, message)
	#end_macro

	#def_macro	assert_le(valueA, valueB, message)
	#end_macro

	#def_macro	assert_eq_fp(valueA, ValueB, message)
	#end_macro

	#def_macro	assert_ne_fp(valueA, ValueB, message)
	#end_macro

	#def_macro	assert_gt_fp(valueA, valueB, message)
	#end_macro

	#def_macro	assert_ge_fp(valueA, valueB, message)
	#end_macro

	#def_macro	assert_lt_fp(valueA, valueB, message)
	#end_macro

	#def_macro	assert_le_fp(valueA, valueB, message)
	#end_macro
#endif

#ifdef __DEBUG

// debug_msg(port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port.
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	debug_msg(port, fmt, ...)
	save	r0, r1
	load	r0, port
	move	r1, "DEBUG: "
	int		iPUTS
	int		iPRINTF
	load	r0, port
	int		iPUT_NL
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
	load	r0, b
	jump	z, $SKIP
	load	r0, port
	move	r1, "DEBUG: "
	int		iPUTS
	int		iCOND_PRINTF
	load	r0, port
	int		iPUT_NL
$SKIP:
	restore	r0, r1
#end_func

ENABLE_ASSERT_EXIT: .dci	1
#defmacro	SET_EXIT_ON_ASSERT_FAILURE(b)
	store	${b}, ENABLE_ASSERT_EXIT
#end_macro

#def_func	assert_failure_exit()
		load	r0, ENABLE_ASSERT_EXIT
		jump	z, $END
		mov		r0, 1
		int		iEXIT
$END:
#end_func

#def_func	assert_true(isTrue, message, filename, line)
		#var	temp
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
		#call	assert_failure_exit()
$ASSERT_IS_TRUE:
#end_func

#def_func	assert_false(isFalse, message, filename, line)
		#var	temp
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
		#call	assert_failure_exit()
$ASSERT_IS_FALSE:
#end_func

#def_macro	COND_ASSERT_BUILDER(cond, condSymbol)
	#def_func	assert_${cond}(valueA, valueB, message, filename, line)
		#var	a,b,m,temp
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
		#call	assert_failure_exit()
$ASSERT_${cond}_PASSED:
	#end_func
#end_macro

#def_macro	COND_ASSERT_BUILDER_FP(cond, condSymbol)
	#def_func	assert_${cond}_fp(valueA, valueB, message, filename, line)
		#var	m,temp
		#fvar	a,b
		load	a,valueA
		load	b,valueB
		load	m,message
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

#endif

DEBUG_ASM_END: nop
