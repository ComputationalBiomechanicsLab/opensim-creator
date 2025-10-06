#!/usr/bin/env bash

set -e

# Default to "Debug" if no arguments are provided
if [ $# -eq 0 ]; then
    configs=("Debug")
else
    configs=("$@")
fi

for config in "${configs[@]}"; do
    cmake -G "Ninja" -S third_party/ -B third_party-build-${config} -DCMAKE_INSTALL_PREFIX=${PWD}/third_party-install-${config} -DCMAKE_BUILD_TYPE=${config}
    cmake --build third_party-build-${config} -v
done
