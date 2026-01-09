#!/usr/bin/env bash
#
# Sets up an Ubuntu >=22.04 machine with the necessary dependencies to build
# OpenSim Creator.

# propagate any failures to this script
set -xeuo pipefail

apt_build_dependencies=(
    cmake                   # top-level build system
    build-essential         # C/C++ compiler, deb packaging, etc.
    pkg-config              # needed by the dependency build
    xdg-desktop-portal-gtk  # needed by vendored SDL3's dialog implementation
    libx11-dev              # needed by vendored SDL3's window implementation
    libxext-dev             # needed by vendored SDL3's window implementation
    libopengl-dev           # needed by liboscar (OSC)
    libgl1-mesa-dev         # needed by liboscar (OSC)
)

sudo apt-get install -y ${apt_build_dependencies[@]}
