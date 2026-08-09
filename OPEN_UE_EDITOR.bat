@echo off
setlocal
set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT_FILE=%~dp0demo_map.uproject"
if not exist "%UE_EDITOR%" exit /b 2
if not exist "%PROJECT_FILE%" exit /b 3
start "Shanmen 0.0.9B Editor" "%UE_EDITOR%" "%PROJECT_FILE%"
exit /b %ERRORLEVEL%
