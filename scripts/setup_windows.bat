@echo off

REM setup project-level python virtual environment
python scripts/setup_venv.py
IF %ERRORLEVEL% NEQ 0 echo Failed to setup virtual environment & exit /b %ERRORLEVEL%

REM ensure the dependencies build uses the same Visual Studio environment
REM as the main build
call "scripts/env_visual-studio.bat"
IF %ERRORLEVEL% NEQ 0 echo Failed to source Visual Studio environment & exit /b %ERRORLEVEL%

REM build dependencies
python scripts/build-dependencies.py
IF %ERRORLEVEL% NEQ 0 echo Failed to build opynsim's dependencies & exit /b %ERRORLEVEL%
