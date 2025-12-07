#include <system/io.def>
#include <system/system.def>
#include <system/string.def>
#include <system/thread.asm>

#macro DEFINE_RECURSIVE_SPINLOCK(STDOUT_LOCK)
#call initializeRecursiveSpinLock(STDOUT_LOCK)
jump	@IO_ASM_END

// PUTS
// sends a string to STDOUT atomically
// str	String Address
#def_func puts(str)
	save	r1, r2
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	load	r2, str
	mov		r1, STDOUT
	int		iPUTS
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r1, r2
#end_func	puts

// FPUTS 
// sends a string to the specified I/O port
// port	I/O Port
// str	String Address

#def_func fputs(port, str)
	save	r1, r2
	load	r2, str
	load	r1, port
	int		iPUTS
	restore	r1, r2
#end_func	fputs

// PUTLINE
// sends a string to STDOUT atomically and adds a newline
// str	String Address
#def_func putline(str)
	save	r1, r2
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	load	r2, str
	mov		r1, STDOUT
	int		iPUT_LINE
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r1, r2
#end_func	puts

// FPUTLINE
// sends a string to the specified I/O port and adds a newline
// port	I/O Port
// str	String Address

#def_func fputline(port, str)
	save	r1, r2
	load	r2, str
	load	r1, port
	int		iPUT_LINE
	restore	r1, r2
#end_func	fputs

// PUTC
// Sends the character to the STDOUT port atomically.  This outputs a 32-bit codepoint not a byte.
// value	Character to output
#def_func	putc(value)
	push	r1
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	load	r1, value
	#macro	out0(r1, STDOUT)
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	pop		r1
#end_func putc

// FPUTC
// Sends the character to the specified I/O port.  This outputs a 32-bit codepoint not a byte.
// value	Character to output
// port	I/O Port
#def_func	fputc(port,value)
	save	r1, r2
	load	r1, port
	load	r2, value
	#macro	out0(r2, r1)
	restore	r1, r2
#end_func fputc

// PUT_INT
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
// base		Base for formatting
#def_func	put_int(value,base)
	save	r1,r3
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	move	r1,STDOUT
	load	r2,value
	load	r3,base
	int		iPUT_INT
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r1,r3
#end_func put_int

// FPUT_INT
// Formats the integer value as a string and then sends to the specified I/O port
// port	I/O Port
// value	Value to format
// base		Base for formatting
#def_func	fput_int(port,value,base)
		save	r1,r3
		load	r1,port
		load	r2,value
		load	r3,base
		int		iPUT_INT
		restore	r1,r3
#end_func fput_int

// PUT_DEC 
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
#def_func	put_dec(value)
	save	r1,r2
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	move	r1,STDOUT
	load	r2,value
	int		iPUT_DEC
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r1,r2
#end_func put_dec

// FPUT_DEC 
// Formats the integer value as a string and then sends to the specified I/O port
// port	I/O Port
// value	Value to format
#def_func	fput_dec(port,value)
	save	r1,r2
	load	r1,port
	load	r2,value
	int		iPUT_DEC
	restore	r1,r2
#end_func fput_dec

// PUT_HEX
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
#def_func	put_hex(value)
	save	r1, r3
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	move	r1, STDOUT
	load	r2, value
	move	r3, 0
	int		iPUT_HEX
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r1, r3
#end_func put_hex

// PUT_HEX_SIZE
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
// size		Minimum number of digits to print, pads with 0
#def_func	put_hex_size(value, size)
	save	r1, r3
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	move	r1, STDOUT
	load	r2,value
	load	r3, size
	int		iPUT_HEX
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	restore	r1, r3
#end_func put_hex

// FPUT_HEX
// Formats the integer value as a string and then sends to the specified I/O port
// port		I/O Port
// value	Value to format
#def_func	fput_hex(port,value)
	save	r1,r2
	load	r1,port
	load	r2,value
	int		iPUT_HEX
	restore	r1,r2
#end_func fput_hex

// FPUT_HEX_SIZE
// Formats the integer value as a string and then sends to the specified I/O port
// port		I/O Port
// value	Value to format
// size		Minimum number of digits to print, pads with 0
#def_func	fput_hex_size(port, value, size)
	save	r1,r3
	load	r1,port
	load	r2,value
	load	r3,size
	int		iPUT_HEX
	restore	r1,r3
#end_func put_hex

// PUT_FP
// Formats the IEEE754 floating point value as a string and then sends to STDOUT atomically
// fpvalue	Value to format
#def_func	put_fp(fpvalue, prec)
	save	r1, r2
	push	f1
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	move	r1, STDOUT
	load	r2, prec
	load	f1, fpvalue
	int		iPUT_FP
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	pop		f1
	restore	r1, r2
#end_func put_fp

// FPUT_FP
// Formats the IEEE754 floating point value as a string and then sends to the specified I/O port
// port	I/O Port
// fpvalue	Value to format
#def_func	fput_fp(port, fpvalue, prec)
	save	r1, r2
	push	f1
	load	r1,port
	load	r2, prec
	load	f1,fpvalue
	int		iPUT_FP
	pop		f1
	restore	r1, r2
#end_func fput_fp

// PUT_NL
// Outputs the newline character to STDOUT atomically
// port	I/O Port

#def_func	put_nl()
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	#macro	out0('\n', STDOUT)
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
#end_func put_nl

// FPUT_NL
// Outputs the newline character to the port specified
// port	I/O Port

#def_func	fput_nl(port)
	#var	port_arg
	load	port_arg,port
	#macro	out0('\n', port_arg)
#end_func put_nl

// fprintf(port, fmt, values...)
// Formats the values on the stack and then sends to the specified I/O port
// port		I/O Port
// fmt		String with formatting information
// values	Values for formatting
#def_func	fprintf(port, fmt, values...)
	int		iPRINTF
#end_func

// printf(fmt, values...)
// Formats the values on the stack and then sends to STDOUT atomically
// fmt		String with formatting information
// values	Values for formatting
#def_func	printf(fmt, values...)
	#var	str
	int		iSPRINTF
	move	str, r0
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	#call	puts(str)
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
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

// fatal(fmt, values...)
// Formats the values on the stack and then sends to STDERR atomically.
// Then terminates the program with error code 1.
// fmt		String with formatting information
// values	Values for formatting
#def_func	fatal(fmt, values...)
	#var	str
	int		iSPRINTF
	move	str, r0
	#call	acquireRecursiveSpinLock(STDOUT_LOCK)
	#call	fputs(STDERR, "\nFATAL: ")
	#call	fputs(STDERR, str)
	#call	releaseRecursiveSpinLock(STDOUT_LOCK)
	move	r1, 1
	int		iEXIT
#end_func

// cond_fatal(cond, fmt, values...)
// Formats the values on the stack and then sends to STDERR atomically.
// Then terminates the program with error code 1.
// cond		Must be TRUE to print and terminate
// fmt		String with formatting information
// values	Values for formatting
#def_func	cond_fatal(b, fmt, values...)
	load	r0, b
	#if_cond_sr	nz
		pop		r0					// Save stored SF
		pop		r1					// Save stored PC (return address)
		pop							// remove b from stack
		push	r1					// Put stored PC back
		push	r0					// Put stored SF back
		move	SF, SP				// Update SF
		int		iSPRINTF
		push	r0
		#call	acquireRecursiveSpinLock(STDOUT_LOCK)
		#call	fputs(STDERR, "\nFATAL: ")
		pop		r1
		#call	fputs(STDERR, r1)
		#call	releaseRecursiveSpinLock(STDOUT_LOCK)
		move	r1, 1
		int		iEXIT
	#end_cond_sr
#end_func

// fgetline
// Read an entire line from the specified I/O port
// port		I/O Port
// Returns address of the line as a string.  Do not free this address.
#def_func	fgetline(port, buffer)
	load	r1, port
	load	r2, buffer
	int		iGET_LINE
#end_func

/*
_PORT_MAP: .dca	256			// Must match Architecture.NUM_PORTS

#def_func	openTextFile(filename, mode)
	#var	fn, m, i, maxPort, foundPort
	save	r1,r2
	load	fn, filename
	load	m, mode
	// find available port in _PORT_MAP
	move	i, 3		// Skip three for STDIN, STDOUT and STDERR
	load	maxPort, _PORT_MAP[-1]
	jump	$FIND_PORT_END
$FIND_PORT:
	load	r1, _PORT_MAP[i]
	jump	z, $FOUND_PORT
	add		i, 1
$FIND_PORT_END:
	cmp		i, maxPort
	jump	lt, $FIND_PORT
// all ports used
	#return	-1
	jump	$FINIS
$FOUND_PORT:
	move	foundPort, i
	
	cmp		m, READ_MODE
	jump	nz, $NOT_READ_MODE
	move	r1, foundPort
	move	r2, fn
	int		iOPEN_FILE_READ
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	$FINIS
$NOT_READ_MODE:
	cmp		m, WRITE_MODE
	jump	nz, $NOT_WRITE_MODE
	move	r1, foundPort
	move	r2, fn
	int		iOPEN_FILE_WRITE
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	$FINIS
$NOT_WRITE_MODE:
	cmp		m, APPEND_MODE
	jump	nz, $NOT_APPEND_MODE
	move	r1, foundPort
	move	r2, fn
	int		iOPEN_FILE_APPEND
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	$FINIS
$NOT_APPEND_MODE:
	move	r0, -1
$FINIS:
	restore	r1,r2
#end_func

#def_func	openRawFile(filename, mode)
	#var	fn, m, i, maxPort, foundPort
	save	r1,r2
	load	fn, filename
	load	m, mode
	// find available port in _PORT_MAP
	move	i, 3		// Skip three for STDIN, STDOUT and STDERR
	load	maxPort, _PORT_MAP[-1]
	jump	$FIND_PORT_END
$FIND_PORT:
	load	r1, _PORT_MAP[i]
	jump	z, $FOUND_PORT
	add		i, 1
$FIND_PORT_END:
	cmp		i, maxPort
	jump	lt, $FIND_PORT
// all ports used
	#return	-1
	jump	$FINIS
$FOUND_PORT:
	move	foundPort, i
	
	cmp		m, READ_MODE
	jump	nz, @NOT_READ_MODE
	move	r1, foundPort
	move	r2, fn
	int		iOPEN_RAW_FILE_READ
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	$FINIS
$NOT_READ_MODE:
	cmp		m, WRITE_MODE
	jump	nz, $NOT_WRITE_MODE
	move	r1, foundPort
	move	r2, fn
	int		iOPEN_RAW_FILE_WRITE
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	$FINIS
$NOT_WRITE_MODE:
	cmp		m, APPEND_MODE
	jump	nz, @NOT_APPEND_MODE
	move	r1, foundPort
	move	r2, fn
	int		iOPEN_RAW_FILE_APPEND
	move	r0, foundPort
	store	TRUE, _PORT_MAP[foundPort]
	jump	$FINIS
$NOT_APPEND_MODE:
	move	r0, -1
$FINIS:
	restore	r1,r2
#end_func

#def_func	closeFile(port)
	load	r1, port
	store	FALSE, _PORT_MAP[r0]
	int		iCLOSE_FILE
#end_func

#def_func	flush(port)
	load	r1, port
	int		iFLUSH
#end_func

#def_func	deleteFile(filespec)
	load	r1, filespec
	int		iDELETE_FILE
#end_func

#def_func	makeDirectory(filespec)
	load	r1, filespec
	int		iMAKE_DIR
#end_func

#def_func	deleteDirectory(filespec)
	load	r1, filespec
	int		iDELETE_DIR
#end_func

#def_func	isDirectory(filespec)
	load	r1, filespec
	int		iIS_DIR
#end_func

#def_func	isFile(filespec)
	load	r1, filespec
	int		iIS_FILE
#end_func

#def_func	fileExists(filespec)
	load	r1, filespec
	int		iFILE_EXISTS
#end_func

#def_func	listFiles(filespec)
	load	r1, filespec
	int		iFILES
#end_func

#def_func	tempDirectory(prefix)
	load	r1, prefix
	int		iTEMP_DIR
#end_func

#def_func	tempFile(prefix, suffix)
	save	r1,r2
	load	r1, prefix
	load	r2, suffix
	int		iTEMP_FILE
	restore	r1,r2
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
*/
IO_ASM_END:	nop
