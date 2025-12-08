#include <system/io.asm>

MAIN:
	#call	putline("\b\t\n\r\f\0")
	#call	putline("!مرحبا أيها العالم")
	#call	putline("你好, 世界!")
	#call	putline("Hello, world!")
	#call	putline("Bonjour le monde!")
	#call	putline("Hallo welt!")
	#call	putline("Γειά σου Κόσμε!")
	#call	putline("!שלום העולם")
	#call	putline("नमस्ते दुनिया!")
	#call	putline("こんにちは世界!")
	#call	putline("안녕, 월드!")
	#call	putline("Привет, мир!")
	#call	putline("¡Hola mundo!")
	stop
	stop
ARRAY:	.DCA	5
INTEGER:.DCI	326
HEX1:	.DCI	0x1000
FLOAT:	.DCF	3.1415
WORD1:	.DCW	1,2,0xff,0x314
CHAR1:  .DCC	'a', 'b', 52
