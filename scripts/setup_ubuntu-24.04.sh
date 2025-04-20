#!/usr/bin/env bash

# `setup_ubuntu-24.04.sh`: sets up an Ubuntu 24.04 machine with the
# necessary dependencies to build OpenSim Creator.

# WSL2 on Ubuntu24: ln -sf  /mnt/wslg/runtime-dir/wayland-* $XDG_RUNTIME_DIR/

sudo apt-get install -y clang clang-tidy cmake pkg-config libgtk-3-dev libblas-dev liblapack-dev xdg-desktop-portal-gtk
