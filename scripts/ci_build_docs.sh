#!/usr/bin/env bash

set -xeuo pipefail

# Determine which activate script exists
if [ -f "docs/venv/bin/activate" ]; then
    source "docs/venv/bin/activate"  # Linux/macOS
elif [ -f "docs/venv/Scripts/activate" ]; then
    source "docs/venv/Scripts/activate"  # Windows (Git Bash, Cygwin, WSL)
else
    echo "ERROR: Could not find a valid Python virtual environment in 'docs/venv/'."
    exit 1
fi

sphinx-build docs/source docs/build
