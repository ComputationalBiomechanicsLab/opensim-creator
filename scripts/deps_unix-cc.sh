#!/usr/bin/env bash

export OSC_BUILD_CONCURRENCY=${OSC_BUILD_CONCURRENCY:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}

set -xeuo pipefail

# Default to "Debug" if no arguments are provided
if [ $# -eq 0 ]; then
    configs=("Debug")
else
    configs=("$@")
fi

for config in "${configs[@]}"; do
    cmake -S third_party/ -B third_party-build-${config} -DCMAKE_INSTALL_PREFIX=${PWD}/third_party-install-${config} -DCMAKE_BUILD_TYPE=${config}
    cmake --build third_party-build-${config} -v -j${OSC_BUILD_CONCURRENCY}
done

