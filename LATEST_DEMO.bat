@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Invoke-Shanmen.ps1" -Action LaunchLatest -TaskId "Manual.LatestDemo"
set "LAUNCH_EXIT=%ERRORLEVEL%"
if not "%LAUNCH_EXIT%"=="0" (
  echo Latest Demo launch failed with exit code %LAUNCH_EXIT%.
  echo Run AUDIT_FOUNDATION.bat for a non-launching diagnostic.
  pause
)
exit /b %LAUNCH_EXIT%
