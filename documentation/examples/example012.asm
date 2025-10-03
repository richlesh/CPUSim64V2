#include <system/io.asm>

	#call	puts("AND Truth Table\n")
	#call	puts(" A   B   A AND B\n")
	mov		r1, 0
	mov		r2, 0
	and		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r2, -1
	and		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r1, -1
	mov		r2, 0
	and		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r2, -1
	and		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	#call	put_nl()

	#call	puts("OR Truth Table\n")
	#call	puts(" A   B   A OR B\n")
	mov		r1, 0
	mov		r2, 0
	or		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r2, -1
	or		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r1, -1
	mov		r2, 0
	or		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r2, -1
	or		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	#call	put_nl()

	#call	puts("XOR Truth Table\n")
	#call	puts(" A   B   A XOR B\n")
	mov		r1, 0
	mov		r2, 0
	xor		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r2, -1
	xor		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r1, -1
	mov		r2, 0
	xor		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	mov		r2, -1
	xor		r0, r1, r2
	#call	fprintf(STDOUT, "%2d  %2d  %2d\n", r1, r2, r0)
	#call	put_nl()

	#call	puts("NOT Truth Table\n")
	#call	puts(" A   NOT A\n")
	mov		r1, 0
	mov		r0, r1
	not		r0
	#call	fprintf(STDOUT, "%2d  %2d\n", r1, r0)
	mov		r1, -1
	mov		r0, r1
	not		r0
	#call	fprintf(STDOUT, "%2d  %2d\n", r1, r0)

FINIS:	
	stop
	stop
