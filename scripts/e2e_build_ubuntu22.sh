#!/usr/bin/env bash

set -xeuo pipefail

# Ensure dependencies are re-checked/re-built if the CI script is ran on
# a potentially stale/cached workspace directory.
export OSCARDEPS_BUILD_ALWAYS=${OSCARDEPS_BUILD_ALWAYS:-ON}

# These are necessary on Ubuntu22 because it has an older compiler
export CC=gcc-12
export CXX=g++-12

# Build dependencies
cd third_party && cmake --workflow --preset oscar_third-party_Release ; cd -

# Run full build workflow
xvfb-run cmake --workflow --preset oscar_Release
