#include <system/io.asm>
#include <system/system.def>

#def_macro TRUTH_TABLE(bool_op)
	#call	puts("${bool_op} Truth Table\n")
	#call	puts(" A   B   A ${bool_op} B\n")
	mov		r1, FALSE
	mov		r2, FALSE
	bool_op	r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	bool_op	r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r1, TRUE
	mov		r2, FALSE
	bool_op	r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	mov		r2, TRUE
	bool_op	r0, r1, r2
	#call	fprintf(STDOUT, @FMT2ARGS, r1, r2, r0)
	#call	put_nl()
#end_macro

PROGRAM_START:
	TRUTH_TABLE(AND)
	TRUTH_TABLE(OR)
	TRUTH_TABLE(XOR)

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

