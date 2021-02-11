#!/usr/bin/env bash

# Ubuntu Xenial: end-2-end build
#
#     - this should build on a clean install of Ubuntu Xenial

# error out of this script if it fails for any reason
set -xeuo pipefail

# if root is running this script then do not use `sudo` (some distros
# do not supply it)
if [[ "${UID}" == 0 ]]; then
    sudo=''
else
    sudo='sudo'
fi

# ----- setup: get dependencies: you will need root/sudo for some of this ----- #

# osmv: get gcc-8/g++-8 - needed for C++17 compilation
${sudo} apt-get update
${sudo} apt-get install -y build-essential software-properties-common
${sudo} add-apt-repository -y ppa:ubuntu-toolchain-r/test
${sudo} apt-get update
${sudo} apt-get install -y gcc-snapshot
${sudo} apt-get install -y gcc-8 g++-8

# osmv: get other tools/libraries used by osmv
${sudo} apt-get install -y cmake make libsdl2-dev

# (if building OpenSim): get OpenSim build dependencies
${sudo} apt-get install -y git cmake freeglut3-dev libxi-dev libxmu-dev liblapack-dev wget

# (if building OpenSim): get OpenSim 4.1 source
git clone --single-branch --branch 4.1 --depth=1 https://github.com/opensim-org/opensim-core


# ----- build: build OpenSim (optional) then build osmv ----- #

# (if building OpenSim): build OpenSim's dependencies
mkdir -p opensim-dependencies-build/
cd opensim-dependencies-build/
cmake ../opensim-core/dependencies -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install
cmake --build . -- -j$(nproc)
cd -

# (if building OpenSim): build OpenSim
mkdir -p opensim-build/
cd opensim-build/
cmake ../opensim-core/ -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ -DCMAKE_INSTALL_PREFIX=../opensim-install/ -DBUILD_JAVA_WRAPPING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install -- -j$(nproc)
cd -

# build osmv /w gcc8
#
# note: set CMAKE_PREFIX_PATH to your OpenSim install if you aren't building OpenSim
#       from source (above)
#
# -static-libstdc++
#
#     statically compile libstdc++, because we are building osmv with
#     a not-standardized-in-xenial gcc (8), which will make the output
#     binary dependent on a libstdc++ that doesn't come with typical
#
# -Wl,--no-as-needed
#
#     the Xenial linker will only link libraries if their symbols are
#     NEEDED by the binary. OSMV doesn't *directly* use symbols in
#     (e.g.) libosimActuators.so, but it does indirectly depend on
#     libosimActuators.so being loaded (because the library has
#     side-effects on loading)
mkdir osmv-build/
cd osmv-build/
CC=gcc-8 CXX=g++-8 cmake ../osmv \
  -DCMAKE_PREFIX_PATH=${PWD}/../opensim-install/lib/cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -Wl,--no-as-needed"
cmake --build . --target package -- -j$(nproc)
cd -

# build osmv again, but in debug mode
#
# this is so we have a debug build /w libASAN etc. available for
# end-users to download and try out. Can be helpful to provide them
# with a debug build for repros
mkdir osmv-debug-build/
cd osmv-debug-build/
CC=gcc-8 CXX=g++-8 cmake ../osmv \
  -DCMAKE_PREFIX_PATH=${PWD}/../opensim-debug-install/lib/cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -Wl,--no-as-needed"
cmake --build . --target package -- -j$(nproc)
cd -

# (if you want to install the .deb onto your system)
# apt-get install -yf ./osmv-*.deb

# (if you want to boot osmv)
# ./osmv  # boot from the build dir
# osmv    # boot the installed package (at /opt/osmv/bin/osmv)
