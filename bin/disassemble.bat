@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

:: Determine the directory and base name of the input file
for %%F in ("%~1") do (
    set "ASM_DIR=%%~dpF"
    set "BASE_NAME=%%~nxF"
)

:: Determine obj directory
set "OBJ_DIR=%ASM_DIR%obj"

java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" cloud.lesh.CPUSim64.Disassembler "%OBJ_DIR%\%BASE_NAME%.o64" %2
