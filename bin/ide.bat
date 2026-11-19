@echo off
set "SCRIPT_DIR=%~dp0"

java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" com.glowingcat.cpusim64ide.IDEApp %*
