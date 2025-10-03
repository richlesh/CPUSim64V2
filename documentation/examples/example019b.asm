#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

#def_macro MIN(dest, a, b)
	cmp		${a}, ${b}
	mov		lt, ${dest}, ${a}, ${b}
#end_macro

#def_macro MAX(dest, a, b)
	cmp		${a}, ${b}
	mov		gt, ${dest}, ${a}, ${b}
#end_macro

PROGRAM_START:
IF1:// if (argc >= 3)
	int		iARGC
	cmp		r0, 3
	jump	lt, @ELSE1			// we need two command arguments
THEN1:
	move	r0, 1
	int		iARGS
	int		iPARSE_INT
	move	r1, r0				// save A
	move	r0, 2
	int		iARGS
	int		iPARSE_INT
	move	r2, r0				// save B
	#call	puts("Min: ")
	MIN(r3, r1, r2)
	#call	put_dec(r3)
	#call	put_nl()
	#call	puts("Max: ")
	MAX(r3, r1, r2)
	#call	put_dec(r3)
	#call	put_nl()
	jump	@ENDIF1
ELSE1:
	#call	puts("You must supply two command line arguments!")
ENDIF1:

FINIS:	
	stop
	stop
