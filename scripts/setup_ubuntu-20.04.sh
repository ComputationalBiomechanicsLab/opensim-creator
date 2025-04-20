#!/usr/bin/env bash

# `setup_ubuntu-20.04.sh`: sets up an Ubuntu 20.04 machine
# with the necessary dependencies to build OpenSim Creator.

# setup system dependencies
sudo apt install -y cmake pkg-config libgtk-3-dev libblas-dev liblapack-dev clang-11 clang-tidy-11 libstdc++-10-dev xdg-desktop-portal-gtk

# install a newer cmake than the one supplied by apt
sudo apt install -y libssl-dev  # OpenSSL headers, for cmake
wget https://github.com/Kitware/CMake/releases/download/v3.31.2/cmake-3.31.2.tar.gz
tar xvf cmake-3.31.2.tar.gz
cd cmake-3.31.2
CXX=clang++-11 CC=clang-11 ./bootstrap
CXX=clang++-11 CC=clang-11 make -j$(nproc) && sudo make install
cd -
