// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <system/io.def>
#include <system/system.def>
#include <system/string.def>
#include <system/thread.asm>

IO_ASM_START:
#macro DEFINE_SHARED_RECURSIVE_SPINLOCK(STDOUT_LOCK_HANDLE)
load	r0, STDOUT_LOCK_HANDLE
#call initializeRecursiveSpinLock(r0)
#macro DEFINE_SHARED_RECURSIVE_SPINLOCK(PORTMAP_LOCK_HANDLE)
load	r0, PORTMAP_LOCK_HANDLE
#call initializeRecursiveSpinLock(r0)

#global __PORT_MAP_HANDLE:   .dci    0
#macro	ALLOC_SHARED(4)			// Must match Architecture.NUM_PORTS / 64
store	r0, __PORT_MAP_HANDLE
#call	__setPortMap(0)			// STDIN
#call	__setPortMap(1)			// STDOUT
#call	__setPortMap(2)			// STDERR

jump	IO_ASM_END

#def_func	__setPortMap(bit)
	#var	b, addr, offset, mask, lock
	load	b, bit
	#macro	COMPARE_RANGE(3, le, b, lt, 256)
	#if_cond_sr z
	#return FALSE
	#end_cond
	load	lock, PORTMAP_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(lock)
	load	addr, __PORT_MAP_HANDLE
	div		offset, mask, b, 64
	debug	b, addr, offset, mask
	lsh		mask, 1, mask
	load	r0, addr[offset]
	or		r0, mask
	store	r0, addr[offset]
	#call	releaseRecursiveSpinLock(lock)
	#return	TRUE
#end_func

#def_func	__unsetPortMap(bit)
	#var	b, addr, offset, mask, lock
	load	b, bit
	#macro	COMPARE_RANGE(3, le, b, lt, 256)
	#if_cond_sr z
		#return FALSE
	#end_cond
	load	lock, PORTMAP_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(lock)
	load	addr, __PORT_MAP_HANDLE
	div		offset, mask, b, 64
	lsh		mask, 1, mask
	compl	mask
	load	r0, addr[offset]
	and		r0, mask
	store	r0, addr[offset]
	#call	releaseRecursiveSpinLock(lock)
	#return	TRUE
#end_func

#def_func	__getPortMap(bit)
	#var	b, addr, offset, mask
	load	b, bit
	#macro	COMPARE_RANGE(3, le, b, lt, 256)
	#if_cond_sr z
		#return TRUE
	#end_cond
	load	addr, __PORT_MAP_HANDLE
	div		offset, mask, b, 64
	lsh		mask, 1, mask
	load	r0, addr[offset]
	and		r0, mask
	move	z, r0, FALSE, TRUE
#end_func

#def_func	__getFreePort()
	#var	i, result, lock
	load	lock, PORTMAP_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(lock)
	move	result, -1
	#for	3, i, lt, 256, 1
		#call	__getPortMap(i)
		test	r0
		#if_cond_sr z
			move	result, i
			#call	__setPortMap(i)
			#break
		#end_cond_sr
	#end_for
	#call	releaseRecursiveSpinLock(lock)
	#return	result
#end_func

// PUTS
// sends a string to STDOUT atomically
// str	String Address
#def_func puts(str)
	save	r1, r2
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	load	r2, str
	mov		r1, STDOUT
	int		iPUTS
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	restore	r1, r2
#end_func

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
#end_func

// PUTLINE
// sends a string to STDOUT atomically and adds a newline
// str	String Address
#def_func putline(str)
	save	r1, r2
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	load	r2, str
	mov		r1, STDOUT
	int		iPUT_LINE
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	restore	r1, r2
#end_func

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
#end_func

// PUTC
// Sends the character to the STDOUT port atomically.  This outputs a 32-bit codepoint not a byte.
// value	Character to output
#def_func	putc(value)
	push	r1
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(R0)
	load	r1, value
	#macro	out0(r1, STDOUT)
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	pop		r1
#end_func

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
#end_func

// PUT_INT
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
// base		Base for formatting
#def_func	put_int(value,base)
	save	r1,r3
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	move	r1,STDOUT
	load	r2,value
	load	r3,base
	int		iPUT_INT
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	restore	r1,r3
#end_func

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
#end_func

// PUT_DEC 
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
#def_func	put_dec(value)
	save	r1,r2
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	move	r1,STDOUT
	load	r2,value
	int		iPUT_DEC
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	restore	r1,r2
#end_func

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
#end_func

// PUT_HEX
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
#def_func	put_hex(value)
	save	r1, r3
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	move	r1, STDOUT
	load	r2, value
	move	r3, 0
	int		iPUT_HEX
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	restore	r1, r3
#end_func

// PUT_HEX_SIZE
// Formats the integer value as a string and then sends to STDOUT atomically
// value	Value to format
// size		Minimum number of digits to print, pads with 0
#def_func	put_hex_size(value, size)
	save	r1, r3
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	move	r1, STDOUT
	load	r2,value
	load	r3, size
	int		iPUT_HEX
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	restore	r1, r3
#end_func

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
#end_func

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
#end_func

// PUT_FP
// Formats the IEEE754 floating point value as a string and then sends to STDOUT atomically
// fpvalue	Value to format
#def_func	put_fp(fpvalue, prec)
	save	r1, r2
	push	f1
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	move	r1, STDOUT
	load	r2, prec
	load	f1, fpvalue
	int		iPUT_FP
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	pop		f1
	restore	r1, r2
#end_func

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
#end_func

// PUT_NL
// Outputs the newline character to STDOUT atomically
// port	I/O Port

#def_func	put_nl()
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	#macro	out0('\n', STDOUT)
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
#end_func

// FPUT_NL
// Outputs the newline character to the port specified
// port	I/O Port

#def_func	fput_nl(port)
	#var	port_arg
	load	port_arg,port
	#macro	out0('\n', port_arg)
#end_func

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
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	#call	puts(str)
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
	move	r1, str
	int		iFREE
#end_func

// cond_printf(cond, fmt, values...)
// Formats the values on the stack and then sends to STDOUT atomically
// cond		Must be TRUE to print
// fmt		String with formatting information
// values	Values for formatting
#def_macro	cond_printf(b, fmt, values...)
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	#call	cond_fprintf(${cond}, STDOUT, ${fmt}, ${values}, ${...})
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
#end_macro

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
	load	r0, STDOUT_LOCK_HANDLE
	#call	acquireRecursiveSpinLock(r0)
	#call	fputs(STDERR, "\nFATAL: ")
	#call	fputs(STDERR, str)
	load	r0, STDOUT_LOCK_HANDLE
	#call	releaseRecursiveSpinLock(r0)
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
		load	r0, STDOUT_LOCK_HANDLE
		#call	acquireRecursiveSpinLock(r0)
		#call	fputs(STDERR, "\nFATAL: ")
		pop		r1
		#call	fputs(STDERR, r1)
		load	r0, STDOUT_LOCK_HANDLE
		#call	releaseRecursiveSpinLock(r0)
		move	r1, 1
		int		iEXIT
	#end_cond_sr
#end_func

// fgetline
// Read an entire line from the specified I/O port
// port		I/O Port
// Returns the (re)allocated buffer or 0 if EOF
#def_func	fgetline(port, buffer)
	load	r1, port
	load	r2, buffer
	int		iGET_LINE
	cmp		r0, -1
	move	z, r0, 0, r2
#end_func

#def_func	openTextFile(filename, mode)
	#var	fn, m, foundPort
	load	fn, filename
	load	m, mode
	// find available port in _PORT_MAP
	#call	__getFreePort()
	#if_cond	r0, eq, -1
		#return	-1				// all ports used
	#end_cond
	move	foundPort, r0
	#if_cond	m, eq, READ_MODE
		move	r1, foundPort
		move	r2, fn
		int		iOPEN_FILE_READ
		#return foundPort
	#else_if_cond	m, eq, WRITE_MODE
		move	r1, foundPort
		move	r2, fn
		int		iOPEN_FILE_WRITE
		#return foundPort
	#else_if_cond	m, eq, APPEND_MODE
		move	r1, foundPort
		move	r2, fn
		int		iOPEN_FILE_APPEND
		#return foundPort
	#else_cond
		#return	-1				// not a valid mode
	#end_cond
#end_func

#def_func	openRawFile(filename, mode)
	#var	fn, m, foundPort
	load	fn, filename
	load	m, mode
	// find available port in _PORT_MAP
	#call	__getFreePort()
	#if_cond	r0, eq, -1
		#return	-1				// all ports used
	#end_cond
	move	foundPort, r0
	#if_cond	m, eq, READ_MODE
		move	r1, foundPort
		move	r2, fn
		int		iOPEN_RAW_FILE_READ
		#return foundPort
	#else_if_cond	m, eq, WRITE_MODE
		move	r1, foundPort
		move	r2, fn
		int		iOPEN_RAW_FILE_WRITE
		#return foundPort
	#else_if_cond	m, eq, APPEND_MODE
		move	r1, foundPort
		move	r2, fn
		int		iOPEN_RAW_FILE_APPEND
		#return foundPort
	#else_cond
		#call	__unsetPortMap(foundPort)
		#return	-1				// not a valid mode
	#end_cond
#end_func

#def_func	closeFile(port)
	#var	p
	load	p, port
	#call	__unsetPortMap(p)
	move	r1, p
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

#def_func	deleteFiles(strList)
	#var	i, len, list, result
	move	result, TRUE
	load	list, strList
	load	len, list[0]
	#for	1, i, le, len, 1
		load	r1, list[i]
		int		iDELETE_FILE
		and		result, r0
	#end_for
	#return	result
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
	save	r1, r2
	load	r1, prefix
	load	r2, suffix
	int		iTEMP_FILE
	restore	r1, r2
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
	#end_while
#end_func

#def_func	copy_raw_file(fromPort, toPort)
	#var	ch, fp, tp
	load	fp, fromPort
	load	tp, toPort
	#macro	IN1(ch, fp)
	#while	ch, ne, -1
		#macro	OUT1(ch, tp)
		#macro	IN1(ch, fp)
	#end_while
#end_func

///////////////////////////////////////////////////////////////////////////////
// printIntArray(a)
// Prints an array of integers as decimal.
// a	Base address of the array to print
///////////////////////////////////////////////////////////////////////////////
#def_func printIntArray(addrArg)
	#var	len, i, addr
	load	addr, addrArg
	load	len, addr[0]
	#for	1, i <= len, 1
		#if_cond	i, ne, 1
			move	r1, STDOUT
			move	r2, ","
			int		iPUTS
		#end_cond
		move	r1, STDOUT
		load	r2, addr[i]
		int		iPUT_DEC
	#end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// printFloatArray(a)
// Prints an array of floats.
// a	Base address of the array to print
///////////////////////////////////////////////////////////////////////////////
#def_func printFloatArray(addrArg)
	#var	len, i, addr
	load	addr, addrArg
	load	len, addr[0]
	#for	1, i <= len, 1
		#if_cond	i, ne, 1
			move	r1, STDOUT
			move	r2, ","
			int		iPUTS
		#end_cond
		move	r1, STDOUT
		load	f1, addr[i]
		int		iPUT_FP
	#end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// printStringArray(a)
// Prints an array of strings.
// a	Base address of the array to print
///////////////////////////////////////////////////////////////////////////////
#def_func printStringArray(addrArg)
	#var	len, i, addr
	load	addr, addrArg
	load	len, addr[0]
	#for	1, i <= len, 1
		#if_cond	i, ne, 1
			move	r1, STDOUT
			move	r2, ","
			int		iPUTS
		#end_cond
		move	r1, STDOUT
		load	r2, addr[i]
		int		iPUTS
	#end_for
#end_func

IO_ASM_END:	nop
