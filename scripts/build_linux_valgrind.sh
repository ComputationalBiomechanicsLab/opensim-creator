#!/usr/bin/env bash
#
# Performs an end-to-end build of OpenSim Creator with build flags
# that make it valgrind-compatible. This is handy for debugging things
# like memory accesses, segfaults, etc.

set -xeuo pipefail

cmake \
    -S third_party/ \
    -B osc-deps-build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=${PWD}/osc-deps-install
cmake --build osc-deps-build -j$(nproc)
cmake \
    -S . \
    -B osc-build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=${PWD}/osc-deps-install
cmake --build osc-build/ -j$(nproc)

export LIBGL_ALWAYS_SOFTWARE=1
valgrind_cmd="valgrind --leak-check=full --trace-children=yes --suppressions=${PWD}/scripts/valgrind_suppressions.supp"
${valgrind_cmd} ctest --test-dir osc-build --output-on-failure
