@echo off
setlocal
set "BUILD_BAT=C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat"
set "PROJECT_FILE=%~dp0demo_map.uproject"
if not exist "%BUILD_BAT%" exit /b 2
if not exist "%PROJECT_FILE%" exit /b 3
call "%BUILD_BAT%" demo_mapEditor Win64 Development -Project="%PROJECT_FILE%" -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if errorlevel 1 exit /b %ERRORLEVEL%
call "%BUILD_BAT%" demo_map Win64 Development -Project="%PROJECT_FILE%" -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
exit /b %ERRORLEVEL%
