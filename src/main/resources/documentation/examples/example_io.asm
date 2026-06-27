#include <system/io.asm>
#include <system/math.def>
#include <system/string.asm>
#include <system/system.asm>

	#call	main()
	move	r1, 0
	int		iEXIT

///////////////////////////////////////////////////////////////////////////////
// main
// Tests the IO interrupts.
///////////////////////////////////////////////////////////////////////////////

#def_func	main()
	move	r1, STDOUT
	move	r2, "IO Interrupts Tests\n"
	int		iPUTS
	
	move	r1, STDOUT
	int		iPUT_NL

// Test iPUT_INT	
	move	r1, STDOUT
	move	r2, 326
	move	r3, 2
	int		iPUT_INT
	move	r1, STDOUT
	int		iPUT_NL

// Test iPUT_DEC	
	move	r1, STDOUT
	move	r2, 326
	int		iPUT_DEC
	move	r1, STDOUT
	int		iPUT_NL

// Test iPUT_HEX	
	move	r1, STDOUT
	move	r2, 326
	int		iPUT_HEX
	move	r1, STDOUT
	int		iPUT_NL

// Test iGETS
	move	r1, 10
	int		iAlloc
	store	r0, SMALL_GETS_BUFFER
	load	r1, SMALL_GETS_BUFFER
	load	r1, r1[-1]
	#call	printf("Small buffer: %8x (%d)\n", SMALL_GETS_BUFFER, r1)
	move	r1, STDOUT
	move	r2, "What is your quest? "
	int		iPUTS
	move	r1, STDIN
	load	r2, SMALL_GETS_BUFFER
	int		iGETS
	store	r2, SMALL_GETS_BUFFER
	move	r1, STDOUT
	int		iPUTS
	move	r1, STDOUT
	int		iPUT_NL
	load	r1, r2[-1]
	#call	printf("Final buffer: %8x (%d)\n", r2, r1)
	move	r1, r2
	int		iFREE
	
	// Test iGET_INT
	move	r1, STDOUT
	move	r2, "What is your favorite binary number? "
	int		iPUTS
	move	r1, STDIN
	move	r2, 2
	int		iGET_INT
	move	r2, r0
	move	r1, STDOUT
	move	r3, 2
	int		iPUT_INT
	move	r1, STDOUT
	int		iPUT_NL

	// Test iGET_DEC
	move	r1, STDOUT
	move	r2, "What is your favorite decimal number? "
	int		iPUTS
	move	r1, STDIN
	int		iGET_DEC
	move	r2, r0
	move	r1, STDOUT
	int		iPUT_DEC
	move	r1, STDOUT
	int		iPUT_NL

	// Test iGET_HEX
	move	r1, STDOUT
	move	r2, "What is your favorite hexadecimal number? "
	int		iPUTS
	move	r1, STDIN
	int		iGET_HEX
	move	r2, r0
	move	r1, STDOUT
	int		iPUT_HEX
	move	r1, STDOUT
	int		iPUT_NL

	// Test iGET_FP
	move	r1, STDOUT
	move	r2, "What is your favorite floating point number? "
	int		iPUTS
	move	r1, STDIN
	int		iGET_FP
	move	r2, 6
	move	r1, STDOUT
	move	f1, f0
	int		iPUT_FP
	move	r1, STDOUT
	int		iPUT_NL

	// Test iPRINTF
	int		iPI
	push	f0
	push	326
	push	'\u{263a}'
	push	"Hello"
	push	"%s %c %x %g\n"
	push	STDOUT
	call	fprintf

	// Test iOPEN_FILE_WRITE
	move	r1, 3
	move	r2, "test.txt"
	int		iOPEN_FILE_WRITE
	move	r1, 3
	move	r2, 326
	int		iPUT_DEC
	move	r1, 3
	int		iPUT_NL
	move	r1, 3
	int		iCLOSE_FILE

	// Test iOPEN_FILE_READ
	move	r1, 3
	move	r2, "test.txt"
	int		iOPEN_FILE_READ
	move	r1, 3
	int		iGET_DEC
	move	r2, r0
	move	r1, STDOUT
	int		iPUT_DEC
	move	r1, STDOUT
	int		iPUT_NL
	move	r1, 3
	int		iCLOSE_FILE

	// Test iOPEN_FILE_APPEND
	move	r1, 3
	move	r2, "test.txt"
	int		iOPEN_FILE_APPEND
	move	r1, 3
	move	r2, 623
	int		iPUT_DEC
	move	r1, 3
	int		iPUT_NL
	move	r1, 3
	int		iCLOSE_FILE

	// Test iOPEN_FILE_READ
	move	r1, 3
	move	r2, "test.txt"
	int		iOPEN_FILE_READ
	move	r1, 3
	int		iGET_DEC
	move	r2, r0
	move	r1, STDOUT
	int		iPUT_DEC
	move	r1, STDOUT
	int		iPUT_NL
	move	r1, 3
	int		iGET_DEC
	move	r2, r0
	move	r1, STDOUT
	int		iPUT_DEC
	move	r1, STDOUT
	int		iPUT_NL
	move	r1, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_WRITE
	move	r1, 3
	move	r2, "test.bin"
	int		iOPEN_RAW_FILE_WRITE
	out		326, LONG, 3
	move	r1, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_READ
	move	r1, 3
	move	r2, "test.bin"
	int		iOPEN_RAW_FILE_READ
	in		r1, LONG, 3
	move	r2, r1
	move	r1, STDOUT
	int		iPUT_DEC
	move	r1, STDOUT
	int		iPUT_NL
	move	r1, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_APPEND
	move	r1, 3
	move	r2, "test.bin"
	int		iOPEN_RAW_FILE_APPEND
	int		iPI
	OUT		f0, DOUBLE, 3
	move	r1, 3
	int		iCLOSE_FILE

	// Test iOPEN_RAW_FILE_READ
	move	r1, 3
	move	r2, "test.bin"
	int		iOPEN_RAW_FILE_READ
	in		r1, LONG, 3
	move	r2, r1
	move	r1, STDOUT
	int		iPUT_DEC
	move	r1, STDOUT
	int		iPUT_NL
	in		f2, DOUBLE, 3
	move	r1, STDOUT
	move	r2, 6
	int		iPUT_FP
	move	r1, STDOUT
	int		iPUT_NL
	move	r1, 3
	int		iCLOSE_FILE

	// Test Filesystem interrupts
	#call	tempDirectory("CPUSim64")
	move	r5, r0
	#call	printf("Created temp directory: %s\n", r5)
	#call	fileExists(r5)
	#call	printf("Dir exists: %d\n", r0)
	#call	deleteDirectory(r5)
	#call	printf("Deleted temp directory: %d\n", r0)
	#call	fileExists(r5)
	#call	printf("Dir exists: %d\n", r0)
	
	#call	tempFile("CPUSim64",".txt")
	move	r5, r0
	#call	printf("Created temp file: %s\n", r5)
	#call	fileExists(r5)
	#call	printf("File exists: %d\n", r0)
	#call	deleteFile(r5)
	#call	printf("Deleted temp file: %d\n", r0)
	#call	fileExists(r5)
	#call	printf("File exists: %d\n", r0)

	#call	makeDirectory("temp")
	move	r5, r0
	#call	printf("Created directory: temp %d\n", r5)
	#call	fileExists("temp")
	#call	printf("Dir exists: %d\n", r0)
	
	#call	openTextFile("temp/temp.txt", WRITE_MODE)
	move	r5, r0
	#call	printf("Created temp file: temp/temp.txt\n")
	#call	fileExists("temp/temp.txt")
	#call	printf("File exists: %d\n", r0)
	#call	closeFile(r5)
	#call	openTextFile("temp/temp2.txt", WRITE_MODE)
	move	r5, r0
	#call	printf("Created temp file: temp/temp2.txt\n")
	#call	fileExists("temp/temp2.txt")
	#call	printf("File exists: %d\n", r0)
	#call	closeFile(r5)
	#call	listFiles("temp")
	move	r5, r0
	load	r4, r5[0]
	#call	printf("listFiles: %d\n", r4)
	#for	1, r2, le, r4, 1
		load	r3, r5[r2]
		#call	puts(r3)
		#call	put_nl()
	#end_for
	#call	puts("List Done\n")
	#call	deleteFiles(r5)
	#call	printf("Deleted temp files: %d\n", r0)
	#call	deleteDirectory("temp")
	#call	printf("Deleted temp directory: %d\n", r0)
	#call	fileExists("temp")
	#call	printf("Dir exists: %d\n", r0)
	#call	freeStrArray(r5)
#end_func

#global SMALL_GETS_BUFFER: .dci 0
	stop
	stop

