#include <system/io.asm>

#define	MY_INT	3261963
#define MY_PI	3.14159265358979323846

PROGRAM_START:
	mov		r1, MY_INT
	load 	r2, @CONST_INT
	mov 	r3, MY_INT
	load 	r4, @CONST_INT
	
	#call	put_dec(r1)
	#call	puts(" 0x")
	#call	put_hex(r3)
	#call	puts(" 0b")
	#call	put_int(3261963,2)
	#call	put_nl()
	#call	put_dec(r2)
	#call	puts(" 0x")
	#call	put_hex(r4)
	#call	puts(" 0b")
	#call	put_int(326,2)
	#call	put_nl()

	mov		f1, MY_PI
	load 	f2, @CONST_FP
	mov 	f3, MY_PI
	load 	f4, @CONST_FP
	
	#call	put_fp(f1)
	#call	putc(' ')
	#call	put_fp(f2)
	#call	put_nl()
	#call	put_fp(f3)
	#call	putc(' ')
	#call	put_fp(f4)
	#call	put_nl()

	#call	puts(CONST_STR)
	load	r1, CONST_CHAR
	#call	putc(r1)
	#call	put_nl()

	clear	r1					// loop variable
	move	r2, @INT_ARRAY		// address of array
	load	r3, @INT_ARRAY[-1]	// length of array
	jump	@END_LOOP1
LOOP1:
	load	r4, r2[r1]
	#call	put_dec(r4)
	#call	putc(' ')
	add		r1, 1
END_LOOP1:
	cmp		r1, r3
	jump	ne, @LOOP1
	#call	put_nl()

	move	r2, @FP_ARRAY		// address of array
	load	f0, r2[0]
	#call	put_fp(f0)
	#call	putc(' ')
	load	f0, r2[1]
	#call	put_fp(f0)
	#call	putc(' ')
	load	f0, r2[2]
	#call	put_fp(f0)
	#call	putc(' ')
	load	f0, r2[3]
	#call	put_fp(f0)
	#call	putc(' ')
	load	f0, r2[4]
	#call	put_fp(f0)
	#call	put_nl()

	stop
	stop

	CONST_INT:	DCI	326
	CONST_FP:	DCF	3.1415
	CONST_CHAR:	DCC '\u263A'
	CONST_STR:	DCS "Hello, Rich!"
	INT_ARRAY:	DCA	1,1,2,3,5,8,13,21
	FP_ARRAY:	DCA	1.00, 1.60, 2.50, 4.00, 6.30

