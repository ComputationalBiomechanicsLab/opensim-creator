#!/usr/bin/env bash

# `build_ubuntu-24.04_debugging.sh`: performs an end-to-end debug build of
# OpenSim Creator on Ubuntu 24.04 systems.

# WSL2 on Ubuntu24: ln -sf  /mnt/wslg/runtime-dir/wayland-* $XDG_RUNTIME_DIR/

./scripts/build_linux_debugging.sh
