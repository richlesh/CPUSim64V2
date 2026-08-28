@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"

:: Determine the directory and base name of the input file
for %%F in ("%~1") do (
    set "ASM_DIR=%%~dpF"
    set "BASE_NAME=%%~nxF"
)

:: Create obj directory relative to the .asm file location
set "OBJ_DIR=%ASM_DIR%obj"
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

:: Skip assembly if .o64 is newer than .asm source
if exist "%OBJ_DIR%\%BASE_NAME%.o64" (
    for %%A in ("%~1.asm") do set "ASM_TIME=%%~tA"
    for %%O in ("%OBJ_DIR%\%BASE_NAME%.o64") do set "O64_TIME=%%~tO"
    :: Use xcopy /D to check if source is newer
    echo n | xcopy /D "%~1.asm" "%OBJ_DIR%\%BASE_NAME%.o64" >nul 2>&1
    if errorlevel 1 (
        echo Up to date: %OBJ_DIR%\%BASE_NAME%.o64
        exit /b 0
    )
)

:: Remove old output files from obj directory
if exist "%OBJ_DIR%\%BASE_NAME%.o64" del "%OBJ_DIR%\%BASE_NAME%.o64"
if exist "%OBJ_DIR%\%BASE_NAME%.sym" del "%OBJ_DIR%\%BASE_NAME%.sym"
if exist "%OBJ_DIR%\%BASE_NAME%.sym1" del "%OBJ_DIR%\%BASE_NAME%.sym1"
if exist "%OBJ_DIR%\%BASE_NAME%.sym2" del "%OBJ_DIR%\%BASE_NAME%.sym2"
if exist "%OBJ_DIR%\%BASE_NAME%.srcmap" del "%OBJ_DIR%\%BASE_NAME%.srcmap"

:: Also clean up any old files in the source directory
if exist "%~1.o64" del "%~1.o64"
if exist "%~1.sym" del "%~1.sym"
if exist "%~1.sym1" del "%~1.sym1"
if exist "%~1.sym2" del "%~1.sym2"
if exist "%~1.srcmap" del "%~1.srcmap"

set "hasMain="
findstr /i /r /c:"^[ 	]*__MAIN__:" "%~1.asm" >nul 2>&1
if not errorlevel 1 set "hasMain=--hasMain"

java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" cloud.lesh.CPUSim64.Assembler "%~1.asm" %hasMain% %2 %3 %4 %5 %6 %7 %8 %9

:: Move output files to obj directory
if exist "%~1.o64" move "%~1.o64" "%OBJ_DIR%\%BASE_NAME%.o64" >nul
if exist "%~1.sym" move "%~1.sym" "%OBJ_DIR%\%BASE_NAME%.sym" >nul
if exist "%~1.sym1" move "%~1.sym1" "%OBJ_DIR%\%BASE_NAME%.sym1" >nul
if exist "%~1.sym2" move "%~1.sym2" "%OBJ_DIR%\%BASE_NAME%.sym2" >nul
if exist "%~1.srcmap" move "%~1.srcmap" "%OBJ_DIR%\%BASE_NAME%.srcmap" >nul
