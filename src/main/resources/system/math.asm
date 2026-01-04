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

#include <system/math.def>
#include <system/system.def>

MATH_ASM_START:
jump	MATH_ASM_END

#def_func	pi()
	int		iPI
#end_func

#def_func	ln_base()
	int		iE
#end_func

#def_func	idiv(i, j)
	#var	fi, fj
	load	fi, i
	load	fj, j
	div		r0, r1, fi, fj
#end_func

#def_func	mod(i, j)
	#var	fi, fj
	load	fi, i
	load	fj, j
	div		r0, r1, fi, fj
	move	r0, r1
#end_func

#def_func	fabs(x)
	load	f1, x
	int		iABS_FP
#end_func	fabs

#def_func	abs(i)
	load	r1, i
	int		iABS
#end_func	abs

#def_func	ceil(x)
	load	f1, x
	int		iCEIL
#end_func	ceil

#def_func	floor(x)
	load	f1, x
	int		iFLOOR
#end_func	floor

#def_func	round(x)
	load	f1, x
	int		iROUND
#end_func	round

#def_func	sqrt(x)
	load	f1, x
	int		iSQRT
#end_func	sqrt

#def_func	exp(x)
	load	f1, x
	int		iEXP
#end_func	exp

#def_func	log(x)
	load	f1, x
	int		iLOG
#end_func	log

#def_func	exp10(x)
	load	f1, 10.
	load	f2, x
	int		iPOW
#end_func	exp10

#def_func	log10(x)
	load	f1, x
	int		iLOG
	load	f1, 2.3025850929940456840179914546844
	div		f0,f1
#end_func	log10

#def_func	exp2(x)
	move	f1, 2.
	load	f2, x
	int		iPOW
#end_func	exp10

#def_func	log2(x)
	load	f1, x
	int		iLOG
	load	f1, 0.693147180559945309417232121458
	div		f0, f1
#end_func	log10

#def_func	pow(x,y)
	load	f1, x
	load	f2, y
	int		iPOW
#end_func	pow

// base is integer, power is integer (non-negtive)
#def_func	ifastpow(base, power)
	#var	product, square, b, p
	load	b, base
	load	p, power
	move	square, b
	move	product, 1
	jump	$LOOP_END
$LOOP_START:
	and		r0, p, 0x1
	jump	z, $LOOP_NEXT
	mult	product, square
$LOOP_NEXT:
	mult	square, square
	rshift	p, 1
$LOOP_END:
	test	p
	jump	nz, $LOOP_START
	move	r0, product
#end_func	ifastpow

// base is FP, power is integer (non-negtive)
#def_func	fastpow(base, power)
	#var	p, negPower
	#fvar	product, square, b
	load	b, base
	load	p, power
	move	negPower, FALSE
	test	p
	jump	nn, $SKIP_NEG
	neg		p
	move	negPower, TRUE
$SKIP_NEG:
	move	square, b
	move	product, 1
	jump	$LOOP_END
$LOOP_START:
	and		r0, p, 0x1
	jump	z, $LOOP_NEXT
	mult	product, square
$LOOP_NEXT:
	mult	square, square
	rshift	p, 1
$LOOP_END:
	test	p
	jump	nz, $LOOP_START
	move	f0, product
	test	negPower
	jump	z, $SKIP_NEG2
	recip	f0
$SKIP_NEG2:
	move	f0, product
#end_func	fastpow

#def_func	random()
	int		iRANDOM
#end_func	random

#def_func	rand(low,high)
	load	r1, low
	load	r2, high
	int		iRAND
#end_func	random

///////////////////////////////////////////////////////////////////////////////
// min(first, second) computes minimum of two integer values
// a		first integer value
// b		second integer value
// returns result in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	min(first, second)	// This pushes first and second on the stack
	#var	a,b					// declare two named integer registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		lt, r0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// max(first, second) computes minimum of two integer values
// a		first integer value
// b		second integer value
// returns result in r0
///////////////////////////////////////////////////////////////////////////////
#def_func	max(first, second)	// This pushes first and second on the stack
	#var	a,b					// declare two named integer registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		gt, r0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// fmin(first, second) computes minimum of two FP values
// a		first FP value
// b		second FP value
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	fmin(first, second)	// This pushes first and second on the stack
	#fvar	a,b					// declare two named FP registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		lt, f0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// fmax(first, second) computes minimum of two FP values
// a		first FP value
// b		second FP value
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	fmax(first, second)	// This pushes first and second on the stack
	#fvar	a,b					// declare two named FP registers
								// and saves original values on stack
	load	a, first			// because the arguments are on the stack we
	load	b, second			// must load them from memory into a register
	cmp		a, b
	mov		gt, f0, a, b
#end_func						// This cleans up the stack and returns

///////////////////////////////////////////////////////////////////////////////
// sum(array) computes the sum of an INTEGER array
// array	address of INTEGER array to sum
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	sum(array)
	#var	i, max, addr, sum
	load	addr, array			// Load array argument from stack
	load	max, addr[0]		// Load count argument from array
	clear	sum
	move	i, 1
	jump	$END_LOOP
$LOOP:
	load	r0, addr[i]			// Load what is in addr[i] to temp f0
	add		sum, r0
	add		i, 1
$END_LOOP:
	cmp		i, max
	jump	le, $LOOP
	#return	sum
#end_func

///////////////////////////////////////////////////////////////////////////////
// fsum(array) computes the sum of an FP array
// array	address of FP array to sum
// returns result in f0
///////////////////////////////////////////////////////////////////////////////
#def_func	fsum(array)
	#var	i, max, addr
	#fvar	sum
	load	addr, array			// Load array argument from stack
	load	max, addr[0]		// Load count argument from array
	clear	sum
	move	i, 1
	jump	$END_LOOP
$LOOP:
	load	f0, addr[i]			// Load what is in addr[i] to temp f0
	add		sum, f0
	add		i, 1
$END_LOOP:
	cmp		i, max
	jump	le, $LOOP
	#freturn	sum
#end_funcs

MATH_ASM_END: nop
