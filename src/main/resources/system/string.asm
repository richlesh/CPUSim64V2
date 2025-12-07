#include <system/string.def>
#include <system/system.def>
#include <system/io.def>

jump	STRING_ASM_END

///////////////////////////////////////////////////////////////////////////////
// sprintf(fmt, values...)
// Formats the values on the stack and returns a heap allocated string.
// fmt		String with formatting information
// values	Values for formatting
///////////////////////////////////////////////////////////////////////////////
#def_func	sprintf(fmt, values...)
	int		iSPRINTF
#end_func sprintf

#def_func	format(fmt, values...)
	int		iFORMAT
#end_func format

///////////////////////////////////////////////////////////////////////////////
// strPrintArray(a)
// Prints an array of string addresses as strings.
// a	Base address of the array to print
///////////////////////////////////////////////////////////////////////////////
#def_func printStrArray(addrArg)
	#var	len, i, addr
	load	addr, addrArg
	load	len, addr[0]
	#for	1, i <= len, 1
		move	r1, STDOUT
		load	r2, addr[i]
		int		iPUTS
		int		iPUT_NL
	#end_for
#end_func

#def_func freeStrArray(addArg)
	#var	len, i, addr
	load	addr, addArg
	load	len, addr[0]
	#for	1, i <= len, 1
		load	r1, addr[i]
		int		iFREE
	#end_for
	move	r1, addr
	int		iFREE
#end_func

STRING_ASM_END: nop
