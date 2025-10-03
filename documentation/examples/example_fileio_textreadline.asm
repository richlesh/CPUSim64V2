#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Reads a file a line at a time and prints it to the console.
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
	#call	read_lines(STDIN)
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
	#call	read_lines(port)
// Close the file
	#call	closeFile(port)
ENDIF1:
	stop
#end_func

// Read lines from port and double space them to STDOUT
#def_func	read_lines(port)
	#var	line,p
	load	p, port
	jump	@LOOP1_END
LOOP1:
	#call	puts(line)
	#call	put_nl()
	#call	put_nl()
LOOP1_END:
	#call	fgetline(p)
	move	line, r0
	jump	nz, @LOOP1
#end_func


	stop
	stop

