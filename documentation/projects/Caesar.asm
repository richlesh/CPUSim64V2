#include <system/io.asm>
#include <system/string.asm>

	#call	main()
	int		iEXIT

#deffunc	main()
	#var	filename, outfilename, key, inport, outport
	// if (argc < 4)
	int		iARGC
	#cond	r0, lt, 4
		#call	puts("Syntax: Caesar [-95 - 95] input_file output_file")
	#elsecond
		// Get first command line argument and put it in key.
		move	r0, 1
		int		iARGS
		TO_INT(r0)
		move	key, r0
		// Get second command line argument and put it in filename.
		move	r0, 2
		int		iARGS
		move	filename, r0
		// Open text file in read mode.
		#call	openTextFile(filename, READ_MODE)
		move	inport, r0
		// If the port returned is -1 we failed.
		#cond	inport, ne, -1
			// Get third command line argument and put it in outfilename.
			move	r0, 3
			int		iARGS
			move	outfilename, r0
			// Open text file in write mode.
			#call	openTextFile(outfilename, WRITE_MODE)
			move	outport, r0
			// If the port returned is -1 we failed.
			#cond	outport, ne, -1
				// Process the input stream
				#call	Caesar(key, inport, outport)
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

#deffunc	Caesar(key, inport, outport)
	#var	charRead,k,p,po
	push	r1
	load	k, key
	load	p, inport
	load	po, outport
LOOP1:
// Read a character
	IN0(charRead,p)
// If it is -1 we are at EOF.
	cmp		charRead, -1
	jump	eq, @LOOP_END1
	COMPARE(charRead, ge, ' ')
	move	r1, r0
	COMPARE(charRead, le, '~')
	and		r0, r1
	#cond	r0, eq, TRUE
		sub		charRead, ' '
		add		charRead, k
		#while	charRead, lt, 0
			add	charRead, 95
		#endwhile
		#while	charRead, ge, 95
			sub	charRead, 95
		#endwhile
		add		charRead, ' '
	#endcond

// Output the character
	#call	fputc(po, charRead)
	jump	@LOOP1
LOOP_END1:
	pop		r1
#endfunc
	stop
	stop
