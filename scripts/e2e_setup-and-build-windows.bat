@echo off
setlocal enabledelayedexpansion

REM performs an end-to-end build of opynsim on Windows

REM set default configuration if none provided
IF "%~1"=="" (
    set CONFIGS=Release
) ELSE (
    set CONFIGS=%*
)

REM create project-level python virtual environment
python scripts/setup_venv.py
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to setup virtual environment
    exit /b %ERRORLEVEL%
)

REM ensure this script uses the Visual Studio environment (C++)
call "scripts/env_visual-studio.bat"
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to source the Visual Studio environment
    exit /b %ERRORLEVEL%
)

REM ensure this script uses the project-level python virtual environment
call "scripts/env_venv.bat"
IF %ERRORLEVEL% NEQ 0 (
    echo Failed to source the project-level venv
    exit /b %ERRORLEVEL%
)

FOR %%C IN (%CONFIGS%) DO (
    REM build dependencies
    echo Entering third_party directory for %%C
    cd third_party
    echo Building dependencies for %%C
    python scripts/setup_dependencies.py %%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build opynsim's dependencies
       exit /b !ERRORLEVEL!
    )
    cd ..

    REM build the project
    echo Building main project for %%C
    cmake --workflow --preset OPynSim_%%C
    IF !ERRORLEVEL! NEQ 0 (
       echo Failed to build opynsim for %%C
       exit /b !ERRORLEVEL!
    )
)

endlocal
