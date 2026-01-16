REM set default configuration if none provided
IF "%~1"=="" (
    set CONFIGS=Release
) ELSE (
    set CONFIGS=%*
)

REM Ensure this script uses the Visual Studio (C++) environment
call "scripts/env_vs-x64.bat" || (
    echo Failed to source the Visual Studio environment
    exit /b %ERRORLEVEL%
)

REM Perform specified end-to-end builds
FOR %%C IN (%CONFIGS%) DO (
    REM build dependencies
    echo Entering third_party directory for %%C
    cd third_party
    cmake --workflow --preset %%C || (
       echo Failed to build dependencies for %%C
       exit /b !ERRORLEVEL!
    )
    cd ..

    REM build the project
    echo Building main project for %%C
    cmake --workflow --preset %%C || (
       echo Failed to build main project for %%C
       exit /b !ERRORLEVEL!
    )
)
