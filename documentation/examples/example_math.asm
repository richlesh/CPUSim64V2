#include <system/io.asm>
#include <system/math.asm>
#include <system/system.def>

	#call	main()
	int		iEXIT

FMT_f:DCS		"(): %f"
FMT_ff:DCS		"(%f): %f"
FMT_fff:DCS		"(%f, %f): %f"
FMT_fdf:DCS		"(%f, %d): %f"
FMT_dd:DCS		"(%d): %d"
FMT_ddd:DCS		"(%d, %d): %d"

#def_macro	FP_TEST_NO_ARG(func, fmt)
	#call	puts("${func}")
	#call	func()
	#call	fprintf(STDOUT, ${fmt}, f0)
	#call	put_nl()
#end_macro

#def_macro	TEST_ONE_ARG(func, a1, fmt)
	#call	puts("${func}")
	#call	func(${a1})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, r0)
	#call	put_nl()
#end_macro

#def_macro	FP_TEST_ONE_ARG(func, a1, fmt)
	#call	puts("${func}")
	#call	func(${a1})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, f0)
	#call	put_nl()
#end_macro

#def_macro	TEST_TWO_ARGS(func, a1, a2, fmt)
	#call	puts("${func}")
	#call	func(${a1}, ${a2})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, ${a2}, r0)
	#call	put_nl()
#end_macro

#def_macro	FP_TEST_TWO_ARGS(func, a1, a2, fmt)
	#call	puts("${func}")
	#call	func(${a1}, ${a2})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, ${a2}, f0)
	#call	put_nl()
#end_macro

#def_func	main
	#var	count
	
	FP_TEST_NO_ARG(pi, FMT_F)
	FP_TEST_NO_ARG(ln_base, FMT_F)

	TEST_ONE_ARG(abs, 326, FMT_dd)
	TEST_ONE_ARG(abs, -326, FMT_dd)
	move	f1, 3.14
	FP_TEST_ONE_ARG(fabs, f1, FMT_ff)
	neg		f1
	FP_TEST_ONE_ARG(fabs, f1, FMT_ff)

	move	f1, 2.5
	FP_TEST_ONE_ARG(ceil, f1, FMT_ff)
	move	f1, -2.5
	FP_TEST_ONE_ARG(ceil, f1, FMT_ff)
	move	f1, 2.5
	FP_TEST_ONE_ARG(floor, f1, FMT_ff)
	move	f1, -2.5
	FP_TEST_ONE_ARG(floor, f1, FMT_ff)
	move	f1, 2.6
	FP_TEST_ONE_ARG(round, f1, FMT_ff)
	move	f1, 2.4
	FP_TEST_ONE_ARG(round, f1, FMT_ff)
	move	f1, -2.4
	FP_TEST_ONE_ARG(round, f1, FMT_ff)
	move	f1, -2.6
	FP_TEST_ONE_ARG(round, f1, FMT_ff)

	move	f1, 2.0
	FP_TEST_ONE_ARG(sqrt, f1, FMT_ff)
	
	move	f1, 2.0
	FP_TEST_ONE_ARG(exp, f1, FMT_ff)
	move	f1, 2.0
	FP_TEST_ONE_ARG(log, f1, FMT_ff)
	move	f1, 2.0
	FP_TEST_ONE_ARG(exp10, f1, FMT_ff)
	move	f1, 10.0
	FP_TEST_ONE_ARG(log10, f1, FMT_ff)

	move	f1, 2.5
	move	f2, 3.1
	FP_TEST_TWO_ARGS(pow, f1, f2, FMT_fff)
	TEST_TWO_ARGS(ifastpow, 2, 15, FMT_ddd)
	FP_TEST_TWO_ARGS(fastpow, f1, 15, FMT_fdf)

	FP_TEST_NO_ARG(random, FMT_F)
	FP_TEST_NO_ARG(random, FMT_F)
	FP_TEST_NO_ARG(random, FMT_F)
	FP_TEST_NO_ARG(random, FMT_F)
	FP_TEST_NO_ARG(random, FMT_F)
	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)

	TEST_TWO_ARGS(min, 2, 4, FMT_ddd)
	TEST_TWO_ARGS(min, 4, 2, FMT_ddd)
	TEST_TWO_ARGS(min, 2, -4, FMT_ddd)
	TEST_TWO_ARGS(min, -4, 2, FMT_ddd)
	TEST_TWO_ARGS(max, 2, 4, FMT_ddd)
	TEST_TWO_ARGS(max, 4, 2, FMT_ddd)
	TEST_TWO_ARGS(max, 2, -4, FMT_ddd)
	TEST_TWO_ARGS(max, -4, 2, FMT_ddd)

	move	f1, 2.12
	move	f2, 4.44
	FP_TEST_TWO_ARGS(fmin, f1, f2, FMT_fff)
	FP_TEST_TWO_ARGS(fmin, f2, f1, FMT_fff)
	move	f2, -4.44
	FP_TEST_TWO_ARGS(fmin, f1, f2, FMT_fff)
	FP_TEST_TWO_ARGS(fmin, f2, f1, FMT_fff)
	move	f2, 4.44
	FP_TEST_TWO_ARGS(fmax, f1, f2, FMT_fff)
	FP_TEST_TWO_ARGS(fmax, f2, f1, FMT_fff)
	move	f2, -4.44
	FP_TEST_TWO_ARGS(fmax, f1, f2, FMT_fff)
	FP_TEST_TWO_ARGS(fmax, f2, f1, FMT_fff)

	#call	puts("IntArray Sum: ")
	load	count, IntArray[-1]
	#call	sum(count, IntArray)
	#call	put_dec(r0)
	#call	put_nl()
	#call	puts("FPArray Sum: ")
	load	count, FPArray[-1]
	#call	fsum(count, FPArray)
	#call	put_fp(f0)
	#call	put_nl()
	#return 0					// Programs should return 0 if all went well.
#end_func
	stop
	stop

IntArray:	DCA		1, 2, 3, 4, 5
FPArray:	DCA		1.1, 2.2, 3.3, 4.4, 5.5
