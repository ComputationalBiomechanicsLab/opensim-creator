#!/usr/bin/env bash
#
# Sets up an Ubuntu 22.04 machine with the  necessary dependencies to
# build and debug OpenSim Creator.

# propagate any failures to this script
set -xeuo pipefail

# at least ensure the normal build dependencies are installed
bash ./scripts/setup_ubuntu.sh

ubuntu_22_dependencies=(
    gcc-12  # to build modern C++20
    g++-12  # to build modern C++20
    xvfb    # so that the test suites can open virtual (hidden) windows
)

# these are the dependencies tools that other scripts (e.g. `build_ubuntu_debugging.sh`) need
sudo apt-get install -y ${ubuntu_22_dependencies[@]}
