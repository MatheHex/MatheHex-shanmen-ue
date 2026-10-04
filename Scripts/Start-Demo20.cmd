@echo off
rem Local uncooked Demo 2.0. Requires the installed UE editor and built project DLLs.
where pwsh.exe >nul 2>&1
if errorlevel 1 (
  powershell.exe -NoProfile -File "%~dp0Start-Demo20.ps1" %*
) else (
  pwsh.exe -NoProfile -File "%~dp0Start-Demo20.ps1" %*
)
if errorlevel 1 pause
