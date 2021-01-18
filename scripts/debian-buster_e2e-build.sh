#!/usr/bin/env bash

# Debian Buster: end-2-end build
#
#     - this should build on a clean install of Debian Buster/Ubuntu 20

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

${sudo} apt-get update

# osmv: main dependencies
${sudo} apt-get install build-essential cmake libsdl2-dev

# osmv: (if CI) also install a software renderer for OpenGL
if [[ -v CI ]]; then
    ${sudo} apt-get install libgl1-mesa-dev libgl1-mesa-glx libopengl0 libglx0 libglu1-mesa  freeglut3-dev
fi

# osmv: transitive dependencies from OpenSim4.1
${sudo} apt-get install -y git freeglut3-dev libxi-dev libxmu-dev liblapack-dev wget

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

# note: set CMAKE_PREFIX_PATH to your OpenSim install if you aren't building OpenSim
#       from source (above)
mkdir osmv-build/
cd osmv-build/
cmake ../osmv -DCMAKE_PREFIX_PATH=../opensim-install/lib/cmake
cmake --build . --target package -- -j$(nproc)

# (if you want to install the .deb onto your system)
# apt-get install -yf ./osmv-*.deb

# (if you want to boot osmv)
# ./osmv  # boot from the build dir
# osmv    # boot the installed package (at /opt/osmv/bin/osmv)
