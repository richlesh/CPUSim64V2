@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

if exist "%~1.pp.asm" del "%~1.pp.asm"

set "hasMain="
findstr /i /r /c:"^[ 	]*__MAIN__:" "%~1.asm" >nul 2>&1
if not errorlevel 1 set "hasMain=--hasMain"

java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" cloud.lesh.CPUSim64.Preprocessor "%~1.asm" %hasMain% %2 %3 %4 %5 %6 %7 %8 %9
