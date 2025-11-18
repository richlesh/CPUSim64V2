#include <system/io.asm>
#include <system/string.asm>

	#call	main()
	int		iEXIT

#deffunc	main()
	#var	filename, outfilename, key, inport, outport
	// if (argc < 4)
	int		iARGC
	#if_cond	r0, lt, 4
		#call	puts("Syntax: Vigenere [-]keyword input_file output_file")
	#else_cond
		// Get first command line argument and put it in key.
		move	r0, 1
		int		iARGS
		move	key, r0
		// Get second command line argument and put it in filename.
		move	r0, 2
		int		iARGS
		move	filename, r0
		// Open text file in read mode.
		#call	openTextFile(filename, READ_MODE)
		move	inport, r0
		// If the port returned is -1 we failed.
		#if_cond	inport, ne, -1
			// Get third command line argument and put it in outfilename.
			move	r0, 3
			int		iARGS
			move	outfilename, r0
			// Open text file in write mode.
			#call	openTextFile(outfilename, WRITE_MODE)
			move	outport, r0
			// If the port returned is -1 we failed.
			#if_cond	outport, ne, -1
				// Process the input stream
				#call	Vigenere(key, inport, outport)
				// Close the files
				#call	closeFile(outport)
				#call	closeFile(inport)
			#else_cond
				#call	closeFile(inport)
				#call	puts("Output file creation failed!")
			#end_cond
		#else_cond
			#call	puts("Input file open failed!")
		#end_cond
	#end_cond

	#return	0
#end_func

#deffunc	Vigenere(key, inport, outport)
	#var	charRead,k,p,po,index,decrypt,keylen,offset
	push	r1
	load	k, key
	load	p, inport
	load	po, outport
	load	r0, k[0]
	move	decrypt, FALSE
	#if_cond	r0, eq, '-'
		move	decrypt, TRUE
		move	k, k+1
	#end_cond
	#call	strlen(k)
	move	keylen, r0
	clear	index
// Read a character
	IN0(charRead,p)
// If it is -1 we are at EOF.
	#while	charRead, ne, -1
		COMPARE(charRead, gt, ' ')
		move	r1, r0
		COMPARE(charRead, le, '~')
		and		r0, r1
		#if_cond	r0, eq, TRUE
			sub		charRead, '!'
			load	offset, k[index]
			sub		offset, '!'
			#if_cond	decrypt, eq, TRUE
				neg	offset
			#end_cond
			add		charRead, offset
			#while	charRead, lt, 0
				add	charRead, 94
			#endwhile
			#while	charRead, ge, 94
				sub	charRead, 94
			#endwhile
			add		charRead, '!'
		#end_cond
		add		index, 1
		#if_cond	index, ge, keylen
			sub	index, keylen
		#end_cond
		#call	fputc(po, charRead)
		IN0(charRead,p)
	#endwhile

	pop		r1
#end_forunc
	stop
	stop
