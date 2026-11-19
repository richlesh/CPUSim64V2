@echo off
:: Links relocatable modules (.ro64 + .rel, made with assemble.bat -r) into a runnable .o64.
:: Usage: link.bat [-o name] [--DEBUG] [-Dsymbol[=value]] module.ro64 [more.ro64 ...]
:: -o is required when more than one module is given. Modules that a module
:: #includes (e.g. <system/io.asm>) are found or assembled automatically.
setlocal
set "SCRIPT_DIR=%~dp0"
java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" cloud.lesh.CPUSim64.Linker %*
exit /b %errorlevel%
