@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"

:: Relocatable modules and linking: assemble.bat -r name [-o out] [options]
::                                   assemble.bat -o out a b ... [options]
:: (inputs may be given with or without .asm; .ro64 modules are linked as they are)
set "NEEDS_LINKER="
set "INPUTS=0"
for %%A in (%*) do (
    set "ARG=%%~A"
    if "!ARG!"=="-r" set "NEEDS_LINKER=1"
    if "!ARG!"=="-o" set "NEEDS_LINKER=1"
    if not "!ARG:~0,1!"=="-" set /a INPUTS+=1
)
if !INPUTS! GTR 1 set "NEEDS_LINKER=1"
if defined NEEDS_LINKER goto :linkmode
goto :singlemode

:linkmode
set "ARGS="
set "RMODE="
set "HAVE_OUT="
set "FIRST="
set "PREV="
for %%A in (%*) do (
    set "ARG=%%~A"
    if "!PREV!"=="-o" (
        set "ARGS=!ARGS! "!ARG!""
        set "PREV="
        set "HAVE_OUT=1"
    ) else if "!ARG!"=="-o" (
        set "ARGS=!ARGS! -o"
        set "PREV=-o"
    ) else if "!ARG!"=="-r" (
        set "ARGS=!ARGS! -r"
        set "RMODE=1"
    ) else if "!ARG:~0,1!"=="-" (
        set "ARGS=!ARGS! !ARG!"
    ) else (
        set "IN=!ARG!"
        if /i not "!IN:~-4!"==".asm" if /i not "!IN:~-5!"==".ro64" set "IN=!IN!.asm"
        set "ARGS=!ARGS! "!IN!""
        if not defined FIRST set "FIRST=!IN!"
    )
)
if defined RMODE if not defined HAVE_OUT if !INPUTS! EQU 1 (
    for %%F in ("!FIRST!") do (
        if not exist "%%~dpFobj" mkdir "%%~dpFobj"
        set "ARGS=!ARGS! -o "%%~dpFobj\%%~nF""
    )
)
java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" cloud.lesh.CPUSim64.Assembler !ARGS!
exit /b %errorlevel%

:singlemode
:: Determine the directory and base name of the input file
for %%F in ("%~1") do (
    set "ASM_DIR=%%~dpF"
    set "BASE_NAME=%%~nxF"
)

:: Create obj directory relative to the .asm file location
set "OBJ_DIR=%ASM_DIR%obj"
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

:: A program without .asm source (e.g. linked with link.bat) is left alone
if not exist "%~1.asm" (
    if exist "%OBJ_DIR%\%BASE_NAME%.o64" (
        echo No source %~1.asm: using the existing %BASE_NAME%.o64
        exit /b 0
    )
    if exist "%~1.o64" (
        echo No source %~1.asm: using the existing %BASE_NAME%.o64
        exit /b 0
    )
    echo Can't find file: %~1.asm
    exit /b 3
)

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
