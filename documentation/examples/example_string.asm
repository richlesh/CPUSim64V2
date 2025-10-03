#include <system/io.asm>
#include <system/string.asm>
#include <system/system.def>

	#call	main()
	int		iEXIT

FMT_sx:DCS		"(\"%s\"): %x"
FMT_sd:DCS		"(\"%s\"): %d"
FMT_ss:DCS		"(\"%s\"): \"%s\""
FMT_ssd:DCS		"(\"%s\",\"%s\"): %d"
FMT_sss:DCS		"(\"%s\",\"%s\"): \"%s\""
FMT_scd:DCS		"(\"%s\",\'%c\'): %d"
FMT_scdd:DCS	"(\"%s\",\'%c\',%d): %d"
FMT_sds:DCS		"(\"%s\",%d): \"%s\""
FMT_sdds:DCS	"(\"%s\",%d,%d): \"%s\""
FMT_ssdd:DCS	"(\"%s\",\"%s\",%d): %d"
FMT_sccs:DCS	"(\"%s\",\'%c\',\'%c\'): \"%s\""
FMT_ssss:DCS	"(\"%s\",\"%s\",\"%s\"): \"%s\""

#def_macro	TEST_ONE_ARG(func, s1, fmt)
	#call	puts("${func}")
	#call	func(${s1})
	#call	fprintf(STDOUT, ${fmt}, ${s1}, r0)
	#call	put_nl()
#end_macro

#def_macro	TEST_TWO_ARGS(func, s1, s2, fmt)
	#call	puts("${func}")
	#call	func(${s1}, ${s2})
	#call	fprintf(STDOUT, ${fmt}, ${s1}, ${s2}, r0)
	#call	put_nl()
#end_macro

#def_macro	TEST_THREE_ARGS(func, s1, s2, s3, fmt)
	#call	puts("${func}")
	#call	func(${s1}, ${s2}, ${s3})
	#call	fprintf(STDOUT, ${fmt}, ${s1}, ${s2}, ${s3}, r0)
	#call	put_nl()
#end_macro

#def_func	main
// Test strlen()
	TEST_ONE_ARG(strlen, STR1, FMT_sd)
	TEST_ONE_ARG(strlen, STR2, FMT_sd)
	TEST_ONE_ARG(strlen, STR3, FMT_sd)
	
// Test strcmp()
	TEST_TWO_ARGS(strcmp, STR1, STR1, FMT_ssd)
	TEST_TWO_ARGS(strcmp, STR1, STR2, FMT_ssd)
	TEST_TWO_ARGS(strcmp, STR1, STR3, FMT_ssd)
	TEST_TWO_ARGS(strcmp, STR1, STR4, FMT_ssd)
	TEST_TWO_ARGS(strcmp, STR2, STR1, FMT_ssd)
	TEST_TWO_ARGS(strcmp, STR3, STR1, FMT_ssd)
	TEST_TWO_ARGS(strcmp, STR4, STR1, FMT_ssd)

	TEST_TWO_ARGS(stricmp, STR1, STR1, FMT_ssd)
	TEST_TWO_ARGS(stricmp, STR2, STR5, FMT_ssd)
	TEST_TWO_ARGS(stricmp, STR1, STR4, FMT_ssd)

	TEST_TWO_ARGS(strStartsWith, STR2, STR1, FMT_ssd)
	TEST_TWO_ARGS(strStartsWith, STR1, STR2, FMT_ssd)
	TEST_TWO_ARGS(strStartsWith, STR1, STR3, FMT_ssd)
	TEST_TWO_ARGS(strStartsWith, STR3, STR1, FMT_ssd)
	TEST_TWO_ARGS(strStartsWith, STR1, STR4, FMT_ssd)

	TEST_TWO_ARGS(strEndsWith, STR1, STR2, FMT_ssd)
	TEST_TWO_ARGS(strEndsWith, STR2, STR1, FMT_ssd)
	TEST_TWO_ARGS(strEndsWith, STR1, STR6, FMT_ssd)

	TEST_ONE_ARG(strHashCode, "", FMT_sx)
	TEST_ONE_ARG(strHashCode, STR1, FMT_sx)
	TEST_ONE_ARG(strHashCode, STR2, FMT_sx)
	TEST_ONE_ARG(strHashCode, STR3, FMT_sx)
	TEST_ONE_ARG(strHashCode, STR4, FMT_sx)

	TEST_TWO_ARGS(strIndexOf, STR1, ' ', FMT_scd)
	TEST_TWO_ARGS(strIndexOf, STR1, 'H', FMT_scd)
	TEST_TWO_ARGS(strIndexOf, STR1, 'w', FMT_scd)
	TEST_TWO_ARGS(strIndexOf, STR1, '!', FMT_scd)
	TEST_THREE_ARGS(strIndexOfFromIndex, STR1, 'l', 1, FMT_scdd)
	TEST_THREE_ARGS(strIndexOfFromIndex, STR1, 'l', 3, FMT_scdd)
	TEST_THREE_ARGS(strIndexOfFromIndex, STR1, 'o', 0, FMT_scdd)
	TEST_THREE_ARGS(strIndexOfFromIndex, STR1, 'o', 7, FMT_scdd)
	TEST_THREE_ARGS(strIndexOfFromIndex, STR1, 'o', 13, FMT_scdd)
	TEST_THREE_ARGS(strIndexOfFromIndex, STR1, 'z', 4, FMT_scdd)

	TEST_TWO_ARGS(strIndexOfSubstring, STR1, STR2, FMT_ssd)
	TEST_TWO_ARGS(strIndexOfSubstring, STR1, STR6, FMT_ssd)
	TEST_TWO_ARGS(strIndexOfSubstring, STR1, STR5, FMT_ssd)
	TEST_THREE_ARGS(strIndexOfSubstringFromIndex, STR1, STR2, 0, FMT_ssdd)
	TEST_THREE_ARGS(strIndexOfSubstringFromIndex, STR1, STR6, 8, FMT_ssdd)
	TEST_THREE_ARGS(strIndexOfSubstringFromIndex, STR3, "!!", 5, FMT_ssdd)
	TEST_THREE_ARGS(strIndexOfSubstringFromIndex, STR3, "!!", 13, FMT_ssdd)
	TEST_THREE_ARGS(strIndexOfSubstringFromIndex, STR2, STR3, 5, FMT_ssdd)

	TEST_TWO_ARGS(strLastIndexOf, STR1, ' ', FMT_scd)
	TEST_TWO_ARGS(strLastIndexOf, STR1, 'H', FMT_scd)
	TEST_TWO_ARGS(strLastIndexOf, STR1, 'l', FMT_scd)
	TEST_TWO_ARGS(strLastIndexOf, STR1, '!', FMT_scd)
	TEST_TWO_ARGS(strLastIndexOf, STR1, 'z', FMT_scd)
	TEST_THREE_ARGS(strLastIndexOfFromIndex, STR1, 'l', 10, FMT_scdd)
	TEST_THREE_ARGS(strLastIndexOfFromIndex, STR1, 'l', 9, FMT_scdd)
	TEST_THREE_ARGS(strLastIndexOfFromIndex, STR1, 'o', 10, FMT_scdd)
	TEST_THREE_ARGS(strLastIndexOfFromIndex, STR1, 'o', 8, FMT_scdd)
	TEST_THREE_ARGS(strLastIndexOfFromIndex, STR1, 'o', 6, FMT_scdd)
	TEST_THREE_ARGS(strLastIndexOfFromIndex, STR1, 'z', 4, FMT_scdd)

	TEST_TWO_ARGS(strLastIndexOfSubstring, STR1, STR2, FMT_ssd)
	TEST_TWO_ARGS(strLastIndexOfSubstring, STR3, STR6, FMT_ssd)
	TEST_TWO_ARGS(strLastIndexOfSubstring, STR1, STR5, FMT_ssd)
	TEST_THREE_ARGS(strLastIndexOfSubstringFromIndex, STR1, STR2, 10, FMT_ssdd)
	TEST_THREE_ARGS(strLastIndexOfSubstringFromIndex, STR3, STR6, 13, FMT_ssdd)
	TEST_THREE_ARGS(strLastIndexOfSubstringFromIndex, STR3, "!!", 14, FMT_ssdd)
	TEST_THREE_ARGS(strLastIndexOfSubstringFromIndex, STR3, "!!", 13, FMT_ssdd)
	TEST_THREE_ARGS(strLastIndexOfSubstringFromIndex, STR3, "!!", 12, FMT_ssdd)
	TEST_THREE_ARGS(strLastIndexOfSubstringFromIndex, STR2, STR3, 5, FMT_ssdd)

	TEST_THREE_ARGS(substring, STR3, 0, 5, FMT_sdds)
	TEST_THREE_ARGS(substring, STR4, 7, 11, FMT_sdds)
	TEST_THREE_ARGS(substring, STR4, -3, 27, FMT_sdds)
	TEST_THREE_ARGS(substring, STR4, 27, 0, FMT_sdds)
	TEST_TWO_ARGS(substringToEnd, STR3, 7, FMT_sds)
	TEST_TWO_ARGS(substringToEnd, STR3, 27, FMT_sds)
	TEST_TWO_ARGS(substringToEnd, STR3, -3, FMT_sds)

	TEST_TWO_ARGS(strcat, STR2, STR6, FMT_sss)

	TEST_THREE_ARGS(strReplace, STR1, 'l', 'L', FMT_sccs)
	TEST_THREE_ARGS(strReplace, STR3, '!', '?', FMT_sccs)

// Matches
	TEST_TWO_ARGS(strMatches, STR1, ".", FMT_ssd)
	TEST_TWO_ARGS(strMatches, STR3, "[,\\.!]", FMT_ssd)
	TEST_TWO_ARGS(strMatches, STR2, ",", FMT_ssd)

// ReplaceFirst
	TEST_THREE_ARGS(strReplaceFirst, STR1, ".", "?", FMT_ssss)
	TEST_THREE_ARGS(strReplaceFirst, STR3, "[,\\.!]", "?", FMT_ssss)
	TEST_THREE_ARGS(strReplaceFirst, STR3, ",", "?", FMT_ssss)

// ReplaceAll
	TEST_THREE_ARGS(strReplaceAll, STR1, ".", "?", FMT_ssss)
	TEST_THREE_ARGS(strReplaceAll, STR3, "[,\\.!]", "?", FMT_ssss)
	TEST_THREE_ARGS(strReplaceAll, STR3, "!", "?", FMT_ssss)

// Split
	#call	fprintf(STDOUT, "strSplit(\"%s\", \"%s\"):\n", STR3, "[od]")
	#call	strSplit(STR3, "[od]")
	#call	strPrintArray(r0)

// Trim
	TEST_ONE_ARG(strTrim, STR1, FMT_ss)
	TEST_ONE_ARG(strTrim, " \ttrimmed\t ", FMT_ss)
	TEST_ONE_ARG(strTrim, " \t\ntrimmed\b\t \r", FMT_ss)

	#return	0
#end_func

STR1:DCS	"Hello, world!"
STR2:DCS	"Hello"
STR3:DCS	"Hello, world!!!..."
STR4:DCS	"Hello, Rich!"
STR5:DCS	"HELLO"
STR6:DCS	"world!"

	stop
	stop
