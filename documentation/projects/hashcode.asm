#include <system/io.asm>
#include <system/system.asm>

	#call	main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main
// Converts raw stream to a hashcode.
// If filename is supplied on the command line it is opened and read.
// If not STDIN is used.
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	#var	filename, port
// if (argc < 2)
	int		iARGC
	cmp		r0, 2
	jump	ge, PROCESS_FILE
// then we don't have command args use STDIN.
	#call	hashcode(STDIN)
	#call	fprintf(STDOUT, ": %016x\n", r0)
	jump	ENDIF1
PROCESS_FILE:
// Get first command line argument and put it in filename.
    #call   args(1)
	move	filename, r0
// Open text file in read mode.
	#call	openRawFile(filename, READ_MODE)
// Save the port returned.
	move	port, r0
// If the port returned is -1 we failed.
	#if_cond   port, ne, -1
// Process the input stream
        #call	hashcode(port)
        #call	printf("%s: %016x\n", filename, r0)
// Close the file
        #call	closeFile(port)
    #else_cond
        #call   printf("Failed to open: %s\n", filename)
    #end_cond
ENDIF1:
	#return	0
#end_func

///////////////////////////////////////////////////////////////////////////////
// Converts a raw stream into a 64-bit hashcode
// Returns the hashcode in r0
///////////////////////////////////////////////////////////////////////////////

#def_func	hashcode(port)
	#var	byteRead,p,hashcode
	load	p, port
	clear	hashcode
LOOP1:
// Read a byte
	#macro  IN1(byteRead,p)
// If it is -1 we are at EOF.
	cmp     byteRead, -1
	jump    eq, LOOP_END1
	mult    hashcode, 31
	add     hashcode, byteRead
    jump    LOOP1
LOOP_END1:
	#return	hashcode
#end_func

	stop
	stop

