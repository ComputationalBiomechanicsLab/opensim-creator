#!/usr/bin/env bash

# setup system dependencies
sudo apt update
sudo apt install -y cmake pkg-config libgtk-3-dev libblas-dev liblapack-dev clang-11 clang-tidy-11 libstdc++-10-dev
export CC=clang-11
export CXX=clang++-11

# install a newer cmake than the one supplied by apt
sudo apt install -y libssl-dev  # OpenSSL headers, for cmake
wget https://github.com/Kitware/CMake/releases/download/v3.31.2/cmake-3.31.2.tar.gz
tar xvf cmake-3.31.2.tar.gz
cd cmake-3.31.2
./bootstrap
make -j$(nproc) && sudo make install
cd -

export CLANG_TIDY=echo  # clang-tidy-11 is screwed on ubuntu20.04
git clone --recurse-submodules https://github.com/ComputationalBiomechanicsLab/opensim-creator
cd opensim-creator
./scripts/build_linux_debugging.sh

