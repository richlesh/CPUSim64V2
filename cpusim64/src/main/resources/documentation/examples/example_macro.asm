#include <system/io.def>

#define iRAND 117

#def_macro rand_dice()
move r1, 1				// low is 1
move r2, 6				// high is 6
int  iRAND				// generate random [1,6]
#end_macro				// result will be in R0

#def_macro rand_int(x)
move r1, 1				// low is 1
move r2, ${x}			// high is x
int  iRAND				// generate random [1,x]
#end_macro				// result will be in R0

#def_macro rand_range(low, high)
move r1, ${low}
move r2, ${high}
int  iRAND
#end_macro

#macro rand_dice()
#macro PUT_DEC(r0)
#macro rand_dice()
#macro PUT_DEC(r0)
#macro rand_dice()
#macro PUT_DEC(r0)

#macro rand_int(100)
#macro PUT_DEC(r0)
#macro rand_range(200, 300)
#macro PUT_DEC(r0)
#macro rand_range(0, 999)
#macro PUT_DEC(r0)
