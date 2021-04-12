#!/usr/bin/env bash

# Mac Catalina: end-2-end build
#
#     - this script should run to completion on a relatively clean Catalina (OSX 10.5)
#       machine with xcode, CMake, etc. installed


# error out of this script if it fails for any reason
set -xeuo pipefail

num_workers=$(sysctl -n hw.physicalcpu)
skip_opensim_download=${skip_opensim_download:0}

# ----- get dependencies ----- #

# OpenSim: get 4.1 sources
if [[ ! -d opensim-core/ ]]; then
    git clone --single-branch --branch 4.1 --depth=1 https://github.com/opensim-org/opensim-core
fi

# ----- build ----- #

# OpenSim: build dependencies
mkdir -p opensim-dependencies-build/
cd opensim-dependencies-build/
cmake ../opensim-core/dependencies \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install
cmake --build . -- -j ${num_workers}
cd -

# OpenSim: build
mkdir -p opensim-build/
cd opensim-build/
cmake ../opensim-core/ \
    -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ \
    -DCMAKE_INSTALL_PREFIX=../opensim-install/ \
    -DBUILD_JAVA_WRAPPING=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install -- -j ${num_workers}
cd -

# osc: build DMG installer
mkdir -p osc-build/
cd osc-build/
cmake .. -DCMAKE_PREFIX_PATH=${PWD}/../opensim-install -DCMAKE_BUILD_TYPE=Release
cmake --build . --target package -- -j ${num_workers}
