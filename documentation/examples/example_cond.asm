#include <system/io.asm>

MAIN:
	#call	func1(10,10)
	#call	func1(10,30)
	#call	func1(30,10)
	#call	func1(30,30)

	#call	func2(5)
	#call	func2(15)
	#call	func2(25)
	#call	func2(35)
	#call	func2(45)
	stop
	stop

#deffunc func1(v1, v2)
	push	r1
	push	r2
	load	r1, v1
	load	r2, v2
	#if_cond	r1, gt, 20
		#call	puts("r1 gt 20\n")
		#if_cond	r2, gt, 20
			#call	puts("r2 gt 20\n")
		#else_cond
			#call	puts("r2 le 20\n")
		#end_cond
	#else_cond
		#call	puts("r1 le 20\n")
		#if_cond	r2, gt, 20
			#call	puts("r2 gt 20\n")
		#else_cond
			#call	puts("r2 le 20\n")
		#end_cond
	#end_cond
	pop		r2
	pop		r1
#end_forunc

#deffunc func2(v1)
	#var	i
	load	i, v1
	#ifcond	i, lt, 10
		#call	puts("i lt 10\n")
	#elseifcond	i, lt, 20
		#call	puts("i lt 20\n")
	#elseifcond	i, lt, 30
		#call	puts("i lt 30\n")
	#elseifcond	i, lt, 40
		#call	puts("i lt 40\n")
	#else_cond
		#call	puts("i ge 40\n")
	#end_cond
	#end_cond
	#end_cond
	#end_cond
#end_forunc
