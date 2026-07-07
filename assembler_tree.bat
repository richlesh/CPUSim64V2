@echo off
setlocal

call "%~dp0preprocess.bat" %1 %2 %3 %4 %5 %6 %7 %8 %9
java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.CPUSim64 program -tokens %1.pp.asm -tokens -gui
