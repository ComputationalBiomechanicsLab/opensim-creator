REM Windows: end-2-end build
REM
REM     - this should build on a clean Windows 2019 Server PC

REM checkout OpenSim 4.1
git clone --single-branch --branch master --depth=1 https://github.com/opensim-org/opensim-core

REM build OpenSim dependencies
dir .
mkdir opensim-dependencies-build
cd opensim-dependencies-build
cmake ../opensim-core/dependencies -G"Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=../dependencies-install
cmake --build . --config Release
dir .
cd ..

REM build OpenSim
mkdir opensim-build
cd opensim-build
cmake ../opensim-core -G"Visual Studio 16 2019" -A x64 -DOPENSIM_DEPENDENCIES_DIR=../dependencies-install -DBUILD_JAVA_WRAPPING=OFF -DCMAKE_INSTALL_PREFIX=../opensim-install
cmake --build . --config Release --target install
cd ..

mkdir osmv-build
cd osmv-build
cmake ../ -G"Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH=../opensim-install
cmake --build . --config Release --target osmv
cd ..
