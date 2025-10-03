#include <system/io.asm>
#include <system/string.asm>

	#call	main()
	int		iEXIT

#deffunc	main()
	#var	filename, outfilename, mode, inport, outport
	// if (argc < 4)
	int		iARGC
	#cond	r0, lt, 4
		#call	puts("Syntax: Base64 [e|d] input_file output_file")
	#elsecond
		// Get first command line argument and put it in mode.
		move	r0, 1
		int		iARGS
		load	r0, r0[0]
		int		iTO_LOWER
		move	mode, r0
		// Get second command line argument and put it in filename.
		move	r0, 2
		int		iARGS
		move	filename, r0
		// Open file in read mode.
		#cond	mode, eq, 'e'
			#call	openRawFile(filename, READ_MODE)
		#elsecond
			#call	openTextFile(filename, READ_MODE)
		#endcond
		move	inport, r0
		// If the port returned is -1 we failed.
		#cond	inport, ne, -1
			// Get third command line argument and put it in outfilename.
			move	r0, 3
			int		iARGS
			move	outfilename, r0
			// Open text file in write mode.
			#cond	mode, eq, 'e'
				#call	openTextFile(outfilename, WRITE_MODE)
			#elsecond
				#call	openRawFile(outfilename, WRITE_MODE)
			#endcond
			move	outport, r0
			// If the port returned is -1 we failed.
			#cond	outport, ne, -1
				// Process the input stream
				#call	Base64(mode, inport, outport)
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

ALPHABET: dcs "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
DECODE_TABLE: dca \
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,\
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,64,-1,-1,\
	-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,\
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
	
#deffunc	Base64(mode, inport, outport)
	#var	inputValue,outputValue,m,p,po,index,buffer,which
	push	r1
	load	m, mode
	load	p, inport
	load	po, outport
	clear	index
	clear	buffer
// Read a character
	#cond	m, eq, 'e'
		IN1(inputValue,p)
	#elsecond
		IN0(inputValue,p)
	#endcond
// If it is -1 we are at EOF.
	#while	inputValue, ne, -1
		#cond	m, eq, 'e'
			div	r0, which, index, 3
			#cond	which, eq, 0
				move	buffer, inputValue
				rshift	outputValue, buffer, 2
				lshift	buffer, 8
				and		buffer, 0x300
				load	r0, ALPHABET[outputValue]
				OUT0(r0,po)
			#elseif_cond	which, eq, 1
				or		buffer, inputValue
				rshift	outputValue, buffer, 4
				lshift	buffer, 8
				and		buffer, 0xF00
				load	r0, ALPHABET[outputValue]
				OUT0(r0,po)
			#elsecond
				or		buffer, inputValue
				rshift	outputValue, buffer, 6
				load	r0, ALPHABET[outputValue]
				OUT0(r0,po)
				and		outputValue, buffer, 0x3F
				load	r0, ALPHABET[outputValue]
				OUT0(r0,po)
			#endcond
			#endcond
			add		index, 1
			div		r0, which, index, 48
			#cond	which, eq, 0
				#call	fput_nl(po)
			#endcond
		#elsecond
			load	inputValue, DECODE_TABLE[inputValue]
			#cond	inputValue, eq, 64
				#break
			#endcond
			#cond	inputValue, ne, -1
				div	r0, which, index, 4
				#cond	which, eq, 0
					move	buffer, inputValue
				#elseif_cond	which, eq, 1
					lshift	buffer, 6
					or		buffer, inputValue
					rshift	outputValue, buffer, 4
					and		buffer, 0xF
					OUT1(outputValue,po)
				#elseif_cond	which, eq, 2
					lshift	buffer, 6
					or		buffer, inputValue
					rshift	outputValue, buffer, 2
					and		buffer, 0x3
					OUT1(outputValue,po)
				#else_cond
					lshift	buffer, 6
					or		buffer, inputValue
					OUT1(buffer,po)
				#endcond
				#endcond
				#endcond
				add		index, 1
			#endcond
		#endcond

		#cond	m, eq, 'e'
			IN1(inputValue,p)
		#elsecond
			IN0(inputValue,p)
		#endcond
	#endwhile

// Add padding if needed
	#cond	m, eq, 'e'
		div	r0, which, index, 3
		#cond	which, eq, 1
			rshift	outputValue, buffer, 4
			load	r0, ALPHABET[outputValue]
			OUT0(r0,po)
			load	r0, ALPHABET[64]
			OUT0(r0,po)
			OUT0(r0,po)
		#elseif_cond	which, eq, 2
			rshift	outputValue, buffer, 6
			load	r0, ALPHABET[outputValue]
			OUT0(r0,po)
			load	r0, ALPHABET[64]
			OUT0(r0,po)
		#endcond
		#endcond
		OUT0('\n',po)
	#endcond
	pop		r1
#endfunc
	stop
	stop
