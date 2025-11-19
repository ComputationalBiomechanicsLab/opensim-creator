@echo off

REM setup project-level python virtual environment
python scripts/setup_venv.py
IF %ERRORLEVEL% NEQ 0 echo Failed to setup virtual environment & exit /b %ERRORLEVEL%

REM build dependencies
cd third_party/ && cmake --workflow --preset OPynSim_third-party_Release && cd ..
IF %ERRORLEVEL% NEQ 0 echo Failed to build opynsim's dependencies & exit /b %ERRORLEVEL%
