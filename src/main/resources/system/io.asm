#include <system/io.def>
#include <system/system.def>

jump	@IO_ASM_END

// PUTS
// sends a string to STDOUT
// str	String Address
#def_func puts(str)
	push	r1
	load	r1,str
	mov		r0,STDOUT
	int		iPUTS
	pop		r1
#end_func	puts

// FPUTS 
// sends a string to the specified I/O port
// port	I/O Port
// str	String Address

#def_func fputs(port,str)
	push	r1
	load	r1,str
	load	r0,port
	int		iPUTS
	pop		r1
#end_func	fputs

// PUTLINE
// sends a string to STDOUT and adds a newline
// str	String Address
#def_func putline(str)
	push	r1
	load	r1,str
	mov		r0,STDOUT
	int		iPUTS
	out		1,STDOUT,'\n'
	pop		r1
#end_func	puts

// FPUTLINE
// sends a string to the specified I/O port and adds a newline
// port	I/O Port
// str	String Address

#def_func fputline(port,str)
	#var	p
	push	r1
	load	r1,str
	load	p,port
	move	r0, p
	int		iPUTS
	out		1,p,'\n'
	pop		r1
#end_func	fputs

// PUTC
// Sends the character to the STDOUT port
// value	Character to output
#def_func	putc(value)
		load	r0,value
		out		CHAR,STDOUT,r0
#end_func putc

// FPUTC
// Sends the character to the specified I/O port
// value	Character to output
// port	I/O Port
#def_func	fputc(port,value)
		push	r1
		load	r0,port
		load	r1,value
		out		CHAR,r0,r1
		pop		r1
#end_func fputc

// PUT_INT
// Formats the integer value as a string and then sends to STDOUT
// value	Value to format
// base		Base for formatting
#def_func	put_int(value,base)
		push	r1
		push	r2
		move	r0,STDOUT
		load	r1,value
		load	r2,base
		int		iPUT_INT
		pop		r2
		pop		r1
#end_func put_int

// FPUT_INT
// Formats the integer value as a string and then sends to the specified I/O port
// port	I/O Port
// value	Value to format
// base		Base for formatting
#def_func	fput_int(port,value,base)
		push	r1
		push	r2
		load	r0,port
		load	r1,value
		load	r2,base
		int		iPUT_INT
		pop		r2
		pop		r1
#end_func fput_int

// PUT_DEC 
// Formats the integer value as a string and then sends to STDOUT
// value	Value to format
#def_func	put_dec(value)
		push	r1
		move	r0,STDOUT
		load	r1,value
		int		iPUT_DEC
		pop		r1
#end_func put_dec

// FPUT_DEC 
// Formats the integer value as a string and then sends to the specified I/O port
// port	I/O Port
// value	Value to format
#def_func	fput_dec(port,value)
		push	r1
		load	r0,port
		load	r1,value
		int		iPUT_DEC
		pop		r1
#end_func fput_dec

// PUT_HEX
// Formats the integer value as a string and then sends to STDOUT
// value	Value to format
#def_func	put_hex(value)
		push	r1
		push	r2
		move	r0,STDOUT
		load	r1,value
		move	r2,-1
		int		iPUT_HEX
		pop		r2
		pop		r1
#end_func put_hex

// PUT_HEX_SIZE
// Formats the integer value as a string and then sends to STDOUT
// value	Value to format
// size		Minimum number of digits to print, pads with 0
#def_func	put_hex_size(value, size)
		push	r1
		push	r2
		move	r0,STDOUT
		load	r1,value
		load	r2,size
		int		iPUT_HEX
		pop		r2
		pop		r1
#end_func put_hex

// FPUT_HEX
// Formats the integer value as a string and then sends to the specified I/O port
// port		I/O Port
// value	Value to format
#def_func	fput_hex(port,value)
		push	r1
		load	r0,port
		load	r1,value
		int		iPUT_HEX
		pop		r1
#end_func fput_hex

// FPUT_HEX_SIZE
// Formats the integer value as a string and then sends to STDOUT
// port		I/O Port
// value	Value to format
// size		Minimum number of digits to print, pads with 0
#def_func	fput_hex_size(port, value, size)
		push	r1
		push	r2
		load	r0,port
		load	r1,value
		load	r2,size
		int		iPUT_HEX
		pop		r2
		pop		r1
#end_func put_hex

// PUT_FP
// Formats the IEEE754 floating point value as a string and then sends to STDOUT
// fpvalue	Value to format
#def_func	put_fp(fpvalue)
		mov	r0,STDOUT
		ld	f0,fpvalue
		int	iPUT_FP
#end_func put_fp

// FPUT_FP
// Formats the IEEE754 floating point value as a string and then sends to the specified I/O port
// port	I/O Port
// fpvalue	Value to format
#def_func	fput_fp(port,fpvalue)
		ld	r0,port
		ld	f0,fpvalue
		int	iPUT_FP
#end_func fput_fp

// PUT_NL
// Outputs the newline character to STDOUT
// port	I/O Port

#def_func	put_nl()
		out		CHAR,STDOUT,'\n'
#end_func put_nl

// FPUT_NL
// Outputs the newline character to the port specified
// port	I/O Port

#def_func	fput_nl(port)
#var	port_arg
		ld		port_arg,port
		out		CHAR,port_arg,'\n'
#end_func put_nl

// fprintf(port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	fprintf(port, fmt, values...)
	int		iPRINTF
#end_func

// cond_fprintf(cond, port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port
// cond		Must be TRUE to print
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	cond_fprintf(b, port, fmt, values...)
	int		iCOND_PRINTF
#end_func

// fatal(port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port.
// Then terminates the program with error code 1.
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	fatal(port, fmt, values...)
	load	r0, port
	#call	fputs(r0, "\nFATAL: ")
	int		iPRINTF
	move	r0, 0
	int		iEXIT
#end_func

// cond_fatal(cond, port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port.
// Then terminates the program with error code 1.
// cond		Must be TRUE to print and terminate
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	cond_fatal(b, port, fmt, values...)
	load	r0, b
	#cond_sr	nz
		load	r0, port
		#call	fputs(r0, "\nFATAL: ")
		int		iCOND_PRINTF
		move	r0, 0
		int		iEXIT
	#end_cond_sr
#end_func

// fgetline
// Read an entire line from the specified I/O port
// port		I/O Port
// Returns address of the line as a string.  Do not free this address.

__FGETLINE_BUFFER: dci 0
#def_func	fgetline(port)
	#var	buffer, bufferLen, charRead, i, p
	push	r1
	load	p, port
	load	buffer, __FGETLINE_BUFFER
	#cond	buffer, eq, 0
		move	r0, 128
		int		iALLOC
		move	buffer, r0
	#endcond
	#cond	buffer, ne, 0
		load	bufferLen, buffer[-1]
		sub		bufferLen, 1
		clear	i
		IN0(charRead, p)
		#while	charRead, ne, -1
			#cond	charRead, eq, '\n'
				#break
			#endcond
			#cond	i, ge, bufferLen
				move	r1, bufferLen
				add		r1, 128
				move	r0, buffer
				int		iREALLOC
				move	buffer, r0
				#cond	buffer, eq, 0
					#break
				#endcond
				load	bufferLen, buffer[-1]
				sub		bufferLen, 1
			#endcond
			store	charRead, buffer[i]
			add		i, 1
			IN0(charRead, p)
		#endwhile
		#cond	buffer, ne, 0
			store	0, buffer[i]
		#endcond
	#endcond
	store	buffer, __FGETLINE_BUFFER
	COMPARE(charRead, eq, -1)
	move	r1, r0
	COMPARE(i, eq, 0)
	and		r0, r1
	move	nz, r0, 0, buffer
	pop		r1
#end_func

#define	READ_MODE	0
#define	WRITE_MODE	1
#define	APPEND_MODE	2

_PORT_MAP: .dca	256			// Must match Architecture.NUM_PORTS

#def_func	openTextFile(filename, mode)
	#var	fn, m, i, maxPort, foundPort
	push	r1
	load	fn, filename
	load	m, mode
	// find available port in _PORT_MAP
	move	i, 3		// Skip three for STDIN, STDOUT and STDERR
	load	maxPort, _PORT_MAP[-1]
	jump	@FIND_PORT_END
FIND_PORT:
	load	r0, _PORT_MAP[i]
	jump	z, @FOUND_PORT
	add		i, 1
FIND_PORT_END:
	cmp		i, maxPort
	jump	lt, @FIND_PORT
// all ports used
	#return	-1
	jump	@FINIS
FOUND_PORT:
	move	foundPort, i
	
	cmp		m, READ_MODE
	jump	nz, @NOT_READ_MODE
	move	r0, foundPort
	move	r1, fn
	int		iOPEN_FILE_READ
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	@FINIS
NOT_READ_MODE:
	cmp		m, WRITE_MODE
	jump	nz, @NOT_WRITE_MODE
	move	r0, foundPort
	move	r1, fn
	int		iOPEN_FILE_WRITE
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	@FINIS
NOT_WRITE_MODE:
	cmp		m, APPEND_MODE
	jump	nz, @NOT_APPEND_MODE
	move	r0, foundPort
	move	r1, fn
	int		iOPEN_FILE_APPEND
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	@FINIS
NOT_APPEND_MODE:
	move	r0, -1
FINIS:
	pop		r1
#end_func

#def_func	openRawFile(filename, mode)
	#var	fn, m, i, maxPort, foundPort
	push	r1
	load	fn, filename
	load	m, mode
	// find available port in _PORT_MAP
	move	i, 3		// Skip three for STDIN, STDOUT and STDERR
	load	maxPort, _PORT_MAP[-1]
	jump	@FIND_PORT_END
FIND_PORT:
	load	r0, _PORT_MAP[i]
	jump	z, @FOUND_PORT
	add		i, 1
FIND_PORT_END:
	cmp		i, maxPort
	jump	lt, @FIND_PORT
// all ports used
	#return	-1
	jump	@FINIS
FOUND_PORT:
	move	foundPort, i
	
	cmp		m, READ_MODE
	jump	nz, @NOT_READ_MODE
	move	r0, foundPort
	move	r1, fn
	int		iOPEN_RAW_FILE_READ
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	@FINIS
NOT_READ_MODE:
	cmp		m, WRITE_MODE
	jump	nz, @NOT_WRITE_MODE
	move	r0, foundPort
	move	r1, fn
	int		iOPEN_RAW_FILE_WRITE
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	@FINIS
NOT_WRITE_MODE:
	cmp		m, APPEND_MODE
	jump	nz, @NOT_APPEND_MODE
	move	r0, foundPort
	move	r1, fn
	int		iOPEN_RAW_FILE_APPEND
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	@FINIS
NOT_APPEND_MODE:
	move	r0, -1
FINIS:
	pop		r1
#end_func

#def_func	closeFile(port)
	load	r0, port
	store	FALSE, _PORT_MAP[r0]
	int		iCLOSE_FILE
#end_func

#def_func	flush(port)
	load	r0, port
	int		iFLUSH
#end_func

#def_func	deleteFile(filespec)
	load	r0, filespec
	int		iDELETE_FILE
#end_func

#def_func	makeDirectory(filespec)
	load	r0, filespec
	int		iMAKE_DIR
#end_func

#def_func	deleteDirectory(filespec)
	load	r0, filespec
	int		iDELETE_DIR
#end_func

#def_func	isDirectory(filespec)
	load	r0, filespec
	int		iIS_DIR
#end_func

#def_func	isFile(filespec)
	load	r0, filespec
	int		iIS_FILE
#end_func

#def_func	fileExists(filespec)
	load	r0, filespec
	int		iFILE_EXISTS
#end_func

#def_func	listFiles(filespec)
	load	r0, filespec
	int		iFILES
#end_func

#def_func	tempDirectory(prefix)
	load	r0, prefix
	int		iTEMP_DIR
#end_func

#def_func	tempFile(prefix, suffix)
	load	r0, prefix
	load	r1, suffix
	int		iTEMP_FILE
#end_func

#def_func	copy_text_file(fromPort, toPort)
	#var	line, fp, tp
	load	fp, fromPort
	load	tp, toPort
	#call	fgetline(fp)
	move	line, r0
	#while	line, ne, 0
		#call	fputs(tp, line)
		#call	fput_nl(tp)
		#call	fgetline(fp)
		move	line, r0
	#endwhile
#end_func

#def_func	copy_raw_file(fromPort, toPort)
	#var	ch, fp, tp
	load	fp, fromPort
	load	tp, toPort
	IN1(ch, fp)
	#while	ch, ne, -1
		OUT1(ch, tp)
		IN1(ch, fp)
	#endwhile
#end_func

IO_ASM_END:	nop
