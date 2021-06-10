REM Setup devenv: QtCreator
REM
REM sets up build dir that can be opened + ran in QtCreator easily
REM
REM the reason this exists is because the main buildscript (build_windows.bat)
REM produces a traditional VS2019 (no integrated CMake support) build, which can
REM have multiple build-time-specified configs (e.g. --config Release) whereas
REM this produces a Linux-style, CMake-style Ninja build that has a 
REM configure-time-specified config (e.g. CMAKE_BUILD_TYPE) that works better
REM with QtCreator
REM
REM usage:
REM     - Build everything as normal (i.e. run build_windows.bat)
REM     - Run this script
REM     - Open source DIR in QtCreator
REM     - Open Project -> Source Dir -> CMakeLists.txt
REM     - "Import existing build"
REM     - Browse to the osc-build-ninja/ dir and open that
REM     - QtCreator *should* understand the dir



REM ----- build osc with Ninja linking to the built OpenSim -----

echo "Printing DIR (for build debugging)"
dir .
mkdir osc-build-ninja
cd osc-build-ninja
cmake .. -GNinja -DCMAKE_C_COMPILER=MSVC -DCMAKE_CXX_COMPILER=MSVC -DCMAKE_PREFIX_PATH=%cd%/../opensim-install/cmake || exit /b
cmake --build . --target osc || exit /b
echo "Printing DIR (for build debugging)"
dir .
cd ..
