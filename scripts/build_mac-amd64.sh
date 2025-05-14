#!/usr/bin/env bash
#
# Performs an end-to-end build of OpenSim Creator for
# amd64 (i.e. intel/AMD x86_64) MacOS systems.

export OSC_CMAKE_CONFIG_EXTRA="-DCMAKE_OSX_DEPLOYMENT_TARGET=14.5 -DCMAKE_OSX_ARCHITECTURES=x86_64"
export OSC_BUILD_CONCURRENCY=$(sysctl -n hw.ncpu)
./scripts/build_mac.sh

