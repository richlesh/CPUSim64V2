#include <system/io.asm>
#include <system/math.asm>
#include <system/string.asm>
#include <system/system.asm>

	#call	main()
	int		iEXIT

PIDS:	dca	20
IMAGE:	DCI 0
X_MIN:	DCF	-2.5
X_MAX:	DCF	0.5
Y_MIN:	DCF	-1.5
Y_MAX:	DCF	1.5
MAX_ITERATION:	DCI 50
ONE_HALF: DCF 0.5
COUNTER: DCI 0

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

	move	r0, imageSize
	mult	r0, imageSize
	#call	alloc(r0)
	store	r0, IMAGE

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
		#call	spawnChild(firstRow, lastRow, imageSize)
		move	pid, r0
		#cond	pid, gt, 0
//			#call	fprintf(STDOUT, "Spawn child for %d...\n", pid)
			store	pid, PIDS[i]
		#endcond
		move	firstRow, lastRow
	#endfor
	#for	i, 0, lt, numChildren, 1
		load	pid, PIDS[i]
		#cond	pid, gt, 0
			#call	fprintf(STDOUT, "Waiting for %d...\n", pid)
			move	r0, pid
			int		iJOIN_THREAD
		#endcond
	#endfor
	#call	combine_output(filename, imageSize)
	#return	0
	jump	@MAIN_END
GET_ARGS_FAILED:
	#call	putline("Syntax: mandelbrot x y radius escape_limit image_size filename")
	#return	1
MAIN_END:
#end_func

#define DATA_FIRST_ROW	0
#define DATA_LAST_ROW	1
#define DATA_WIDTH		2
#define DATA_SIZE		3

#def_func spawnChild(firstRow, lastRow, width)
	#var	first, last, w, data
	load	first, firstRow
	load	last, lastRow
	load	w, width
	#call	alloc(DATA_SIZE)
	move	data, r0
	store	first, data[DATA_FIRST_ROW]
	store	last, data[DATA_LAST_ROW]
	store	w, data[DATA_WIDTH]
	move	r0, run
	move	r1, data
	int		iTHREAD
#end_func

#def_func run(int data)
	#var	first, last, w
	#load_args
	load	first, data[DATA_FIRST_ROW]
	load	last, data[DATA_LAST_ROW]
	load	w, data[DATA_WIDTH]
	#call	fprintf(STDOUT,"Child %x executing...%d %d %d\n", data, first, last, w)
RUN_LOOP:
	#call	get_and_increment(COUNTER)
	move	first, r0
	add		last, first, 1
	#cond	first, ge, w
		jump	@RUN_DONE
	#endcond
//	#call	fprintf(STDOUT,"Child %x computing...%d %d %d\n", data, first, last, w)
	#call	compute_mandelbrot(first, last, w)
	jump	@RUN_LOOP
RUN_DONE:
	#call	fprintf(STDOUT, "Child %x done!\n", data)
	#call	free(data)
#end_func

#def_func compute_mandelbrot(firstRow, lastRow, width)
	#var	i, j, first, last, w, level, image_buffer
	#fvar	x0, y0, xWidth, yHeight, xMin, yMin, xMax, yMax, one_half
	load	first, firstRow
	load	last, lastRow
	load	w, width
	load	xMin, X_MIN
	load	xMax, X_MAX
	load	yMin, Y_MIN
	load	yMax, Y_MAX
	load	image_buffer, IMAGE
	load	one_half, ONE_HALF
	sub		xWidth, xMax, xMin
	sub		yHeight, yMax, yMin
	// first row is at image_buffer + first * width
	mult	r0, first, w
	add		image_buffer, r0
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
			move	level, r0
			store	level, image_buffer[i]
		#endfor
		add	image_buffer, w
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
    	clear	r0
//#call debug(STDOUT,"MAXITER\n")    	
    #elsecond
		move	r0, iteration
	#endcond
#end_func

#def_func	combine_output(filename, imageSize)
	#var	i, fn, size, out_port, image_buffer, pix_value, histogram, max_iteration, subtotal, totalCount
	load	fn, filename
	load	size, imageSize
	load	image_buffer, IMAGE
	load	max_iteration, MAX_ITERATION
	#call	alloc(max_iteration)
	move	histogram, r0
// Create text file in write mode.
	#call	sprintf("%s.pgm", fn)
	#call	openRawFile(r0, WRITE_MODE)
	#cond	r0, eq, -1
		#call	fprintf(STDOUT, "Can\'t open %s.pgm...\n", fn)
		return
	#endcond
	move	out_port, r0
	#call	fprintf(STDOUT, "Writing %s.pgm...\n", fn)
	#call	fprintf(out_port, "P5\n%d %d\n%d\n", size, size, 255)
	mult	size, size

	#for	i, 0, lt, max_iteration, 1
		store	0, histogram[i]
	#endfor
	
	// Compute histogram for pixel value frequency
	clear	totalCount
	#for	i, 0, lt, size, 1
		load	pix_value, image_buffer[i]
		#cond	pix_value, gt, 0
			load	r0, histogram[pix_value]
			add		r0, 1
			store	r0, histogram[pix_value]
			add		totalCount, 1
		#endcond
	#endfor

	// compute cumulative histogram
	clear	subtotal
	#for	i, 0, lt, max_iteration, 1
		load	r0, histogram[i]
		add		subtotal, r0
		move	f0, subtotal
		move	f1, totalCount
		div		f0, f1
		mult	f0, 256
		move	r0, f0
		store	r0, histogram[i]
	#endfor
	
	#for	i, 0, lt, size, 1
		load	pix_value, image_buffer[i]
		load	r0, histogram[pix_value]
		OUT1(r0, out_port)
	#endfor

// Close the file
	#call	closeFile(out_port)
	#call	puts("Finis")
#end_func

	stop
	stop
	