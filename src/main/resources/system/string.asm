// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STRING_ASM
#define _STRING_ASM
#include <system/string.def>
#include <system/system.def>
#include <system/io.def>

STRING_ASM_START:
jump	STRING_ASM_END

///////////////////////////////////////////////////////////////////////////////
// sprintf(fmt, values...)
// Formats the values on the stack and returns a heap allocated string.
// fmt		String with formatting information
// values	Values for formatting
///////////////////////////////////////////////////////////////////////////////
#def_func	sprintf(fmt, values...)
	int		iSPRINTF
#end_func

#def_func	format(fmt, values...)
	int		iFORMAT
#end_func

///////////////////////////////////////////////////////////////////////////////
// printStringArray(a)
// Prints an array of UTF-8 strings with the index and value.
// a	Base address of the array to print
// size	Number of elements to print
///////////////////////////////////////////////////////////////////////////////
#def_func printStringArray(a)
	#var	base, i, len
	#fvar	v
	load	base, a
	load	len, base[0]
	move	i, 1
	jump	$LOOP1_END
$LOOP1:
	load	v, base[i]
	#call	put_dec(i)
	#call	putc(':')
	#call	puts(v)
	#call	put_nl()
	add		i, 1
$LOOP1_END:
	cmp		i, len
	jump	le, $LOOP1
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
#endif
