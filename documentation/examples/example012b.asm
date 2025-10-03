#include <system/io.asm>
#include <system/system.def>

PROGRAM_START:
	#call	puts("AND Truth Table\n")
	#call	puts(" A   B   A AND B\n")
	mov		r1, FALSE
	mov		r2, FALSE
	and		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	and		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r1, TRUE
	mov		r2, FALSE
	and		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	and		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	#call	put_nl()

	#call	puts("OR Truth Table\n")
	#call	puts(" A   B   A OR B\n")
	mov		r1, FALSE
	mov		r2, FALSE
	or		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	or		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r1, TRUE
	mov		r2, FALSE
	or		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	or		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	#call	put_nl()

	#call	puts("XOR Truth Table\n")
	#call	puts(" A   B   A XOR B\n")
	mov		r1, FALSE
	mov		r2, FALSE
	xor		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	xor		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r1, TRUE
	mov		r2, FALSE
	xor		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	xor		r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	#call	put_nl()

	#call	puts("NOT Truth Table\n")
	#call	puts(" A   NOT A\n")
	mov		r1, FALSE
	mov		r0, r1
	not		r0
	#call	fprintf(STDOUT, @FMT1ARG, r1, r0)
	mov		r1, TRUE
	mov		r0, r1
	not		r0
	#call	fprintf(STDOUT, @FMT1ARG, r1, r0)

FINIS:	
	stop
	stop
FMT2ARGS:	DCS	"%2d  %2d  %2d\n"
FMT1ARG:	DCS	"%2d  %2d\n"

