@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Invoke-Shanmen.ps1" -Action BuildBoth -Configuration Development -TaskId "Manual.BuildDevelopment"
exit /b %ERRORLEVEL%
