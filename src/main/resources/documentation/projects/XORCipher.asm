#include <system/io.asm>
#include <system/string.asm>

	#call	main()
	int		iEXIT

#deffunc	main()
	#var	filename, outfilename, key, inport, outport
	// if (argc < 4)
	int		iARGC
	#if_cond	r0, lt, 4
		#call	puts("Syntax: XORCipher keyword input_file output_file")
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
		#call	openRawFile(filename, READ_MODE)
		move	inport, r0
		// If the port returned is -1 we failed.
		#if_cond	inport, ne, -1
			// Get third command line argument and put it in outfilename.
			move	r0, 3
			int		iARGS
			move	outfilename, r0
			// Open text file in write mode.
			#call	openRawFile(outfilename, WRITE_MODE)
			move	outport, r0
			// If the port returned is -1 we failed.
			#if_cond	outport, ne, -1
				// Process the input stream
				#call	XORCipher(key, inport, outport)
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

#deffunc	XORCipher(key, inport, outport)
	#var	byteRead,k,p,po,index,keylen,mask
	push	r1
	load	k, key
	load	p, inport
	load	po, outport
	load	r0, k[0]
	#call	strlen(k)
	move	keylen, r0
	clear	index
// Read a character
	IN1(byteRead,p)
// If it is -1 we are at EOF.
	#while	byteRead, ne, -1
		load	mask, k[index]
		xor		byteRead, mask
		add		index, 1
		#if_cond	index, ge, keylen
			sub	index, keylen
		#end_cond
		OUT1(byteRead,po)
		IN1(byteRead,p)
	#endwhile

	pop		r1
#end_forunc
	stop
	stop
