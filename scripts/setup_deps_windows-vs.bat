REM Builds the project's third-party dependencies on Windows, ready for the
REM main build to proceed (either via an IDE or build script).

@echo off
setlocal

REM Handle (optional) user arguments, which can be a list of build types that the
REM dependencies should be built for (e.g. Debug, RelWithDebInfo, Release, MinSizeRel)
if "%~1"=="" (
    REM No arguments, default to Debug
    set CONFIGS=Debug
) else (
    REM Use all arguments as configurations
    set CONFIGS=%*
)

REM Activate Visual Studio environment (puts ninja on the PATH, etc.)
call scripts/env_vs-x64.bat

REM Loop over each configuration and build the dependencies
for %%C in (%CONFIGS%) do (
    echo === Building dependencies for %%C ===
    cmake -G "Ninja" -S "third_party" -B "third_party-build-%%C" -DCMAKE_INSTALL_PREFIX="%cd%\third_party-install-%%C" -DCMAKE_BUILD_TYPE="%%C"
    if errorlevel 1 exit /b 1
    cmake --build "third_party-build-%%C"
    if errorlevel 1 exit /b 1
)

endlocal
