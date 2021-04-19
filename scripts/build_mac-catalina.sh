#!/usr/bin/env bash

# Mac Catalina: end-2-end build
#
#     - this script should run to completion on a relatively clean
#       Catalina (OSX 10.5) machine with xcode, CMake, etc. installed


# error out of this script if it fails for any reason
set -xeuo pipefail


# ----- build parameters ----- #

# the number of build workers (parallel build jobs)
num_workers=$(sysctl -n hw.physicalcpu)


# ----- get dependencies ----- #

# reinstall gcc, because OpenSim4.2 seems to depend on gfortran (for
# libBLAS).
#
# this is likely a misconfigured dependency in OpenSim 4.2, because
# libLAPACK/libBLAS are actually available on OSX natively
brew reinstall gcc

# install `wget`
#
#     seems to be used by a transitive dependency of OpenSim 4.2
#     (Metis), which uses wget to download other dependencies
#
# install `automake`
#
#     seems to be used by a transitive dependency of OpenSim 4.2
#     (adolc), which uses `aclocal` for configuration
brew install wget automake

# get OpenSim 4.2 sources
if [[ ! -d opensim-core/ ]]; then
    git clone \
        --single-branch \
        --branch 4.2 \
        --depth=1 \
        https://github.com/opensim-org/opensim-core
fi


# ----- build ----- #

# OpenSim: build dependencies
mkdir -p opensim-dependencies-build/
cd opensim-dependencies-build/
cmake ../opensim-core/dependencies \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install \
    -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer"
cmake --build . -- -j ${num_workers}
cd -

# OpenSim: build OpenSim
mkdir -p opensim-build/
cd opensim-build/
cmake ../opensim-core/ \
    -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ \
    -DCMAKE_INSTALL_PREFIX=../opensim-install/ \
    -DBUILD_JAVA_WRAPPING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" \
    -DOPENSIM_WITH_CASADI=YES \
    -DOPENSIM_WITH_TROPTER=NO
cmake --build . --target install -- -j ${num_workers}
cd -

# osc: build DMG installer
mkdir -p osc-build/
cd osc-build/
cmake .. \
    -DCMAKE_PREFIX_PATH=${PWD}/../opensim-install \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --target package -- -j ${num_workers}
