rm *.out
runout.sh hello_world > hello_world.out
compile.sh example001 > example001c.out
debug.sh example001 > example001d.out
run.sh example001 > example001r.out
trace.sh example001 > example001t.out
debugout.sh example003 > example003.out
debugout.sh example004 > example004.out
debugout.sh example005 > example005.out
debugout.sh example006 > example006.out
debugout.sh example007 > example007.out
debugout.sh example008 > example008.out
debugout.sh example009 > example009.out
debugout.sh example010 > example010.out
debugout.sh example011 > example011.out
runout.sh example012 > example012.out
debugout.sh example013 > example013.out
runout.sh example014 > example014.out
runout.sh example015 326 Hello 3.14 > example015.out
runout.sh example016 326 > example016.out
(echo; echo) >> example016.out
runout.sh example016 3.1415 >> example016.out
(echo; echo) >> example016.out
runout.sh example016 abcd >> example016.out
runout.sh example017 > example017.out
(echo; echo) >> example017.out
runout.sh example017 326 >> example017.out
(echo; echo) >> example017.out
runout.sh example017 3.14 Hello >> example017.out
runout.sh example018 > example018.out
runout.sh example019 5 12 > example019.out
(echo; echo) >> example019.out
runout.sh example019 8 -3 >> example019.out
runout.sh example020 5 12 > example020.out
(echo; echo) >> example020.out
runout.sh example020 8 -3 >> example020.out
runout.sh example021 5 12 > example021.out
(echo; echo) >> example021.out
runout.sh example021 8 -3 >> example021.out
runout.sh example022 > example022.out
runout.sh example_math > example_math.out
runout.sh example_io < example_io.in > example_io.out
runout.sh example_string > example_string.out
runout.sh example_system > example_system.out
runout.sh example_fileio_textwrite alphabets.txt > example_fileio_textwrite.out
runout.sh example_fileio_textread alphabets.txt > example_fileio_textread.out
runout.sh example_fileio_textreadline alphabets.txt > example_fileio_textreadline.out
runout.sh example_fileio_rawwrite test.raw > example_fileio_rawwrite.out
runout.sh example_fileio_rawread test.raw > example_fileio_rawread.out
runout.sh example_for > example_for.out
runout.sh example_while > example_while.out
runout.sh example_break > example_break.out
runout.sh example_fork > example_fork.out
runout.sh example_fork2 > example_fork2.out
runout.sh example_thread > example_thread.out
runout.sh example_mutex > example_mutex.out
runout.sh example_heap > example_heap.out
runout.sh example_vector > example_vector.out
