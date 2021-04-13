#!/usr/bin/env bash

# Ubuntu 20 (Groovy) / Debian Buster: end-2-end build
#
#     - this script should run to completion on a clean install of the
#       OSes and produce a ready-to-use osc build
#
#     - run this from the repo root dir


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

${sudo} apt-get update

# osc: main dependencies
${sudo} apt-get install -y build-essential cmake libsdl2-dev libgtk-3-dev

# osc: transitive dependencies from OpenSim
${sudo} apt-get install -y git freeglut3-dev libxi-dev libxmu-dev liblapack-dev wget

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
echo "DEBUG: listing contents of OpenSim dependencies build dir"
ls .
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
echo "DEBUG: listing contents of OpenSim build dir"
ls .
cd -

# osc: build DEB installer
#
#     note #1: if you're providing your own OpenSim install, set
#     CMAKE_PREFIX_PATH to that install path
#
#     note #2: if you just want to build osc, switch
#              `--target package` for `--target osc`
mkdir -p osc-build/
cd osc-build/
cmake .. -DCMAKE_PREFIX_PATH=../opensim-install/lib/cmake
cmake --build . --target package -- -j$(nproc)
echo "DEBUG: listing contents of final build dir"
ls .


# ----- install (example) -----

# apt-get install -yf ./osc-*.deb

# ----- boot (example) -----

# ./osc  # boot from the build dir
# osc    # boot the installed DEB (at /opt/osc/bin/osc)
