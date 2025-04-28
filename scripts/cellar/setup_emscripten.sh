#!/usr/bin/env bash
#
# Sets up a Linux machine with the necessary dependencies to build OpenSim
# Creator for Emscripten (wasm) architectures.

# propagate any failures to this script
set -xeuo pipefail

# install emsdk
if [ ! -d emsdk ]; then
     git clone https://github.com/emscripten-core/emsdk.git
     cd emsdk/
     ./emsdk install latest
     cd -
fi

# note: the caller should activate/source emsdk like this
# ./emsdk/emsdk activate latest
# source ./emsdk/emsdk_env.sh
