#!/usr/bin/env bash
#
# Performs an end-to-end CI build of OpenSim Creator. This is what build
# agents should run if they want to build release arm64 binaries of OpenSim
# Creator on MacOS.

set -xeuo pipefail

export OSC_OSX_ARCHITECTURES=arm64

./scripts/ci_build_mac.sh
