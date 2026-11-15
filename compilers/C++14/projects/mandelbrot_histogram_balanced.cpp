/*
 * mandelbrot_histogram_balanced.cpp
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
 * C++ version of documentation/projects/mandelbrot_histogram_balanced.asm; it uses
 * <thread.h>.
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
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <thread.h>

static const long MAX_THREADS = 100;

static double gXMin = -2.5, gXMax = 0.5, gYMin = -1.5, gYMax = 1.5;
static long gMaxIteration = 50;
static long *gImage;                // the iteration count of every pixel, row by row (on the shared heap)
static long gNumThreads = MAX_THREADS;
static long gCounter = -1;          // the last row taken

// the number of iterations of the point (x0, y0) before it escapes, 0 if it never does
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

// fills in the rows first .. last-1 of the image buffer
static void compute_mandelbrot(long first, long last, long w)
{
    double xWidth = gXMax - gXMin, yHeight = gYMax - gYMin;
    double inverse_width = 1.0 / (double)w;
    long *row = gImage + first * w;
    for (long j = first; j < last; j++) {
        double y0 = gYMax - ((double)j + 0.5) * yHeight * inverse_width;
        for (long i = 0; i < w; i++) {
            double x0 = ((double)i + 0.5) * xWidth * inverse_width + gXMin;
            row[i] = compute_escape(x0, y0);
        }
        row += w;
    }
}

// the thread function: computes rows until there are none left
static void run(void *arg)
{
    long w = (long)arg, pid = get_pid();
    std::printf("Thread %ld executing...%ld\n", pid, w);
    for (;;) {
        long first = atomic_increment(&gCounter);
        if (first >= w) break;
        compute_mandelbrot(first, first + 1, w);
    }
    std::printf("Thread %ld done!\n", pid);
}

// starts a thread that computes rows of a width x width image; returns its id, or -1
static long spawn_thread(long width)
{
    return thread_create(run, (void *)width);
}

// writes the image as filename.pgm with the gray levels from the histogram
static int combine_output(const std::string &filename, long size)
{
    std::vector<long> histogram(gMaxIteration, 0);
    std::string name = filename + ".pgm";
    std::FILE *out = std::fopen(name.c_str(), "wb");
    if (!out) {
        std::printf("Can't open %s.pgm...\n", filename.c_str());
        return -1;
    }
    std::fprintf(out, "P5\n%ld %ld\n%d\n", size, size, 255);
    long total = 0, subtotal = 0, pixels = size * size;
    for (long i = 0; i < pixels; i++) {             // the number of pixels of each iteration count
        long v = gImage[i];
        if (v > 0) {
            histogram[v]++;
            total++;
        }
    }
    for (long &h : histogram) {                     // the gray level of an iteration count
        subtotal += h;
        h = (long)((double)subtotal / (double)total * 256);
    }
    for (long i = 0; i < pixels; i++) std::fputc((int)histogram[gImage[i]], out);
    std::fclose(out);
    return 0;
}

int main(int argc, char **argv)
{
    long tids[MAX_THREADS + 1];
    if (argc != 7) {
        std::puts("Syntax: mandelbrot_histogram_balanced x y radius escape_limit image_size filename");
        return 1;
    }
    double x = std::atof(argv[1]);
    double y = std::atof(argv[2]);
    double radius = std::atof(argv[3]);
    gMaxIteration = std::atol(argv[4]);
    long imageSize = std::atol(argv[5]);
    std::string filename = argv[6];
    std::printf("Image: %f, %f, %f\n", x, y, radius);
    gXMin = x - radius;
    gXMax = x + radius;
    gYMin = y - radius;
    gYMax = y + radius;
    std::printf("Image Bounds: %f, %f, %f, %f\n", gXMin, gXMax, gYMin, gYMax);
    gImage = new long[imageSize * imageSize];
    long cores = get_num_cores();
    if (cores < gNumThreads) gNumThreads = cores;
    if (imageSize < gNumThreads) gNumThreads = imageSize;
    std::printf("Number of threads: %ld\n", gNumThreads);
    for (long i = 1; i <= gNumThreads; i++) {
        long tid = spawn_thread(imageSize);
        if (tid > 0) {
            std::printf("Spawn thread for %ld\n", tid);
            tids[i] = tid;
        } else {
            std::printf("Spawn thread for %ld failed!\n", tid);
            tids[i] = 0;
        }
    }
    for (long i = 1; i <= gNumThreads; i++) {
        if (tids[i] > 0) {
            std::printf("Waiting for %ld...\n", tids[i]);
            thread_join(tids[i]);
        }
    }
    combine_output(filename, imageSize);
    delete[] gImage;
    return 0;
}
