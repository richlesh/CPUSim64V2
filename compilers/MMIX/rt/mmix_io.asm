// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// mmix_io.asm - the assembly functions of the MMIX run-time library.
//
// mmix_rt.c (compiled into mmix_rt.asm) calls these with the C11 compiler's
// convention: arguments on the stack at sf[3], sf[4], ..., the result in r0,
// only r0-r4 changed.  A translated MMIX program includes this file after
// mmix_rt.asm.
///////////////////////////////////////////////////////////////////////////////

// mmx_putb(port, b): writes the byte b to a port
__c_mmx_putb:
	load	r1, sf[3]
	load	r2, sf[4]
	out	r2, BYTE, r1
	return

// mmx_getb(port): the next byte from a port, or -1 at the end
__c_mmx_getb:
	load	r1, sf[3]
	in	r0, BYTE, r1
	return

// mmx_open(name): opens the file (packed string) for random access; the
// port, or -1
__c_mmx_open:
	load	r1, sf[3]
	#call	openRandomAccessFile(r1)
	return

// mmx_isfile(name): TRUE if name is a file
__c_mmx_isfile:
	load	r1, sf[3]
	int	iIS_FILE
	return

// mmx_delete(name): deletes the file
__c_mmx_delete:
	load	r1, sf[3]
	int	iDELETE_FILE
	return

// mmx_close(port)
__c_mmx_close:
	load	r1, sf[3]
	#call	closeFile(r1)
	move	r0, 0
	return

// mmx_seek(port, pos): the new position, or -1
__c_mmx_seek:
	load	r1, sf[3]
	load	r2, sf[4]
	int	iSEEK
	return

// mmx_tell(port): the position, or -1
__c_mmx_tell:
	load	r1, sf[3]
	int	iTELL
	return

// mmx_alloc(words): a zeroed block of memory, or 0
__c_mmx_alloc:
	load	r1, sf[3]
	int	iALLOC
	test	r0
	jump	z, __mmx_alloc_done
	push	r0
	load	r2, sf[3]
	move	r1, r0
	int	iMEMCLEAR
	pop	r0
__mmx_alloc_done:
	return

// mmx_exit(code): ends the program
__c_mmx_exit:
	load	r1, sf[3]
	int	iEXIT
	return

// mmx_argc(): the number of command line arguments, with the program name
__c_mmx_argc:
	int	iARGC
	return

// mmx_arg(i): command line argument i (a packed string)
__c_mmx_arg:
	load	r1, sf[3]
	int	iARGS
	return
