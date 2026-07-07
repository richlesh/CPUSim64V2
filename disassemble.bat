@echo off
setlocal

java -Dfile.encoding=UTF8 -cp "%~dp0lib\*" Disassembler %1.o64 %2
