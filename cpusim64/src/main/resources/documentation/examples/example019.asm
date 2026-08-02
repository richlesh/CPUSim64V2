#include <system/io.asm>
#include <system/math.def>
#include <system/string.def>
#include <system/system.def>

// min(a,b) = (a + b - |a - b|)/2
#def_macro MIN(dest, a, b)
	sub		r1, ${a}, ${b}
	int		iABS
	mov		${dest}, r0
	neg		${dest}
	add		${dest}, ${a}
	add		${dest}, ${b}
	arshift	${dest}, 1
#end_macro

// max(a,b) = (a + b + |a - b|)/2
#def_macro MAX(dest, a, b)
	sub		r1, ${a}, ${b}
	int		iABS
	debug	r0
	mov		${dest}, r0
	add		${dest}, ${a}
	add		${dest}, ${b}
	arshift	${dest}, 1
#end_macro

PROGRAM_START:
IF1:// if (argc >= 3)
	int		iARGC
	cmp		r0, 3
	jump	lt, ELSE1			// we need two command arguments
THEN1:
	move	r1, 1
	int		iARGS
	move	r1, r0
	int		iPARSE_INT
	move	r4, r0				// save A
	move	r1, 2
	int		iARGS
	move	r1, r0
	int		iPARSE_INT
	move	r5, r0				// save B
	#call	puts("Min: ")
	#macro	MIN(r6, r4, r5)
	#call	put_dec(r6)
	#call	put_nl()
	#call	puts("Max: ")
	#macro	MAX(r6, r4, r5)
	#call	put_dec(r6)
	#call	put_nl()
	jump	ENDIF1
ELSE1:
	#call	puts("You must supply two command line arguments!")
ENDIF1:

FINIS:	
	stop
	stop
