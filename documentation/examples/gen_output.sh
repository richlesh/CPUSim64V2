rm *.obj.gz
rm *.sym*
rm *.out
echo hello_world.asm
./runout.sh hello_world > hello_world.out
echo example001.asm
../../assemble.sh example001 > example001c.out
../../debug.sh example001 > example001d.out
../../run.sh example001 > example001r.out
echo example003.asm
./debugout.sh example003 > example003.out
echo example004.asm
./debugout.sh example004 > example004.out
echo example005.asm
./debugout.sh example005 > example005.out
echo example006.asm
./debugout.sh example006 > example006.out
echo example007.asm
./debugout.sh example007 > example007.out
echo example008.asm
./debugout.sh example008 > example008.out
echo example009.asm
./debugout.sh example009 > example009.out
echo example010.asm
./debugout.sh example010 > example010.out
echo example011.asm
./debugout.sh example011 > example011.out
echo example012.asm
./runout.sh example012 > example012.out
echo example013.asm
./runout.sh example013 > example013.out
echo example014.asm
./runout.sh example014 > example014.out
echo example015.asm
./runout.sh example015 326 Hello 3.14 > example015.out
echo example016.asm
./runout.sh example016 326 > example016.out
(echo; echo) >> example016.out
./runout.sh example016 3.1415 >> example016.out
(echo; echo) >> example016.out
./runout.sh example016 abcd >> example016.out
echo example017.asm
./runout.sh example017 > example017.out
(echo; echo) >> example017.out
./runout.sh example017 326 >> example017.out
(echo; echo) >> example017.out
./runout.sh example017 3.14 Hello >> example017.out
echo example018.asm
./runout.sh example018 > example018.out
echo example019.asm
./runout.sh example019 5 12 > example019.out
(echo; echo) >> example019.out
./runout.sh example019 8 -3 >> example019.out
echo example020.asm
./runout.sh example020 5 12 > example020.out
(echo; echo) >> example020.out
./runout.sh example020 8 -3 >> example020.out
echo example021.asm
./runout.sh example021 5 12 > example021.out
(echo; echo) >> example021.out
./runout.sh example021 8 -3 >> example021.out
echo example022.asm
./runout.sh example022 > example022.out
echo example_math.asm
./runout.sh example_math > example_math.out
echo example_io.asm
./runout.sh example_io < example_io.in > example_io.out
echo example_string.asm
./runout.sh example_string > example_string.out
echo example_system.asm
./runout.sh example_system > example_system.out
echo example_fileio_textwrite.asm
./runout.sh example_fileio_textwrite alphabets.txt > example_fileio_textwrite.out
echo example_fileio_textread.asm
./runout.sh example_fileio_textread alphabets.txt > example_fileio_textread.out
echo example_fileio_textreadline.asm
./runout.sh example_fileio_textreadline alphabets.txt > example_fileio_textreadline.out
echo example_fileio_rawwrite.asm
./runout.sh example_fileio_rawwrite test.raw > example_fileio_rawwrite.out
echo example_fileio_rawread.asm
./runout.sh example_fileio_rawread test.raw > example_fileio_rawread.out
echo example_for.asm
./runout.sh example_for > example_for.out
echo example_while.asm
./runout.sh example_while > example_while.out
echo example_break.asm
./runout.sh example_break > example_break.out
echo example_fork.asm
./runout.sh example_fork > example_fork.out
echo example_fork2.asm
./runout.sh example_fork2 > example_fork2.out
echo example_thread.asm
./runout.sh example_thread > example_thread.out
echo example_mutex.asm
./runout.sh example_mutex > example_mutex.out
echo example_heap.asm
./runout.sh example_heap > example_heap.out
echo example_vector.asm
./runout.sh example_vector > example_vector.out

rm *.obj.gz
rm *.sym*
