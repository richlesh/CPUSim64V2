///////////////////////////////////////////////////////////////////////////////
// CAS (Run in Debug Mode)
//
// Tests the CAS instructions.
//
//	CAS	RRAO, CCAO, RCAO, CRAO
//
// Author: Richard Lesh
// Modified: 2026/06/17
///////////////////////////////////////////////////////////////////////////////

#include <system/debug.asm>

	#call main()
	stop

#global ATOMIC: .dci 0

#def_func main()
	#var	atomic_addr
	move	atomic_addr, ATOMIC
	
	cas		0, 1, atomic_addr, 0
	#if_cond_sr no
		#macro ASSERT_TRUE(1, "Overflow not set")
	#end_cond
	load	r0, atomic_addr
	#macro ASSERT_EQ(1, r0, "cas failed")
	debug	ATOMIC, 1

	cas		0, 1, atomic_addr, 0
	#if_cond_sr o
		#macro ASSERT_TRUE(1, "Overflow incorrectly set")
	#end_cond
	load	r0, atomic_addr
	#macro ASSERT_EQ(1, r0, "cas failed")
	debug	ATOMIC, 1

#end_func
