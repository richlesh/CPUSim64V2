#include <system/io.asm>
#include <system/system.def>
#include <system/math.asm>

	#call	main()
	move	r0, 0
	int		iEXIT

FMT_f:		.DCS		"(): %f"
FMT_ff:		.DCS		"(%f): %f"
FMT_fff:	.DCS		"(%f, %f): %f"
FMT_fdf:	.DCS		"(%f, %d): %f"
FMT_dd:		.DCS		"(%d): %d"
FMT_ddd:	.DCS		"(%d, %d): %d"

#def_macro	FP_TEST_NO_ARG(func, fmt)
	#call	puts("${func}")
	#call	${func}()
	#call	fprintf(STDOUT, ${fmt}, f0)
	#call	put_nl()
#end_macro

#def_macro	TEST_ONE_ARG(func, a1, fmt)
	#call	puts("${func}")
	#call	${func}(${a1})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, r0)
	#call	put_nl()
#end_macro

#def_macro	FP_TEST_ONE_ARG(func, a1, fmt)
	#call	puts("${func}")
	#call	${func}(${a1})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, f0)
	#call	put_nl()
#end_macro

#def_macro	TEST_TWO_ARGS(func, a1, a2, fmt)
	#call	puts("${func}")
	#call	${func}(${a1}, ${a2})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, ${a2}, r0)
	#call	put_nl()
#end_macro

#def_macro	FP_TEST_TWO_ARGS(func, a1, a2, fmt)
	#call	puts("${func}")
	#call	${func}(${a1}, ${a2})
	#call	fprintf(STDOUT, ${fmt}, ${a1}, ${a2}, f0)
	#call	put_nl()
#end_macro

#def_func	main()
	#var	count
	
	#macro	FP_TEST_NO_ARG(pi, FMT_F)
	#macro	FP_TEST_NO_ARG(ln_base, FMT_F)

	#macro	TEST_ONE_ARG(abs, 326, FMT_dd)
	#macro	TEST_ONE_ARG(abs, -326, FMT_dd)
	load	F4, 3.14
	#macro	FP_TEST_ONE_ARG(fabs, F4, FMT_ff)
	neg		F4
	#macro	FP_TEST_ONE_ARG(fabs, F4, FMT_ff)

	load	F4, 2.5
	#macro	FP_TEST_ONE_ARG(ceil, F4, FMT_ff)
	load	F4, -2.5
	#macro	FP_TEST_ONE_ARG(ceil, F4, FMT_ff)
	load	F4, 2.5
	#macro	FP_TEST_ONE_ARG(floor, F4, FMT_ff)
	load	F4, -2.5
	#macro	FP_TEST_ONE_ARG(floor, F4, FMT_ff)
	load	F4, 2.6
	#macro	FP_TEST_ONE_ARG(round, F4, FMT_ff)
	load	F4, 2.4
	#macro	FP_TEST_ONE_ARG(round, F4, FMT_ff)
	load	F4, -2.4
	#macro	FP_TEST_ONE_ARG(round, F4, FMT_ff)
	load	F4, -2.6
	#macro	FP_TEST_ONE_ARG(round, F4, FMT_ff)

	load	F4, 2.0
	#macro	FP_TEST_ONE_ARG(sqrt, F4, FMT_ff)
	
	load	F4, 2.0
	#macro	FP_TEST_ONE_ARG(exp, F4, FMT_ff)
	load	F4, 2.0
	#macro	FP_TEST_ONE_ARG(log, F4, FMT_ff)
	load	F4, 2.0
	#macro	FP_TEST_ONE_ARG(exp10, F4, FMT_ff)
	load	F4, 10.0
	#macro	FP_TEST_ONE_ARG(log10, F4, FMT_ff)

	load	F4, 2.5
	load	F5, 3.1
	#macro	FP_TEST_TWO_ARGS(pow, F4, F5, FMT_fff)
	#macro	TEST_TWO_ARGS(ifastpow, 2, 15, FMT_ddd)
	#macro	FP_TEST_TWO_ARGS(fastpow, F4, 15, FMT_fdf)

	#macro	FP_TEST_NO_ARG(random, FMT_F)
	#macro	FP_TEST_NO_ARG(random, FMT_F)
	#macro	FP_TEST_NO_ARG(random, FMT_F)
	#macro	FP_TEST_NO_ARG(random, FMT_F)
	#macro	FP_TEST_NO_ARG(random, FMT_F)
	#macro	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	#macro	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	#macro	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	#macro	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)
	#macro	TEST_TWO_ARGS(rand, 2, 4, FMT_ddd)

	#macro	TEST_TWO_ARGS(min, 2, 4, FMT_ddd)
	#macro	TEST_TWO_ARGS(min, 4, 2, FMT_ddd)
	#macro	TEST_TWO_ARGS(min, 2, -4, FMT_ddd)
	#macro	TEST_TWO_ARGS(min, -4, 2, FMT_ddd)
	#macro	TEST_TWO_ARGS(max, 2, 4, FMT_ddd)
	#macro	TEST_TWO_ARGS(max, 4, 2, FMT_ddd)
	#macro	TEST_TWO_ARGS(max, 2, -4, FMT_ddd)
	#macro	TEST_TWO_ARGS(max, -4, 2, FMT_ddd)

	load	F4, 2.12
	load	F5, 4.44
	#macro	FP_TEST_TWO_ARGS(fmin, F4, F5, FMT_fff)
	#macro	FP_TEST_TWO_ARGS(fmin, F5, F4, FMT_fff)
	load	F5, -4.44
	#macro	FP_TEST_TWO_ARGS(fmin, F4, F5, FMT_fff)
	#macro	FP_TEST_TWO_ARGS(fmin, F5, F4, FMT_fff)
	load	F5, 4.44
	#macro	FP_TEST_TWO_ARGS(fmax, F4, F5, FMT_fff)
	#macro	FP_TEST_TWO_ARGS(fmax, F5, F4, FMT_fff)
	load	F5, -4.44
	#macro	FP_TEST_TWO_ARGS(fmax, F4, F5, FMT_fff)
	#macro	FP_TEST_TWO_ARGS(fmax, F5, F4, FMT_fff)

	#call	puts("IntArray Sum: ")
	load	count, IntArray[-1]
	#call	sum(IntArray)
	#call	put_dec(r0)
	#call	put_nl()
	#call	puts("FPArray Sum: ")
	load	count, FPArray[-1]
	#call	fsum(FPArray)
	#call	put_fp(f0, 2)
	#call	put_nl()
	#return 0					// Programs should return 0 if all went well.
#end_func
	stop
	stop

IntArray:	.DCW		1, 2, 3, 4, 5
FPArray:	.DCW		1.1, 2.2, 3.3, 4.4, 5.5
