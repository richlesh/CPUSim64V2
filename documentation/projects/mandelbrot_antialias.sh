#!/bin/bash

large=$(($5*2));

run.sh mandelbrot_raw $1 $2 $3 $4 $large mandelbrot_$1_$2_$3_$4_$large

// Uses ImageMagick commands
convert mandelbrot_$1_$2_$3_$4_$large.pgm -filter Gaussian -resize 50% mandelbrot_$1_$2_$3_$4_$5.pgm

open mandelbrot_$1_$2_$3_$4_$5.pgm