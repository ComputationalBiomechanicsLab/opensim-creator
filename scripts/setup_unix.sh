#!/usr/bin/env bash

set -euo pipefail

# setup project-level python virtual environment
./scripts/setup_venv.py

# build bundled dependencies
cd third_party/ && cmake --workflow --preset OPynSim_third-party_Release && cd ..
