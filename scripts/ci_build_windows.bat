@echo off
setlocal enabledelayedexpansion

REM Performs an end-to-end CI build of OpenSim Creator.
REM This is what build agents should run to build release amd64 binaries of OpenSim Creator on Windows.

REM set default configuration if none provided
IF "%~1"=="" (
    set CONFIGS=Release
) ELSE (
    set CONFIGS=%*
)

REM Activate the latest Visual Studio environment (so that Ninja, cmake, etc. are available)
call scripts/env_vs-x64.bat
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to source the Visual Studio environment
    exit /b %ERRORLEVEL%
)

REM Perform specified end-to-end builds
FOR %%C IN (%CONFIGS%) DO (
    REM Configure + build dependencies
    echo Entering third_party directory for %%C
    cd third_party
    cmake --workflow --preset %%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build dependencies
       exit /b !ERRORLEVEL!
    )
    cd ..

    REM Build OpenSim Creator
    echo Building main project for %%C
    cmake --workflow --preset %%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build %%C
       exit /b !ERRORLEVEL!
    )
)
