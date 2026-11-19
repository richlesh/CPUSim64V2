@echo off
setlocal enabledelayedexpansion

set "MEMSIZE=16M"
set "STACKSIZE=8K"

set "SCRIPT_DIR=%~dp0"

:: Determine the directory and base name of the input file
for %%F in ("%~1") do (
    set "ASM_DIR=%%~dpF"
    set "BASE_NAME=%%~nxF"
)

:: Determine obj directory
set "OBJ_DIR=%ASM_DIR%obj"

call "%SCRIPT_DIR%assemble.bat" %1 --DEBUG

:: Look for .o64 in obj directory, fall back to same directory as .asm
set "O64_FILE="
if exist "%OBJ_DIR%\%BASE_NAME%.o64" (
    set "O64_FILE=%OBJ_DIR%\%BASE_NAME%.o64"
) else if exist "%~1.o64" (
    set "O64_FILE=%~1.o64"
)

if defined O64_FILE (
    java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" cloud.lesh.CPUSim64.Simulation "%O64_FILE%" --verbose --trace --mem=%MEMSIZE% --stack=%STACKSIZE% %2 %3 %4 %5 %6 %7 %8 %9
)
