#!/usr/bin/env bash
#
# Performs an end-to-end CI build of OpenSim Creator. This is what build
# agents should run if they want to build release amd64 binaries of OpenSim
# Creator on Ubuntu 22.04

set -xeuo pipefail

# If no arguments, default to a "Release" build
if [ "$#" -eq 0 ]; then
    CONFIGS=("Release")
else
    CONFIGS=("$@")
fi

# Ensure dependencies are re-checked/re-built if the CI script is ran on
# a potentially stale/cached workspace directory.
export OSCDEPS_BUILD_ALWAYS=${OSCDEPS_BUILD_ALWAYS:-ON}
export CC=gcc-12  # Needed for C++20
export CXX=g++-12  # Needed for C++20

for CONFIG in "${CONFIGS[@]}"; do
    echo "=== Building configuration: $CONFIG ==="

    # build bundled dependencies
    cd third_party && cmake --workflow --preset "$CONFIG" && cd -

    # build the main project under `xvfb-run` (for UI tests)
    xvfb-run cmake --workflow --preset "$CONFIG"
done

