#include <system/string.def>
#include <system/system.def>

jump	@STRING_ASM_END

///////////////////////////////////////////////////////////////////////////////
// strAlloc(size)
// Allocates an empty string with room for size characters.
// Returns alloc block for string which should eventually be freed.
///////////////////////////////////////////////////////////////////////////////
#def_func	strAlloc(size)
	#var	len, addr
	load	len, size
	add		len, 1
	ALLOC(len)
	move	addr, r0
	store	0, addr
	#return	addr
#end_func

///////////////////////////////////////////////////////////////////////////////
// strResize(size)
// Reallocates a heap string with room for size characters.  Old string is
// copied to the new string.
// Returns alloc block for string which should eventually be freed.
///////////////////////////////////////////////////////////////////////////////
#def_func	strResize(size)
	#var	len, addr
	load	len, size
	add		len, 1
	REALLOC(addr, len)
#end_func

///////////////////////////////////////////////////////////////////////////////
// strcopy(str)
// Allocates a string that is a duplicate of the string passed.
// Returns alloc block for string which should eventually be freed.
///////////////////////////////////////////////////////////////////////////////
#def_func	strcopy(str)
	#var	len, addrOld, addrNew
	load	addrOld, str
	#call	strlen(addrOld)
	move	len, r0
	add		len, 1
	ALLOC(len)
	move	addrNew, r0
	MEMMOVE(addrNew, addrOld, len)
	#return	addrNew
#end_func

///////////////////////////////////////////////////////////////////////////////
// charAt(str, which)
// Returns the character at position which in str.  The position must be
// in the range [0, strlen(str))
///////////////////////////////////////////////////////////////////////////////
#def_func charAt(str, which)
	#var	i, c, len, addr
	load	addr, str
	load	i, which
	move	c, 0
	#cond	addr, ne, 0
		#call	strlen(addr)
		move	len, r0
		COMPARE_RANGE(0, le, i, lt, len)
		#condsr	nz
			load	c, addr[i]
		#endcondsr
	#endcond
	#return	c
#end_func

///////////////////////////////////////////////////////////////////////////////
// setCharAt(str, which, newChar)
// Sets the character at position which in str.  The position must be
// in the range [0, strlen(str)).  Returns the old character.
///////////////////////////////////////////////////////////////////////////////
#def_func setCharAt(str, which, newChar)
	#var	i, c, len, addr, oldChar
	load	addr, str
	load	i, which
	load	c, newChar
	move	oldChar, 0
	#cond	addr, ne, 0
		#call	strlen(addr)
		move	len, r0
		COMPARE_RANGE(0, le, i, lt, len)
		#condsr	nz
			load	oldChar, addr[i]
			store	c, addr[i]
		#endcondsr
	#endcond
	#return	oldChar
#end_func

///////////////////////////////////////////////////////////////////////////////
// strlen(s) - Computes length of null-terminated string (array of codepoints)
// s	Base address of string to count
// Returns length in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	strlen(s)
	#var	count, sBase
	load	sBase, s
	clear	count
	jump	@LOOP1_END
LOOP1:
	add		count, 1
LOOP1_END:
	load	r0, sBase[count]
	jump	nz, @LOOP1
	#return	count
#end_func

///////////////////////////////////////////////////////////////////////////////
// strcmp(s1, s2) - Compares two strings based on lexicographic ordering
// s1	Base address of first string
// s1	Base address of second string
// Returns 0 if both are equal, <0 if first is less than second and
//			>0 if first is greater than second
///////////////////////////////////////////////////////////////////////////////
#def_func	strcmp(s1, s2)
	#var	result, c1, c2, s1Base, s2Base
	push	r1
	push	r2
	load	s1Base, s1
	load	s2Base, s2
	jump	@LOOP1_END
LOOP1:
	add		s1Base, 1
	add		s2Base, 1
LOOP1_END:
	load	c1, s1Base
	load	c2, s2Base
	sub		result, c1, c2
// loop if !result && c1 
	TO_NOT_BOOLEAN(result)
	move	r1, r0
	TO_BOOLEAN(c1)
	move	r2, r0
	and		r0, r1, r2
	jump	nz, @LOOP1
	pop		r2
	pop		r1
	#return	result
#end_func

///////////////////////////////////////////////////////////////////////////////
// stricmp(s1, s2) - Compares two strings based on lexicographic ordering but
//					ignores case
// s1	Base address of first string
// s1	Base address of second string
// Returns 0 if both are equal, <0 if first is less than second and
//			>0 if first is greater than second
///////////////////////////////////////////////////////////////////////////////
#def_func	stricmp(s1, s2)
	#var	s1Base, s2Base, result
	load	s1Base, s1
	load	s2Base, s2
	#call	strToLower(s1Base)
	move	s1Base, r0
	#call	strToLower(s2Base)
	move	s2Base, r0
	#call	strcmp(s1Base, s2Base)
	move	result, r0
	FREE(s1Base)
	FREE(s2Base)
	#return	result
#end_func

///////////////////////////////////////////////////////////////////////////////
// strToLower(s) - Converts a string to lower case and returns as a new string
//				   in the heap that must be freed.
// s	Base address of the string to convert
// Returns address of newly allocated converted string in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	strToLower(s)
	load	r0, s
	int		iTO_LOWER_STR
#end_func

///////////////////////////////////////////////////////////////////////////////
// strToUpper(s) - Converts a string to upper case and returns as a new string
//				   in the heap that must be freed.
// s	Base address of the string to convert
// Returns address of newly allocated converted string in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	strToUpper(s)
	load	r0, s
	int		iTO_UPPER_STR
#end_func

///////////////////////////////////////////////////////////////////////////////
// strStartsWith(s, prefix) - Checks to see if the string begins with prefix
// s		Base address of string to search
// prefix	Prefix to cehck for
// Returns 	TRUE if prefix matches, otherwise FALSE
///////////////////////////////////////////////////////////////////////////////
#def_func	strStartsWith(s, prefix)
	#var	result, c1, c2, s1Base, s2Base
	push	r1
	push	r2
	load	s1Base, s
	load	s2Base, prefix
	jump	@LOOP1_END
LOOP1:
	add		s1Base, 1
	add		s2Base, 1
LOOP1_END:
	load	c1, s1Base
	load	c2, s2Base
	sub		result, c1, c2
// loop if !result && c2 
	TO_NOT_BOOLEAN(result)
	move	r1, r0
	TO_BOOLEAN(c2)
	move	r2, r0
	and		r0, r1, r2
	jump	nz, @LOOP1
	pop		r2
	pop		r1
	test	c2
	move	z, r0, TRUE, FALSE
#end_func

///////////////////////////////////////////////////////////////////////////////
// strEndsWith(s, suffix) - Checks to see if the string ends with suffix
// s		Base address of string to search
// suffix	Suffix to cehck for
// Returns 	TRUE if suffix matches, otherwise FALSE
///////////////////////////////////////////////////////////////////////////////
#def_func	strEndsWith(s, suffix)
	#var	result, c1, c2, s1Base, s2Base, sLen, suffixLen
	push	r1
	push	r2
	clear	result
	load	s1Base, s
	load	s2Base, suffix
	#call	strlen(s1Base)
	move	sLen, r0
	#call	strlen(s2Base)
	move	suffixLen, r0
	// if sLen < suffixLen we can't end with suffix
	cmp		sLen, suffixLen
	jump	lt, @END
	sub		r0, sLen, suffixLen
	move	r0, s1Base[r0]
	#call	strcmp(r0, s2Base)
	TO_NOT_BOOLEAN(r0)
	move	result, r0	
END:
	#return result
#end_func

///////////////////////////////////////////////////////////////////////////////
// strHashCode(s) - Computes the hashcode for the string
// s	Base address of the string to use for hashcode computation
// Returns the 32-bit hashcode in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	strHashCode(s)
	#var	hashcode, sBase, c
	load	sBase, s
	clear	hashcode
	load	c, sBase
	jump	@LOOP1_END
LOOP1:
	mult	hashcode, 31
	add		hashcode, c
	add		sBase, 1
	load	c, sBase
LOOP1_END:
	test	c
	jump	nz, @LOOP1
	and		hashcode, 0xffffffff
	#return	hashcode
#end_func

///////////////////////////////////////////////////////////////////////////////
// strIndexOf(s, c) - Returns the index within this string of the first
// 					  occurrence of the specified character.
// s	Base address of the string to search
// c	Codepoint to search for
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strIndexOf(s, c)
	#var	index, sBase, codepoint, searchCodepoint
	load	sBase, s
	clear	index
	load	searchCodepoint, c
	jump	@LOOP1_END
LOOP1:
	cmp		codepoint, searchCodepoint
	jump	z, @END
	add		index, 1
	add		sBase, 1
LOOP1_END:
	load	codepoint, sBase
	jump	nz, @LOOP1
	move	index, -1
END:
	#return	index
#end_func

///////////////////////////////////////////////////////////////////////////////
// strIndexOfFromIndex(s, c, i) - Returns the index within this string of the 
// 					first occurrence of the specified character starting 
//					with the specified index.
// s	Base address of the string to search
// c	Codepoint to search for
// i	Index into the string to start search (0 - strlen-1)
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strIndexOfFromIndex(s, c, i)
	#var	index, sBase, searchCodepoint, startIndex
	load	sBase, s
	move	index, -1
	load	searchCodepoint, c
	load	startIndex, i
	#call	strlen(sBase)
	cmp		startIndex, r0
	jump	ge, @END
	add		sBase, startIndex
	#call	strIndexOf(sBase, searchCodepoint)
	move	index, r0
	cmp		index, -1
	jump	z, @END
	add		index, startIndex
END:
	#return	index
#end_func

///////////////////////////////////////////////////////////////////////////////
// strIndexOfSubstring(s, substr) - Returns the index within this string of the
//					first occurrence of the specified substring.
// s	Base address of the string to search
// substr	Substring to search for
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strIndexOfSubstring(s, substr)
	#var	sBase, searchSubstr
	load	sBase, s
	load	searchSubstr, substr
	push	r1
	move	r0, sBase
	move	r1, searchSubstr
	int		iSUBSTRING_SEARCH
	pop		r1
#end_func

///////////////////////////////////////////////////////////////////////////////
// strIndexOfSubstringFromIndex(s, substr, i) - Returns the index within this
//					string of the first occurrence of the specified substring
//					starting with the specified index.
// s	Base address of the string to search
// substr	Substring to search for
// i	Index into the string to start search (0 - strlen-1)
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strIndexOfSubstringFromIndex(s, substr, i)
	#var	sBase, searchSubstr, startIndex, index
	load	sBase, s
	load	searchSubstr, substr
	load	startIndex, i
	move	index, -1
	push	r1
	#call	strlen(sBase)
	cmp		startIndex, r0
	jump	ge, @END
	move	r0, sBase
	add		r0, startIndex
	move	r1, searchSubstr
	int		iSUBSTRING_SEARCH
	move	index, r0
	cmp		index, -1
	jump	z, @END
	add		index, startIndex
END:
	pop		r1
	#return	index
#end_func

///////////////////////////////////////////////////////////////////////////////
// strLastIndexOf(s, c) - Returns the index within this string of the last
// 					  occurrence of the specified character.
// s	Base address of the string to search
// c	Codepoint to search for
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strLastIndexOf(s, c)
	#var	index, sBase, codepoint, searchCodepoint
	load	sBase, s
	#call	strlen(sBase)
	move	index, r0
	sub		index, 1
	load	searchCodepoint, c
	jump	@LOOP1_END
LOOP1:
	load	codepoint, sBase[index]
	cmp		codepoint, searchCodepoint
	jump	z, @END
	sub		index, 1
LOOP1_END:
	test	index
	jump	nn, @LOOP1
	move	index, -1
END:
	#return	index
#end_func

///////////////////////////////////////////////////////////////////////////////
// strLastIndexOfFromIndex(s, c, i) - Returns the index within this string of  
// 					the last occurrence of the specified character no greater 
//					than the specified index.
// s	Base address of the string to search
// c	Codepoint to search for
// i	Index into the string to start search (0 - strlen-1)
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strLastIndexOfFromIndex(s, c, i)
	#var	index, sBase, codepoint, searchCodepoint, startIndex
	load	sBase, s
	#call	strlen(sBase)
	move	index, r0
	sub		index, 1
	load	startIndex, i
	cmp		startIndex, index
	move	lt, index, startIndex, index
	load	searchCodepoint, c
	jump	@LOOP1_END
LOOP1:
	load	codepoint, sBase[index]
	cmp		codepoint, searchCodepoint
	jump	z, @END
	sub		index, 1
LOOP1_END:
	test	index
	jump	nn, @LOOP1
	move	index, -1
END:
	#return	index
#end_func

///////////////////////////////////////////////////////////////////////////////
// strLastIndexOfSubstring(s, substr) - Returns the index within this string 
//				of the last occurrence of the specified substring.
// s	Base address of the string to search
// substr	Substring to search for
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strLastIndexOfSubstring(s, substr)
	#var	sBase, searchSubstr
	load	sBase, s
	load	searchSubstr, substr
	push	r1
	push	r2
	#call	strlen(sBase)
	move	r2, r0
	sub		r2, 1
	move	r1, searchSubstr
	move	r0, sBase
	int		iLAST_SUBSTRING_SEARCH
	pop		r2
	pop		r1
#end_func

///////////////////////////////////////////////////////////////////////////////
// strLastIndexOfSubstringFromIndex(s, substr, i) - Returns the index within 
//					this string of the first occurrence of the specified 
//					substring no greater than the specified index.
// s	Base address of the string to search
// substr	Substring to search for
// i	Index into the string to start search (0 - strlen-1)
// Returns the index into the string in r0 if found otherwise -1
///////////////////////////////////////////////////////////////////////////////
#def_func	strLastIndexOfSubstringFromIndex(s, substr, i)
	#var	sBase, searchSubstr, startIndex, index
	push	r1
	push	r2
	load	r0, s
	load	r1, substr
	load	r2, i
	int		iLAST_SUBSTRING_SEARCH
	pop		r2
	pop		r1
#end_func

///////////////////////////////////////////////////////////////////////////////
// substring(s, first, last)
// Returns the substring of s starting with first and ending with last - 1
// s	Base address of the string
// first	Index of the beginning of the substring
// last		Index of the character beyond the end of the substring
// Returns a newly allocated string that must be freed when no longer needed.
///////////////////////////////////////////////////////////////////////////////
#def_func substring(s, first, last)
	#var	sBase, pos1, pos2, len, i, substr, c
	load	sBase, s
	load	pos1, first
	load	pos2, last
	#call	strlen(sBase)
	move	len, r0
	test	pos1
	move	ge, pos1, pos1, 0
	cmp		pos1, len
	move	le, pos1, pos1, len
	test	pos2
	move	ge, pos2, pos2, 0
	cmp		pos2, len
	move	le, pos2, pos2, len
	cmp		pos1, pos2
	move	le, pos2, pos2, pos1
	clear	i
	sub		r0, pos2, pos1
	add		r0, 1
	int		iALLOC
	move	substr, r0
	jump	@LOOP1_END
LOOP1:
	load	c, sBase[pos1]
	store	c, substr[i]
	add		pos1, 1
	add		i, 1
LOOP1_END:
	cmp		pos1, pos2
	jump	lt, @LOOP1
	store	0, substr[i]
#end_func

///////////////////////////////////////////////////////////////////////////////
// substringToEnd(s, first)
// Returns the substring of s starting with first and continuing to the end
// of the string.
// s	Base address of the string
// first	Index of the beginning of the substring
// Returns a newly allocated string that must be freed when no longer needed.
///////////////////////////////////////////////////////////////////////////////
#def_func substringToEnd(s, first)
	#var	sBase, pos, len, i, substr, c
	load	sBase, s
	load	pos, first
	#call	strlen(sBase)
	move	len, r0
	test	pos
	move	ge, pos, pos, 0
	cmp		pos, len
	move	le, pos, pos, len
	clear	i
	sub		r0, len, pos
	add		r0, 1
	int		iALLOC
	move	substr, r0
	jump	@LOOP1_END
LOOP1:
	load	c, sBase[pos]
	store	c, substr[i]
	add		pos, 1
	add		i, 1
LOOP1_END:
	cmp		pos, len
	jump	lt, @LOOP1
	store	0, substr[i]
#end_func

///////////////////////////////////////////////////////////////////////////////
// strcat(s1, s2)
// Returns the concatenation of strings s1 and s2.
// s1	Base address of the first string
// s2	Base address of the second string
// Returns a newly allocated string that must be freed when no longer needed.
///////////////////////////////////////////////////////////////////////////////
#def_func strcat(s1, s2)
	#var	sBase1, sBase2, strcat, len, len1, len2
	load	sBase1, s1
	load	sBase2, s2
	#call	strlen(sBase1)
	move	len1, r0
	#call	strlen(sBase2)
	move	len2, r0
	add		len2, 1
	add		len, len1, len2
	ALLOC(len)
	move	strcat, r0
	MEMMOVE(strcat, sBase1, len1)
	add		sBase1, strcat, len1
	MEMMOVE(sBase1, sBase2, len2)
	#return	strcat
#end_func

///////////////////////////////////////////////////////////////////////////////
// strReplace(s, oldChar, newChar)
// Returns a new string resulting from replacing all occurrences of oldChar in
// this string with newChar.
// s	Base address of the string
// oldChar	Char to find
// newchar	Char to replace
// Returns a newly allocated string that must be freed when no longer needed.
///////////////////////////////////////////////////////////////////////////////
#def_func strReplace(s, oldChar, newChar)
	#var 	sBase, newBase, oc, nc, i, c, len
	push	r2
	push	r1
	load	sBase, s
	load	oc, oldChar
	load	nc, newChar
	#call	strlen(sBase)
	move	len, r0
	int		iALLOC
	move	newBase, r0
	move	r1, sBase
	move	r2, len
	int		iMEMMOVE
	clear	i
	jump	@LOOP1_END
LOOP1:
	cmp		c, oc
	jump	ne, @SKIP
	store	nc, newBase[i]
SKIP:
	add		i, 1
LOOP1_END:
	load	c, newBase[i]
	jump	nz, @LOOP1
	pop		r1
	pop		r2
	#return	newBase
#end_func

///////////////////////////////////////////////////////////////////////////////
// strMatches(s, regex)
// Returns true if the regex matches in the string.
// s	Base address of the string to search
// regex	Regular expression search pattern
// Returns TRUE or FALSE.
///////////////////////////////////////////////////////////////////////////////
#def_func strMatches(s, regex)
	push	r1
	load	r0, s
	load	r1, regex
	int		iMATCHES
	pop		r1
#end_func

///////////////////////////////////////////////////////////////////////////////
// strReplaceFirst(s, regex, replacement)
// Returns new string with first regex match replaced by replacement.
// s	Base address of the string to search
// regex	Regular expression search pattern
// replacement	Replacement string
// Returns a newly allocated string that must be freed when no longer needed.
///////////////////////////////////////////////////////////////////////////////
#def_func strReplaceFirst(s, regex, replacement)
	push	r2
	push	r1
	load	r0, s
	load	r1, regex
	load	r2, replacement
	int		iREPLACE_FIRST
	pop		r1
	pop		r2
#end_func

///////////////////////////////////////////////////////////////////////////////
// strReplaceAll(s, regex, replacement)
// Returns new string with all regex matches replaced by replacement.
// s	Base address of the string to search
// regex	Regular expression search pattern
// replacement	Replacement string
// Returns a newly allocated string that must be freed when no longer needed.
///////////////////////////////////////////////////////////////////////////////
#def_func strReplaceAll(s, regex, replacement)
	push	r2
	push	r1
	load	r0, s
	load	r1, regex
	load	r2, replacement
	int		iREPLACE_ALL
	pop		r1
	pop		r2
#end_func

///////////////////////////////////////////////////////////////////////////////
// strSplit(s, regex)
// Returns an array of strings based on splittin s with regex.
// s	Base address of the string to split
// regex	Regular expression search pattern
// Returns An array of strings, all of which must be freed when no longer
// needed.
///////////////////////////////////////////////////////////////////////////////
#def_func strSplit(s, regex)
	push	r1
	load	r0, s
	load	r1, regex
	int		iSPLIT
	pop		r1
#end_func

///////////////////////////////////////////////////////////////////////////////
// strTrim(s)
// Returns a new string with trimmed version of s.
// s	Base address of the string to search
// Returns a newly allocated string that must be freed when no longer needed.
///////////////////////////////////////////////////////////////////////////////
#def_func strTrim(s)
	#var	sBase, pos1, pos2, c
	load	sBase, s
	clear	pos1
	#call	strlen(sBase)
	move	pos2, r0
	jump	@LOOP1_END
LOOP1:
	add		pos1, 1
LOOP1_END:
	load	c, sBase[pos1]
	test	c
	jump	z, @DONE1
	cmp		c, ' '
	jump	le, @LOOP1
DONE1:
	jump	@LOOP2_END
LOOP2:
	sub		pos2, 1
LOOP2_END:
	load	c, sBase[pos2]
	cmp		pos2, pos1
	jump	le, @DONE2
	cmp		c, ' '
	jump	le, @LOOP2
	add		pos2, 1
DONE2:
	#call	substring(sBase, pos1, pos2)
#end_func

///////////////////////////////////////////////////////////////////////////////
// strPrintArray(a)
// Prints an array of string addresses as strings.
// a	Base address of the array to print
///////////////////////////////////////////////////////////////////////////////
#def_func strPrintArray(a)
	#var	base, i, len, str
	load	base, a
	load	len, base[-1]
	clear	i
	jump	@LOOP1_END
LOOP1:
	#call	put_dec(i)
	#call	putc(':')
	#call	puts(str)
	#call	put_nl()
	add		i, 1
LOOP1_END:
	cmp		i, len
	jump	ge, @LOOP1_DONE
	load	str, base[i]
	jump	nz, @LOOP1
LOOP1_DONE:
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

#def_func freeArrayOfStrings(addr)
	#var	a,i,s,len
	load	a, addr
	#cond	a, gt, 0
		load	len, a[-1]
		#for	i, 0, lt, len, 1
			load	s,a[i]
			#cond	s, eq, 0
				#break
			#endcond
			move	r0, s
			int		iFREE
		#endfor
		move	r0, a
		int		iFREE
	#endcond
#end_func

STRING_ASM_END: nop
