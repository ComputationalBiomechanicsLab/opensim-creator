#!/usr/bin/env bash
#
# `build_emscripten.sh`: performs an end-to-end build of the `hellotriangle` demo
# of OpenSim Creator using the Emscripten SDK (`emmake`, `emcc`, etc.) toolchain
# to build a wasm version of the binary
#
#     usage (must be ran in repository root): `bash build_emscripten.sh`

set -xeuo pipefail

# "base" build type to use when build types haven't been specified
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for all of OSC's dependencies
OSC_DEPS_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE-`echo ${OSC_BASE_BUILD_TYPE}`}

# install emsdk
if [ ! -d emsdk ]; then
     git clone https://github.com/emscripten-core/emsdk.git
     cd emsdk/
     ./emsdk install latest
     cd -
fi

# activate emsdk
./emsdk/emsdk activate latest
source ./emsdk/emsdk_env.sh

# build oscar (only) with emsdk
#
# - this is a custom build for now because I can't build all dependencies
#   with emsdk yet

CXXFLAGS="-fexceptions --use-port=sdl2" emcmake cmake -S third_party/ -B osc-deps-build -DOSCDEPS_GET_SDL=OFF -DOSCDEPS_GET_GLEW=OFF -DOSCDEPS_GET_NATIVEFILEDIALOG=OFF -DOSCDEPS_GET_OPENSIM=OFF -DCMAKE_INSTALL_PREFIX=${PWD}/osc-deps-install -DCMAKE_INSTALL_LIBDIR=${PWD}/osc-deps-install/lib -DCMAKE_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE}
emmake cmake --build osc-deps-build -j$(nproc) -v

LDFLAGS="-fexceptions -sNO_DISABLE_EXCEPTION_CATCHING=1 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sFULL_ES2=1 -sFULL_ES3=1 -sUSE_SDL=2" CXXFLAGS="-fexceptions --use-port=sdl2" emcmake cmake -S . -B osc-build -DOSC_BUILD_OPENSIMCREATOR=OFF -DOSC_DISCOVER_TESTS=OFF -DOSC_EMSCRIPTEN=ON -DCMAKE_PREFIX_PATH=${PWD}/osc-deps-install -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH -DCMAKE_BUILD_TYPE=-DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE}
emmake cmake --build osc-build --target testoscar -v -j$(nproc)
emmake cmake --build osc-build --target hellotriangle -v -j$(nproc)

# run test suite, excluding tests that depend on window/files (work-in-progress)
node osc-build/tests/testoscar/testoscar.js  --gtest_filter=-Renderer*:Image*:ResourceStream*
