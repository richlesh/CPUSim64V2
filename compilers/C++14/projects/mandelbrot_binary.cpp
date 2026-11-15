/*
 * mandelbrot_binary.cpp
 *
 * Multi-process Mandelbrot set image generator.  It renders the set as a
 * grayscale PGM image in the binary (P5) format, one byte for each pixel.
 * The image is cut into bands of rows, one per processor core (at most
 * MAX_CHILDREN); each band is computed by a child process made with fork()
 * that writes its rows to a temporary file filename_<n>.tmp.  The parent
 * waits for the children and merges the files into filename.pgm.
 * C++ version of documentation/projects/mandelbrot_binary.asm; it uses <thread.h>.
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
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread.h>

static const long MAX_CHILDREN = 100;

static double gXMin = -2.5, gXMax = 0.5, gYMin = -1.5, gYMax = 1.5;
static long gMaxIteration = 50;
static long gPids[MAX_CHILDREN + 1];
static long gNumChildren = MAX_CHILDREN;

// the escape time of the point (x0, y0): iterations / gMaxIteration, 0 if it never escapes
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

// writes the rows first .. last-1 of a width x width image as bytes
static void compute_mandelbrot(std::FILE *out, long first, long last, long w)
{
    double xWidth = gXMax - gXMin, yHeight = gYMax - gYMin;
    for (long j = first; j < last; j++) {
        for (long i = 0; i < w; i++) {
            double x0 = ((double)i + 0.5) * xWidth / (double)w + gXMin;
            double y0 = gYMax - ((double)j + 0.5) * yHeight / (double)w;
            long level = (long)(compute_escape(x0, y0) * 256);
            std::fputc((int)level, out);
        }
    }
}

// starts a child process that writes the rows first .. last-1 to the file name; returns its pid, or -1
static long spawn_child(const std::string &name, long first, long last, long width)
{
    std::printf("spawnChild(%s, %ld, %ld, %ld)\n", name.c_str(), first, last, width);
    std::fflush(stdout);            // the child would get a copy of output not yet written
    long child_pid = fork();
    std::printf("spawnChild result: %ld\n", child_pid);
    if (child_pid == -1) {
        std::printf("Fork %s failed!\n", name.c_str());
        return -1;
    }
    if (child_pid != 0) {
        std::printf("Child %s forked: %ld\n", name.c_str(), child_pid);
        return child_pid;
    }
    std::printf("Child %s executing...%ld %ld %ld\n", name.c_str(), first, last, width);
    std::FILE *out = std::fopen(name.c_str(), "wb");
    std::printf("Opened port %d\n", out ? 1 : -1);
    if (out) {
        compute_mandelbrot(out, first, last, width);
        std::fclose(out);
        std::printf("Child %s done!\n", name.c_str());
    }
    std::exit(0);
}

// copies a file to out
static void copy_file(std::FILE *in, std::FILE *out)
{
    int c;
    while ((c = std::fgetc(in)) != EOF) std::fputc(c, out);
}

// merges the band files into filename.pgm and deletes them
static int combine_output(const std::string &filename, long size)
{
    std::string name = filename + ".pgm";
    std::FILE *out = std::fopen(name.c_str(), "wb");
    if (!out) {
        std::printf("Can't open %s.pgm...\n", filename.c_str());
        return -1;
    }
    std::fprintf(out, "P5\n%ld %ld\n%d\n", size, size, 255);
    std::printf("NUM FILES: %ld\n", gNumChildren);
    for (long i = 1; i <= gNumChildren; i++) {
        name = filename + "_" + std::to_string(i) + ".tmp";
        std::printf("Merging %s\n", name.c_str());
        std::FILE *in = std::fopen(name.c_str(), "rb");
        if (in) {
            copy_file(in, out);
            std::fclose(in);
        }
        std::remove(name.c_str());
    }
    std::fclose(out);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 7) {
        std::puts("Syntax: mandelbrot_binary x y radius escape_limit image_size filename");
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
    long cores = get_num_cores();
    if (cores < gNumChildren) gNumChildren = cores;
    if (imageSize < gNumChildren) gNumChildren = imageSize;
    std::printf("Number of children: %ld\n", gNumChildren);
    long firstRow = 0;
    for (long i = 1; i <= gNumChildren; i++) {
        std::string child = filename + "_" + std::to_string(i) + ".tmp";
        long lastRow = (long)((double)imageSize / (double)gNumChildren * (double)i + 0.5);
        long pid = spawn_child(child, firstRow, lastRow, imageSize);
        if (pid > 0) {
            std::printf("Spawn child for %ld...into %s\n", pid, child.c_str());
            gPids[i] = pid;
        } else {
            std::printf("Spawn child for %ld failed!\n", pid);
            gPids[i] = 0;
        }
        firstRow = lastRow;
    }
    for (long i = 1; i <= gNumChildren; i++) {
        if (gPids[i] > 0) {
            std::printf("Waiting for %ld...\n", gPids[i]);
            wait_pid(gPids[i]);
        }
    }
    combine_output(filename, imageSize);
    return 0;
}
