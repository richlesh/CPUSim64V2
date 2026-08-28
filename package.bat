@echo off
setlocal

call "%~dp0clean.bat"
if exist lib\* del /q lib\*
call mvn clean
call mvn package -DskipTests=true
copy target\CPUSim64*.jar lib\ >nul
copy ide\target\CPUSim64IDE-2.8.0.jar lib\ >nul
copy cpusim64\target\cpusim64-2.8.0.jar lib\ >nul
