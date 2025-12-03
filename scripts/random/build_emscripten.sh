#!/usr/bin/env bash
#
# `build_emscripten.sh`: performs an end-to-end build of the `hellotriangle` demo
# of OpenSim Creator using the Emscripten SDK (`emmake`, `emcc`, etc.) toolchain
# to build a wasm version of the binary
#
#     usage (must be ran in repository root): `bash scripts/build_emscripten.sh`
#     note: assumes `emsdk` is already active (e.g. `./emsdk/emsdk activate latest`)
#     note: assumes `emsdk` is sourced (e.g. `./emsdk/emsdk_env.sh`)

set -xeuo pipefail

# "base" build type to use when build types haven't been specified
OSC_BASE_BUILD_TYPE=${OSC_BASE_BUILD_TYPE:-Release}

# build type for all of OSC's dependencies
OSC_DEPS_BUILD_TYPE=${OSC_DEPS_BUILD_TYPE:-`echo ${OSC_BASE_BUILD_TYPE}`}

# build type for OSC
OSC_BUILD_TYPE=${OSC_BUILD_TYPE-`echo ${OSC_BASE_BUILD_TYPE}`}

# build oscar (only) with emsdk
#
# - this is a custom build for now because I can't build all dependencies
#   with emsdk yet

CXXFLAGS="-fexceptions" emcmake cmake -S third_party/ -B third_party-build-Debug \
    -DOSCDEPS_BUILD_SDL=OFF \
    -DOSCDEPS_BUILD_OPENSIM=OFF \
    -DCMAKE_INSTALL_PREFIX="${PWD}/third_party-install-Debug" \
    -DCMAKE_INSTALL_LIBDIR="${PWD}/third_party-install-Debug/lib"
emmake cmake --build third_party-build-Debug -j$(nproc) -v

LDFLAGS="-fexceptions -sNO_DISABLE_EXCEPTION_CATCHING=1 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sFULL_ES2=1 -sFULL_ES3=1 -sUSE_SDL=2" CXXFLAGS="-fexceptions --use-port=sdl2" emcmake cmake -S . -B build/ \
    -DOSC_DISCOVER_TESTS=OFF \
    -DOSC_EMSCRIPTEN=ON \
    -DCMAKE_PREFIX_PATH="${PWD}/third_party-install-Debug" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH \
    -DCMAKE_BUILD_TYPE=-DCMAKE_BUILD_TYPE=${OSC_BUILD_TYPE}
emmake cmake --build build/ --target testoscar -v -j$(nproc)
emmake cmake --build build/ --target hellotriangle -v -j$(nproc)

# run test suite, excluding tests that depend on window/files (work-in-progress)
node build/liboscar/testing/testoscar/testoscar.js  --gtest_filter=-Renderer*:ShaderTest*:MaterialTest*:Image*:ResourceStream*:load_texture2D_from_image*
