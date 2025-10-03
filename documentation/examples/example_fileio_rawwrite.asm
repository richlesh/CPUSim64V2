#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	move	r0, TRUE
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Writes binary representations of data to raw file.
// If filename is supplied on the command line it is created.
// If not STDOUT is used.
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	#var	filename, port
// if (argc < 2)
	int		iARGC
	cmp		r0, 2
	jump	ge, @PROCESS_FILE
// then we don't have command args use STDOUT.
	#call	write_data(STDOUT)
	jump	@ENDIF1
PROCESS_FILE:
// Get first command line argument and put it in filename.
	move	r0, 1
	int		iARGS
	move	filename, r0
// Create text file in write mode.
	#call	openRawFile(filename, WRITE_MODE)
// Save the port returned.
	move	port, r0
// If the port returned is -1 we failed.
	cmp		port, -1
	jump	z, @ENDIF1
// Write out the data
	#call	write_data(port)
// Close the file
	#call	closeFile(port)
ENDIF1:
#end_func

#def_func	write_data(port)
	#var	p, i, end
	load	p, port
	OUT1(0x46,p)
	OUT1(0x6f,p)
	OUT1(0x75,p)
	OUT1(0x72,p)
	OUT2(0x2073,p)
	OUT2(0x636f,p)
	OUT2(0x7265,p)
	OUT2(0x2061,p)

	load	end, BIG_4INTS[-1]
	clear	i
	jump	@LOOP1_END
LOOP1:
	load	r0, BIG_4INTS[i]
	OUT4(r0,p)
	add		i, 1
LOOP1_END:
	cmp		i, end
	jump	nz, @LOOP1

	load	end, BIG_8INTS[-1]
	clear	i
	jump	@LOOP2_END
LOOP2:
	load	r0, BIG_8INTS[i]
	OUT8(r0,p)
	add		i, 1
LOOP2_END:
	cmp		i, end
	jump	nz, @LOOP2
#end_func

BIG_4INTS: dca	0x6e642073,0x6576656e,0x20796561,0x72732061
BIG_8INTS: dca	0x676f206f75722066,0x6174686572732062, \
				0x726f756768742066,0x6f727468206f6e20
	stop
	stop

