@echo off
setlocal

if exist "%~1.pp.asm" del "%~1.pp.asm"

set "hasMain="
findstr /i /r /c:"^[[:space:]]*__MAIN__:" "%~1.asm" >nul 2>&1
if not errorlevel 1 set "hasMain=--hasMain"

java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" Preprocessor %1.asm %hasMain% %2 %3 %4 %5 %6 %7 %8 %9
