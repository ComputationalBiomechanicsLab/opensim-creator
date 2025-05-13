#!/usr/bin/env bash
#
# Performs an end-to-end build of OpenSim Creator for
# arm64 (i.e. Apple Silicon) MacOS systems.

export OSC_CMAKE_CONFIG_EXTRA="-DCMAKE_OSX_DEPLOYMENT_TARGET=14.2 -DCMAKE_OSX_ARCHITECTURES=arm64"
export OSC_BUILD_CONCURRENCY=$(sysctl -n hw.ncpu)
./scripts/build_mac.sh
