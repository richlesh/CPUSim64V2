#include <system/io.asm>

#define	MY_INT	3261963
#define MY_PI	3.14159265358979323846
#define MY_CHAR '©'
#define MY_STR	"2022 All rights reserved\n"
#define MY_STR2	"¡Hola Mundo!  你好世界  שלום עולם\n"
#define MY_STR3	"\t\t\"\xc4\" \b\'\u20ac\'\r\U00263a\n"

PROGRAM_START:
	mov		r1, MY_INT		// 3261963 is substituted here
	load 	r2, @CONST_INT	// 326 is loaded from DCI at end of code
	mov 	r3, MY_INT		// 3261963 is substituted here
	load 	r4, @CONST_INT	// 326 is loaded from DCI at end of code
	
	debug	r1,r2,r3,r4

	mov		f1, MY_PI		// 3.141592... is substituted here
							// This causes the mov to be changed to a load of
							// the FP value stored at the end of code
	load 	f2, @CONST_FP	// 3.1415 is loaded from DCF at end of code
	mov 	f3, MY_PI		// 3.141592... is substituted here
							// This causes the mov to be changed to a load of
							// the FP value previously stored at the end of code
	load 	f4, @CONST_FP	// 3.1415 is loaded from DCF at end of code
	
	debug	f1,f2,f3,f4

	#call	putc(MY_CHAR)	// Character (Integer) literal is substituted here
	#call	puts(MY_STR)	// String literal is substituted here
							// This causes the string to be created at the end
							// of the code and its address placed here
	#call	puts(MY_STR2)	// Lots of Unicode characters
	#call	puts(MY_STR3)	// Lots of escaped characters
	#call	put_nl()

	#call	puts(CONST_STR)	// Address of string constant defined by DCS at the   
							// end of the code is used here
	load	r1, CONST_CHAR	// Address of character (integer) constant is used
							// here to load the character
	#call	putc(r1)
	#call	put_nl()
	
	clear	r1					// loop variable
	move	r2, @INT_ARRAY		// address of array
	load	r3, @INT_ARRAY[-1]	// length of array
	jump	@END_LOOP1
LOOP1:
	load	r4, r2[r1]			// Load from address r2 + r1
	debug	r4
	add		r1, 1
END_LOOP1:
	cmp		r1, r3
	jump	ne, @LOOP1

	move	r2, @FP_ARRAY		// address of array
	load	f0, r2[0]			// Load from address r2 + 0
	debug	f0
	load	f0, r2[1]			// Load from address r2 + 1
	debug	f0
	load	f0, r2[2]			// Load from address r2 + 2
	debug	f0
	load	f0, r2[3]			// Load from address r2 + 3
	debug	f0
	load	f0, r2[4]			// Load from address r2 + 4
	debug	f0

	stop
	stop

	CONST_INT:	DCI	326
	CONST_FP:	DCF	3.1415
	CONST_CHAR:	DCC '\u263A'
	CONST_STR:	DCS "Hello, Rich!"
	INT_ARRAY:	DCA	1,1,2,3,5,8,13,21
	FP_ARRAY:	DCA	1.00, 1.60, 2.50, 4.00, 6.30
