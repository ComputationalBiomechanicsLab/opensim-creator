@echo off

REM `run_via_fontforge_python.bat`
REM
REM A hack to run python scripts that use the FontForge python API to work
REM on a Windows install.

REM Setup environment variables that FontForge needs
set FF=C:\Program Files (x86)\FontForgeBuilds\
set "PYTHONHOME=%FF%"
set "PATH=%FF%;%FF%\bin;%PATH:"=%"
set FF_PATH_ADDED=TRUE

REM Put FontForge's modules onto the PYTHONPATH environment variable
for /F "tokens=* USEBACKQ" %%f IN (`dir /b "%FF%lib\python*"`) do (
    set "PYTHONPATH=%FF%lib\%%f"
)

REM Forward this script's arguments to FontForge's python interpreter
ffpython %*
