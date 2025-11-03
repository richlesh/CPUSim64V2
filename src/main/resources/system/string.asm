#include <system/string.def>
#include <system/system.def>
#include <system/io.def>

jump	STRING_ASM_END

///////////////////////////////////////////////////////////////////////////////
// strStartsWith(s, prefix) - Checks to see if the string begins with prefix
// s		Base address of string to search
// prefix	Prefix to cehck for
// Returns 	TRUE if prefix matches, otherwise FALSE
///////////////////////////////////////////////////////////////////////////////
#def_func	strStartsWith(strArg, prefixArg)
	#var	str, prefix
	load	str, strArg
	load	prefix, prefixArg
	#macro	SUBSTRING_SEARCH(str, prefix, 0)
	move	z, r0, TRUE, FALSE
#end_func

///////////////////////////////////////////////////////////////////////////////
// strEndsWith(s, suffix) - Checks to see if the string ends with suffix
// s		Base address of string to search
// suffix	Suffix to cehck for
// Returns 	TRUE if suffix matches, otherwise FALSE
///////////////////////////////////////////////////////////////////////////////
#def_func	strEndsWith(strArg, suffixArg)
	#var	len, lenSuffix, pos, str, suffix
	load	str, strArg
	load	suffix, suffixArg
	move	len, str[0]
	load	lenSuffix, suffix[0]
	sub		pos, len, lenSuffix
	#macro	LAST_SUBSTRING_SEARCH(str, suffix, 0)
	cmp		pos, -1
	jump	ne, $FOUND
	#return	FALSE
$FOUND:
	cmp		r0, pos
	move	eq, r0, TRUE, FALSE
#end_func

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
		move	r0, STDOUT
		load	r1, addr[i]
		int		iPUTS
		int		iPUT_NL
	#endfor
#end_func

#def_func freeStrArray(addArg)
	#var	len, i, addr
	load	addr, addArg
	load	len, addr[0]
	#for	1, i <= len, 1
		load	r0, addr[i]
		int		iFREE
	#endfor
	move	r0, addr
	int		iFREE
#end_func

STRING_ASM_END: nop
