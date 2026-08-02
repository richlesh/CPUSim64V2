#include <system/io.asm>

MAIN:
	move	r1, 1
	#while	r1, le, 64
		#call	put_dec(r1)
		clear	r2
		#while	r2, lt, 2		
			#call	put_nl()
			add	r2, 1
		#end_while
		mult	r1, 2
	#end_while
	#call	func1(15)
	mov		f1, 1.0
	mov		f2, 0.1
	#call	func2(f1, f2)
	stop
	stop

#def_func func1(max)
	#var	i, j, m
	load	m, max
	move	i, m
	#while	i, gt, 0
		#call	put_hex(i)
		#call	putc(' ')
		sub	i, 1
	#end_while
	#call	put_nl()
#end_func

#def_func func2(max, inc)
	#fvar	i, j, m, incr
	load	m, max
	load	incr, inc
	clear	i
	#while	i, le, m
		#call	put_fp(i)
		#call	putc(' ')
		add		i, incr
	#end_while
	#call	put_nl()
#end_func