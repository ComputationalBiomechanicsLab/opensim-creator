#!/usr/bin/env bash

# Performs an end-to-end build of OpenSim Creator with debugging options
# enabled. This is usaully used to verify a release, or lint the source
# code.

set -xeuo pipefail

# Configure + build dependencies
cd third_party && cmake --workflow --preset ErrorCheck && cd -

# Build OpenSim Creator
cmake --workflow --preset ErrorCheck
