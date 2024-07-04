#!/usr/bin/env bash

# `build_linux_valgrind-compat`: performs an end-to-end build of OpenSim Creator
# with build flags that make it valgrind-compatible. This is handy for debugging
# things like memory accesses, segfaults, etc.
#
#     usage (must be ran in repository root): `bash build_linux_valgrind-compat.sh`

set -xeuo pipefail

# the reason why 'gdwarf-4' is necessary is because the clang compiler
# in Ubuntu20 emits gdwarf-5 symbols, but the valgrind in Ubuntu20 isn't
# new enough to support them
export CFLAGS=-gdwarf-4
export CXXFLAGS=-gdwarf-4
export CC=clang
export CXX=clang++

cmake -S third_party/ -B osc-deps-build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=${PWD}/osc-deps-install
cmake --build osc-deps-build -j20
cmake -S . -B osc-build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=${PWD}/osc-deps-install
cmake --build osc-build/ -j20

valgrind_cmd="valgrind --leak-check=full"

${valgrind_cmd} -- ./osc-build/tests/testoscar/testoscar
${valgrind_cmd} -- ./osc-build/tests/testoscar_demos/testoscar_demos
${valgrind_cmd} -- ./osc-build/tests/testoscar_learnopengl/testoscar_learnopengl
${valgrind_cmd} -- ./osc-build/tests/TestOpenSimCreator/TestOpenSimCreator
