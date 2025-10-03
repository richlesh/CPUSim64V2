#include <system/io.asm>
#include <system/math.asm>
#include <system/string.asm>
#include <system/system.asm>

	#call	main()
	int		iEXIT

PIDS:	dca	8
X_MIN:	DCF	-2.5
X_MAX:	DCF	0.5
Y_MIN:	DCF	-1.5
Y_MAX:	DCF	1.5
MAX_ITERATION:	DCI 50
ONE_HALF: DCF 0.5

#def_func	main()
	#var	argc, escape_limit, imageSize, filename, pid, i, numChildren, firstRow, lastRow
	#fvar	x, y, radius, x_min, x_max, y_min, y_max

	int		iARGC
	move	argc, r0
	cmp		argc, 7
	jump	lt, @GET_ARGS_FAILED
GET_ARGS:
	move	r0, 1
	int		iARGS
	int		iPARSE_FLOAT
	move	x, f0
	move	r0, 2
	int		iARGS
	int		iPARSE_FLOAT
	move	y, f0
	move	r0, 3
	int		iARGS
	int		iPARSE_FLOAT
	move	radius, f0
	move	r0, 4
	int		iARGS
	int		iPARSE_INT
	move	escape_limit, r0
	store	escape_limit, MAX_ITERATION
	move	r0, 5
	int		iARGS
	int		iPARSE_INT
	move	imageSize, r0
	move	r0, 6
	int		iARGS
	move	filename, r0
	#call	fprintf(STDOUT, "Image: %f, %f, %f\n", x, y, radius)

// Compute limits of image
	sub		x_min, x, radius
	store	x_min, X_MIN
	add		x_max, x, radius
	store	x_max, X_MAX
	sub		y_min, y, radius
	store	y_min, Y_MIN
	add		y_max, y, radius
	store	y_max, Y_MAX
	#call	fprintf(STDOUT, "Image Bounds: %f, %f, %f, %f\n", x_min, x_max, y_min, y_max)

	load	numChildren, PIDS[-1]
	move	firstRow, 0
	#for	i, 0, lt, numChildren, 1
		move	f0, imageSize
		div		f0, numChildren
		add		r0, i, 1
		mult	f0, r0
		int		iROUND
		move	lastRow, f0
		#call	sprintf("%s_%d.tmp", filename, i)
		#call	spawnChild(r0, firstRow, lastRow, imageSize)
		move	pid, r0
		#cond	pid, gt, 0
			#call	fprintf(STDOUT, "Spawn child for %d...\n", pid)
			store	pid, PIDS[i]
		#endcond
		move	firstRow, lastRow
	#endfor
	#for	i, 0, lt, numChildren, 1
		load	pid, PIDS[i]
		#cond	pid, gt, 0
			#call	fprintf(STDOUT, "Waiting for %d...\n", pid)
			move	r0, pid
			int		iWAIT_PID
		#endcond
	#endfor
	#call	combine_output(filename, imageSize)
	#return	0
	jump	@MAIN_END
GET_ARGS_FAILED:
	#call	puts("Syntax: mandelbrot x y radius escape_limit image_size filename")
	#return	1
MAIN_END:
#end_func
	
#def_func spawnChild(name, firstRow, lastRow, width)
	#var	child_pid, childName, first, last, w, port
	load	childName, name
	load	first, firstRow
	load	last, lastRow
	load	w, width
//	#call	fprintf(STDOUT, "%s, %d %d %d\n", childName, first, last, w)
	int		iFORK
	move	child_pid, r0
	cmp		child_pid, -1
	jump	eq, @FORK_FAILED
	test	child_pid
	jump	z, @CHILD_FORK
//	#call	fprintf(STDOUT,"Child %s forked: %d\n", childName, r0)
	#return	child_pid
	jump	@END
CHILD_FORK:
	#call	fprintf(STDOUT,"Child %s executing...%d %d %d\n", childName, first, last, w)
// Create text file in write mode.
	#call	openRawFile(childName, WRITE_MODE)
// Save the port returned.
	move	port, r0
// If the port returned is -1 we failed.
	cmp		port, -1
	jump	z, @ENDIF1
	#call	compute_mandelbrot(port, first, last, w)
// Close the file
	#call	closeFile(port)
ENDIF1:

	#call	fprintf(STDOUT, "Child %s done!\n", childName)
	stop
FORK_FAILED:
	#call	fprintf(STDOUT, "Fork %s failed!\n", childName)
	#return	-1
END:
#end_func

#def_func compute_mandelbrot(port, firstRow, lastRow, width)
	#var	i, j, first, last, w, p, level
	#fvar	x0, y0, xWidth, yHeight, xMin, yMin, xMax, yMax, one_half
	load	first, firstRow
	load	last, lastRow
	load	w, width
	load	p, port
	load	xMin, X_MIN
	load	xMax, X_MAX
	load	yMin, Y_MIN
	load	yMax, Y_MAX
	load	one_half, ONE_HALF
	sub		xWidth, xMax, xMin
	sub		yHeight, yMax, yMin
	#for	j, first, lt, last, 1
		#for	i, 0, lt, w, 1
			// x0 = xWidth / width * (i + 0.5) + xMin
			move	f0, i
			add		f0, one_half
			mult	f0, xWidth
			div		f0, w
			add		x0, f0, xMin
			// y0 = yMax - yHeight / width * (j + 0.5)
			move	f0, j
			add		f0, one_half
			mult	f0, yHeight
			div		f0, w
			sub		y0, yMax, f0
			#call	compute_escape(x0, y0)
			mult	f0, 256
			move	level, f0
			OUT1(level, p)
		#endfor
	#endfor
#end_func

FOUR:		DCF 4.0
#def_func	compute_escape(fp x0, fp y0)
	#var	iteration, max_iteration
	#fvar	x, y, x2, y2, four, one_half, normalized
	load	max_iteration, MAX_ITERATION
	clear	iteration
	clear	x2
	clear	y2
	clear	x
	clear	y
	load	four, FOUR
	push	r1
	#LOAD_ARGS

//#call debug(STDOUT,"compute_escape(%f,%f)\n", x0, y0)
	jump	@LOOP_COND
LOOP_START:
//    y:= 2 * x * y + y0
	mult	y, x
	mult	y, 2
	add		y, y0
//    x:= x2 - y2 + x0
	sub		x, x2, y2
	add		x, x0
//    x2:= x * x
	mult	x2, x, x
//    y2:= y * y
	mult	y2, y, y
	add		iteration, 1
//#call debug(STDOUT,"%f %f\n", x2, y2)    	
LOOP_COND:
// while (x2 + y2 ≤ 4 and iteration < max_iteration) do
	add		f0, x2, y2
	COMPARE(f0, le, four)
	move	r1, r0
	COMPARE(iteration, lt, max_iteration)
	and		r0, r1
	jump	nz, @LOOP_START
    
    pop		r1
    #cond	iteration, eq, max_iteration
    	clear	f0
//#call debug(STDOUT,"MAXITER\n")    	
    #elsecond
// Linear coloring
		move	f0, iteration
		div		f0, max_iteration
//		mult	f0, 5			// five color bands
//		move	normalized, f0
//		int		iFLOOR
//		sub		f0, normalized, f0
	#endcond
#end_func

#def_func	combine_output(filename, imageSize)
	#var	i, fn, size, in_port, out_port, numChildren, tempFile
	load	fn, filename
	load	size, imageSize
// Create text file in write mode.
	#call	sprintf("%s.pgm", fn)
	#call	openRawFile(r0, WRITE_MODE)
	move	out_port, r0
	#call	fprintf(out_port, "P5\n%d %d\n%d\n", size, size, 255)
	
	load	numChildren, PIDS[-1]
	#call	fprintf(STDOUT, "NUM FILES: %d\n", numChildren)
	#for	i, 0, lt, numChildren, 1
#call fprintf(STDOUT, "Merging %d\n", i)
		#call	sprintf("%s_%d.tmp", fn, i)
		move	tempFile, r0
		#call	openRawFile(tempFile, READ_MODE)
		move	in_port, r0
		#call	copy_raw_file(in_port, out_port)
		#call	closeFile(in_port)
		#call	deleteFile(tempFile)
	#endfor

// Close the file
	#call	closeFile(out_port)
#end_func

	stop
	stop
	