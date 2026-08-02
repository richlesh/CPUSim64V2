///////////////////////////////////////////////////////////////////////////////
// Input/Output (Run in Debug Mode)
//
// Tests the Input/Output instructions.
//
//	IN		XZZ,XRR,XRZ,XZR
//	OUT		QZZ,QRR,QRZ,QZR
//
// Author: Richard Lesh
// Modified: 2026/06/17
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/debug.asm>

	#call main()
	stop

#def_func main()
	#var	port, value, i
	#fvar	fvalue, fexpected

	#call	openRawFile("test.bin", WRITE_MODE)
	move	port, r0
	
	// output 1 byte values 0-256
	#for 0, i, lt, 256, 1
		out	0x12, 1, port
	#end_for
	
	// output 2 byte values
	move	value, 0x1234
	#for 0, i, lt, 256, 1
		out	value, 2, port
		add	value, 1
	#end_for
	
	// output 4 byte values
	move	r1, 4
	move	value, 0x12345678
	#for 0, i, lt, 256, 1
		out	value, r1, port
		add	value, 1
	#end_for
	
	// output 8 byte values
	move	r1, 8
	load	fvalue, 123456789.
	#for 0, i, lt, 256, 1
		out	fvalue, r1, port
		add	fvalue, 1
	#end_for
	
	#call	closeFile(port)

	#call	openRawFile("test.bin", READ_MODE)
	move	port, r0
	
	// input 1 byte values 0-256
	#for 0, i, lt, 256, 1
		in	value, 1, port
		#macro ASSERT_EQ(0x12, value, "1 byte value read error")
	#end_for
	
	// input 2 byte values
	#for 0x1234, i, lt, 0x1234 + 256, 1
		in	value, 2, port
		#macro ASSERT_EQ(i, value, "2 byte value read error")
	#end_for
	
	// input 4 byte values
	move	r1, 4
	#for 0x12345678, i, lt, 0x12345678 + 256, 1
		in	value, r1, port
		#macro ASSERT_EQ(i, value, "4 byte value read error")
	#end_for
	
	// input 8 byte values
	move	r1, 8
	load	fexpected, 123456789.
	#for 0, i, lt, 256, 1
		in	fvalue, r1, port
		#macro ASSERT_EQ_FP(fexpected, fvalue, "8 byte value read error")
		add	fexpected, 1
	#end_for
	
	#call	closeFile(port)

#end_func

	stop
	stop
