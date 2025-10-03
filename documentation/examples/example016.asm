#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>
	
	int		iARGC
	cmp		r0, 2
	jump	eq, ONE_ARG_GOOD
	#call	puts("Must supply one argument to convert.")
	jump	@FINIS
ONE_ARG_GOOD:
	move	r0, 1
	int		iARGS
	move	r1, r0		// save for later
	#call	puts("Input:")
	#call	puts(r1)
	#call	put_nl()

	#call	puts("iPARSE_INT+1:")
	move	r0, r1
	int		iPARSE_INT		// convert cmd arg string to integer
	add		r0, 1			// We can do arithmetic on it now
	#call	put_dec(r0)
	#call	put_nl()

	#call	puts("iPARSE_DEC+1:")
	move	r0, r1
	int		iPARSE_DEC		// convert cmd arg string to integer base 10
	add		r0, 1			// We can do arithmetic on it now
	#call	put_dec(r0)
	#call	put_nl()

	#call	puts("iPARSE_HEX+1:0x")
	move	r0, r1
	int		iPARSE_HEX		// convert cmd arg string to integer base 16
	add		r0, 1			// We can do arithmetic on it now
	#call	put_hex(r0)
	#call	put_nl()

	#call	puts("iPARSE_FP+1:")
	move	r0, r1
	int		iPARSE_FLOAT	// convert cmd arg string to floating point
	add		f0, 1			// We can do arithmetic on it now
	#call	put_fp(f0)
	#call	put_nl()
	
FINIS:
	stop
	stop
