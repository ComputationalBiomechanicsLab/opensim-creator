@echo off

REM setup the build machine (e.g. build dependencies)
call "scripts/setup_windows.bat"
IF %ERRORLEVEL% NEQ 0 echo Failed to setup windows & exit /b %ERRORLEVEL%

REM ensure this script uses the Visual Studio environment (C++) and
REM project-level virtual environment (python)
call "scripts/env_visual-studio.bat"
IF %ERRORLEVEL% NEQ 0 echo Failed to source the Visual Studio environment & exit /b %ERRORLEVEL%
call "scripts/env_venv.bat"
IF %ERRORLEVEL% NEQ 0 echo Failed to source the project-level venv & exit /b %ERRORLEVEL%

REM build the project
python scripts/build.py --generator=Ninja
IF %ERRORLEVEL% NEQ 0 echo Failed to build opynsim & exit /b %ERRORLEVEL%
