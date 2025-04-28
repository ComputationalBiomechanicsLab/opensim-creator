#!/usr/bin/env bash
#
# Sets up an Ubuntu >=22.04 machine with the  necessary dependencies to
# build and debug OpenSim Creator.

# propagate any failures to this script
set -xeuo pipefail

# at least ensure the normal build dependencies are installed
bash ./scripts/setup_ubuntu.sh

apt_debug_dependencies=(
    clang       # for an easier clang-tidy build
    clang-tidy  # for static analysis of OSC's code
    gdb         # for interactive debugging of any runtime issues
    valgrind    # for debugging memory leaks, bad memory accesses, etc.
)

# these are the dependencies tools that other scripts (e.g. `build_ubuntu_debugging.sh`) need
sudo apt-get install -y ${apt_debug_dependencies[@]}
