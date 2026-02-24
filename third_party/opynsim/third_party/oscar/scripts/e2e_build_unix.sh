#!/usr/bin/env bash
#
# Performs an end-to-end build on unixes

set -xeuo pipefail

# If no arguments, default to a "Release" build
if [ "$#" -eq 0 ]; then
    CONFIGS=("Release")
else
    CONFIGS=("$@")
fi

for CONFIG in "${CONFIGS[@]}"; do
    echo "=== Building configuration: $CONFIG ==="

    # Configure + build dependencies
    cd third_party && cmake --workflow --preset "$CONFIG" && cd -

    # Build project
    cmake --workflow --preset "$CONFIG"
done
