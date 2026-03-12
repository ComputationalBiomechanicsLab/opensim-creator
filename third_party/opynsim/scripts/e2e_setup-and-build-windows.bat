@echo off
setlocal enabledelayedexpansion

REM Performs an end-to-end build of opynsim on Windows

REM If no arguments, default to "Release"
IF "%~1"=="" (
    set CONFIGS=Release
) ELSE (
    set CONFIGS=%*
)

REM Setup project-level python virtual environment
python scripts/setup_venv.py
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to setup virtual environment
    exit /b %ERRORLEVEL%
)

REM Activate Visual Studio (C++) environment for this script, so that `cmake` etc. work
call "scripts/env_vs-x64.bat"
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to source the Visual Studio environment
    exit /b %ERRORLEVEL%
)

REM Build each specified configuration
FOR %%C IN (%CONFIGS%) DO (

    REM Build bundled dependencies
    echo Entering third_party directory for %%C
    cd third_party
    cmake --workflow --preset %%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build opynsim's dependencies
       exit /b !ERRORLEVEL!
    )
    cd ..

    REM # Build the main project
    echo Building main project for %%C
    cmake --workflow --preset %%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build opynsim for %%C
       exit /b !ERRORLEVEL!
    )
)

endlocal
