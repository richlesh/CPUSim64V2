#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Converts raw stream to hex.
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
	#call	output_hex(STDIN)
	jump	@ENDIF1
PROCESS_FILE:
// Get first command line argument and put it in filename.
	move	r0, 1
	int		iARGS
	move	filename, r0
// Open text file in read mode.
	#call	openRawFile(filename, READ_MODE)
// Save the port returned.
	move	port, r0
// If the port returned is -1 we failed.
	cmp		port, -1
	jump	z, @ENDIF1
// Process the input stream
	#call	output_hex(port)
// Close the file
	#call	closeFile(port)
ENDIF1:
#end_func

#def_func	output_hex(port)
	#var	byteRead,p
	load	p, port
LOOP1:
// Read a byte
	IN1(byteRead,p)
// If it is -1 we are at EOF.
	cmp		byteRead, -1
	jump	eq, @LOOP_END1
// Output the read byte as hex.
	#call	fput_hex_size(STDOUT, byteRead, 2)
	jump	@LOOP1
LOOP_END1:
	#call	put_nl()
#end_func

	stop
	stop

