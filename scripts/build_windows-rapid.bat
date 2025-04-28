REM Performs an end-to-end build of OpenSim Creator REM on Windows systems using
REM Ninja, which comes bundled with Visual Studio.
REM
REM This uses hard-coded paths etc. because this is mostly useful for rapidly getting
REM a build environment up on my Windows machine. Use at your own peril etc.

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
python scripts/build_windows.py --build-type=RelWithDebInfo --generator=Ninja
