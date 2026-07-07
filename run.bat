@echo off
setlocal

set MEMSIZE=16M
set STACKSIZE=8K

if exist "%~1.o64" del "%~1.o64"

java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" Assembler %1.asm
if exist "%~1.o64" (
    java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" Simulation %1.o64 --verbose --mem=%MEMSIZE% --stack=%STACKSIZE% %2 %3 %4 %5 %6 %7 %8 %9
)
