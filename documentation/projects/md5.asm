#include <system/io.asm>
#include <system/math.def>
#include <system/string.def>
#include <system/system.def>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Converts raw stream to a 128-bit MD5 message digest.
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
	#call	md5(STDIN)
	#call	putc(':')
	#call	md5(port)
	#call	printbits(STDOUT, r0, 128)
	#cal	put_nl()
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
	#call	puts(filename)
	#call	putc(':')
	#call	md5(port)
	#call	printbits(STDOUT, r0, 128)
	#call	put_nl()
// Close the file
	#call	closeFile(port)
ENDIF1:
	#return	0
#end_func

///////////////////////////////////////////////////////////////////////////////
// Converts a raw stream into a 128-bit MD5 message digest
// Returns the digest in r0
///////////////////////////////////////////////////////////////////////////////

MD5_hashcode: dca	4

// s specifies the per-round shift amounts
SHIFT_ARRAY: 
	dca	7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22 \
		5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20 \
		4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23 \
		6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
// Use binary integer part of the sines of integers (Radians) as constants:
K_ARRAY: dca 64

#def_func	setup_k_array()
	#var	i
	#for	i, 0, lt, 64, 1
		move	f0, i
		add		f0, 1
		int		iSIN
		int		iABS_FP
		mult	f0, 0x100000000
		int		iFLOOR
		move	r0, f0
		store	r0, K_ARRAY[i]
//		#call	put_hex_size(r0, 8)
//		#call	putc(' ')
	#endfor
#endfunc

a0: dci 0x67452301   // A
b0:	dci 0xefcdab89   // B
c0: dci 0x98badcfe   // C
d0: dci 0x10325476   // D

//a0: dci 0x01234567   // A
//b0:	dci 0x89abcdef   // B
//c0: dci 0xfedcba98   // C
//d0: dci 0x76543210   // D

#def_func	md5(port)
	#var	i, M, p, a, b, c, d, F, g
	push	r1
	push	r2
	push	r3
	load	p, port
	store	0, MD5_hashcode
	store	0, MD5_hashcode[1]
	#call	setup_k_array()
	
	jump	@LOOP1_END
LOOP1:
	move	M, r0
	load	a, a0
	load	b, b0
	load	c, c0
	load	d, d0
	// main loop
	#for	i, 0, lt, 64, 1
		#cond	i, lt, 16
            // F := (B and C) or ((not B) and D)
            // alternatly F := D xor (B and (C xor D))
			xor		F, c, d
			and		F, b
			xor		F, d
			// g := i
            move	g, i
		#elseif_cond	i, lt, 32
            // F := (D and B) or ((not D) and C)
            // alternatly F := C xor (D and (B xor C))
            xor		F, b, c
            and		F, d
            xor		F, c
            // g := (5×i + 1) mod 16
            mult	r0, 5, i
            add		r0, 1
            div		r0, g, r0, 16
		#elseif_cond	i, lt, 48
            // F := B xor C xor D
            xor		r0, b, c
            xor		F, r0, d
            // g := (3×i + 5) mod 16
            mult	r0, 3, i
            add		r0, 5
            div		r0, g, r0, 16
        #else_cond
            // F := C xor (B or (not D))
            move	r0, d
            compl	r0
            or		r0, b
            xor		F, c, r0
            // g := (7×i) mod 16
            mult	r0, 7, i
            div		r0, g, r0, 16
        #endcond
        #endcond
        #endcond
	// F := F + A + K[i] + M[g]  // M[g] must be a 32-bits block
        add		F, a
        load	r0, K_ARRAY[i]
        add		F, r0
        load	r0, M[g]
        add		F, r0
		and		F, 0xffffffff
		move	a, d
		move	d, c
		move	c, b
		// 32-bit left rotate
		load	r0, SHIFT_ARRAY[i]
		lshift	F, r0
		move	r0, F
		rshift	r0, 32
		or		F, r0
		and		F, 0xffffffff
		add		b, F
		and		b, 0xffffffff
	#endfor
	
	// add a into a0, b into b0, etc.
	load	r0, a0
	add		a, r0
	and		a, 0xffffffff
	store	a, a0
	load	r0, b0
	add		b, r0
	and		b, 0xffffffff
	store	b, b0
	load	r0, c0
	add		c, r0
	and		c, 0xffffffff
	store	c, c0
	load	r0, d0
	add		d, r0
	and		d, 0xffffffff
	store	d, d0
	
LOOP1_END:
	#call	get512bits(p)
	test	r0
	jump	nz, @LOOP1

	load	r0, a0
	store	r0, MD5_hashcode[0]
	load	r0, b0
	store	r0, MD5_hashcode[1]
	load	r0, c0
	store	r0, MD5_hashcode[2]
	load	r0, d0
	store	r0, MD5_hashcode[3]
	pop		r3
	pop		r2
	pop		r1
	#return	MD5_hashcode
#end_func

#def_func	printbits(port, bits, num)
	#var	p, b, n, i, v
	load	p, port
	load	b, bits
	load	n, num
	#cal	put_nl()
	#for	i, 0, lt, n, 32
		load	v, b
		#call	put_hex_little_endian(v, 4)
		add		b, 1
	#endfor
#end_func

INPUT_LENGTH: dci 0
// input is sixteen 32-bit words
BUFFER: dca 16
INPUT_DONE: dci	FALSE
NEEDS_EXTRA_BUFFER: dci FALSE

#def_func	get512bits(port)
	#var	i, byteRead, p, len
	load	p, port
	
	#for	i, 0, lt, 16, 1
		store	0, BUFFER[i]
	#endfor
	load	r0, NEEDS_EXTRA_BUFFER
	jump	z, @SKIP1
	load	r0, INPUT_LENGTH
	#call	storeLengthInBuffer(r0)
	store	FALSE, NEEDS_EXTRA_BUFFER
	#return	BUFFER
	jump 	@END
SKIP1:
	load	r0, INPUT_DONE
	jump	z, @SETUP
	#return	0
	jump	@END
SETUP:
	push	r1
	push	r2
LOOP1:
// Read a byte
	IN1(byteRead,p)
// If it is -1 we are at EOF.
	cmp		byteRead, -1
	jump	eq, @LOOP1_END
// add to buffer
	load	len, INPUT_LENGTH
	#call	storeByteInBuffer(byteRead, len)	
	add		len, 1
	store	len, INPUT_LENGTH
	div		r1, r2, len, 64
	test	r2
	jump	z,@FINIS
	jump	@LOOP1
LOOP1_END:
// padd with zeros and length
	load	r0, INPUT_LENGTH
	div		r1, r2, r0, 64
	#cond	r2, le, 55
		#call	storeByteInBuffer(0x80, r2)
		load	r0, INPUT_LENGTH
		#call	storeLengthInBuffer(r0)
	#else_cond
		#call	storeByteInBuffer(0x80, r2)
		store	TRUE, NEEDS_EXTRA_BUFFER
	#end_cond
	store	TRUE, INPUT_DONE
FINIS:
	pop		r2
	pop		r1
	#return	BUFFER
END:
#end_func

#def_func storeByteInBuffer(v, length)
	#var	len, wordNum, shiftBits, byteValue
	load	len, length
	load	byteValue, v
	div		r0, len, len, 64
	div		wordNum, shiftBits, len, 4
//	sub		shiftBits, 3, shiftBits
	mult	shiftBits, 8
	load	r0, BUFFER[wordNum]
	lshift	byteValue, shiftBits
	or		r0, byteValue
	store	r0, BUFFER[wordNum]
#end_func

#def_func storeLengthInBuffer(length)
	#var	len, byte, i
	load	len, length
	mult	len, 8
	#for	i, 56, lt, 64, 1
		and		byte, len, 0xff
		#call	storeByteInBuffer(byte, i)
		rshift	len, 8
	#endfor
#end_func

#def_func	put_hex_little_endian(value, sizeInBytes)
	#var	i, v, size
	load	v, value
	load	size, sizeInBytes
	#for	i, 0, lt, size, 1
		move	r0, v
		and		r0, 0xff
		#call	put_hex_size(r0, 2)
		rshift	v, 8
	#endfor
#end_func
	stop
	stop

