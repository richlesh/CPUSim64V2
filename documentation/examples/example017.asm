#include <system/io.asm>
#include <system/system.def>
	
	// If we don't have command args, print a message
IF1:// if (argc < 2)
	int		iARGC
	cmp		r0, 2
	jump	ge, @ENDIF1		// Skip THEN clause if argc ≥ 2
THEN1:
	#call	puts("There are no command line arguments!\n")
ENDIF1:

	// Print how many command args, otherwise print zero
IF2:// if (argc >= 2)
	int		iARGC
	cmp		r0, 2
	jump	lt, @ELSE2		// Skip THEN clause if argc < 2
THEN2:
	sub		r0, 1
	#call	fprintf(STDOUT, "%d command args!\n", r0)
	jump	@ENDIF2			// Skip ELSE clause
ELSE2:
	#call	puts("Zero command args!\n")
ENDIF2:	
	
	stop
	stop

