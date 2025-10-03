///////////////////////////////////////////////////////////////////////////////
// Assertion Functionality Test
//
// Test the debug.asm routines
//
// Author: Richard Lesh
// Modified: 2022/10/30
///////////////////////////////////////////////////////////////////////////////

#include <system/debug.asm>

BEGIN:	#call	enable_assertion_exit(0)		// No exit on assertion failures
		move	r1, 326
		move	f1, 3.14159267
		debug_msg("Debug Int:",r1)
		debug_fp_msg("Debug FP:",f1)
		printCPU()
		
		move	r1, 326
		assert_true(r1, "true(true) failed!")
		assert_false(r1, "false(true) failed!")
		move	r1, 0
		assert_true(r1, "true(false) failed!")
		assert_false(r1, "false(false) failed!")
		
		move	r1, 1
		move	r2,	2
		
		// test when r1 < r2
		assert_eq(r1, r2, "equal failed!")
		assert_ne(r1, r2, "not equal failed!")
		assert_lt(r1, r2, "less than failed!")
		assert_le(r1, r2, "less than or equal failed!")
		assert_gt(r1, r2, "greater than failed!")
		assert_ge(r1, r2, "greater than or equal failed!")

		// test when r2 > r1
		assert_eq(r2, r1, "equal failed!")
		assert_ne(r2, r1, "not equal failed!")
		assert_lt(r2, r1, "less than failed!")
		assert_le(r2, r1, "less than or equal failed!")
		assert_gt(r2, r1, "greater than failed!")
		assert_ge(r2, r1, "greater than or equal failed!")

		// test when r1 == r1
		assert_eq(r1, r1, "equal failed!")
		assert_ne(r1, r1, "not equal failed!")
		assert_lt(r1, r1, "less than failed!")
		assert_le(r1, r1, "less than or equal failed!")
		assert_gt(r1, r1, "greater than failed!")
		assert_ge(r1, r1, "greater than or equal failed!")

		move	f1, 1
		move	f2,	2
		
		// test when f1 < f2
		assert_eq_fp(f1, f2, "equal failed!")
		assert_ne_fp(f1, f2, "not equal failed!")
		assert_lt_fp(f1, f2, "less than failed!")
		assert_le_fp(f1, f2, "less than or equal failed!")
		assert_gt_fp(f1, f2, "greater than failed!")
		assert_ge_fp(f1, f2, "greater than or equal failed!")

		// test when f2 > f1
		assert_eq_fp(f2, f1, "equal failed!")
		assert_ne_fp(f2, f1, "not equal failed!")
		assert_lt_fp(f2, f1, "less than failed!")
		assert_le_fp(f2, f1, "less than or equal failed!")
		assert_gt_fp(f2, f1, "greater than failed!")
		assert_ge_fp(f2, f1, "greater than or equal failed!")

		// test when f1 == f1
		assert_eq_fp(f1, f1, "equal failed!")
		assert_ne_fp(f1, f1, "not equal failed!")
		assert_lt_fp(f1, f1, "less than failed!")
		assert_le_fp(f1, f1, "less than or equal failed!")
		assert_gt_fp(f1, f1, "greater than failed!")
		assert_ge_fp(f1, f1, "greater than or equal failed!")
END:	stop
		stop
