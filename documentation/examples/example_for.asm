#include <system/io.asm>

MAIN:
	#for r1, 0, lt, 10, 1
		#call	put_dec(r1)
		#for	r2, 0, lt, 2, 1		
			#call	put_nl()
		#endfor
	#endfor
	#call	func1(15)
	mov		f1, 1.0
	mov		f2, 0.1
	#call	func2(f1, f2)
	stop
	stop

#deffunc func1(max)
	#var	i, j, m
	load	m, max
	#for	i, m, gt, 0, -1
		#call	put_hex(i)
		#call	putc(' ')
	#endfor
	#call	put_nl()
#endfunc

#deffunc func2(max, inc)
	#fvar	i, j, m, incr
	load	m, max
	load	incr, inc
	#for	i, 0, le, m, incr
		#call	put_fp(i)
		#call	putc(' ')
	#endfor
	#call	put_nl()
#endfunc