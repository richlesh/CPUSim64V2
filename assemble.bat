@echo off
setlocal

if exist "%~1.o64" del "%~1.o64"

set "hasMain="
findstr /i /r /c:"^[[:space:]]*__MAIN__:" "%~1.asm" >nul 2>&1
if not errorlevel 1 set "hasMain=--hasMain"

java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" Assembler %1.asm %hasMain% %2 %3 %4 %5 %6 %7 %8 %9
