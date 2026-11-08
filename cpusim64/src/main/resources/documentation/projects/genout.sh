RUN=../../../../../../run.sh

rm *.o64
rm *.sym*
rm *.srcmap
rm *.out
rm -r obj
rm -r output

echo Base64.asm
$RUN Base64 e data/Lincoln.txt Base64.cipher > /dev/null
$RUN Base64 d Base64.cipher Base64.out > /dev/null

echo BBP_hex_digits.asm
$RUN BBP_hex_digits 100000 > BBP_hex_digits.out

echo BBP_hex_digits_threaded.asm
$RUN BBP_hex_digits_threaded 1000 > BBP_hex_digits_threaded.out

echo BubbleSort.asm
$RUN BubbleSort > BubbleSort.out

echo BubbleSortFloat.asm
$RUN BubbleSortFloat > BubbleSortFloat.out

echo Caesar.asm
$RUN Caesar 13 data/Lincoln.txt Caesar.cipher > /dev/null
$RUN Caesar -13 Caesar.cipher Caesar.out > /dev/null

echo CAS_printf_stress.asm
$RUN CAS_printf_stress > CAS_printf_stress.out

echo Congruent.asm
$RUN Congruent 180 > Congruent.out

echo Cubic.asm
$RUN Cubic 1 -1 -2 2 > Cubic.out
$RUN Cubic 1 -2 2 -1 >> Cubic.out

echo Fibonacci.asm
$RUN Fibonacci > Fibonacci.out

echo Hailstone.asm
$RUN Hailstone > Hailstone.out

echo Hailstone2.asm
$RUN Hailstone2 10000 > Hailstone2.out

echo Hailstone3.asm
$RUN Hailstone3 10000 > Hailstone3.out

echo Hailstone4.asm
$RUN Hailstone4 10000000 > Hailstone4.out

echo Hailstone5.asm
$RUN Hailstone5 10000000 > Hailstone5.out

echo hashcode.asm
$RUN hashcode data/Lincoln.txt > hashcode.out

echo hashcode2.asm
$RUN hashcode2 data/Lincoln.txt > hashcode2.out

echo holy_grail.asm
echo "Rich\ngrail\n800\n" | $RUN holy_grail  > holy_grail.out
echo "Rich\ngrail\nEuropean or African\n" | $RUN holy_grail  >> holy_grail.out

echo InsertSorted.asm
$RUN InsertSorted > InsertSorted.out

x=-0.0452407411
y=0.9868162204352258
radius=5e-9
escape_limit=1000
image_size=512

echo mandelbrot.asm
$RUN mandelbrot $x $y $radius $escape_limit $image_size mandelbrot > /dev/null

echo mandelbrot_binary.asm
$RUN mandelbrot_binary $x $y $radius $escape_limit $image_size mandelbrot_binary > /dev/null

echo mandelbrot_histogram.asm
$RUN mandelbrot_histogram $x $y $radius $escape_limit $image_size mandelbrot_histogram > /dev/null

echo mandelbrot_histogram_balanced.asm
$RUN mandelbrot_histogram_balanced $x $y $radius $escape_limit $image_size mandelbrot_histogram_balanced > /dev/null

echo md5.asm
$RUN md5 data/Lincoln.txt > md5.out

echo Pi.asm
$RUN Pi 11 > Pi.out

echo PI_spigot.asm
$RUN PI_spigot 1000 > PI_spigot.out

echo progress_bar.asm
$RUN progress_bar > progress_bar.out

echo progress_bar2.asm
$RUN progress_bar2 > progress_bar2.out

echo Quadratic.asm
$RUN Quadratic 1 -1 -2 > Quadratic.out
$RUN Quadratic 1 -2 2 >> Quadratic.out

echo QuickSort.asm
$RUN QuickSort > QuickSort.out

echo QuickSortFloat.asm
$RUN QuickSortFloat > QuickSortFloat.out

echo rainbow.asm
$RUN rainbow > rainbow.out

echo Renard.asm
$RUN Renard 20 > Renard.out

echo Renard2.asm
$RUN Renard2 20 > Renard2.out

echo rot13.asm
$RUN rot13 data/Lincoln.txt rot13.cipher > /dev/null
$RUN rot13 rot13.cipher rot13.out > /dev/null

echo Spinner.asm
$RUN Spinner > Spinner.out

echo Tower_of_Hanoi.asm
$RUN Tower_of_Hanoi 10 > Tower_of_Hanoi.out

echo Unicode.asm
$RUN Unicode > Unicode.out

echo Vigenere.asm
$RUN Vigenere password data/Lincoln.txt Vigenere.cipher > /dev/null
$RUN Vigenere -password Vigenere.cipher Vigenere.out > /dev/null

echo working_bar.asm
$RUN working_bar > working_bar.out

echo Vigenere.asm
$RUN XORCipher password data/Lincoln.txt XORCipher.cipher > /dev/null
$RUN XORCipher password XORCipher.cipher XORCipher.out > /dev/null

mkdir output
mv *.out output
mv *.cipher output
mv *.pgm output

rm -r obj
