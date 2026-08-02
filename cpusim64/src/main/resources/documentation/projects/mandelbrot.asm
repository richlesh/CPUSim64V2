///////////////////////////////////////////////////////////////////////////////
// mandelbrot.asm
//
// Multi-process Mandelbrot set image generator.
//
// This program renders the Mandelbrot set as a grayscale PGM (Portable Gray
// Map) image. To speed up rendering, the image is divided into horizontal
// bands and each band is computed by a separate child process (created with
// iFORK). Each child writes its band to a temporary text file. After all
// children finish, the parent process merges (concatenates) the temporary
// files into a single .pgm image file with the proper text PGM header.
//
// Usage:
//   mandelbrot x y radius escape_limit image_size filename
//
//   x            - x coordinate of the center of the view (float)
//   y            - y coordinate of the center of the view (float)
//   radius       - half-width/half-height of the square view region (float)
//   escape_limit - maximum iterations before a point is considered in the set
//   image_size   - width and height of the square output image in pixels
//   filename     - base name for output; produces <filename>.pgm
//
//
// Example:
// mandelbrot -0.75 0 1.5 50 1024 mandelbrot_test
//
// Output:
//   <filename>.pgm   - the final grayscale image
//   <filename>_N.tmp - temporary per-band files (N = 1..numChildren)
//
// Author: Richard Lesh
// Modified: 2026/06/28
// Original: 2023/02/26
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

// PIDS[0] holds the number of children (band count). PIDS[1..N] hold the
// process IDs of the spawned child processes.
#global PIDS:   .dca    16

// Global bounds of the complex plane region being rendered. These are computed
// by main() from the command line center (x,y) and radius and read back by the
// child processes in compute_mandelbrot().
#global gX_MIN: .dcf    -2.5
#global gX_MAX: .dcf    0.5
#global gY_MIN: .dcf    -1.5
#global gY_MAX: .dcf    1.5

// Maximum iteration count used by the escape-time algorithm.
#global gMAX_ITERATION: .dci 50
kONE_HALF: .dcf 0.5

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Program entry point. Parses the command line arguments, computes the bounds
// of the complex plane region to render, divides the image into horizontal
// bands (one per child), spawns a child process to render each band, waits for
// all children to finish, then merges the per-band temporary files into the
// final .pgm image.
//
// Command line (via argc/args):
//   1: x            center x        (float)
//   2: y            center y        (float)
//   3: radius       view radius     (float)
//   4: escape_limit max iterations  (int)
//   5: image_size   pixel dimension (int)
//   6: filename     output basename (string)
//
// Returns:
//   0 on success, 1 if too few command line arguments were supplied.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    argc, escape_limit, imageSize, filename, pid, i, numChildren, firstRow, lastRow, childFilename
    #fvar   x, y, radius, x_min, x_max, y_min, y_max

    int     iARGC
    move    argc, r0
    #if_cond    argc != 7
        #call   puts("Syntax: mandelbrot x y radius escape_limit image_size filename")
        #return 1
    #end_cond

GET_ARGS:
    #call   args(1)
    #macro  PARSE_FLOAT(r0)
    move    x, f0
    #call   args(2)
    #macro  PARSE_FLOAT(r0)
    move    y, f0
    #call   args(3)
    #macro  PARSE_FLOAT(r0)
    move    radius, f0
    #call   args(4)
    #macro  PARSE_INT(r0)
    move    escape_limit, r0
    store   escape_limit, gMAX_ITERATION
    #call   args(5)
    #macro  PARSE_INT(r0)
    move    imageSize, r0
    #call   args(6)
    move    filename, r0
    #call   printf("Image: %f, %f, %f\n", x, y, radius)

// Compute limits of image from center (x,y) and radius, store in globals so
// the child processes can read them.
    sub     x_min, x, radius
    store   x_min, gX_MIN
    add     x_max, x, radius
    store   x_max, gX_MAX
    sub     y_min, y, radius
    store   y_min, gY_MIN
    add     y_max, y, radius
    store   y_max, gY_MAX
    #call   printf("Image Bounds: %f, %f, %f, %f\n", x_min, x_max, y_min, y_max)

// Divide the image into numChildren horizontal bands. For each band, compute
// its first and last row, generate the temp filename, and spawn a child to
// render it. The child's PID is recorded in PIDS[i].
    load    numChildren, PIDS[0]
    #call   printf("Number of children: %d\n", numChildren)
    move    firstRow, 0
    #for    1, i <= numChildren, 1
        move    f0, imageSize
        div     f0, numChildren
        mult    f0, i
        #call   round(f0)
        move    lastRow, f0
        #call   sprintf("%s_%d.tmp", filename, i)
        move    childFilename, r0
        #call   spawnChild(childFilename, firstRow, lastRow, imageSize)
        move    pid, r0
        #if_cond    pid, gt, 0
            #call   printf("Spawn child for %d...into %s\n", pid, childFilename)
        store   pid, PIDS[i]
        #end_cond
        move    firstRow, lastRow
        add     firstRow, 1
    #end_for

// Wait for each spawned child to finish before merging output.
    #for    1, i <= numChildren, 1
        load    pid, PIDS[i]
        #if_cond    pid, gt, 0
            #call   printf("Waiting for %d...\n", pid)
            move    r1, pid
            int iWAIT_PID
        #end_cond
    #end_for

// Merge all per-band temp files into the final .pgm image.
    #call   combine_output(filename, imageSize)
    #return 0
MAIN_END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// spawnChild(name, firstRow, lastRow, width)
//
// Forks a child process to render a single horizontal band of the image. The
// parent returns immediately with the child's PID. The child opens the named
// temporary file for writing, renders its assigned rows by calling
// compute_mandelbrot(), closes the file, and then halts (stop).
//
// Arguments:
//   name     - temporary output filename for this band (string)
//   firstRow - first image row (inclusive) this child should render (int)
//   lastRow  - last image row this child should render (int)
//   width    - width of the image in pixels (int)
//
// Returns (in parent only):
//   child PID on success, -1 if the fork failed.
// Note: the child process does not return; it stops after writing its band.
///////////////////////////////////////////////////////////////////////////////

#def_func spawnChild(name, firstRow, lastRow, width)
    #var    child_pid, childName, first, last, w, port
    load    childName, name
    load    first, firstRow
    load    last, lastRow
    load    w, width
    #call   printf("spawnChild(%s, %d, %d, %d)\n", childName, first, last, w)
    int     iFORK
    move    child_pid, r0
    #call   printf("spawnChild result: %d\n", child_pid)
// Fork failed if child_pid is -1
    #if_cond    child_pid != -1
        #if_cond    child_pid != 0
// Parent path: child_pid is the new child's PID; return it to the caller.
            #call   printf("Child %s forked: %d\n", childName, child_pid)
            #return child_pid    
        #else_cond
// Child path: render the assigned band into the temp file.
            #call   printf("Child %s executing...%d %d %d\n", childName, first, last, w)
// Create text file in write mode.
            #call   openTextFile(childName, WRITE_MODE)
// Save the port returned.
            move    port, r0
            #call   printf("Opened port %d\n", port)
// If the port returned is -1 we failed.
            #if_cond    port != -1
                #call   compute_mandelbrot(port, first, last, w)
// Close the file
                #call   closeFile(port)
                #call   printf("Child %s done!\n", childName)
            #end_cond
            stop
        #end_cond
    #else_cond
        #call   printf("Fork %s failed!\n", childName)
        #return -1
    #end_cond
END:
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_mandelbrot(port, firstRow, lastRow, width)
//
// Renders a range of image rows (a horizontal band) of the Mandelbrot set and
// writes the pixel intensity values as ASCII decimal numbers to the given
// output port. For each pixel, the corresponding complex coordinate (x0, y0)
// is computed from the global plane bounds, the escape value is obtained from
// compute_escape(), scaled to a 0-255 grayscale value, and written out. A
// space (or newline every 20 values) separates pixels and a newline ends each
// row to keep output lines reasonably short.
//
// Arguments:
//   port     - output port (file) to write pixel values to (int)
//   firstRow - first row (inclusive) of this band (int)
//   lastRow  - last row (exclusive) of this band (int)
//   width    - image width in pixels (int)
///////////////////////////////////////////////////////////////////////////////

#def_func compute_mandelbrot(port, firstRow, lastRow, width)
    #var    i, j, first, last, w, p, level, quotient, remainder
    #fvar   x0, y0, xWidth, yHeight, xMin, yMin, xMax, yMax, one_half
    load    first, firstRow
    load    last, lastRow
    load    w, width
    load    p, port
    load    xMin, gX_MIN
    load    xMax, gX_MAX
    load    yMin, gY_MIN
    load    yMax, gY_MAX
    load    one_half, kONE_HALF
    #call   printf("compute_mandelbrot(%d, %d, %d, %d)\n", p, first, last, w)
    sub     xWidth, xMax, xMin
    sub     yHeight, yMax, yMin
    #for    first, j <= last, 1
        #for    0, i < w, 1
            // Insert a space (or newline every 20 values) between pixels to
            // keep output lines short.
            #if_cond    i, ne, 0
                div     quotient, remainder, i, 20
                cmp     remainder, 0
                move    eq, r0, '\n', 32
                #call   fputc(p, r0)
            #end_cond
            // x0 = xWidth / width * (i + 0.5) + xMin
            move    f0, i
            add     f0, one_half
            mult    f0, xWidth
            div     f0, w
            add     x0, f0, xMin
            // y0 = yMax - yHeight / width * (j + 0.5)
            move    f0, j
            add     f0, one_half
            mult    f0, yHeight
            div     f0, w
            sub     y0, yMax, f0
            #call   compute_escape(x0, y0)
            mult    f0, 256
            move    level, f0
            #call   fput_dec(p, level)
        #end_for
        #call   fput_nl(p)
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// compute_escape(x0_arg, y0_arg)
//
// Implements the Mandelbrot set escape-time algorithm for a single complex
// point (x0, y0). Starting from z = 0, the function iterates the recurrence
//
//   z := z^2 + c     where c = x0 + y0*i
//
// which is computed component-wise as:
//   y := 2*x*y + y0
//   x := x^2 - y^2 + x0
//
// Iteration continues while |z|^2 = x^2 + y^2 <= 4 and the iteration count
// is less than gMAX_ITERATION. Points that never escape (iteration reaches
// gMAX_ITERATION) are considered inside the set and return 0.0. Points that
// do escape return a normalized value in the range (0.0, 1.0] computed as
// iteration / max_iteration, suitable for linear grayscale coloring.
//
// Arguments:
//   x0_arg - real part of the complex point c (float)
//   y0_arg - imaginary part of the complex point c (float)
//
// Returns:
//   f0 - 0.0 if the point is in the set (max iterations reached), otherwise
//        a value in (0.0, 1.0] proportional to how quickly the point escaped.
///////////////////////////////////////////////////////////////////////////////

kFOUR:      .dcf 4.0
#def_func   compute_escape(x0_arg, y0_arg)
    #var    iteration, max_iteration
    #fvar   x, y, x0, y0, x2, y2, four, one_half, normalized
    load    max_iteration, gMAX_ITERATION
    clear   iteration
    clear   x2
    clear   y2
    clear   x
    clear   y
    load    four, kFOUR
    load    x0, x0_arg
    load    y0, y0_arg
    push    r1

    #do_while
//    y:= 2 * x * y + y0
        mult    y, x
        mult    y, 2
        add     y, y0
//    x:= x2 - y2 + x0
        sub     x, x2, y2
        add     x, x0
//    x2:= x * x
        mult    x2, x, x
//    y2:= y * y
        mult    y2, y, y
        add     iteration, 1
LOOP_COND:
// while (x2 + y2 ≤ 4 and iteration < max_iteration) do
        add     f0, x2, y2
        #macro COMPARE(f0, le, four)
        move    r1, r0
        #macro COMPARE(iteration, lt, max_iteration)
        and     r0, r1
    #end_do_while r0 != 0
    
    pop     r1
    #if_cond    iteration, eq, max_iteration
        clear   f0
    #else_cond
        move    f0, iteration
        div     f0, max_iteration
    #end_cond
#end_func

//////////////////////////////////////////////////////////////////////////////
// combine_output(filename, imageSize)
//
// Assembles the final PGM image file from the per-band temporary files
// written by the child processes. Opens the output file <filename>.pgm for
// writing, emits the PGM P2 header, then iterates over each child's temporary
// file (<filename>_N.tmp, N = 1..numChildren), copying its pixel data
// verbatim into the output file. The temporary files are opened and closed one
// at a time to avoid exhausting available ports.
//
// The output file is a valid PGM P2 (ASCII grayscale) image with:
//   - width  = imageSize pixels
//   - height = imageSize pixels
//   - max grayscale value = 255
//
// Arguments:
//   filename  - base name used to form <filename>.pgm (output) and
//               <filename>_N.tmp (per-band input files) (string)
//   imageSize - width and height of the square image in pixels (int)
///////////////////////////////////////////////////////////////////////////////

#def_func   combine_output(filename, imageSize)
    #var    i, fn, size, in_port, out_port, numChildren, tempFile
    load    fn, filename
    load    size, imageSize
// Create text file in write mode.
    #call   sprintf("%s.pgm", fn)
    #call   openTextFile(r0, WRITE_MODE)
    move    out_port, r0
    #call   fprintf(out_port, "P2\n%d %d\n%d\n", size, size, 255)
    
    load    numChildren, PIDS[0]
    #call   printf("NUM FILES: %d\n", numChildren)
    #for    1, i <= numChildren, 1
        #call   sprintf("%s_%d.tmp", fn, i)
        move    tempFile, r0
        #call   printf("Merging %s\n", tempFile)
        #call   openTextFile(tempFile, READ_MODE)
        move    in_port, r0
        #call   copy_text_file(in_port, out_port)
        #call   closeFile(in_port)
        #call   deleteFile(tempFile)
    #end_for

    #call   closeFile(out_port)
#end_func

    stop
    stop
    