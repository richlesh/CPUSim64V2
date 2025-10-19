#include <system/io.def>
#include <system/system.def>

jump	DEBUG_ASM_END
ENABLE_ASSERT_EXIT: .dci	1

ASSERTION_FAILED: 	.dcs	"Assertion Failed (Filename: "
ASSERTION_LINE:		.dcs	", Line: "
ASSERTION_MESSAGE:	.dcs	"): "

#ifdef __DEBUG
	#defmacro	DEBUG(message, ...)
		#call	debug(STDOUT, ${message}, ${...})
	#end_macro
#else
	#defmacro	DEBUG(message, ...)
	#end_macro
#endif

// debug(port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port.
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#ifdef __DEBUG
#def_func	debug(port, fmt, ...)
	load	r0, port
	#call	fputs(r0, "\nDEBUG: ")
	int		iPRINTF
#end_func

// cond_debug(cond, port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port.
// cond		Must be TRUE to print
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	cond_debug(b, port, fmt, ...)
#ifdef __DEBUG
	load	r0, b
	#cond_sr	nz
		load	r0, port
		#call	fputs(r0, "\nDEBUG: ")
		int		iCOND_PRINTF
	#end_cond_sr
#endif
#end_func

#def_func	assert_exit()
		load	r0, ENABLE_ASSERT_EXIT
		jump	z, @END
		mov		r0, 1
		int		iEXIT
END:
#end_func

#def_func	assert_true_func(isTrue, message, filename, line)
		#var	temp
		ld		r0, isTrue
		jmp		nz, @ASSERT_IS_TRUE
		#call	puts(ASSERTION_FAILED)
		ld		temp, filename
		#call	puts(temp)
		#call	puts(ASSERTION_LINE)
		ld		temp, line
		#call	put_int(temp,10)
		#call	puts(ASSERTION_MESSAGE)
		ld		temp, message
		#call	puts(temp)
		#call	put_nl()
		#call	assert_exit()
ASSERT_IS_TRUE:
#end_func

#def_func	assert_false_func(isFalse, message, filename, line)
		#var	temp
		ld		r0, isFalse
		jmp		z, @ASSERT_IS_FALSE
		#call	puts(ASSERTION_FAILED)
		ld		temp, filename
		#call	puts(temp)
		#call	puts(ASSERTION_LINE)
		ld		temp, line
		#call	put_int(temp,10)
		#call	puts(ASSERTION_MESSAGE)
		ld		temp, message
		#call	puts(temp)
		#call	put_nl()
		#call	assert_exit()
ASSERT_IS_FALSE:
#end_func

#def_macro	assert_macro(cond, condSymbol)
	#def_func	assert_${cond}_func(valueA, valueB, message, filename, line)
		#var	a,b,m,temp
		load	a,valueA
		load	b,valueB
		load	m,message
		cmp		a,b
		jmp		${cond}, @ASSERT_${cond}_PASSED
		#call	puts(ASSERTION_FAILED)
		ld		temp, filename
		#call	puts(temp)
		#call	puts(ASSERTION_LINE)
		ld		temp, line
		#call	put_int(temp,10)
		#call	puts(ASSERTION_MESSAGE)
		#call	puts(m)
		#call	putc(' ')
		#call	put_int(a,10)
		#call	putc(${condSymbol})
		#call	put_int(b,10)
		#call	put_nl()
		#call	assert_exit()
ASSERT_${cond}_PASSED:
	#end_func
#end_macro

#def_macro	assert_fp_macro(cond, condSymbol)
	#def_func	assert_${cond}_fp_func(valueA, valueB, message, filename, line)
		#var	m,temp
		#fvar	a,b
		load	a,valueA
		load	b,valueB
		load	m,message
		cmp		a,b
		jmp		${cond}, @ASSERT_${cond}_FP_PASSED
		#call	puts(ASSERTION_FAILED)
		ld		temp, filename
		#call	puts(temp)
		#call	puts(ASSERTION_LINE)
		ld		temp, line
		#call	put_int(temp,10)
		#call	puts(ASSERTION_MESSAGE)
		#call	puts(m)
		#call	putc(' ')
		#call	put_fp(a)
		#call	putc(${condSymbol})
		#call	put_fp(b)
		#call	put_nl()
		#call	assert_exit()
ASSERT_${cond}_FP_PASSED:
	#end_func
#end_macro

assert_macro(z, '≠')
assert_macro(nz, '=')
assert_macro(p, '≤')
assert_macro(np, '>')
assert_macro(n, '≥')
assert_macro(nn, '<')
assert_fp_macro(z, '≠')
assert_fp_macro(nz, '=')
assert_fp_macro(p, '≤')
assert_fp_macro(np, '>')
assert_fp_macro(n, '≥')
assert_fp_macro(nn, '<')

#if_def	_debug
	#def_macro	are_assertions_enabled()
		move	r0, 1
	#end_macro

	#def_macro	debug_msg(message, value)
		#call	debug_func(${message}, ${value})
	#end_macro

	#def_macro	debug_fp_msg(message, value)
		#call	debug_fp_func(${message}, ${value})
	#end_macro

	#def_macro	printCPU()
		int		iPrintCPUState
	#end_macro

	#def_macro	assert_true(isTrue, message)
		#call	assert_true_func(${isTrue}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_false(isFalse, message)
		#call	assert_false_func(${isFalse}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_eq(valueA, valueB, message)
		#call	assert_z_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ne(valueA, valueB, message)
		#call	assert_nz_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_gt(valueA, valueB, message)
		#call	assert_p_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ge(valueA, valueB, message)
		#call	assert_nn_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_lt(valueA, valueB, message)
		#call	assert_n_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_le(valueA, valueB, message)
		#call	assert_np_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_eq_fp(valueA, valueB, message)
		#call	assert_z_fp_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ne_fp(valueA, valueB, message)
		#call	assert_nz_fp_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_gt_fp(valueA, valueB, message)
		#call	assert_p_fp_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_ge_fp(valueA, valueB, message)
		#call	assert_nn_fp_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_lt_fp(valueA, valueB, message)
		#call	assert_n_fp_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro

	#def_macro	assert_le_fp(valueA, valueB, message)
		#call	assert_np_fp_func(${valueA}, ${valueB}, ${message},__FILE__,__LINE__)
	#end_macro
#else
	#def_macro	are_assertions_enabled()
		move	r0, 0
	#end_macro

	#def_macro	debug_msg(message, value)
	#end_macro

	#def_macro	debug_fp_msg(message, value)
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
#end_if

#def_func	enable_assertion_exit(value)
		are_assertions_enabled()
		jump	z, @END
		#call	puts, "Assertions Enabled\n"
		load	r0, value
		store	r0, ENABLE_ASSERT_EXIT
		jump	z, DISABLED_MSG
		#call	puts, "Assertion Exit Enabled\n"
		jump	@END
DISABLED_MSG:
		#call	puts, "Assertion Exit Disabled\n"
END:
#end_func


DEBUG_ASM_END: nop
