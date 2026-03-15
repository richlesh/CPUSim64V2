#include <system/system.def>
#include <system/io.def>        // Defines PUT_FP(x)

	load   f1, 3.14159
	move   r1, 3
	#call  sample_func(r1, f1)
	#macro PUT_FP(f0, 6)
	int	   iEXIT

#def_func sample_func(int_arg, float_arg)
	#svar s1, s2		// creates two stack variables
	#var  i, j			// assigns i -> r28, j -> r27
	#fvar x, y			// assigns x -> f31, y -> f30
	
	load  i, int_arg
	load  x, float_arg
	mult  j, i, i
	mult  y, x, x
	store s1, j
	store s2, y
	#freturn y			// sets f0 to y, restores r28, r27, f31 and f30
						// s1 and s2 are destroyed, finally returns
#end_func
