REM Windows: end-2-end build
REM
REM     - this script should be able to build osc on a clean Windows 2019 Server PC /w basic
REM       toolchain installs (Visual Studio, MSVC, CMake)


REM ----- checkout OpenSim 4.1 sources from GitHub -----

echo "Printing DIR (for build debugging)"
dir .
git clone --single-branch --branch 4.1 --depth=1 https://github.com/opensim-org/opensim-core || exit /b
echo "Printing DIR (for build debugging)"
dir opensim-core


REM ----- build OpenSim's dependencies (e.g. Simbody) -----

dir .
echo "Printing DIR (for build debugging)"
mkdir opensim-dependencies-build
cd opensim-dependencies-build
cmake ../opensim-core/dependencies -G"Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=../dependencies-install || exit /b
cmake --build . --config Release || exit /b
echo "Printing DIR (for build debugging)"
dir .
cd ..


REM ----- build OpenSim itself -----

echo "Printing DIR (for build debugging)"
dir .
mkdir opensim-build
cd opensim-build
cmake ../opensim-core -G"Visual Studio 16 2019" -A x64 -DOPENSIM_DEPENDENCIES_DIR=../dependencies-install -DBUILD_JAVA_WRAPPING=OFF -DCMAKE_INSTALL_PREFIX=../opensim-install || exit /b
cmake --build . --config Release --target install || exit /b
echo "Printing DIR (for build debugging)"
dir .
cd ..


REM ----- build osc, linking to the built OpenSim -----

echo "Printing DIR (for build debugging)"
dir .
mkdir osc-build
cd osc-build
cmake .. -G"Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH=%cd%/../opensim-install/cmake || exit /b
cmake --build . --config RelWithDebInfo --target package || exit /b
echo "Printing DIR (for build debugging)"
dir .
cd ..


REM ----- final checks: if there was an error, try to exit this script with an error -----
if %errorlevel% neq 0 exit /b %errorlevel%
