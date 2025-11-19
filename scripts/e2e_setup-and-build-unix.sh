#!/usr/bin/env bash

# performs an end-to-end build of opynsim on Unixes (e.g. MacOS, Linux)

set -xeuo pipefail

# If no arguments, default to "Release"
if [ "$#" -eq 0 ]; then
    CONFIGS=("Release")
else
    CONFIGS=("$@")
fi

# setup project-level python virtual environment
./scripts/setup_venv.py

# source the project-level virtual environment
source ./scripts/env_venv.sh

for CONFIG in "${CONFIGS[@]}"; do
    echo "=== Building configuration: $CONFIG ==="

    # build bundled dependencies
    cd third_party/ && cmake --workflow --preset "OPynSim_third-party_$CONFIG" && cd ..

    # build the main project
    cmake --workflow --preset "OPynSim_$CONFIG"
done
