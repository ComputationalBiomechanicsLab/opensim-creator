#!/usr/bin/env bash

set -xeuo pipefail

# Ensure dependencies are re-checked/re-built if the CI script is ran on
# a potentially stale/cached workspace directory.
export OSCARDEPS_BUILD_ALWAYS=${OSCARDEPS_BUILD_ALWAYS:-ON}

# Build dependencies
cd third_party && cmake --workflow --preset Release; cd -

# Run full build workflow
cmake --workflow --preset Release
