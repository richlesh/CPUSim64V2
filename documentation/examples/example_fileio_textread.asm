#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Converts text stream to uppercase.
// If filename is supplied on the command line it is opened and read.
// If not STDIN is used.
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	#var	filename, port
// if (argc < 2)
	int		iARGC
	cmp		r0, 2
	jump	ge, @PROCESS_FILE
// then we don't have command args use STDIN.
	#call	output_lower(STDIN)
	jump	@ENDIF1
PROCESS_FILE:
// Get first command line argument and put it in filename.
	move	r0, 1
	int		iARGS
	move	filename, r0
// Open text file in read mode.
	#call	openTextFile(filename, READ_MODE)
// Save the port returned.
	move	port, r0
// If the port returned is -1 we failed.
	cmp		port, -1
	jump	z, @ENDIF1
// Process the input stream
	#call	output_lower(port)
// Close the file
	#call	closeFile(port)
ENDIF1:
#end_func

#def_func	output_lower(port)
	#var	charRead,p
	load	p, port
LOOP1:
// Read a character
	IN0(charRead,p)
// If it is -1 we are at EOF.
	cmp		charRead, -1
	jump	eq, @LOOP_END1
// Convert the read character to uppercase.
	move	r0, charRead
	int		iTO_UPPER
// Output the character
	#call	putc(r0)
	jump	@LOOP1
LOOP_END1:
#end_func

	stop
	stop

