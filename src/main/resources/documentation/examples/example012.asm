#include <system/io.asm>

	#call	puts("AND Truth Table\n")
	#call	puts(" A   B   A AND B\n")
	move	r1, 0
	move	r2, 0
	and		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, 0
	move	r2, -1
	and		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, -1
	move	r2, 0
	and		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, -1
	move	r2, -1
	and		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	#call	put_nl()

	#call	puts("OR Truth Table\n")
	#call	puts(" A   B   A OR B\n")
	move	r1, 0
	move	r2, 0
	or		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, 0
	move	r2, -1
	or		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, -1
	move	r2, 0
	or		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, -1
	move	r2, -1
	or		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	#call	put_nl()

	#call	puts("XOR Truth Table\n")
	#call	puts(" A   B   A XOR B\n")
	move	r1, 0
	move	r2, 0
	xor		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, 0
	move	r2, -1
	xor		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, -1
	move	r2, 0
	xor		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	move	r1, -1
	move	r2, -1
	xor		r0, r1, r2
	#call	printf("%2d  %2d  %2d\n", r1, r2, r0)
	#call	put_nl()

	#call	puts("NOT Truth Table\n")
	#call	puts(" A   NOT A\n")
	move	r1, 0
	move	r0, r1
	compl	r0
	#call	printf("%2d  %2d\n", r1, r0)
	move	r1, -1
	move	r0, r1
	compl	r0
	#call	printf("%2d  %2d\n", r1, r0)

FINIS:	
	stop
	stop
