///////////////////////////////////////////////////////////////////////////////
// Pack/Unpack (Run in Debug Mode)
//
// Tests the Pack/Unpack instructions.
//
//	PACK	RR, RRRR
//	PACK64	RR, RRRR
//	UNPACK	RR, RRRR
//	UNPACK64	RR, RRRR
//
// Author: Richard Lesh
// Modified: 2026/06/17
///////////////////////////////////////////////////////////////////////////////

#include <system/debug.asm>

	#call main()
	stop

#def_func main()
	move	r1, 0x1234
	move	r2, 0x5678
	pack 	r1, r2
	#macro ASSERT_EQ(r1, 0x12345678, "2x 16-bit pack failed")

	clear 	r2
	unpack	r1, r2
	#macro ASSERT_EQ(r1, 0x1234, "2x 16-bit unpack failed")
	#macro ASSERT_EQ(r2, 0x5678, "2x 16-bit unpack failed")

	move	r1, 0x12
	move	r2, 0x34
	move	r3, 0x56
	move	r4, 0x78
	pack 	r1, r2, r3, r4
	#macro ASSERT_EQ(r1, 0x12345678, "4x 8-bit pack failed")

	clear	r2, r3, r4
	unpack	r1, r2, r3, r4
	#macro ASSERT_EQ(r1, 0x12, "4x 8-bit unpack failed")
	#macro ASSERT_EQ(r2, 0x34, "4x 8-bit unpack failed")
	#macro ASSERT_EQ(r3, 0x56, "4x 8-bit unpack failed")
	#macro ASSERT_EQ(r4, 0x78, "4x 8-bit unpack failed")

	move	r1, 0x12345678
	move	r2, 0x90abcdef
	pack64 	r1, r2
	load	r0, EXPECTED
	#macro ASSERT_EQ(r1, r0, "2x 32-bit pack failed")

	clear 	r2
	unpack64	r1, r2
	#macro ASSERT_EQ(r1, 0x12345678, "2x 32-bit unpack failed")
	#macro ASSERT_EQ(r2, 0x90abcdef, "2x 32-bit unpack failed")

	move	r1, 0x1234
	move	r2, 0x5678
	move	r3, 0x90ab
	move	r4, 0xcdef
	pack64 	r1, r2, r3, r4
	load	r0, EXPECTED
	#macro ASSERT_EQ(r1, r0, "4x 16-bit pack failed")

	clear	r2, r3, r4
	unpack64	r1, r2, r3, r4
	#macro ASSERT_EQ(r1, 0x1234, "4x 16-bit unpack failed")
	#macro ASSERT_EQ(r2, 0x5678, "4x 16-bit unpack failed")
	#macro ASSERT_EQ(r3, 0x90ab, "4x 16-bit unpack failed")
	#macro ASSERT_EQ(r4, 0xcdef, "4x 16-bit unpack failed")
#end_func

	stop
	stop
	
EXPECTED: .dci 0x1234567890abcdef
