#!/usr/bin/env bash
#
# Performs an end-to-end build of OpenSim Creator with build flags
# that make it valgrind-compatible. This is handy for debugging things
# like memory accesses, segfaults, etc.

set -xeuo pipefail

cd third_party && cmake --workflow --preset RelWithDebInfo && cd -
cmake --workflow --preset RelWithDebInfo

LIBGL_ALWAYS_SOFTWARE=1 valgrind \
    --leak-check=full \
    --trace-children=yes \
    --suppressions=${PWD}/scripts/suppressions_valgrind.supp \
    -- \
    ctest --test-dir build/RelWithDebInfo --output-on-failure
