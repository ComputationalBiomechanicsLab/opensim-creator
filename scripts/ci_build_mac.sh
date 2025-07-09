#!/usr/bin/env bash
#
# Performs an end-to-end CI build of OpenSim Creator. This is what build
# agents should run if they want to build release binaries of OpenSim
# Creator on MacOS.

set -xeuo pipefail

# Specifies the target CPU architecture for the build
export OSC_OSX_ARCHITECTURES=${OSC_OSX_ARCHITECTURES:-x86_64}

# Specifies the target OSX version for the build
export OSC_TARGET_OSX_VERSION=${OSC_TARGET_OSX_VERSION:-14.5}

# Hide window creation in CI, because CI runners typically do not have
# a desktop environment.
export OSC_INTERNAL_HIDE_WINDOW=${OSC_INTERNAL_HIDE_WINDOW:-1}

# Ensure dependencies are re-checked/re-built if the CI script is ran on
# a potentially stale/cached workspace directory.
export OSCDEPS_BUILD_ALWAYS=${OSCDEPS_BUILD_ALWAYS:-ON}

# Explicitly set architecture and SDK version in CI, so that release
# binaries work on users' with supported systems.
export OSC_CMAKE_CONFIG_EXTRA="-DCMAKE_OSX_ARCHITECTURES=${OSC_OSX_ARCHITECTURES} -DCMAKE_OSX_DEPLOYMENT_TARGET=${OSC_TARGET_OSX_VERSION} -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX${OSC_TARGET_OSX_VERSION}.sdk/"

# Run generic MacOS buildscript
./scripts/build_mac.sh

# run after-build OS-specific test scripts
./scripts/macos_check-dependencies.py build/osc/osc
./scripts/macos_check-sdk.py "${OSC_TARGET_OSX_VERSION}" build/osc/osc
