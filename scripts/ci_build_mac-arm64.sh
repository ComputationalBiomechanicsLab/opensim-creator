#!/usr/bin/env bash
#
# Performs an end-to-end CI build of OpenSim Creator. This is what build
# agents should run if they want to build release amd64 binaries of OpenSim
# Creator on MacOS.

set -xeuo pipefail

OSCDEPS_BUILD_ALWAYS=ON ./scripts/build.py \
    --osx-architectures=arm64 \
    --osx-deployment-target=14.5 \
    "$@"

