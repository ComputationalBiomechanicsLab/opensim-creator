REM usage: `call scripts\env_vs-x64.bat`
REM
REM sets up a batch/cmd.exe terminal with Visual Studio's variables
REM (ninja on the PATH, etc.) and the python virtual environment

@echo off

REM Use `vswhere` to assign `VSPATH` to the caller's Visual Studio install location.
REM This is necessary because the user may have installed their Visual Studio elsewhere,
REM or might be using a different version.
REM
REM this is effectveily a hardened version of: call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
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
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -host_arch=x64 -arch=amd64

REM Activate the project-local Python virtual environment
call "scripts/env_venv.bat"
