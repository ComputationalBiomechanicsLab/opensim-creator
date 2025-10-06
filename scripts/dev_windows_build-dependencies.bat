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

REM Use `vswhere` to assign `VSPATH` to the caller's Visual Studio install location.
REM This is necessary because the user may have installed their Visual Studio elsewhere,
REM or might be using a different version.
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    echo vswhere.exe not found in default location, you might want to set VSPATH in this script instead
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VSPATH=%%i
)
if not defined VSPATH (
    echo Visual Studio with VC++ tools not found
    echo     You might need to hack a bit and set VSPATH in this script to the install location, e.g.:
    echo         set VSPATH=C:\Program Files\Microsoft Visual Studio\2022\Community\
    exit /b 1
)

REM Pull `VSPATH`'s variables, PATH, etc. into this script, so that we have direct access
REM to Visual Studio's copy of `cmake`, `Ninja`, etc.
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=amd64

REM Loop over each configuration and build the dependencies
for %%C in (%CONFIGS%) do (
    echo === Building dependencies for %%C ===
    cmake -G "Ninja" -S "third_party" -B "third_party-build-%%C" -DCMAKE_INSTALL_PREFIX="%cd%\third_party-install-%%C" -DCMAKE_BUILD_TYPE="%%C"
    if errorlevel 1 exit /b 1
    cmake --build "third_party-build-%%C"
    if errorlevel 1 exit /b 1
)

endlocal
