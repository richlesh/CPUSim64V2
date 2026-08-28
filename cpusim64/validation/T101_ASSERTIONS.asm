///////////////////////////////////////////////////////////////////////////////
// Assertion Functionality Test
//
// Test the debug.asm assertions.
//
// Author: Richard Lesh
// Modified: 2022/10/30
///////////////////////////////////////////////////////////////////////////////

#include <system/debug.def>
#include <system/debug.asm>

BEGIN:
		#macro	SET_EXIT_ON_ASSERT_FAILURE(0)		// No exit on assertion failures
		move	r1, 326
		load	f1, 3.14159267
		#macro	DEBUG_MSG("Debug Int: %d", r1)
		#macro	DEBUG_MSG("Debug FP: %f", f1)
		#macro	PRINTCPU()
		
		move	r1, 326
		#macro	assert_is_true(r1, "true(true) failed!")
		#macro	assert_is_false(r1, "false(true) failed!")
		move	r1, 0
		#macro	assert_is_true(r1, "true(false) failed!")
		#macro	assert_is_false(r1, "false(false) failed!")
		
		move	r1, 1
		move	r2,	2
		
		// test when r1 < r2
		#macro	assert_eq(r1, r2, "equal failed!")
		#macro	assert_ne(r1, r2, "not equal failed!")
		#macro	assert_lt(r1, r2, "less than failed!")
		#macro	assert_le(r1, r2, "less than or equal failed!")
		#macro	assert_gt(r1, r2, "greater than failed!")
		#macro	assert_ge(r1, r2, "greater than or equal failed!")

		// test when r2 > r1
		#macro	assert_eq(r2, r1, "equal failed!")
		#macro	assert_ne(r2, r1, "not equal failed!")
		#macro	assert_lt(r2, r1, "less than failed!")
		#macro	assert_le(r2, r1, "less than or equal failed!")
		#macro	assert_gt(r2, r1, "greater than failed!")
		#macro	assert_ge(r2, r1, "greater than or equal failed!")

		// test when r1 == r1
		#macro	assert_eq(r1, r1, "equal failed!")
		#macro	assert_ne(r1, r1, "not equal failed!")
		#macro	assert_lt(r1, r1, "less than failed!")
		#macro	assert_le(r1, r1, "less than or equal failed!")
		#macro	assert_gt(r1, r1, "greater than failed!")
		#macro	assert_ge(r1, r1, "greater than or equal failed!")

		move	f1, 1
		move	f2,	2
		
		// test when f1 < f2
		#macro	assert_eq_fp(f1, f2, "equal failed!")
		#macro	assert_ne_fp(f1, f2, "not equal failed!")
		#macro	assert_lt_fp(f1, f2, "less than failed!")
		#macro	assert_le_fp(f1, f2, "less than or equal failed!")
		#macro	assert_gt_fp(f1, f2, "greater than failed!")
		#macro	assert_ge_fp(f1, f2, "greater than or equal failed!")

		// test when f2 > f1
		#macro	assert_eq_fp(f2, f1, "equal failed!")
		#macro	assert_ne_fp(f2, f1, "not equal failed!")
		#macro	assert_lt_fp(f2, f1, "less than failed!")
		#macro	assert_le_fp(f2, f1, "less than or equal failed!")
		#macro	assert_gt_fp(f2, f1, "greater than failed!")
		#macro	assert_ge_fp(f2, f1, "greater than or equal failed!")

		// test when f1 == f1
		#macro	assert_eq_fp(f1, f1, "equal failed!")
		#macro	assert_ne_fp(f1, f1, "not equal failed!")
		#macro	assert_lt_fp(f1, f1, "less than failed!")
		#macro	assert_le_fp(f1, f1, "less than or equal failed!")
		#macro	assert_gt_fp(f1, f1, "greater than failed!")
		#macro	assert_ge_fp(f1, f1, "greater than or equal failed!")
END:	stop
		stop
