@echo off

REM ensure this script uses the Visual Studio environment (C++)
call "scripts/env_visual-studio.bat"
IF %ERRORLEVEL% NEQ 0 echo Failed to source the Visual Studio environment & exit /b %ERRORLEVEL%

REM setup the build machine (e.g. build dependencies)
call "scripts/setup_windows.bat"
IF %ERRORLEVEL% NEQ 0 echo Failed to setup windows & exit /b %ERRORLEVEL%

REM source the created project-level virtual environment
call "scripts/env_venv.bat"
IF %ERRORLEVEL% NEQ 0 echo Failed to source the project-level venv & exit /b %ERRORLEVEL%

REM build the project
cmake --workflow --preset OPynSim_Release
