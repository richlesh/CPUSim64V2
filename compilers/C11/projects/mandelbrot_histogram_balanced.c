/*
 * mandelbrot_histogram_balanced.c
 *
 * Multi-threaded Mandelbrot set image generator with histogram coloring.  It
 * renders the set as a binary (P5) PGM image.  One thread per processor core
 * (at most MAX_THREADS) takes the next row of the image with an atomic
 * counter until none is left, so the threads stay busy to the end although
 * some rows take much longer than others.  Each thread stores the iteration
 * count of every pixel of its rows in a shared image buffer.  The main thread
 * then counts how many pixels have each iteration count and gives a pixel the
 * gray level 256 * (the fraction of the pixels with an iteration count up to
 * its own), which spreads the gray levels evenly over the image.  Pixels in
 * the set (count 0) stay black.
 * C11 version of documentation/projects/mandelbrot_histogram_balanced.asm; it uses
 * <threads.h> and <stdatomic.h>.
 *
 * Usage:
 *     mandelbrot_histogram_balanced x y radius escape_limit image_size filename
 *
 *     x, y         - the center of the view (floats)
 *     radius       - half the width and height of the view (float)
 *     escape_limit - the iterations after which a point counts as in the set
 *     image_size   - the width and height of the image in pixels
 *     filename     - the image is written to filename.pgm
 *
 * Example (the image of the documentation):
 *     > mandelbrot_histogram_balanced -0.748127681290498 0.09408613781852182 1e-6 10000 128 mandelbrot_histogram_balanced
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>
#include <unistd.h>

#define MAX_THREADS 100

static double gXMin = -2.5, gXMax = 0.5, gYMin = -1.5, gYMax = 1.5;
static long gMaxIteration = 50;
static long *gImage;                /* the iteration count of every pixel, row by row */
static long gNumThreads = MAX_THREADS;
static atomic_long gCounter = -1;         /* the last row taken */

/* the number of iterations of the point (x0, y0) before it escapes, 0 if it never does */
static long compute_escape(double x0, double y0)
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
    return iteration;
}

/* fills in the rows first .. last-1 of the image buffer */
static void compute_mandelbrot(long first, long last, long w)
{
    long i, j;
    double xWidth = gXMax - gXMin, yHeight = gYMax - gYMin;
    double inverse_width = 1.0 / (double)w;
    long *row = gImage + first * w;
    for (j = first; j < last; j++) {
        double y0 = gYMax - ((double)j + 0.5) * yHeight * inverse_width;
        for (i = 0; i < w; i++) {
            double x0 = ((double)i + 0.5) * xWidth * inverse_width + gXMin;
            row[i] = compute_escape(x0, y0);
        }
        row += w;
    }
}

/* the thread function: computes rows until there are none left */
static int run(void *arg)
{
    long w = (long)arg, pid = thrd_current();
    printf("Thread %ld executing...%ld\n", pid, w);
    for (;;) {
        long first = atomic_fetch_add(&gCounter, 1) + 1;
        if (first >= w) break;
        compute_mandelbrot(first, first + 1, w);
    }
    printf("Thread %ld done!\n", pid);
    return 0;
}

/* starts a thread that computes rows of a width x width image; returns its id, or -1 */
static long spawn_thread(long width)
{
    thrd_t tid;
    if (thrd_create(&tid, run, (void *)width) != thrd_success) return -1;
    return tid;
}

/* writes the image as filename.pgm with the gray levels from the histogram */
static int combine_output(const char *filename, long size)
{
    char name[512];
    FILE *out;
    long *histogram;
    long i, total = 0, subtotal = 0, pixels = size * size;
    sprintf(name, "%s.pgm", filename);
    histogram = calloc(gMaxIteration, sizeof(long));
    if (!histogram) {
        puts("Error: histogram allocation failed!");
        return 1;
    }
    out = fopen(name, "wb");
    if (!out) {
        printf("Can't open %s.pgm...\n", filename);
        free(histogram);
        return -1;
    }
    fprintf(out, "P5\n%ld %ld\n%d\n", size, size, 255);
    for (i = 0; i < pixels; i++) {              /* the number of pixels of each iteration count */
        long v = gImage[i];
        if (v > 0) {
            histogram[v]++;
            total++;
        }
    }
    for (i = 0; i < gMaxIteration; i++) {       /* the gray level of an iteration count */
        subtotal += histogram[i];
        histogram[i] = (long)((double)subtotal / (double)total * 256);
    }
    for (i = 0; i < pixels; i++) fputc((int)histogram[gImage[i]], out);
    fclose(out);
    free(histogram);
    return 0;
}

int main(int argc, char **argv)
{
    long imageSize, i, cores;
    thrd_t tids[MAX_THREADS + 1];
    double x, y, radius;
    const char *filename;
    if (argc != 7) {
        puts("Syntax: mandelbrot_histogram_balanced x y radius escape_limit image_size filename");
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
    gImage = malloc(imageSize * imageSize * sizeof(long));
    if (!gImage) {
        puts("Error: image allocation failed!");
        return 1;
    }
    cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores < gNumThreads) gNumThreads = cores;
    if (imageSize < gNumThreads) gNumThreads = imageSize;
    printf("Number of threads: %ld\n", gNumThreads);
    for (i = 1; i <= gNumThreads; i++) {
        long tid = spawn_thread(imageSize);
        if (tid > 0) {
            printf("Spawn thread for %ld\n", tid);
            tids[i] = tid;
        } else {
            printf("Spawn thread for %ld failed!\n", tid);
            tids[i] = 0;
        }
    }
    for (i = 1; i <= gNumThreads; i++) {
        if (tids[i] > 0) {
            printf("Waiting for %ld...\n", tids[i]);
            thrd_join(tids[i], NULL);
        }
    }
    combine_output(filename, imageSize);
    free(gImage);
    return 0;
}
