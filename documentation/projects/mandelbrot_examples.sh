#!/bin/bash

size=500

./mandelbrot.sh -0.75 0 1.25 50 $size
./mandelbrot.sh -0.16 1.0405 0.026 200 $size
./mandelbrot.sh -0.722 0.246 0.019 250 $size
./mandelbrot.sh -0.7453 0.1127 7E-4 500 $size
./mandelbrot.sh -0.745428 0.113009 3.5E-5 500 $size
./mandelbrot.sh 0.2549870375144766 -0.0005679790528465 1.0E-4 500 $size
./mandelbrot.sh 0.2549870375144766 -0.0005679790528465 1.0E-5 500 $size
./mandelbrot.sh 0.2549870375144766 -0.0005679790528465 1.0E-6 500 $size
./mandelbrot.sh 0.2549870375144766 -0.0005679790528465 1.0E-10 1500 $size
