#!/usr/bin/env bash

set -xeuo pipefail

# setup the build machine (e.g. build dependencies)
./scripts/setup_unix.sh

# source the project-level virtual environment
source ./scripts/env_venv.sh

# build the project
python scripts/build.py

