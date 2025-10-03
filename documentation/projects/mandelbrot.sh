#!/bin/bash

run.sh mandelbrot_raw $1 $2 $3 $4 $5 mandelbrot_$1_$2_$3_$4_$5

open mandelbrot_$1_$2_$3_$4_$5.pgm
