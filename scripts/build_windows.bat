REM Windows: end-2-end build
REM
REM     - this script should be able to build osc on a clean Windows 2019
REM       Server PC /w basic toolchain installs (Visual Studio, MSVC, CMake)

REM build types:
REM
REM DRAGONS: it seems that this *must* be a `Release` build in OpenSim >4.2
REM DRAGONS: because the upstream developers lazily packaged Release-mode
REM DRAGONS: library binaries
IF NOT DEFINED OSC_OPENSIM_DEPS_BUILD_TYPE (set OSC_OPENSIM_DEPS_BUILD_TYPE=Release)
IF NOT DEFINED OSC_OPENSIM_BUILD_TYPE (set OSC_OPENSIM_BUILD_TYPE=Release)
IF NOT DEFINED OSC_BUILD_TYPE (set OSC_BUILD_TYPE=Release)

REM maximum number of build jobs to run concurrently
IF NOT DEFINED OSC_BUILD_CONCURRENCY (set OSC_BUILD_CONCURRENCY=%NUM_PROCESSORS%)

REM which OSC build target to build
REM
REM     osc        just build the osc binary
REM     package    package everything into a .deb installer
IF NOT DEFINED OSC_BUILD_TARGET (set OSC_BUILD_TARGET=package)

REM if in-built HTML/sphinx documentation should be built
IF NOT DEFINED OSC_BUILD_DOCS (set OSC_BUILD_DOCS=OFF)

REM change this if you want to try a different generator+arch combo
set OSC_CMAKE_GENFLAGS=-G"Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%OSC_BUILD_TYPE% -DOSC_BUILD_DOCS=%OSC_BUILD_DOCS%

echo "----- starting build -----"
echo ""
echo "----- printing build parameters -----"
echo ""
echo "    OSC_OPENSIM_DEPS_BUILD_TYPE = %OSC_OPENSIM_DEPS_BUILD_TYPE%"
echo "    OSC_OPENSIM_BUILD_TYPE = %OSC_OPENSIM_BUILD_TYPE%"
echo "    OSC_BUILD_TYPE = %OSC_BUILD_TYPE%"
echo "    OSC_BUILD_CONCURRENCY = %OSC_BUILD_CONCURRENCY%"
echo "    OSC_BUILD_TARGET = %OSC_BUILD_TARGET%"
echo ""
echo "----- printing system info -----"
dir .

REM ----- Get Spinx, if necessary -----
set OSC_SHOULD_PIP_INSTALL=NO
IF %OSC_BUILD_DOCS% == "ON" (set OSC_SHOULD_PIP_INSTALL="YES")
IF %OSC_BUILD_DOCS% == "1" (set OSC_SHOULD_PIP_INSTALL="YES")
IF %OSC_BUILD_DOCS% == 1 (set OSC_SHOULD_PIP_INSTALL="YES")
IF %OSC_BUILD_DOCS% == "TRUE" (set OSC_SHOULD_PIP_INSTALL="YES")
IF %OSC_BUILD_DOCS% == "YES" (set OSC_SHOULD_PIP_INSTALL="YES")

IF %OSC_SHOULD_PIP_INSTALL% == "YES" (
    pip install -r docs/requirements.txt
)

REM ----- ensure all submodules are up-to-date -----
git submodule update --init --recursive

echo "----- building OpenSim's dependencies (e.g. Simbody) -----"
mkdir opensim-dependencies-build
cd opensim-dependencies-build
cmake ../third_party/opensim-core/dependencies %OSC_CMAKE_GENFLAGS% -DOPENSIM_WITH_CASADI=OFF -DOPENSIM_WITH_TROPTER=OFF -DCMAKE_INSTALL_PREFIX=../dependencies-install || exit /b
cmake --build . --config %OSC_OPENSIM_DEPS_BUILD_TYPE% -j%OSC_BUILD_CONCURRENCY% || exit /b

echo "----- dependencies built, printing build dir -----"
dir .

echo "----- dependencies built, printing install dir -----"
dir ../dependencies-install

REM pop back into the top-level dir
cd ..


echo "----- building OpenSim -----"
mkdir opensim-build
cd opensim-build
cmake ../third_party/opensim-core %OSC_CMAKE_GENFLAGS% -DOPENSIM_WITH_CASADI=OFF -DOPENSIM_WITH_TROPTER=OFF -DBUILD_API_ONLY=ON -DOPENSIM_DISABLE_LOG_FILE=ON -DOPENSIM_BUILD_INDIVIDUAL_APPS=OFF -DOPENSIM_DEPENDENCIES_DIR=../dependencies-install -DBUILD_JAVA_WRAPPING=OFF -DCMAKE_INSTALL_PREFIX=../opensim-install || exit /b
cmake --build . --config %OSC_OPENSIM_BUILD_TYPE% --target install -j%OSC_BUILD_CONCURRENCY% || exit /b

echo "----- OpenSim built, printing build dir -----"
dir .

echo "----- OpenSim built, printing install dir -----"
dir ../opensim-install

REM pop back into the top-level dir
cd ..

echo "----- building OSC -----"
mkdir osc-build
cd osc-build
cmake .. %OSC_CMAKE_GENFLAGS% -DCMAKE_PREFIX_PATH=%cd%/../opensim-install/cmake -DCMAKE_INSTALL_PATH=%cd%/../osc-install || exit /b
cmake --build . --config %OSC_BUILD_TYPE% --target %OSC_BUILD_TARGET% -j%OSC_BUILD_CONCURRENCY% || exit /b

echo "----- osc built, printing build dir -----"
dir .

REM ----- final checks: if there was an error, try to exit this script with an error -----
if %errorlevel% neq 0 exit /b %errorlevel%
