@echo off
setlocal

set "OUTPUT=CPUSim64V2.zip"

:: Remove existing file if needed
if exist "%OUTPUT%" del "%OUTPUT%"

:: Create zip using PowerShell (available on all modern Windows)
powershell -NoProfile -Command ^
    "Compress-Archive -Path 'pom.xml','*.sh','*.bat','lib','src','documentation','NOTICE','LICENSE','README.md' -DestinationPath '%OUTPUT%' -Force"

echo Created %OUTPUT%
