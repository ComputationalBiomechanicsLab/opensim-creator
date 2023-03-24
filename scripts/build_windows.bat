REM Windows: end-2-end build
REM
REM     - this script should be able to build osc on a clean Windows 2019
REM       Server PC /w basic toolchain installs (Visual Studio, MSVC, CMake)

REM build types:
REM
REM DRAGONS: it seems that this *must* be a `Release` build in OpenSim >4.2
REM DRAGONS: because the upstream developers lazily packaged Release-mode
REM DRAGONS: library binaries
IF NOT DEFINED OSC_OPENSIM_DEPS_BUILD_TYPE (set OSC_OPENSIM_DEPS_BUILD_TYPE=RelWithDebInfo)
IF NOT DEFINED OSC_OPENSIM_BUILD_TYPE (set OSC_OPENSIM_BUILD_TYPE=RelWithDebInfo)
IF NOT DEFINED OSC_BUILD_TYPE (set OSC_BUILD_TYPE=RelWithDebInfo)

REM maximum number of build jobs to run concurrently
REM
REM defaulted to 1, rather than `%NUMBER_OF_PROCESSORS%`, because OpenSim
REM requires a large amount of RAM--more than most machines have--to build
REM concurrently, #659
IF NOT DEFINED OSC_BUILD_CONCURRENCY (set OSC_BUILD_CONCURRENCY=1)

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

REM ----- build dependencies -----
mkdir osc-dependencies-build
cd osc-dependencies-build
cmake -S ../third_party -B . %OSC_CMAKE_GENFLAGS% -DCMAKE_INSTALL_PREFIX="../osc-dependencies-install" || exit /b
cmake --build . --config %OSC_BUILD_TYPE% -j%OSC_BUILD_CONCURRENCY% || exit /b
echo "----- dependencies built, printing build dir -----"
dir .

echo "----- dependencies built, printing install dir -----"
dir ../osc-dependencies-install

REM pop back into the top-level dir
cd ..

REM ----- build osc -----
mkdir osc-build
cd osc-build
cmake ..  %OSC_CMAKE_GENFLAGS% -DCMAKE_PREFIX_PATH="%cd%/../osc-dependencies-install" || exit \b

REM ----- test osc -----
REM
REM but don't run Renderer tests, because the Windows CI machine doesn't have OpenGL4 :<
cmake --build . --target testosc --config %OSC_BUILD_TYPE% -j%OSC_BUILD_CONCURRENCY% || exit \b
.\%OSC_BUILD_TYPE%\testosc --gtest_filter='-Renderer*' || exit \b

# then build final package
cmake --build . --target package --config %OSC_BUILD_TYPE% || exit \b

REM ----- final checks: if there was an error, try to exit this script with an error -----
if %errorlevel% neq 0 exit /b %errorlevel%
