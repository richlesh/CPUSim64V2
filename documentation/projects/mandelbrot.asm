#include <system/io.asm>
#include <system/math.asm>
#include <system/string.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

#global PIDS:   .dca    2
#global gX_MIN: .dcf    -2.5
#global gX_MAX: .dcf    0.5
#global gY_MIN: .dcf    -1.5
#global gY_MAX: .dcf    1.5
#global gMAX_ITERATION: .dci 50
kONE_HALF: .dcf 0.5

#def_func   main()
    #var    argc, escape_limit, imageSize, filename, pid, i, numChildren, firstRow, lastRow, childFilename
    #fvar   x, y, radius, x_min, x_max, y_min, y_max

    int     iARGC
    move    argc, r0
    cmp     argc, 7
    jump    lt, GET_ARGS_FAILED
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

// Compute limits of image
    sub     x_min, x, radius
    store   x_min, gX_MIN
    add     x_max, x, radius
    store   x_max, gX_MAX
    sub     y_min, y, radius
    store   y_min, gY_MIN
    add     y_max, y, radius
    store   y_max, gY_MAX
    #call   printf("Image Bounds: %f, %f, %f, %f\n", x_min, x_max, y_min, y_max)

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

    #for    1, i <= numChildren, 1
        load    pid, PIDS[i]
        #if_cond    pid, gt, 0
            #call   printf("Waiting for %d...\n", pid)
            #macro  JOIN_THREAD(pid)
        #end_cond
    #end_for
    #call   sleep(5000)
    #call   combine_output(filename, imageSize)
    #return 0
    jump    MAIN_END
GET_ARGS_FAILED:
    #call   puts("Syntax: mandelbrot x y radius escape_limit image_size filename")
    #return 1
MAIN_END:
#end_func
    
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
    cmp     child_pid, -1
    jump    eq, FORK_FAILED
    test    child_pid
    jump    z, CHILD_FORK
    #call   printf("Child %s forked: %d\n", childName, child_pid)
    #return child_pid
    jump    END
CHILD_FORK:
    #call   printf("Child %s executing...%d %d %d\n", childName, first, last, w)
// Create text file in write mode.
    #call   openTextFile(childName, WRITE_MODE)
// Save the port returned.
    move    port, r0
    #call   printf("Opened port %d\n", port)
// If the port returned is -1 we failed.
    cmp     port, -1
    jump    z, ENDIF1
    #call   compute_mandelbrot(port, first, last, w)
// Close the file
    #call   closeFile(port)
ENDIF1:

    #call   printf("Child %s done!\n", childName)
    stop
FORK_FAILED:
    #call   printf("Fork %s failed!\n", childName)
    #return -1
END:
#end_func

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
    #for    first, j < last, 1
 //       #call   printf("j: %d\n", j)
        #for    0, i < w, 1
 //           #call   printf("i: %d\n", i)
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

//#call printf("compute_escape(%f,%f)\n", x0, y0)
    jump    LOOP_COND
LOOP_START:
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
//#call debug(STDOUT,"%f %f\n", x2, y2)     
LOOP_COND:
// while (x2 + y2 ≤ 4 and iteration < max_iteration) do
    add     f0, x2, y2
    #macro COMPARE(f0, le, four)
    move    r1, r0
    #macro COMPARE(iteration, lt, max_iteration)
    and     r0, r1
    jump    nz, LOOP_START
    
    pop     r1
    #if_cond    iteration, eq, max_iteration
        clear   f0
//    #call printf("MAXITER\n")       
    #else_cond
// Linear coloring
//        #call printf("ITER: %d\n", iteration)
        move    f0, iteration
        div     f0, max_iteration
//      mult    f0, 5           // five color bands
//      move    normalized, f0
//      int     iFLOOR
//      sub     f0, normalized, f0
    #end_cond
#end_func

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
 //       #call   deleteFile(tempFile)
    #end_for

// Close the file
    #call   closeFile(out_port)
#end_func

    stop
    stop
    