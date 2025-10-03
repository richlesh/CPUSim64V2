#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Writes some alphabets to a text file.
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
	#call	write_alphabets(STDOUT)
	jump	@ENDIF1
PROCESS_FILE:
// Get first command line argument and put it in filename.
	move	r0, 1
	int		iARGS
	move	filename, r0
// Create text file in write mode.
	#call	openTextFile(filename, WRITE_MODE)
// Save the port returned.
	move	port, r0
// If the port returned is -1 we failed.
	cmp		port, -1
	jump	z, @ENDIF1
// Write out the alphabets
	#call	write_alphabets(port)
// Close the file
	#call	closeFile(port)
ENDIF1:
#end_func

#def_func	write_alphabets(port)
	#var	p
	load	p, port
	// Latin
	#call	write_alphabet(p, 'A', 'Z')
	// Greek
	#call	write_alphabet(p, 0x391, 0x3A9)
	// Hebrew
	#call	write_alphabet(p, 0x5D0, 0x5EA)
	// Currency
	#call	write_alphabet(p, 0x20A0, 0x20BD)
	// Dingbats
	#call	write_alphabet(p, 0x2700, 0x2757)
	// Hiragana
	#call	write_alphabet(p, 0x3041, 0x3096)
	// Ancient Greek Numbers
	#call	write_alphabet(p, 0x10140, 0x1018E)
#end_func

#def_func	write_alphabet(port, start, end)
	#var	c, p, i, loopEnd
	load	p, port
	load	i, start
	load	loopEnd, end
	jump	@LOOP_END1
LOOP1:
	OUT0(i,p)
	add		i, 1
LOOP_END1:
	cmp		i, loopEnd
	jump	nz, @LOOP1
	#call	fput_nl(p)
#end_func

	stop
	stop

