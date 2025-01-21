#!/usr/bin/env bash

# WSL2 on Ubuntu24: ln -sf  /mnt/wslg/runtime-dir/wayland-* $XDG_RUNTIME_DIR/

sudo apt install clang clang++ clang-tidy cmake pkg-config libgtk-3-dev libblas-dev liblapack-dev
git clone --recurse-submodules https://github.com/ComputationalBiomechanicsLab/opensim-creator
cd opensim-creator
./scripts/build_linux_debugging.sh

