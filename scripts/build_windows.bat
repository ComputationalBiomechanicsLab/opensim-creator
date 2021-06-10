REM Windows: end-2-end build
REM
REM     - this script should be able to build osc on a clean Windows 2019 Server PC /w basic
REM       toolchain installs (Visual Studio, MSVC, CMake)

REM this can be any branch identifier from opensim-core
set OPENSIM_VERSION=4.2

REM change this if you want to try a different generator+arch combo
set GENFLAGS=-G"Visual Studio 16 2019" -A x64

REM it seems that this *must* be Release in OpenSim >4.2 because upstream packaged
REM Release-mode binaries (fml)
set CONFIG=Release

REM ----- checkout OpenSim sources from GitHub -----

echo "Printing DIR (for build debugging)"
dir .
if not exist "opensim-core" (
    git clone --single-branch --branch %OPENSIM_VERSION% --depth=1 https://github.com/opensim-org/opensim-core || exit /b
)
echo "Printing DIR (for build debugging)"
dir opensim-core


REM ----- build OpenSim's dependencies (e.g. Simbody) -----

dir .
echo "Printing DIR (for build debugging)"
mkdir opensim-dependencies-build
cd opensim-dependencies-build
cmake ../opensim-core/dependencies %GENFLAGS% -DCMAKE_INSTALL_PREFIX=../dependencies-install || exit /b
cmake --build . --config %CONFIG% -j%NUMBER_OF_PROCESSORS% || exit /b
echo "Printing DIR (for build debugging)"
dir .
cd ..


REM ----- build OpenSim itself -----

echo "Printing DIR (for build debugging)"
dir .
mkdir opensim-build
cd opensim-build
cmake ../opensim-core %GENFLAGS% -DOPENSIM_DEPENDENCIES_DIR=../dependencies-install -DBUILD_JAVA_WRAPPING=OFF -DCMAKE_INSTALL_PREFIX=../opensim-install || exit /b
cmake --build . --config %CONFIG% --target install -j%NUMBER_OF_PROCESSORS% || exit /b
echo "Printing DIR (for build debugging)"
dir .
cd ..


REM ----- build osc, linking to the built OpenSim -----

echo "Printing DIR (for build debugging)"
dir .
mkdir osc-build
cd osc-build
cmake .. %GENFLAGS% -DCMAKE_PREFIX_PATH=%cd%/../opensim-install/cmake || exit /b
cmake --build . --config %CONFIG% --target package -j%NUMBER_OF_PROCESSORS% || exit /b
echo "Printing DIR (for build debugging)"
dir .
cd ..


REM ----- final checks: if there was an error, try to exit this script with an error -----
if %errorlevel% neq 0 exit /b %errorlevel%
