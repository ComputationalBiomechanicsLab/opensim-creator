@echo off
setlocal enabledelayedexpansion

REM performs an end-to-end build of opynsim on Windows

REM set default configuration if none provided
IF "%~1"=="" (
    set CONFIGS=Release
) ELSE (
    set CONFIGS=%*
)

REM Environment setup: Create a project-level python virtual environment
python scripts/setup_venv.py
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to setup virtual environment
    exit /b %ERRORLEVEL%
)

REM Environment activation: ensure this script uses the Visual Studio (C++) and python virtual environment
call "scripts/env_vs-x64.bat"
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to source the Visual Studio environment
    exit /b %ERRORLEVEL%
)

REM Perform specified end-to-end builds
FOR %%C IN (%CONFIGS%) DO (
    REM build dependencies
    echo Entering third_party directory for %%C
    cd third_party
    cmake --workflow --preset %%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build opynsim's dependencies
       exit /b !ERRORLEVEL!
    )
    cd ..

    REM build the project
    echo Building main project for %%C
    cmake --workflow --preset %%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build opynsim for %%C
       exit /b !ERRORLEVEL!
    )
)

endlocal
