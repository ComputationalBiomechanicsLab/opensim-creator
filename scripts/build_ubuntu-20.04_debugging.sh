#!/usr/bin/env bash

export CC=clang-11
export CXX=clang++-11
export CLANG_TIDY=echo  # clang-tidy-11 is screwed on ubuntu20.04

./scripts/build_linux_debugging.sh

