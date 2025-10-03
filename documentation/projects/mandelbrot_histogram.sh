#!/bin/bash

run.sh mandelbrot_histogram_balanced $1 $2 $3 $4 $5 mandelbrot_histogram_$1_$2_$3_$4_$5

open mandelbrot_histogram_$1_$2_$3_$4_$5.pgm
