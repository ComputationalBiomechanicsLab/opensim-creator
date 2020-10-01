# osmv

Lightweight UI for OpenSim

**Unstable WIP: do not use**

`osmv` is a lightweight OpenSim model viewer, coded in C++20, that is
only externally dependent on
[opensim-core](https://github.com/opensim-org/opensim-core) (v4.1) and
OpenGL.


# Building

Built with CMake and a suitable C++ compiler (I tried it with g++,
clang, and MSVC).

Requires that you have built OpenSim 4.1 according to its manual. Set
`CMAKE_PREFIX_PATH` to point to the *install* created by that build.

## Linux/Mac

```bash
#!/usr/bin/env sh

# CHANGE ME: ensure you have built + installed opensim 4.1 somewhere
OPENSIM_INSTALL=~/Desktop/osc/master/opensim-core-install/lib/cmake

# get source code
git clone https://github.com/adamkewley/osmv.git

# configure
mkdir osmv/RelWithDebInfo-build
cd osmv/RelWithDebInfo-build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=${OPENSIM_INSTALL}

# build
cmake --build .

# run
./osmv ../resources/Rajagopal2015.osim
```

## Windows

TODO
