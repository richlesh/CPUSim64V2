#include <system/io.asm>
#include <system/string.asm>
#include <system/system.def>

	#call	main()
	int		iEXIT

FMT_sx:.DCS		"(\"%s\"): %x"
FMT_sd:.DCS		"(\"%s\"): %d"
FMT_ss:.DCS		"(\"%s\"): \"%s\""
FMT_ssd:.DCS	"(\"%s\",\"%s\"): %d"
FMT_sss:.DCS	"(\"%s\",\"%s\"): \"%s\""
FMT_scd:.DCS	"(\"%s\",\'%c\'): %d"
FMT_scdd:.DCS	"(\"%s\",\'%c\',%d): %d"
FMT_sds:.DCS	"(\"%s\",%d): \"%s\""
FMT_sdds:.DCS	"(\"%s\",%d,%d): \"%s\""
FMT_ssdd:.DCS	"(\"%s\",\"%s\",%d): %d"
FMT_sccs:.DCS	"(\"%s\",\'%c\',\'%c\'): \"%s\""
FMT_ssss:.DCS	"(\"%s\",\"%s\",\"%s\"): \"%s\""

#def_macro	TEST_ONE_ARG(func, s1, fmt)
	#call	puts("${func}")
	#macro	${func}(${s1})
	#call	fprintf(STDOUT, ${fmt}, ${s1}, r0)
	#call	put_nl()
#end_macro

#def_macro	TEST_TWO_ARGS(func, s1, s2, fmt)
	#call	puts("${func}")
	#macro	${func}(${s1}, ${s2})
	#call	fprintf(STDOUT, ${fmt}, ${s1}, ${s2}, r0)
	#call	put_nl()
#end_macro

#def_macro	TEST_THREE_ARGS(func, s1, s2, s3, fmt)
	#call	puts("${func}")
	#macro	${func}(${s1}, ${s2}, ${s3})
	#call	fprintf(STDOUT, ${fmt}, ${s1}, ${s2}, ${s3}, r0)
	#call	put_nl()
#end_macro

#def_func	main()
// Test strlen()
	#macro	TEST_ONE_ARG(strlen, STR1, FMT_sd)
	#macro	TEST_ONE_ARG(strlen, STR2, FMT_sd)
	#macro	TEST_ONE_ARG(strlen, STR3, FMT_sd)
	
// Test strcmp()
	#macro	TEST_TWO_ARGS(strcmp, STR1, STR1, FMT_ssd)
	#macro	TEST_TWO_ARGS(strcmp, STR1, STR2, FMT_ssd)
	#macro	TEST_TWO_ARGS(strcmp, STR1, STR3, FMT_ssd)
	#macro	TEST_TWO_ARGS(strcmp, STR1, STR4, FMT_ssd)
	#macro	TEST_TWO_ARGS(strcmp, STR2, STR1, FMT_ssd)
	#macro	TEST_TWO_ARGS(strcmp, STR3, STR1, FMT_ssd)
	#macro	TEST_TWO_ARGS(strcmp, STR4, STR1, FMT_ssd)

	#macro	TEST_TWO_ARGS(stricmp, STR1, STR1, FMT_ssd)
	#macro	TEST_TWO_ARGS(stricmp, STR2, STR5, FMT_ssd)
	#macro	TEST_TWO_ARGS(stricmp, STR1, STR4, FMT_ssd)

	#macro	TEST_TWO_ARGS(STARTS_WITH, STR2, STR1, FMT_ssd)
	#macro	TEST_TWO_ARGS(STARTS_WITH, STR1, STR2, FMT_ssd)
	#macro	TEST_TWO_ARGS(STARTS_WITH, STR1, STR3, FMT_ssd)
	#macro	TEST_TWO_ARGS(STARTS_WITH, STR3, STR1, FMT_ssd)
	#macro	TEST_TWO_ARGS(STARTS_WITH, STR1, STR4, FMT_ssd)

	#macro	TEST_TWO_ARGS(ENDS_WITH, STR1, STR2, FMT_ssd)
	#macro	TEST_TWO_ARGS(ENDS_WITH, STR2, STR1, FMT_ssd)
	#macro	TEST_TWO_ARGS(ENDS_WITH, STR1, STR6, FMT_ssd)

	#macro	TEST_ONE_ARG(HASH_CODE, "", FMT_sx)
	#macro	TEST_ONE_ARG(HASH_CODE, STR1, FMT_sx)
	#macro	TEST_ONE_ARG(HASH_CODE, STR2, FMT_sx)
	#macro	TEST_ONE_ARG(HASH_CODE, STR3, FMT_sx)
	#macro	TEST_ONE_ARG(HASH_CODE, STR4, FMT_sx)

	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, ' ', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'H', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'w', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, '!', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'l', 1, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'l', 3, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'o', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'o', 7, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'o', 13, FMT_scdd)
	#macro	TEST_THREE_ARGS(CHAR_SEARCH, STR1, 'z', 4, FMT_scdd)

	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR1, STR2, 0, FMT_ssdd)
	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR1, STR6, 0, FMT_ssdd)
	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR1, STR5, 0, FMT_ssdd)
	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR1, STR2, 0, FMT_ssdd)
	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR1, STR6, 8, FMT_ssdd)
	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR3, "!!", 5, FMT_ssdd)
	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR3, "!!", 13, FMT_ssdd)
	#macro	TEST_THREE_ARGS(SUBSTRING_SEARCH, STR2, STR3, 5, FMT_ssdd)

	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, ' ', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'H', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'l', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, '!', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'z', 0, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'l', 10, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'l', 9, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'o', 10, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'o', 8, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'o', 6, FMT_scdd)
	#macro	TEST_THREE_ARGS(LAST_CHAR_SEARCH, STR1, 'z', 4, FMT_scdd)

	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR1, STR2, 0, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR3, STR6, 0, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR1, STR5, 0, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR1, STR2, 10, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR3, STR6, 13, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR3, "!!", 14, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR3, "!!", 13, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR3, "!!", 12, FMT_ssdd)
	#macro	TEST_THREE_ARGS(LAST_SUBSTRING_SEARCH, STR2, STR3, 5, FMT_ssdd)

	#macro	TEST_THREE_ARGS(substring, STR3, 0, 5, FMT_sdds)
	#macro	TEST_THREE_ARGS(substring, STR4, 7, 11, FMT_sdds)
	#macro	TEST_THREE_ARGS(substring, STR4, -3, 27, FMT_sdds)
	#macro	TEST_THREE_ARGS(substring, STR4, 27, 0, FMT_sdds)
	#macro	TEST_THREE_ARGS(substring, STR3, 7, -1, FMT_sdds)
	#macro	TEST_THREE_ARGS(substring, STR3, 27, -1, FMT_sdds)
	#macro	TEST_THREE_ARGS(substring, STR3, -3, -1, FMT_sdds)

	#macro	TEST_TWO_ARGS(strcat, STR2, STR6, FMT_sss)

// Matches
	#macro	TEST_TWO_ARGS(MATCHES, STR1, ".", FMT_ssd)
	#macro	TEST_TWO_ARGS(MATCHES, STR3, "[,\\.!]", FMT_ssd)
	#macro	TEST_TWO_ARGS(MATCHES, STR2, ",", FMT_ssd)

// ReplaceFirst
	#macro	TEST_THREE_ARGS(REPLACE_FIRST, STR1, ".", "?", FMT_ssss)
	#macro	TEST_THREE_ARGS(REPLACE_FIRST, STR3, "[,\\.!]", "?", FMT_ssss)
	#macro	TEST_THREE_ARGS(REPLACE_FIRST, STR3, ",", "?", FMT_ssss)

// ReplaceAll
	#macro	TEST_THREE_ARGS(REPLACE_ALL, STR1, ".", "?", FMT_ssss)
	#macro	TEST_THREE_ARGS(REPLACE_ALL, STR3, "[,\\.!]", "?", FMT_ssss)
	#macro	TEST_THREE_ARGS(REPLACE_ALL, STR3, "!", "?", FMT_ssss)

// Split
	#call	printf("strSplit(\"%s\", \"%s\"):\n", STR3, "[od]")
	#macro	SPLIT(STR3, "[od]")
	#call	printStrArray(r0)

// Trim
	#macro	TEST_ONE_ARG(TRIM, STR1, FMT_ss)
	#macro	TEST_ONE_ARG(TRIM, " \ttrimmed\t ", FMT_ss)
	#macro	TEST_ONE_ARG(TRIM, " \t\ntrimmed\b\t \r", FMT_ss)

	#return	0
#end_func

STR1:.DCS	"Hello, world!"
STR2:.DCS	"Hello"
STR3:.DCS	"Hello, world!!!..."
STR4:.DCS	"Hello, Rich!"
STR5:.DCS	"HELLO"
STR6:.DCS	"world!"

	stop
	stop
