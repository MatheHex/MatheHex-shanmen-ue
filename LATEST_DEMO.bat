@echo off
setlocal

rem The development game EXE is not packaged/cooked yet, so it cannot load
rem its shader library directly. Run the current demo map in game mode via
rem the installed UE editor instead.
set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT_FILE=%~dp0demo_map.uproject"
set "DEMO_MAP=/Game/M01/Maps/L_M01_Expedition?Name=Player"

if not exist "%UE_EDITOR%" (
    echo Unreal Editor was not found:
    echo %UE_EDITOR%
    exit /b 2
)

if not exist "%PROJECT_FILE%" (
    echo Demo project was not found:
    echo %PROJECT_FILE%
    exit /b 3
)

start "Shanmen 0.0.9B Latest Demo" "%UE_EDITOR%" "%PROJECT_FILE%" "%DEMO_MAP%" -game
exit /b %ERRORLEVEL%
