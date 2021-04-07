#!/usr/bin/env bash

set -xeuo pipefail

num_workers=$(sysctl -n hw.physicalcpu)
skip_opensim_download=${skip_opensim_download:0}

# (if building OpenSim): get OpenSim 4.1 source
if [[ "${skip_opensim_download}" -eq "0" ]]; then
    git clone --single-branch --branch 4.2 --depth=1 https://github.com/opensim-org/opensim-core
fi

# ----- setup: get dependencies ----- #

# because a transitive dependency internally uses wget
brew install wget

# because a transitive dependency (ipopt) needs gfortran
brew reinstall gcc

# ----- build: build OpenSim (optional) then build osmv ----- #

# (if building OpenSim): build OpenSim's dependencies
mkdir -p opensim-dependencies-build/
cd opensim-dependencies-build/
cmake ../opensim-core/dependencies \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install \
    -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer"
cmake --build . -- -j ${num_workers}
cd -

# (if building OpenSim): build OpenSim
mkdir -p opensim-build/
cd opensim-build/
cmake ../opensim-core/ \
    -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ \
    -DCMAKE_INSTALL_PREFIX=../opensim-install/ \
    -DBUILD_JAVA_WRAPPING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" \
    -DOPENSIM_WITH_CASADI=NO \
    -DOPENSIM_WITH_TROPTER=NO
cmake --build . --target install -- -j ${num_workers}
cd -

# build OSMV
mkdir -p osmv-build/
cd osmv-build/
cmake .. \
    -DCMAKE_PREFIX_PATH=${PWD}/../opensim-install \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --target package -- -j ${num_workers}

