REM Setup devenv: VS2019
REM
REM VS2019 comes with built-in support for CMake, so the "trick" here is
REM just to ensure that CMake can initially run in VS2019.
REM
REM it mostly can "just work" with VS2019's native CMake implementation. The
REM only drawback is that it can't locate an OpenSim installation
