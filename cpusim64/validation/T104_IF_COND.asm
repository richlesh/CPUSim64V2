///////////////////////////////////////////////////////////////////////////////
// Preprocessor IF_COND Functionality Test
//
// Test the preprocessor IF_COND directives
//
// Author: Richard Lesh
// Modified: 2026/06/18
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>

	#call main()
	stop

#def_func main()
	#var i, j
	
	move i, 0
	move j, 2
	
	#if_cond i == 0
		#call printf("i is 0 OK\n")
	#else_cond
		#call printf("i is not 0 BAD\n")
	#end_cond

	#if_cond i != 0
		#call printf("i is not 0 BAD\n")
	#else_cond
		#call printf("i is 0 OK\n")
	#end_cond

	#if_cond i == 0
		#if_cond j == 0
			#call printf("j is 0 BAD\n")
		#else_cond
			#call printf("j is not 0 OK\n")
		#end_cond
	#else_cond
		#if_cond j == 0
			#call printf("i is not 0 BAD\n")
		#else_cond
			#call printf("i is not 0 BAD\n")
		#end_cond
	#end_cond
	
	move i, 0
	#if_cond_sr z
		#call printf("i is Z OK\n")
	#else_cond
		#call printf("i is not Z BAD\n")
	#end_cond

	move i, 0
	#if_cond_sr nz
		#call printf("i is NZ BAD\n")
	#else_cond
		#call printf("i is not NZ OK\n")
	#end_cond

	move i, 3
	#if_cond_sr pe
		#call printf("i is PE OK\n")
	#else_cond
		#call printf("i is not PE BAD\n")
	#end_cond

	move i, 3
	#if_cond_sr po
		#call printf("i is PO BAD\n")
	#else_cond
		#call printf("i is not PO OK\n")
	#end_cond

	move i, -2
	#if_cond_sr n
		#call printf("i is N OK\n")
	#else_cond
		#call printf("i is not N BAD\n")
	#end_cond

	move i, -2
	#if_cond_sr p
		#call printf("i is P BAD\n")
	#else_cond
		#call printf("i is not P OK\n")
	#end_cond

#end_func
