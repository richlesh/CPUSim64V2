///////////////////////////////////////////////////////////////////////////////
// Preprocessor IF/ELSE Functionality Test
//
// Test the preprocessor IF/ELSE directives
//
// Author: Richard Lesh
// Modified: 2026/06/18
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>

#define	FRED	3 - 2
#define WILMA	1 + 1
#define Barney	6 / 2
#define Betty	2 + 2
#undef	Betty

MAIN:
#define	WHO	1
#if WHO == FRED
	#call printf("FRED OK\n")
#else
	#call printf("OTHERS BAD\n")
#end_if

#undef	WHO
#define	WHO	7 - 6
#if WHO == FRED
	#call printf("FRED OK\n")
#else
	#call printf("OTHERS BAD\n")
#end_if

#undef	WHO
#define	WHO	FRED
#if WHO == FRED
	#call printf("FRED OK\n")
#else
	#call printf("OTHERS BAD\n")
#end_if

#undef	WHO
#define	WHO	WILMA / 2
#if WHO == FRED
	#call printf("FRED OK\n")
#else
	#call printf("OTHERS BAD\n")
#end_if

#undef	WHO
#define	WHO	2
#IF WHO == FRED
	#call printf("FRED BAD\n")
#ELSE
	#call printf("OTHERS OK\n")
#ENDIF

#undef	WHO
#define	WHO	3
#IF WHO == FRED
	#call printf("FRED BAD\n")
#ELSE_IF WHO == Barney
	#call printf("BARNEY OK\n")
#ELSE
	#call printf("OTHERS BAD\n")
#ENDIF

#IF WHO < Barney
	#call printf("BARNEY BAD\n")
#ELSE
	#call printf("OTHERS OK\n")
#ENDIF

#IF WHO > Barney
	#call printf("BARNEY BAD\n")
#ELSE
	#call printf("OTHERS OK\n")
#ENDIF

#IF WHO <= Barney
	#call printf("BARNEY OK\n")
#ELSE
	#call printf("OTHERS BAD\n")
#ENDIF

#IF WHO >= Barney
	#call printf("BARNEY OK\n")
#ELSE
	#call printf("OTHERS BAD\n")
#ENDIF

#IF WHO != Barney
	#call printf("BARNEY BAD\n")
#ELSE
	#call printf("OTHERS OK\n")
#ENDIF

#IFDEF WHO
	#call printf("DEFINED WHO OK\n")
#ELSE
	#call printf("UNDEF WHO BAD\n")
#ENDIF

#IFDEF BADWOLF
	#call printf("DEFINED BADWOLF BAD\n")
#ELSE
	#call printf("UNDEF BADWOLF OK\n")
#ENDIF

#IFNDEF WHO
	#call printf("UNDEF WHO BAD\n")
#ELSE
	#call printf("DEFINED WHO OK\n")
#ENDIF

#IFNDEF BADWOLF
	#call printf("UNDEF BADWOLF OK\n")
#ELSE
	#call printf("DEFINED BADWOLF BAD\n")
#ENDIF

END:	stop
		stop
