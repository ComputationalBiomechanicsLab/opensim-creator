#!/usr/bin/env bash
#
# Performs an end-to-end CI build of OpenSim Creator. This is what build
# agents should run if they want to build release arm64 binaries of OpenSim
# Creator on MacOS.

set -xeuo pipefail

# Hide window creation in CI, because CI runners typically do not have
# a desktop environment.
export OSC_INTERNAL_HIDE_WINDOW="1"

# Ensure dependencies are re-checked/re-built if the CI script is ran on
# a potentially stale/cached workspace directory.
export OSCDEPS_BUILD_ALWAYS=${OSCDEPS_BUILD_ALWAYS:-ON}

export OSC_OSX_ARCHITECTURES=arm64

./scripts/ci_build_mac.sh
