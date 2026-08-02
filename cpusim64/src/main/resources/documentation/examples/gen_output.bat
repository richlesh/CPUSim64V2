@echo off
del *.out
call runout.bat hello_world > hello_world.out
call compile.bat example001 > example001c.out
call debug.bat example001 > example001d.out
call run.bat example001 > example001r.out
call trace.bat example001 > example001t.out
call debugout.bat example003 > example003.out
call debugout.bat example004 > example004.out
call debugout.bat example005 > example005.out
call debugout.bat example006 > example006.out
call debugout.bat example007 > example007.out
call debugout.bat example008 > example008.out
call debugout.bat example009 > example009.out
call debugout.bat example010 > example010.out
call debugout.bat example011 > example011.out
call runout.bat example012 > example012.out
call debugout.bat example013 > example013.out
call runout.bat example014 > example014.out
call runout.bat example015 326 Hello 3.14 > example015.out
call runout.bat example016 326 > example016.out
echo. >> example016.out
echo. >> example016.out
call runout.bat example016 3.1415 >> example016.out
echo. >> example016.out
echo. >> example016.out
call runout.bat example016 abcd >> example016.out
call runout.bat example017 > example017.out
echo. >> example017.out
echo. >> example017.out
call runout.bat example017 326 >> example017.out
echo. >> example017.out
echo. >> example017.out
call runout.bat example017 3.14 Hello >> example017.out
call runout.bat example018 > example018.out
call runout.bat example019 5 12 > example019.out
echo. >> example019.out
echo. >> example019.out
call runout.bat example019 8 -3 >> example019.out
call runout.bat example020 5 12 > example020.out
echo. >> example020.out
echo. >> example020.out
call runout.bat example020 8 -3 >> example020.out
call runout.bat example021 5 12 > example021.out
echo. >> example021.out
echo. >> example021.out
call runout.bat example021 8 -3 >> example021.out
call runout.bat example022 > example022.out
call runout.bat example_math > example_math.out
call runout.bat example_string > example_string.out
call runout.bat example_system > example_system.out
call runout.bat example_fileio_textwrite alphabets.txt > example_fileio_textwrite.out
call runout.bat example_fileio_textread alphabets.txt > example_fileio_textread.out
call runout.bat example_fileio_rawwrite test.raw > example_fileio_rawwrite.out
call runout.bat example_fileio_rawread test.raw > example_fileio_rawread.out

