/*
 * mandelbrot_binary.c
 *
 * Multi-process Mandelbrot set image generator.  It renders the set as a
 * grayscale PGM image in the binary (P5) format, one byte for each pixel.
 * The image is cut into bands of rows, one per processor core (at most
 * MAX_CHILDREN); each band is computed by a child process made with fork()
 * that writes its rows to a temporary file filename_<n>.tmp.  The parent
 * waits for the children and merges the files into filename.pgm.
 * C11 version of documentation/projects/mandelbrot_binary.asm; it uses <_Thread.h>.
 *
 * Usage:
 *     mandelbrot_binary x y radius escape_limit image_size filename
 *
 *     x, y         - the center of the view (floats)
 *     radius       - half the width and height of the view (float)
 *     escape_limit - the iterations after which a point counts as in the set
 *     image_size   - the width and height of the image in pixels
 *     filename     - the image is written to filename.pgm
 *
 * Example (the image of the documentation):
 *     > mandelbrot_binary -0.748127681290498 0.09408613781852182 1e-6 10000 128 mandelbrot_binary
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <_Thread.h>

#define MAX_CHILDREN 100

static double gXMin = -2.5, gXMax = 0.5, gYMin = -1.5, gYMax = 1.5;
static long gMaxIteration = 50;
static long gPids[MAX_CHILDREN + 1];
static long gNumChildren = MAX_CHILDREN;

/* the escape time of the point (x0, y0): iterations / gMaxIteration, 0 if it never escapes */
static double compute_escape(double x0, double y0)
{
    double x = 0, y = 0, x2 = 0, y2 = 0;
    long iteration = 0;
    do {
        y = y * x * 2 + y0;
        x = x2 - y2 + x0;
        x2 = x * x;
        y2 = y * y;
        iteration++;
    } while (x2 + y2 <= 4.0 && iteration < gMaxIteration);
    if (iteration == gMaxIteration) return 0;
    return (double)iteration / (double)gMaxIteration;
}

/* writes the rows first .. last-1 of a width x width image as bytes */
static void compute_mandelbrot(FILE *out, long first, long last, long w)
{
    long i, j;
    double xWidth = gXMax - gXMin, yHeight = gYMax - gYMin;
    for (j = first; j < last; j++) {
        for (i = 0; i < w; i++) {
            double x0 = ((double)i + 0.5) * xWidth / (double)w + gXMin;
            double y0 = gYMax - ((double)j + 0.5) * yHeight / (double)w;
            long level = (long)(compute_escape(x0, y0) * 256);
            fputc((int)level, out);
        }
    }
}

/* starts a child process that writes the rows first .. last-1 to the file name; returns its pid, or -1 */
static long spawn_child(const char *name, long first, long last, long width)
{
    long child_pid;
    FILE *out;
    printf("spawnChild(%s, %ld, %ld, %ld)\n", name, first, last, width);
    fflush(stdout);                 /* the child would get a copy of output not yet written */
    child_pid = fork();
    printf("spawnChild result: %ld\n", child_pid);
    if (child_pid == -1) {
        printf("Fork %s failed!\n", name);
        return -1;
    }
    if (child_pid != 0) {
        printf("Child %s forked: %ld\n", name, child_pid);
        return child_pid;
    }
    printf("Child %s executing...%ld %ld %ld\n", name, first, last, width);
    out = fopen(name, "wb");
    printf("Opened port %d\n", out ? 1 : -1);
    if (out) {
        compute_mandelbrot(out, first, last, width);
        fclose(out);
        printf("Child %s done!\n", name);
    }
    exit(0);
}

/* copies a file to out */
static void copy_file(FILE *in, FILE *out)
{
    int c;
    while ((c = fgetc(in)) != EOF) fputc(c, out);
}

/* merges the band files into filename.pgm and deletes them */
static int combine_output(const char *filename, long size)
{
    char name[512];
    FILE *out, *in;
    long i;
    sprintf(name, "%s.pgm", filename);
    out = fopen(name, "wb");
    if (!out) {
        printf("Can't open %s.pgm...\n", filename);
        return -1;
    }
    fprintf(out, "P5\n%ld %ld\n%d\n", size, size, 255);
    printf("NUM FILES: %ld\n", gNumChildren);
    for (i = 1; i <= gNumChildren; i++) {
        sprintf(name, "%s_%ld.tmp", filename, i);
        printf("Merging %s\n", name);
        in = fopen(name, "rb");
        if (in) {
            copy_file(in, out);
            fclose(in);
        }
        remove(name);
    }
    fclose(out);
    return 0;
}

int main(int argc, char **argv)
{
    long imageSize, i, firstRow = 0, cores;
    double x, y, radius;
    const char *filename;
    if (argc != 7) {
        puts("Syntax: mandelbrot_binary x y radius escape_limit image_size filename");
        return 1;
    }
    x = atof(argv[1]);
    y = atof(argv[2]);
    radius = atof(argv[3]);
    gMaxIteration = atol(argv[4]);
    imageSize = atol(argv[5]);
    filename = argv[6];
    printf("Image: %f, %f, %f\n", x, y, radius);
    gXMin = x - radius;
    gXMax = x + radius;
    gYMin = y - radius;
    gYMax = y + radius;
    printf("Image Bounds: %f, %f, %f, %f\n", gXMin, gXMax, gYMin, gYMax);
    cores = get_num_cores();
    if (cores < gNumChildren) gNumChildren = cores;
    if (imageSize < gNumChildren) gNumChildren = imageSize;
    printf("Number of children: %ld\n", gNumChildren);
    for (i = 1; i <= gNumChildren; i++) {
        char child[512];
        long lastRow = (long)((double)imageSize / (double)gNumChildren * (double)i + 0.5);
        long pid;
        sprintf(child, "%s_%ld.tmp", filename, i);
        pid = spawn_child(child, firstRow, lastRow, imageSize);
        if (pid > 0) {
            printf("Spawn child for %ld...into %s\n", pid, child);
            gPids[i] = pid;
        } else {
            printf("Spawn child for %ld failed!\n", pid);
            gPids[i] = 0;
        }
        firstRow = lastRow;
    }
    for (i = 1; i <= gNumChildren; i++) {
        if (gPids[i] > 0) {
            printf("Waiting for %ld...\n", gPids[i]);
            wait_pid(gPids[i]);
        }
    }
    combine_output(filename, imageSize);
    return 0;
}
