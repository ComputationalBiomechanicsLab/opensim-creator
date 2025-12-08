@echo off
REM Performs an end-to-end CI build of OpenSim Creator.
REM This is what build agents should run to build release amd64 binaries of OpenSim Creator on Windows.

REM Ensure dependencies are re-checked/re-built if the CI script is run on
REM a potentially stale/cached workspace directory.
set "OSCDEPS_BUILD_ALWAYS=ON"

REM Activate the latest Visual Studio environment (so that Ninja, cmake, etc. are available)
call scripts/env_vs-x64.bat

REM --system-version is necessary because, otherwise, the wrong Windows SDK might be chosen,
REM resulting in either missing headers or shipping a binary that doesn't run on target systems.
REM The specified SDK supports Windows 10, version 1507 or higher.
python scripts\build.py --system-version=10.0.26100.0 --generator=Ninja %*
if errorlevel 1 (
    echo Command failed with exit code %errorlevel%
    exit /b %errorlevel%
)
