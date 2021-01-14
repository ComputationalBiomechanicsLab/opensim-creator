# osmv

Lightweight UI for OpenSim

**Unstable WIP: do not use**

`osmv` is a lightweight OpenSim model viewer, coded in C++20, that is
only externally dependent on
[opensim-core](https://github.com/opensim-org/opensim-core) (v4.1) and
OpenGL.


# Building

## Ubuntu (Xenial, /w `gcc-9` from toolchain test repo)

This builds everything (incl. OpenSim) from scratch. Skip to `BUILD
OSMV` if you already have `gcc-9` and an OpenSim install (specify with
`CMAKE_PREFIX_PATH`, like below).

```bash
# DEPENDENCIES: get gcc-9
apt-get update
apt-get install -y build-essential software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install -y gcc-snapshot
apt-get install -y gcc-9 g++-9
# note: this installs a newer libstdc++: you *should* be able to run OSMVs
#       compiled with gcc9+ at this point

# OPTIONAL: build OpenSim from source

# OpenSim: acquire binary dependencies
apt-get install git cmake freeglut3-dev libxi-dev libxmu-dev liblapack-dev wget

# OpenSim: get 4.1 release source
#     - note: 4.2 has *a lot* more dependencies, and doesn't seem to
#             build as easily on Xenial
git clone --single-branch --branch 4.1 --depth=1 https://github.com/opensim-org/opensim-core

# OpenSim: build OpenSim's source dependencies
mkdir -p opensim-dependencies-build/
cd opensim-dependencies-build/
CC=gcc-9 CXX=g++-9 cmake ../opensim-core/dependencies -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../opensim-dependencies-install
cmake --build . -- -j$(nproc)
cd -

# OpenSim: build OpenSim
mkdir -p opensim-build/
cd opensim-build/
CC=gcc-9 CXX=g++-9 cmake ../opensim-core/ -DOPENSIM_DEPENDENCIES_DIR=../opensim-dependencies-install/ -DCMAKE_INSTALL_PREFIX=../opensim-install/ -DBUILD_JAVA_WRAPPING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install -- -j$(nproc)
cd -


# BUILD OSMV

# OSMV OPTIONAL DEPENDENCIES (only do this on headless servers):
#
#     - install a OpenGL software renderer
apt-get install libgl1-mesa-dev libglu1-mesa-dev

# osmv: acquire binary dependencies
apt-get install cmake make libsdl2-dev

# osmv: get source
git clone https://github.com/adamkewley/osmv

# osmv: build
mkdir osmv-build/
cd osmv2-build/
CC=gcc-9 CXX=g++-9 cmake ../osmv -DCMAKE_PREFIX_PATH=../opensim-install/lib/cmake
cmake --build . --target osmv -- -j$(nproc)

# (optional): package build into a standalone .deb for
# distribution to end-users
# cmake --build . --target package

# (optional #2): install the package
# apt-get install -yf ./osmv-0.0.1-Linux.deb

# (optional #3): boot osmv
# ./osmv  # boot from the build dir
# osmv    # boot installed package
```

## Windows

You can either use the CMake GUI to configure a visual studio
project or run this in a terminal:

```

cmake -S . -B build/
cmake --build build/ --config Release --target package
```
