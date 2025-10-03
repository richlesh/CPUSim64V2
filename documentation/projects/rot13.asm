#include <system/io.asm>
#include <system/string.asm>

	#call	main()
	int		iEXIT

#deffunc	main()
	#var	filename, outfilename, inport, outport
	// if (argc < 3)
	int		iARGC
	#cond	r0, lt, 3
		#call	puts("Syntax: rot13 input_file output_file")
	#elsecond
		// Get first command line argument and put it in filename.
		move	r0, 1
		int		iARGS
		move	filename, r0
		// Open text file in read mode.
		#call	openTextFile(filename, READ_MODE)
		move	inport, r0
		// If the port returned is -1 we failed.
		#cond	inport, ne, -1
			// Get second command line argument and put it in outfilename.
			move	r0, 2
			int		iARGS
			move	outfilename, r0
			// Open text file in write mode.
			#call	openTextFile(outfilename, WRITE_MODE)
			move	outport, r0
			// If the port returned is -1 we failed.
			#cond	outport, ne, -1
				// Process the input stream
				#call	rot13(inport, outport)
				// Close the files
				#call	closeFile(outport)
				#call	closeFile(inport)
			#elsecond
				#call	closeFile(inport)
				#call	puts("Output file creation failed!")
			#endcond
		#elsecond
			#call	puts("Input file open failed!")
		#endcond
	#endcond

	#return	0
#end_func

UPPER: DCS	"NOPQRSTUVWXYZABCDEFGHIJKLM"
LOWER: DCS	"nopqrstuvwxyzabcdefghijklm"

#deffunc	rot13(inport, outport)
	#var	charRead,p,po,index
	push	r1
	load	p, inport
	load	po, outport
LOOP1:
// Read a character
	IN0(charRead,p)
// If it is -1 we are at EOF.
	cmp		charRead, -1
	jump	eq, @LOOP_END1
// Convert the read character rot13.
	COMPARE(charRead, ge, 'A')
	move	r1, r0
	COMPARE(charRead, le, 'Z')
	and		r0, r1
	#cond	r0, eq, TRUE
		sub		r0, charRead, 'A'
		load	charRead, UPPER[r0]
	#elsecond
		COMPARE(charRead, ge, 'a')
		move	r1, r0
		COMPARE(charRead, le, 'z')
		and		r0, r1
		#cond	r0, eq, TRUE
			sub		r0, charRead, 'a'
			load	charRead, LOWER[r0]
		#endcond
	#endcond

// Output the character
	#call	fputc(po, charRead)
	jump	@LOOP1
LOOP_END1:
	pop		r1
#endfunc
	stop
	stop
