#!/usr/bin/env bash

# `build_ubuntu-20.04_debugging.sh`: performs an end-to-end debug build of
# OpenSim Creator on Ubuntu 20.04 systems.

export CC=clang-11
export CXX=clang++-11
export CLANG_TIDY=echo  # clang-tidy-11 is screwed on ubuntu20.04

./scripts/build_linux_debugging.sh
