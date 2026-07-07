@echo off
setlocal

set MEMSIZE=1M
set STACKSIZE=8K

:: Determine the directory and base name of the input file
set "ASM_DIR=%~dp1"
set "BASE_NAME=%~n1"
set "OBJ_DIR=%ASM_DIR%obj"

:: Create obj directory if it doesn't exist
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

:: Remove old output files
if exist "%OBJ_DIR%\%BASE_NAME%.o64" del "%OBJ_DIR%\%BASE_NAME%.o64"
if exist "%~1.o64" del "%~1.o64"

java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" Assembler %1.asm --DEBUG

:: Move output files to obj directory
if exist "%~1.o64" move "%~1.o64" "%OBJ_DIR%\%BASE_NAME%.o64" >nul
if exist "%~1.sym" move "%~1.sym" "%OBJ_DIR%\%BASE_NAME%.sym" >nul
if exist "%~1.srcmap" move "%~1.srcmap" "%OBJ_DIR%\%BASE_NAME%.srcmap" >nul

:: Look for .o64 in obj directory, fall back to same directory as .asm
set "O64_FILE="
if exist "%OBJ_DIR%\%BASE_NAME%.o64" (
    set "O64_FILE=%OBJ_DIR%\%BASE_NAME%.o64"
) else if exist "%~1.o64" (
    set "O64_FILE=%~1.o64"
)

if defined O64_FILE (
    java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" Simulation "%O64_FILE%" --verbose --trace --mem=%MEMSIZE% --stack=%STACKSIZE% %2 %3 %4 %5 %6 %7 %8 %9
)
