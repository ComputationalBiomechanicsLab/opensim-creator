#!/usr/bin/env bash

# (if building OpenSim): get OpenSim 4.1 source
git clone --single-branch --branch 4.1 --depth=1 https://github.com/opensim-org/opensim-core

# ----- build: build OpenSim (optional) then build osmv ----- #

# (if building OpenSim): build OpenSim's dependencies
mkdir -p opensim-dependencies-build/
cd opensim-dependencies-build/
cmake ../opensim-core/dependencies -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install
cmake --build . -- -j 4
cd -

# (if building OpenSim): build OpenSim
mkdir -p opensim-build/
cd opensim-build/
cmake ../opensim-core/ -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ -DCMAKE_INSTALL_PREFIX=../opensim-install/ -DBUILD_JAVA_WRAPPING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install -- -j 4
cd -

mkdir osmv-build/
cd osmv-build/
cmake ../osmv -DCMAKE_PREFIX_PATH=../opensim-install/lib/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --target osmv -- -j 4

