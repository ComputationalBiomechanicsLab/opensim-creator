#!/usr/bin/env bash
#
# Performs an end-to-end build of opynsim on Unix-like
# operating systems (i.e. macOS or Linux).

# If any part of this script fails, exit early.
set -xeuo pipefail

# If no arguments, default to "Release"
if [ "$#" -eq 0 ]; then
    CONFIGS=("Release")
else
    CONFIGS=("$@")
fi

# Setup project-level python virtual environment
./scripts/setup_venv.py

# Build each specified configuration
for CONFIG in "${CONFIGS[@]}"; do
    echo "=== Building configuration: $CONFIG ==="

    # Build bundled dependencies
    cd third_party && cmake --workflow --preset "$CONFIG" && cd -

    # Build the main project
    cmake --workflow --preset "$CONFIG"
done
