#!/usr/bin/env bash

# Performs an end-to-end build of OpenSim Creator with debugging options
# enabled. This is usaully used to verify a release, or lint the source
# code.

set -xeuo pipefail

# configure+build dependencies
cd third_party && cmake --workflow --preset ErrorCheck && cd -

# configure+build+test OpenSim Creator
cmake --workflow --preset ErrorCheck

