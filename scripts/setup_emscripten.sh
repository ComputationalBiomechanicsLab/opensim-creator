#!/usr/bin/env bash
#
# `setup_emscripten.sh`: installs any necessary dependencies for
# building OpenSim Creator via emscripten.

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
