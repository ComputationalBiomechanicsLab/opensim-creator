#!/usr/bin/env bash

set -euo pipefail

# setup project-level python virtual environment
./scripts/setup_venv.py

# build bundled dependencies
./scripts/build-dependencies.py $*

