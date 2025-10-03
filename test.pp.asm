.LINE «test.asm», 6
			START:
			move	r2, 123456789012345
			buckwheat
			.DCW -326
			spanky
			.DCW 0x1234, 0x5678, 0x9ABC, 0xDEF0
			alfalfa
			.DCW 3.1415926, -2.7182818, 0.
			.DCB 0x61, 0xA, 0xE9, 0x1234
.LINE_BEGIN «test.asm», 15
push r3
push r2
push r1
call func
add sp, 3
.LINE_END
.LINE «test.asm», 16
			MOVE	r2, 123
			darla
			FINIS:

