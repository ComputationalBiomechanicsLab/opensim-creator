#!/usr/bin/env bash
#
# Performs an end-to-end CI build of OpenSim Creator. This is what build
# agents should run if they want to build release amd64 binaries of OpenSim
# Creator on Ubuntu 22.04

set -xeuo pipefail

# Ensure dependencies are re-checked/re-built if the CI script is ran on
# a potentially stale/cached workspace directory.
export OSCDEPS_BUILD_ALWAYS=${OSCDEPS_BUILD_ALWAYS:-ON}

# Run buildscript under virtual desktop with `xvfb-run` (for UI tests)
CC=gcc-12 CXX=g++-12 OSC_BUILD_CONCURRENCY=$(nproc) xvfb-run ./scripts/build.py
