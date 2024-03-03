#!/usr/bin/env bash

set -xeuo pipefail

# install emsdk
#if [ -e emsdk/ ];
     #git clone https://github.com/emscripten-core/emsdk.git
     #cd emsdk/
     #./emsdk install latest
     #cd -
#fi

# activate emsdk
#./emsdk/emsdk activate latest
#source ./emsdk/emsdk_env.sh

# build oscar (only) with emsdk
#
# - this is a custom build for now because I can't build all dependencies
#   with emsdk yet

CXXFLAGS="-fexceptions --use-port=sdl2" CC=emcc CXX=em++ cmake -S third_party/ -B osc-deps-build -DOSCDEPS_GET_BENCHMARK=OFF -DOSCDEPS_GET_SDL=OFF -DOSCDEPS_GET_GLEW=OFF -DOSCDEPS_GET_NATIVEFILEDIALOG=OFF -DOSCDEPS_GET_OPENSIM=OFF -DOSCDEPS_EMSCRIPTEN=ON -DCMAKE_INSTALL_PREFIX=${PWD}/osc-deps-install -DCMAKE_INSTALL_LIBDIR=${PWD}/osc-deps-install/lib
cmake --build osc-deps-build -j20 -v

LDFLAGS="-fexceptions -sNO_DISABLE_EXCEPTION_CATCHING=1 -sLEGACY_GL_EMULATION=1 -sUSE_WEBGL2=1 -sUSE_SDL=2" CXXFLAGS="-fexceptions --use-port=sdl2" CC=emcc CXX=em++ cmake -S . -B osc-build -DOSC_EXECUTABLE_SUFFIX=".html" -DOSC_BUILD_OPENSIMCREATOR=OFF -DOSC_BUILD_BENCHMARKS=OFF -DOSC_BUILD_INSTALLABLE_PACKAGE=OFF -DOSC_DISCOVER_TESTS=OFF -DOSC_EMSCRIPTEN=ON -DCMAKE_PREFIX_PATH=${PWD}/osc-deps-install
cmake --build osc-build --target testoscar -v -j20

# run test suite, excluding tests that depend on window/files (work-in-progress)
node osc-build/tests/testoscar/testoscar.js  --gtest_filter=-Renderer*:Image*:ResourceStream*
