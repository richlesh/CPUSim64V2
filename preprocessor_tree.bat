@echo off
setlocal

java -cp "%~dp0lib\*" org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.Preprocessor preproc -tokens %1.asm -tokens -gui
