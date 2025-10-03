#include <system/io.asm>
#include <system/system.def>
	
	#call	puts("Number of command line arguments: ")
	int		iARGC
	move	r2, r0
	#call	put_dec(r2)
	#call	put_nl()
	
	clear	r1
	jump	@LOOP_END
LOOP:
	#call	put_dec(r1)
	#call	putc(':')
	move	r0, r1
	int		iARGS
	#call	puts(r0)
	#call	put_nl()
	add		r1, 1
LOOP_END:
	cmp		r1, r2
	jump	ne, @LOOP
	
	stop
	stop
