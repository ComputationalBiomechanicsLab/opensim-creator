#!/usr/bin/env bash

# Ubuntu Xenial: end-2-end build
#
#     - this should build on a clean install of Ubuntu Xenial /w basic toolchains

# error out of this script if it fails for any reason
set -xeuo pipefail

# if root is running this script then do not use `sudo` (some distros
# do not supply it)
if [[ "${UID}" == 0 ]]; then
    sudo=''
else
    sudo='sudo'
fi


# ----- get dependencies ----- #

# osc: get gcc-8/g++-8 - needed for C++17 compilation
${sudo} apt-get update
${sudo} apt-get install -y build-essential software-properties-common
${sudo} add-apt-repository -y ppa:ubuntu-toolchain-r/test
${sudo} apt-get update
${sudo} apt-get install -y gcc-snapshot
${sudo} apt-get install -y gcc-8 g++-8

# osc: get other tools/libraries used by osc
${sudo} apt-get install -y cmake make libsdl2-dev libgtk-3-dev

# osc: get transitive dependencies from OpenSim
${sudo} apt-get install -y git cmake freeglut3-dev libxi-dev libxmu-dev liblapack-dev wget

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
cmake --build . -- -j$(nproc)
cd -

# OpenSim: build
mkdir -p opensim-build/
cd opensim-build/
cmake ../opensim-core/ \
    -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ \
    -DCMAKE_INSTALL_PREFIX=../opensim-install/ \
    -DBUILD_JAVA_WRAPPING=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install -- -j$(nproc)
cd -

# osc: build DEB installer /w gcc8 (for C++17 support)
#
# note: set CMAKE_PREFIX_PATH to your OpenSim install if you aren't building OpenSim
#       from source (above)
#
# -static-libstdc++
#
#     statically compile libstdc++, because we are building osc with
#     a not-standardized-in-xenial gcc (8), which will make the output
#     binary dependent on a libstdc++ that doesn't come with typical distros
mkdir osc-build/
cd osc-build/
CC=gcc-8 CXX=g++-8 cmake .. \
  -DCMAKE_PREFIX_PATH=${PWD}/../opensim-install/lib/cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++"
cmake --build . --target package -- -j$(nproc)
cd -

# ----- install (example) -----

# apt-get install -yf ./osc-*.deb

# ----- boot (example) -----

# ./osc  # boot from the build dir
# osc    # boot the installed DEB (at /opt/osc/bin/osc)
