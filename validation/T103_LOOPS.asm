///////////////////////////////////////////////////////////////////////////////
// Preprocessor LOOP Functionality Test
//
// Test the preprocessor LOOP directives
//
// Author: Richard Lesh
// Modified: 2026/06/18
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>

	#call main()
	stop

#def_func main()
	#var i, j
	#fvar x, y, start, end, incr
	
	#for 0, i < 10, 2
		#call printf("%d ", i)
	#end_for
	#call put_nl()

	#for 0, i, <, 10, 2
		#for 0, j < 10, 1
			#call printf("%d%d ", i, j)
		#end_for
	#end_for
	#call put_nl()

	load	start, 0.
	load	end, 1.
	load	incr, 0.1
	#for start, x < end, incr
		#call printf("%f ", x)
	#end_for
	#call put_nl()

	load	y, 1.
	load	end, 10.
	#while y <= end
		#call printf("%f ", y)
		add y, 1
	#end_while
	#call put_nl()

	load	y, 1.
	load	end, 10.
	#while y <= end
		#call printf("%f ", y)
		add y, 1
	#end_while
	#call put_nl()
	
	move	i, 1
	#do_while
		#call printf("%d ", i)
		add i, 1
	#end_do_while i < 10
	#call put_nl()
	
	move	i, 1
	#do_while
		div r0, r1, i, 2
		#if_cond r1 == 0
			add i, 1
			#continue
		#end_cond
		#call printf("%d ", i)
		add i, 1
	#end_do_while i < 10
	#call put_nl()
	
	move	i, 1
	#do_while
		#if_cond i == 5
			#break
		#end_cond
		#call printf("%d ", i)
		add i, 1
	#end_do_while i < 10
	#call put_nl()
	
#end_func
