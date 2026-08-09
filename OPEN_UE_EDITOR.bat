@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Invoke-Shanmen.ps1" -Action OpenEditor -TaskId "Manual.Editor"
exit /b %ERRORLEVEL%
