#include <system/io.asm>

MAIN:
	#for 0, r1, lt, 10, 1
		#call	put_dec(r1)
		#for	0, r2, lt, 2, 1		
			#call	put_nl()
		#end_for
	#end_for
	#call	func1(15)
	load	f1, 1.0
	load	f2, 0.1
	#call	func2(f1, f2)
	stop
	stop

#def_func func1(max)
	#var	i, j, m
	load	m, max
	#for	m, i, gt, 0, -1
		#call	put_hex(i)
		#call	putc(' ')
	#end_for
	#call	put_nl()
#end_func

#def_func func2(max, increment)
	#fvar	i, m, incr
	load	m, max
	load	incr, increment
	#for	0, i, le, m, incr
		#call	put_fp(i, 6)
		#call	putc(' ')
	#end_for
	#call	put_nl()
#end_func