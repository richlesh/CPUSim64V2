#include <system/io.asm>
#include <system/math.def>
#include <system/string.asm>
#include <system/system.asm>

	#call	main()
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Tests the IO interrupts.
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	move	r0, STDOUT
	move	r1, "IO Interrupts Tests\n"
	int		iPUTS
	
	move	r0, STDOUT
	int		iPUT_NL

// Test iPUT_INT	
	move	r0, STDOUT
	move	r1, 326
	move	r2, 2
	int		iPUT_INT
	move	r0, STDOUT
	int		iPUT_NL

// Test iPUT_DEC	
	move	r0, STDOUT
	move	r1, 326
	int		iPUT_DEC
	move	r0, STDOUT
	int		iPUT_NL

// Test iPUT_HEX	
	move	r0, STDOUT
	move	r1, 326
	int		iPUT_HEX
	move	r0, STDOUT
	int		iPUT_NL

// Test iGETS
	load	r0, SMALL_GETS_BUFFER[-1]
	#call	fprintf(STDOUT, "Small buffer: %8x (%d)\n", SMALL_GETS_BUFFER, r0)
	move	r0, STDOUT
	move	r1, "What is your quest? "
	int		iPUTS
	move	r0, STDIN
	move	r1, SMALL_GETS_BUFFER
	int		iGETS
	move	r0, STDOUT
	int		iPUTS
	int		iPUT_NL
	load	r0, r1[-1]
	#call	fprintf(STDOUT, "Final buffer: %8x (%d)\n", r1, r0)
	move	r0, r1
	int		iFREE
	
	// Test iGET_INT
	move	r0, STDOUT
	move	r1, "What is your favorite binary number? "
	int		iPUTS
	move	r0, STDIN
	move	r1, 2
	int		iGET_INT
	move	r1, r0
	move	r0, STDOUT
	move	r2, 2
	int		iPUT_INT
	move	r0, STDOUT
	int		iPUT_NL

	// Test iGET_DEC
	move	r0, STDOUT
	move	r1, "What is your favorite decimal number? "
	int		iPUTS
	move	r0, STDIN
	int		iGET_DEC
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_DEC
	move	r0, STDOUT
	int		iPUT_NL

	// Test iGET_HEX
	move	r0, STDOUT
	move	r1, "What is your favorite hexadecimal number? "
	int		iPUTS
	move	r0, STDIN
	int		iGET_HEX
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_HEX
	move	r0, STDOUT
	int		iPUT_NL

	// Test iGET_FP
	move	r0, STDOUT
	move	r1, "What is your favorite floating point number? "
	int		iPUTS
	move	r0, STDIN
	int		iGET_FP
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_FP
	move	r0, STDOUT
	int		iPUT_NL

	// Test iPRINTF
	int		iPI
	push	f0
	push	326
	push	'\u263a'
	push	"Hello"
	push	"%s %c %x %g\n"
	push	STDOUT
	call	fprintf

	// Test iOPEN_FILE_WRITE
	move	r0, 3
	move	r1, "test.txt"
	int		iOPEN_FILE_WRITE
	move	r0, 3
	move	r1, 326
	int		iPUT_DEC
	move	r0, 3
	int		iPUT_NL
	move	r0, 3
	int		iCLOSE_FILE

	// Test iOPEN_FILE_READ
	move	r0, 3
	move	r1, "test.txt"
	int		iOPEN_FILE_READ
	move	r0, 3
	int		iGET_DEC
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_DEC
	move	r0, STDOUT
	int		iPUT_NL
	move	r0, 3
	int		iCLOSE_FILE

	// Test iOPEN_FILE_APPEND
	move	r0, 3
	move	r1, "test.txt"
	int		iOPEN_FILE_APPEND
	move	r0, 3
	move	r1, 623
	int		iPUT_DEC
	move	r0, 3
	int		iPUT_NL
	move	r0, 3
	int		iCLOSE_FILE

	// Test iOPEN_FILE_READ
	move	r0, 3
	move	r1, "test.txt"
	int		iOPEN_FILE_READ
	move	r0, 3
	int		iGET_DEC
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_DEC
	move	r0, STDOUT
	int		iPUT_NL
	move	r0, 3
	int		iGET_DEC
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_DEC
	move	r0, STDOUT
	int		iPUT_NL
	move	r0, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_WRITE
	move	r0, 3
	move	r1, "test.bin"
	int		iOPEN_RAW_FILE_WRITE
	out		LONG, 3, 326
	move	r0, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_READ
	move	r0, 3
	move	r1, "test.bin"
	int		iOPEN_RAW_FILE_READ
	in		r0, LONG, 3
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_DEC
	move	r0, STDOUT
	int		iPUT_NL
	move	r0, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_APPEND
	move	r0, 3
	move	r1, "test.bin"
	int		iOPEN_RAW_FILE_APPEND
	int		iPI
	OUT		DOUBLE, 3, f0
	move	r0, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_READ
	move	r0, 3
	move	r1, "test.bin"
	int		iOPEN_RAW_FILE_READ
	in		r0, LONG, 3
	move	r1, r0
	move	r0, STDOUT
	int		iPUT_DEC
	move	r0, STDOUT
	int		iPUT_NL
	in		f0, DOUBLE, 3
	move	r0, STDOUT
	int		iPUT_FP
	move	r0, STDOUT
	int		iPUT_NL
	move	r0, 3
	int		iCLOSE_FILE

	// Test Filesystem interrupts
	#call	tempDirectory("CPUSim64")
	move	r5, r0
	#call	fprintf(STDOUT, "Created temp directory: %s\n", r5)
	#call	fileExists(r5)
	#call	fprintf(STDOUT, "Dir exists: %d\n", r0)
	#call	deleteDirectory(r5)
	#call	fprintf(STDOUT, "Deleted temp directory: %d\n", r0)
	#call	fileExists(r5)
	#call	fprintf(STDOUT, "Dir exists: %d\n", r0)
	
	#call	tempFile("CPUSim64",".txt")
	move	r5, r0
	#call	fprintf(STDOUT, "Created temp file: %s\n", r5)
	#call	fileExists(r5)
	#call	fprintf(STDOUT, "File exists: %d\n", r0)
	#call	deleteFile(r5)
	#call	fprintf(STDOUT, "Deleted temp file: %d\n", r0)
	#call	fileExists(r5)
	#call	fprintf(STDOUT, "File exists: %d\n", r0)

	#call	makeDirectory("temp")
	move	r5, r0
	#call	fprintf(STDOUT, "Created directory: temp %d\n", r5)
	#call	fileExists("temp")
	#call	fprintf(STDOUT, "Dir exists: %d\n", r0)
	
	#call	openTextFile("temp/temp.txt", WRITE_MODE)
	move	r5, r0
	#call	fprintf(STDOUT, "Created temp file: temp/temp.txt\n")
	#call	fileExists("temp/temp.txt")
	#call	fprintf(STDOUT, "File exists: %d\n", r0)
	#call	closeFile(r5)
	#call	openTextFile("temp/temp2.txt", WRITE_MODE)
	move	r5, r0
	#call	fprintf(STDOUT, "Created temp file: temp/temp2.txt\n")
	#call	fileExists("temp/temp2.txt")
	#call	fprintf(STDOUT, "File exists: %d\n", r0)
	#call	closeFile(r5)
	#call	listFiles("temp")
	move	r5, r0
	load	r4, r0[-1]
	#call	fprintf(STDOUT, "listFiles: %d\n", r4)
	#for	r1, 0, lt, r4, 1
		load	r2, r5[r1]
		#cond	r2, eq, 0
			#break
		#endcond
		#call	puts(r2)
		#call	put_nl()
	#endfor
	#call	freeArrayOfStrings(r5)
#end_func

SMALL_GETS_BUFFER: dca 10
	stop
	stop

